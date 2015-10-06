/*
    Copyright (C) 2012  Spatial Transcriptomics AB,
    read LICENSE for licensing terms.
    Contact : Jose Fernandez Navarro <jose.fernandez.navarro@scilifelab.se>

*/
#ifndef GENESELECTIONTABLEVIEW_H
#define GENESELECTIONTABLEVIEW_H

#include <QTableView>
#include <QPointer>

class GeneSelectionItemModel;
class SortGenesProxyModel;

// An abstraction of QTableView for the gene selections table in the user selections window
class GeneSelectionTableView : public QTableView
{
    Q_OBJECT

public:
    explicit GeneSelectionTableView(QWidget* parent = 0);
    virtual ~GeneSelectionTableView();

    // returns the current selection mapped to the sorting model
    QItemSelection geneTableItemSelection() const;

public slots:

    // slot used to set a search filter for the table
    void setGeneNameFilter(QString);

private:
    // references to model and proxy model
    QPointer<GeneSelectionItemModel> m_geneSelectionModel;
    QPointer<SortGenesProxyModel> m_sortGenesProxyModel;

    Q_DISABLE_COPY(GeneSelectionTableView)
};

#endif // GENESELECTIONTABLEVIEW_H
