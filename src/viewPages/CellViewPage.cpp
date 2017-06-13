#include "CellViewPage.h"

#include <QDebug>
#include <QString>
#include <QStringList>
#include <QPrintDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPrinter>
#include <QImageReader>
#include <QImageWriter>
#include <QPainter>
#include <QDateTime>

#include "viewRenderer/CellGLView.h"
#include "dialogs/SelectionDialog.h"
#include "analysis/AnalysisQC.h"
#include "SettingsWidget.h"
#include "SettingsStyle.h"

#include <algorithm>

#include "ui_cellviewPage.h"

using namespace Style;

CellViewPage::CellViewPage(QWidget *parent)
    : QWidget(parent)
    , m_ui(new Ui::CellView())
    , m_legend(nullptr)
    , m_gene_plotter(nullptr)
    , m_image(nullptr)
    , m_settings(nullptr)
    , m_dataset()

{
    m_ui->setupUi(this);

    // setting style to main UI Widget (frame and widget must be set specific to avoid propagation)
    setWindowFlags(Qt::FramelessWindowHint);
    m_ui->cellViewPageWidget->setStyleSheet("QWidget#cellViewPageWidget " + PAGE_WIDGETS_STYLE);
    m_ui->frame->setStyleSheet("QFrame#frame " + PAGE_FRAME_STYLE);

    // make selection button use different icon when selected
    m_ui->selection->setStyleSheet(
        "QPushButton {border-image: url(:/images/selection.png); } "
        "QPushButton:checked {border-image: url(:/images/selection2.png); }");

    // instantiate Settings Widget
    m_settings.reset(new SettingsWidget());
    Q_ASSERT(!m_settings.isNull());

    // initialize rendering pipeline
    initRenderer();

    // create toolbar and all the connections
    createConnections();
}

CellViewPage::~CellViewPage()
{
}

void CellViewPage::clear()
{
    // reset visualization objects
    m_image->clearData();
    m_gene_plotter->clearData();
    m_legend->clearData();
    m_ui->view->clearData();
    m_ui->view->update();
    m_settings->reset();
    m_dataset = Dataset();
}

void CellViewPage::loadDataset(const Dataset &dataset)
{
    //NOTE we allow to re-open the same dataset (in case it has been edited)

    // update Status tip with the name of the currently selected dataset
    setStatusTip(tr("Dataset loaded %1").arg(dataset.name()));

    // store the dataset
    m_dataset = dataset;

    // update gene plotter rendering object with the dataset
    m_gene_plotter->clearData();
    m_gene_plotter->attachData(dataset.data());

    // load cell tissue (to load the dataset's cell tissue image)
    // create tiles textures from the image
    m_image->clearData();
    const bool image_loaded = m_image->createTiles(dataset.imageFile());
    if (!image_loaded) {
        QMessageBox::warning(this, tr("Tissue image"),
                              tr("Error loading tissue image"));
    } else {
        m_ui->view->setScene(m_image->boundingRect());
        // Update the image aligment with the image's dimension
        // if it is not given by the user
        QTransform alignment = dataset.imageAlignment();
        if (alignment.isIdentity()) {
            // TODO these should be given or taken from config
            const int chip_x2 = 33;
            const int chip_y2 = 35;
            const int width_image = m_image->boundingRect().width();
            const int height_image = m_image->boundingRect().height();
            const float a11 = width_image / (chip_x2 - 1);
            const float a12 = 0.0;
            const float a13 = 0.0;
            const float a21 = 0.0;
            const float a22 = height_image / (chip_y2 - 1);
            const float a23 = 0.0;
            const float a31 = -a11;
            const float a32 = -a22;
            const float a33 = 1.0;
            alignment.setMatrix(a11, a12, a13, a21, a22, a23, a31, a32, a33);
        }
        qDebug() << "Setting alignment matrix to " << alignment;
        m_gene_plotter->setTransform(alignment);
    }
    // call for an update
    m_ui->view->update();
}

void CellViewPage::clearSelections()
{
    m_dataset.data()->clearSelection();
    m_ui->view->update();
}

void CellViewPage::slotGenesUpdate()
{
    m_gene_plotter->slotUpdate();
    m_ui->view->update();
}

void CellViewPage::slotSpotsUpdated()
{
    m_gene_plotter->slotUpdate();
    m_ui->view->update();
}

void CellViewPage::createConnections()
{

    // settings menu
    connect(m_ui->genemenu, &QPushButton::clicked, m_settings.data(), &SettingsWidget::show);

    // show/hide cell image
    connect(m_settings.data(), &SettingsWidget::signalShowImage, this,
            [=](bool visible){m_image->setVisible(visible);});

    // show/hide spots
    connect(m_settings.data(), &SettingsWidget::signalShowSpots, this,
            [=](bool visible){m_gene_plotter->setVisible(visible);});

    // show/hide legend
    connect(m_settings.data(), &SettingsWidget::signalShowLegend, this,
            [=](bool visible){m_legend->setVisible(visible);});

    // rendering settings changed
    connect(m_settings.data(), &SettingsWidget::signalSpotRendering, this,
            [=](){
        m_gene_plotter->slotUpdate();
        m_legend->slotUpdate();
        m_ui->view->update();
    });

    // graphic view signals
    connect(m_ui->zoomin, &QPushButton::clicked, m_ui->view, &CellGLView::zoomIn);
    connect(m_ui->zoomout, &QPushButton::clicked, m_ui->view, &CellGLView::zoomOut);

    // print canvas
    connect(m_ui->save, &QPushButton::clicked, this, &CellViewPage::slotSaveImage);
    connect(m_ui->print, &QPushButton::clicked, this, &CellViewPage::slotPrintImage);

    // selection mode
    connect(m_ui->selection, &QPushButton::clicked, [=] {
        m_ui->view->setSelectionMode(m_ui->selection->isChecked());
    });
    connect(m_ui->lasso_selection, &QPushButton::clicked, [=] {
        m_ui->view->setLassoSelectionMode(m_ui->lasso_selection->isChecked());
    });
    connect(m_ui->regexpselection, &QPushButton::clicked,
            this, &CellViewPage::slotSelectByRegExp);

    // create selection object from the selections made
    connect(m_ui->createSelection, &QPushButton::clicked,
            this, &CellViewPage::signalUserSelection);

    // show QC widget
    connect(m_ui->histogram, &QPushButton::clicked, this, &CellViewPage::slotShowQC);

    // show Clustering widget
    connect(m_ui->clustering, &QPushButton::clicked, this, &CellViewPage::slowClustering);
}


void CellViewPage::initRenderer()
{
    // the OpenGL main view object is initialized in the UI form class

    // image texture graphical object
    m_image = QSharedPointer<ImageTextureGL>(new ImageTextureGL());
    m_ui->view->addRenderingNode(m_image);

    // gene plotter component
    m_gene_plotter = QSharedPointer<GeneRendererGL>(
                new GeneRendererGL(m_settings->renderingSettings()));
    m_ui->view->addRenderingNode(m_gene_plotter);

    // heatmap component
    m_legend = QSharedPointer<HeatMapLegendGL>(
                new HeatMapLegendGL(m_settings->renderingSettings()));
    m_ui->view->addRenderingNode(m_legend);
}

void CellViewPage::slotPrintImage()
{
    QPrinter printer;
    printer.setColorMode(QPrinter::Color);
    printer.setOrientation(QPrinter::Landscape);

    QScopedPointer<QPrintDialog> dialog(new QPrintDialog(&printer, this));
    if (dialog->exec() != QDialog::Accepted) {
        return;
    }

    QPainter painter(&printer);
    QRect rect = painter.viewport();
    QImage image = m_ui->view->grabPixmapGL();
    QSize size = image.size();
    size.scale(rect.size(), Qt::KeepAspectRatio);
    painter.setViewport(QRect(QPoint(0, 0), size));
    painter.setWindow(image.rect());
    painter.drawImage(0, 0, image);
}

void CellViewPage::slotSaveImage()
{
    QString filename = QFileDialog::getSaveFileName(this,
                                                    tr("Save Image"),
                                                    QDir::homePath(),
                                                    QString("%1;;%2;;%3")
                                                        .arg(tr("JPEG Image Files (*.jpg *.jpeg)"))
                                                        .arg(tr("PNG Image Files (*.png)"))
                                                        .arg(tr("BMP Image Files (*.bmp)")));
    // early out
    if (filename.isEmpty()) {
        return;
    }

    const QFileInfo fileInfo(filename);
    const QFileInfo dirInfo(fileInfo.dir().canonicalPath());
    if (!fileInfo.exists() && !dirInfo.isWritable()) {
        qDebug() << "Saving the image, the directory is not writtable";
        return;
    }

    const int quality = 100; // quality format (100 max, 0 min, -1 default)
    const QString format = fileInfo.suffix().toLower();
    QImage image = m_ui->view->grabPixmapGL();
    if (!image.save(filename, format.toStdString().c_str(), quality)) {
        qDebug() << "Saving the image, the image coult not be saved";
    }
}

void CellViewPage::slotSelectByRegExp()
{
    SelectionDialog selectGenes(this);
    if (selectGenes.exec() == QDialog::Accepted) {
        if (selectGenes.isValid()) {
            m_dataset.data()->selectGenes(selectGenes.getRegExp(), selectGenes.selectNonVisible());
            m_ui->view->update();
        }
    }
}

UserSelection CellViewPage::createSelection()
{
    //TODO check and throw exception if no spots are currently selected
    // create selection object
    UserSelection new_selection(m_dataset.data());
    // proposes as selection name as DATASET NAME plus current timestamp
    new_selection.name(m_dataset.name() + " " + QDateTime::currentDateTimeUtc().toString());
    new_selection.dataset(m_dataset.name());
    // clear the selection
    clearSelections();
    return new_selection;
}

void CellViewPage::slotShowQC()
{
    AnalysisQC *qc = new AnalysisQC(m_dataset.data()->data(), this, Qt::Window);
    qc->show();
}

void CellViewPage::slowClustering()
{

}
