/*
    Copyright (C) 2012  Spatial Transcriptomics AB,
    read LICENSE for licensing terms.
    Contact : Jose Fernandez Navarro <jose.fernandez.navarro@scilifelab.se>

*/

#ifndef AUTHORIZATIONMANAGER_H
#define AUTHORIZATIONMANAGER_H

#include <QObject>
#include <QPointer>
#include <QUuid>

#include "auth/TokenStorage.h"

class NetworkManager;
class OAuth2;
class QWidget;
class Error;
class DataProxy;

//  This class gives an interface for oAuth2 authorization
//  trough the objects OAuth2 and TokenStorage.
//  It emits signals for error, authorized and abortion.
//  It allows for local storage of access token.

class AuthorizationManager : public QObject
{
    Q_OBJECT

public:

    AuthorizationManager(QPointer<DataProxy> dataProxy, QObject* parent = 0);
    virtual ~AuthorizationManager();

    //start the login process
    void startAuthorization();

    //clean access token
    void cleanAccesToken();

    //true if the user is already authenticated
    bool isAuthenticated() const;

    //user acces token methods to check
    //if the user is already logged in
    bool hasAccessToken() const;
    QUuid getAccessToken() const;

signals:

    //the login is successful
    void signalAuthorize();
    //there was an error trying to log in
    void signalError(QSharedPointer<Error> error);

private slots:

    //internal slot to process login object from OAuth2
    void slotLoginDone(const QUuid& accessToken, int expiresIn,
                       const QUuid& refreshToken);

private:

    QPointer<OAuth2> m_oAuth2;
    TokenStorage m_tokenStorage;

    //reference to dataProxy (to be passed along to OAuth2
    QPointer<DataProxy> m_dataProxy;

    Q_DISABLE_COPY(AuthorizationManager)
};

#endif  /* AUTHORIZATIONMANAGER_H */

