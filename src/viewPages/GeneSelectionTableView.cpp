/*
    Copyright (C) 2012  Spatial Transcriptomics AB,
    read LICENSE for licensing terms.
    Contact : Jose Fernandez Navarro <jose.fernandez.navarro@scilifelab.se>

*/

#include "GeneSelectionTableView.h"

#include <QHeaderView>
#include "model/SortGenesProxyModel.h"
#include "model/GeneSelectionItemModel.h"

GeneSelectionTableView::GeneSelectionTableView(QWidget *parent)
    : QTableView(parent)
{
    m_geneSelectionModel = new GeneSelectionItemModel(this);

    SortGenesProxyModel *sortProxyModel = new SortGenesProxyModel(this);
    sortProxyModel->setSourceModel(m_geneSelectionModel);
    sortProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    sortProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    setModel(sortProxyModel);
    setSortingEnabled(true);
    sortByColumn(0, Qt::AscendingOrder);
    horizontalHeader()->setSortIndicatorShown(true);

    setSortingEnabled(true);
    horizontalHeader()->setSortIndicatorShown(true);
    sortByColumn(0, Qt::AscendingOrder);

    resizeColumnsToContents();
    resizeRowsToContents();

    setSelectionBehavior(QAbstractItemView::SelectItems);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setSelectionMode(QAbstractItemView::NoSelection);

    setShowGrid(true);

    horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    horizontalHeader()->setSectionResizeMode(2, QHeaderView::Stretch);

    verticalHeader()->hide();
    model()->submit(); //support for caching (speed up)
}

GeneSelectionTableView::~GeneSelectionTableView()
{

}
