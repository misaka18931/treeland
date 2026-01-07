// Copyright (C) 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "wglobal.h"

#include <xcb/xproto.h>

WAYLIB_SERVER_BEGIN_NAMESPACE
class WSocket;
class WXWayland;
WAYLIB_SERVER_END_NAMESPACE

WAYLIB_SERVER_USE_NAMESPACE

class SettingManager;

struct Session : QObject {
    Q_OBJECT
public:
    int id = 0;
    uid_t uid = 0;
    QString username = {};
    WSocket *socket = nullptr;
    WXWayland *xwayland = nullptr;
    quint32 noTitlebarAtom = XCB_ATOM_NONE;
    SettingManager *settingManager = nullptr;
    QThread *settingManagerThread = nullptr;

    ~Session();

Q_SIGNALS:
    void aboutToBeDestroyed();
};

/**
 * SessionManager manages user sessions, including their associated
 * WSocket and WXWayland instances.
 */
struct SessionManager : QObject {
    Q_OBJECT
public:
    explicit SessionManager(QObject *parent = nullptr);
    ~SessionManager();

    static SessionManager *instance();

    QList<std::shared_ptr<Session>> sessions() const;

    bool activeSocketEnabled() const;
    void setActiveSocketEnabled(bool newEnabled);

    std::shared_ptr<Session> ensureSession(int id, QString username);
    void updateActiveUserSession(const QString &username, int id);
    void removeSession(std::shared_ptr<Session> session);
    std::shared_ptr<Session> sessionForId(int id) const;
    std::shared_ptr<Session> sessionForUid(uid_t uid) const;
    std::shared_ptr<Session> sessionForUser(const QString &username) const;
    std::shared_ptr<Session> sessionForXWayland(WXWayland *xwayland) const;
    std::shared_ptr<Session> sessionForSocket(WSocket *socket) const;
    std::weak_ptr<Session> activeSession() const;
    WXWayland *xwaylandForUid(uid_t uid) const;
    WSocket *waylandSocketForUid(uid_t uid) const;
    WSocket *globalWaylandSocket() const;
    WXWayland *globalXWayland() const;

Q_SIGNALS:
    void socketFileChanged();
    void sessionChanged();

private:
    static SessionManager *m_instance;

    std::weak_ptr<Session> m_activeSession;
    QList<std::shared_ptr<Session>> m_sessions;
};
