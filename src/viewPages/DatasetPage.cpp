#include "DatasetPage.h"

#include <QDebug>
#include <QModelIndex>
#include <QSortFilterProxyModel>
#include <QMessageBox>
#include <QUuid>
#include <QDateTime>
#include <QFileDialog>
#include <QDirIterator>
#include <QJsonObject>
#include <QJsonDocument>
#include <QPushButton>
#include <QStandardPaths>

#include "model/DatasetItemModel.h"
#include "dialogs/EditDatasetDialog.h"
#include "data/DatasetImporter.h"
#include "SettingsStyle.h"

#include "ui_datasetsPage.h"

using namespace Style;

DatasetPage::DatasetPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::DataSets())
    , m_importedDatasets()
    , m_open_dataset()
{
    m_ui->setupUi(this);

    // setting style to main UI Widget (frame and widget must be set specific to avoid propagation)
    m_ui->DatasetPageWidget->setStyleSheet("QWidget#DatasetPageWidget " + PAGE_WIDGETS_STYLE);
    m_ui->frame->setStyleSheet("QFrame#frame " + PAGE_FRAME_STYLE);

    // connect signals
    connect(m_ui->filterLineEdit,
            &QLineEdit::textChanged,
            datasetsProxyModel(),
            &QSortFilterProxyModel::setFilterFixedString);
    connect(m_ui->datasetsTableView,
            &DatasetsTableView::clicked,
            this,
            &DatasetPage::slotDatasetSelected);
    connect(m_ui->datasetsTableView,
            &DatasetsTableView::doubleClicked,
            this,
            &DatasetPage::slotSelectAndOpenDataset);
    connect(m_ui->deleteDataset, &QPushButton::clicked, this, &DatasetPage::slotRemoveDataset);
    connect(m_ui->editDataset, &QPushButton::clicked, this, &DatasetPage::slotEditDataset);
    connect(m_ui->openDataset, &QPushButton::clicked, this, &DatasetPage::slotOpenDataset);
    connect(m_ui->importDataset, &QPushButton::clicked, this, &DatasetPage::slotImportDataset);
    connect(m_ui->importFolder, &QPushButton::clicked, this, &DatasetPage::slotImportDatasetFolder);

    // reset controls
    clearControls();
}

DatasetPage::~DatasetPage()
{
}

void DatasetPage::clean()
{
    m_open_dataset = nullptr;
    m_importedDatasets.clear();
    datasetsModel()->clear();
    clearControls();
}

QSortFilterProxyModel *DatasetPage::datasetsProxyModel()
{
    QSortFilterProxyModel *datasetsProxyModel
            = qobject_cast<QSortFilterProxyModel *>(m_ui->datasetsTableView->model());
    Q_ASSERT(datasetsProxyModel);
    return datasetsProxyModel;
}

DatasetItemModel *DatasetPage::datasetsModel()
{
    DatasetItemModel *model =
            qobject_cast<DatasetItemModel *>(datasetsProxyModel()->sourceModel());
    Q_ASSERT(model);
    return model;
}

void DatasetPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    clearControls();
}

void DatasetPage::clearControls()
{
    // clear selection
    m_ui->datasetsTableView->clearSelection();

    // controls disable by default
    m_ui->deleteDataset->setEnabled(false);
    m_ui->editDataset->setEnabled(false);
    m_ui->openDataset->setEnabled(false);
}

void DatasetPage::slotDatasetSelected(QModelIndex index)
{
    const auto selected = m_ui->datasetsTableView->datasetsTableItemSelection();
    const auto currentDatasets = datasetsModel()->getDatasets(selected);
    // Check if the selection is valid
    if (!index.isValid() || currentDatasets.empty()) {
        return;
    }
    // Enable only remove if more than one selected
    const bool more_than_one = currentDatasets.size() > 1;
    m_ui->deleteDataset->setEnabled(true);
    m_ui->editDataset->setEnabled(!more_than_one);
    m_ui->openDataset->setEnabled(!more_than_one);
}

void DatasetPage::slotSelectAndOpenDataset(QModelIndex index)
{
    slotDatasetSelected(index);
    slotOpenDataset();
}

void DatasetPage::slotEditDataset()
{
    const auto selected = m_ui->datasetsTableView->datasetsTableItemSelection();
    const auto currentDatasets = datasetsModel()->getDatasets(selected);
    // Can only edit 1 valid dataset
    if (currentDatasets.size() != 1) {
        return;
    }
    const auto dataset = currentDatasets.front();
    DatasetImporter importer(dataset);
    // Launch the dialog
    const int result = importer.exec();
    if (result == QDialog::Accepted) {
        // Check that the name does not exist
        const QString datasetName = importer.datasetName();
        if (nameExist(datasetName) && datasetName != dataset.name()) {
            QMessageBox::critical(this, tr("Datasert import"),
                                  tr("There is another dataset with the same name"));
        } else {
            const int index = m_importedDatasets.indexOf(dataset);
            Q_ASSERT(index != -1);
            Dataset updated_dataset(importer);
            m_importedDatasets.replace(index, updated_dataset);
            if (dataset == *(m_open_dataset.data())
                    && (dataset.dataFile() != updated_dataset.dataFile()
                        || dataset.imageFile() != updated_dataset.imageFile()
                        || dataset.imageAlignmentFile() != updated_dataset.imageAlignmentFile()
                        || dataset.spotsFile() != updated_dataset.spotsFile())) {
                m_open_dataset = QSharedPointer<Dataset>(new Dataset(updated_dataset));
                emit signalDatasetUpdated(updated_dataset.name());
            }
            slotDatasetsUpdated();
        }
    }
}

void DatasetPage::slotOpenDataset()
{
    const auto selected = m_ui->datasetsTableView->datasetsTableItemSelection();
    const auto currentDatasets = datasetsModel()->getDatasets(selected);
    // Can only open 1 valid dataset
    if (currentDatasets.size() != 1) {
        return;
    }
    auto dataset = currentDatasets.front();
    QGuiApplication::setOverrideCursor(Qt::WaitCursor);
    if (!dataset.load_data()) {
        QMessageBox::critical(this, tr("Datasert import"), tr("Error opening ST dataset"));
    }
    QGuiApplication::restoreOverrideCursor();
    // Set selected dataset
    m_open_dataset = QSharedPointer<Dataset>(new Dataset(dataset));
    // Notify that the dataset was open
    emit signalDatasetOpen(dataset.name());
}

void DatasetPage::slotRemoveDataset()
{
    const auto selected = m_ui->datasetsTableView->datasetsTableItemSelection();
    const auto currentDatasets = datasetsModel()->getDatasets(selected);
    // Can remove multiple datasets
    if (currentDatasets.empty()) {
        return;
    }

    const int answer
            = QMessageBox::warning(this,
                                   tr("Remove Dataset"),
                                   tr("Are you really sure you want to remove the dataset/s?"),
                                   QMessageBox::Yes,
                                   QMessageBox::No | QMessageBox::Escape);

    if (answer != QMessageBox::Yes) {
        return;
    }

    for (auto dataset: currentDatasets) {
        Q_ASSERT(m_importedDatasets.removeOne(dataset));
        if (*(m_open_dataset.data()) == dataset) {
            emit signalDatasetRemoved(dataset.name());
            m_open_dataset = nullptr;
        }
    }

    slotDatasetsUpdated();
}

void DatasetPage::slotImportDataset()
{
    DatasetImporter importer;
    // Launch the dialog
    const int result = importer.exec();
    if (result == QDialog::Accepted) {
        // Check that the name does not exist
        const QString datasetName = importer.datasetName();
        if (nameExist(datasetName)) {
            QMessageBox::critical(this, tr("Datasert import"),
                                  tr("There is another dataset with the same name"));
        } else {
            Dataset dataset(importer);
            m_importedDatasets.append(dataset);
            slotDatasetsUpdated();
        }
    } else {
        QMessageBox::critical(this, tr("Datasert import"), tr("Error importing dataset"));
    }
}

QSharedPointer<Dataset> DatasetPage::getCurrentDataset() const
{
    return m_open_dataset;
}

void DatasetPage::slotDatasetsUpdated()
{
    // update model and clear controls
    datasetsModel()->loadDatasets(m_importedDatasets);
    clearControls();
}

bool DatasetPage::nameExist(const QString &name)
{
    return std::find_if(m_importedDatasets.begin(), m_importedDatasets.end(),
                        [&name](const Dataset& dataset)
    {return dataset.name() == name;}) != m_importedDatasets.end();
}

void DatasetPage::slotImportDatasetFolder()
{
    QFileDialog dialog(this, tr("Select folder with the ST Dataset"), QDir::homePath());
    dialog.setFileMode(QFileDialog::Directory);
    dialog.setViewMode(QFileDialog::Detail);
    if (dialog.exec()) {
        QDir selectedDir = dialog.directory();
        selectedDir.setFilter(QDir::Files);
        Dataset dataset;
        QDirIterator it(selectedDir, QDirIterator::NoIteratorFlags);
        while (it.hasNext()) {
            const QString file = it.next();
            qDebug() << "Parsing dataset file " << file;
            if (file.contains("stdata.tsv")) {
                dataset.dataFile(file);
            } else if (file.contains("image.jpg")) {
                dataset.imageFile(file);
            } else if (file.contains("alignment.txt")) {
                dataset.imageAlignmentFile(file);
            } else if (file.contains("spots.txt")) {
                dataset.spotsFile(file);
            } else if (file.contains("info.json")) {
                QFile file_data(file);
                if (file_data.open(QIODevice::ReadOnly)) {
                    const QByteArray &data = file_data.readAll();
                    const QJsonDocument &loadDoc = QJsonDocument::fromJson(data);
                    const QJsonObject &jsonObject = loadDoc.object();
                    if (jsonObject.contains("name")) {
                        dataset.name(jsonObject["name"].toString());
                    }
                    if (jsonObject.contains("species")) {
                        dataset.statSpecies(jsonObject["species"].toString());
                    }
                    if (jsonObject.contains("tissue")) {
                        dataset.statTissue(jsonObject["tissue"].toString());
                    }
                    if (jsonObject.contains("comments")) {
                        dataset.statComments(jsonObject["comments"].toString());
                    }
                } else {
                    qDebug() << "Error parsing info.json for dataset";
                }
            }
        }

        // Validate dataset
        if (!dataset.name().isEmpty() && !dataset.dataFile().isEmpty()
                && !dataset.imageFile().isEmpty()) {
            if (nameExist(dataset.name())) {
                QMessageBox::critical(this, tr("Datasert import"),
                                      tr("There is another dataset with the same name"));
            } else {
                m_importedDatasets.append(dataset);
                slotDatasetsUpdated();
            }
        } else {
            QMessageBox::critical(this, tr("Datasert import"), tr("Error importing dataset"));
        }
    }
}
