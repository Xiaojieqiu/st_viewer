/*
    Copyright (C) 2012  Spatial Transcriptomics AB,
    read LICENSE for licensing terms.
    Contact : Jose Fernandez Navarro <jose.fernandez.navarro@scilifelab.se>

*/

#ifndef stVi_H
#define stVi_H

#include <QMainWindow>
#include <QPointer>
#include "data/DataProxy.h"

class QSettings;
class QCloseEvent;
class QAction;
class QStatusBar;
class QMenu;
class Error;
class QVBoxLayout;
class QWidget;
class AuthorizationManager;
class DatasetPage;
class CellViewPage;
class UserSelectionsPage;
class GenesWidget;

// TODO add somewhere a QLabel to show the user that is logged in
// TODO action checked does not work with the dataset/selection
// TODO add shortcuts for the views
class stVi : public QMainWindow
{
    Q_OBJECT

public:
    explicit stVi(QWidget* parent = 0);
    ~stVi();

    // initialize main visual components
    void init();

    // check that the host has all the required hardware (true yes : false no )
    bool checkSystemRequirements() const;

    // system settings
    void loadSettings();
    void saveSettings() const;

    // tries to find a cached access token otherwise it will show a log in dialog
    void startAuthorization();

private slots:

    // exit the application
    void slotExit();

    // clear the cache and local stored files
    void slotClearCache();

    // opens pop up static widget to show info about the application
    void slotShowAbout();

    // to handle when the min version is downloaded (if not supported app will close)
    // status the status of the network request
    // type is the type of request
    void slotDownloadFinished(const DataProxy::DownloadStatus status,
                              const DataProxy::DownloadType type);

    // to handle different authorization call backs
    void slotAuthorizationError(QSharedPointer<Error> error);
    void slotAuthorized();
    // when user clicks to log out, shows log in dialog
    void slotLogOutButton();

private:
    // create all the widgets
    void setupUi();
    // initialize and configure layout
    void createLayouts();
    // load style sheets
    void initStyle();
    // create keyboard shortcuts
    void createShorcuts();
    // create some connections
    void createConnections();

    // overloaded close Event function to handle the exit
    void closeEvent(QCloseEvent* event) override;

    // reference to some UI elements
    QPointer<QAction> m_actionExit;
    QPointer<QAction> m_actionHelp;
    QPointer<QAction> m_actionVersion;
    QPointer<QAction> m_actionAbout;
    QPointer<QAction> m_actionClear_Cache;
    QPointer<QAction> m_actionDatasets;
    QPointer<QAction> m_actionLogOut;
    QPointer<QAction> m_actionSelections;

    // stVi owns dataProxy and AuthorizationManager
    QPointer<DataProxy> m_dataProxy;
    QPointer<AuthorizationManager> m_authManager;

    // pages
    QPointer<DatasetPage> m_datasets;
    QPointer<CellViewPage> m_cellview;
    QPointer<UserSelectionsPage> m_user_selections;
    QPointer<GenesWidget> m_genes;
    // TODO we want the cell view visual settings to be a separate dockable menu
    // TODO we might want the cell view tool bar to be in the main window tool bar
};

#endif // stVi_H
