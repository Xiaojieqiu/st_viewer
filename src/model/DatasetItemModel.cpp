/*
    Copyright (C) 2012  Spatial Transcriptomics AB,
    read LICENSE for licensing terms.
    Contact : Jose Fernandez Navarro <jose.fernandez.navarro@scilifelab.se>

*/

#include "DatasetItemModel.h"

#include <QModelIndex>
#include <QModelIndex>
#include <QStandardItemModel>
#include <QItemSelection>
#include <QDateTime>

#include "dataModel/Dataset.h"

#include <set>

static const int COLUMN_NUMBER = 9;

DatasetItemModel::DatasetItemModel(QObject* parent)
    : QAbstractTableModel(parent)
{

}

DatasetItemModel::~DatasetItemModel()
{

}

bool DatasetItemModel::setData(const QModelIndex& index,
                               const QVariant& value, int role)
{
    return QAbstractItemModel::setData(index, value, role);
}

QVariant DatasetItemModel::data(const QModelIndex& index, int role) const
{
    // early out
    if (!index.isValid() || m_datasets_reference.isEmpty()) {
        return QVariant(QVariant::Invalid);
    }

    if (role == Qt::DisplayRole) {
        DataProxy::DatasetPtr item = m_datasets_reference.at(index.row());
        Q_ASSERT(!item.isNull());
        switch (index.column()) {
        case Name: return item->name();
        case Tissue: return item->statTissue();
        case Specie: return item->statSpecie();
        case Barcodes: return item->statBarcodes();
        case Genes: return item->statGenes();
        case UBarcodes: return item->statUniqueBarcodes();
        case UGenes: return item->statUniqueGenes();
        case Created:
            return QDateTime::fromMSecsSinceEpoch(item->created().toLongLong());
        case LastModified:
            return QDateTime::fromMSecsSinceEpoch(item->lastModified().toLongLong());
        default: Q_ASSERT_X(false, "DatasetItemModel", "Unknown column!");
        }
    } else if (role == Qt::ForegroundRole && index.column() == Name) {
        return QColor(0, 155, 60);
    } else if (index.column() >= Barcodes
               && index.column() <= LastModified && role == Qt::TextAlignmentRole) {
        return Qt::AlignRight;
    }

    return QVariant(QVariant::Invalid);
}

QVariant DatasetItemModel::headerData(int section,
                                      Qt::Orientation orientation, int role) const
{
    if (role == Qt::ToolTipRole) {
        switch (section) {
        case Name : return tr("Dataset name");
        case Tissue : return tr("Tissue name");
        case Specie : return tr("Species name");
        case Barcodes : return tr("Number of reads");
        case Genes : return tr("Number of gene detection events");
        case UBarcodes : return tr("Number of uniquely detected barcodes");
        case UGenes : return tr("Number of uniquely detected genes");
        case Created: return tr("Created at this date");
        case LastModified: return tr("Last Modified at this date");
        default: Q_ASSERT_X(false, "DatasetItemModel", "Unknown column!");
        }
    }

    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Horizontal) {
            switch (section) {
            case Name : return tr("Name");
            case Tissue : return tr("Tissue");
            case Specie : return tr("Species");
            case Barcodes : return tr("Unique events");
            case Genes : return tr("Transcript reads");
            case UBarcodes : return tr("Detected Barcodes");
            case UGenes : return tr("Detected Genes");
            case Created: return tr("Created");
            case LastModified: return tr("Last Modified");
            default: Q_ASSERT_X(false, "DatasetItemModel", "Unknown column!");
            }
        } else if (orientation == Qt::Vertical) {
            return (section + 1);
        }
    }

    if (role == Qt::TextAlignmentRole) {
        return Qt::AlignLeft;
    }

    return QVariant(QVariant::Invalid);
}

int DatasetItemModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : COLUMN_NUMBER;
}

int DatasetItemModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid()  ? 0 : m_datasets_reference.count();
}

Qt::ItemFlags DatasetItemModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags flags = QAbstractItemModel::flags(index);
    if (index.isValid()) {
        flags |= Qt::ItemIsSelectable | Qt::ItemIsEnabled;
    }
    return flags;
}

bool DatasetItemModel::setHeaderData(int, Qt::Orientation, const QVariant &, int)
{
    return false;
}

void DatasetItemModel::loadDatasets(const DataProxy::DatasetList &datasetList)
{
    beginResetModel();
    m_datasets_reference.clear();
    m_datasets_reference = datasetList;
    endResetModel();
}

DataProxy::DatasetList
DatasetItemModel::getDatasets(const QItemSelection &selection)
{
    std::set<int> rows;
    for (const auto &index : selection.indexes()) {
        rows.insert(index.row());
    }

    DataProxy::DatasetList datasetList;
    for (const auto &row : rows) {
        auto selection = m_datasets_reference.at(row);
        datasetList.push_back(selection);
    }

    return datasetList;
}
