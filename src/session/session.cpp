// Copyright (C) 2026 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "session.h"

#include "common/treelandlogging.h"
#include "core/rootsurfacecontainer.h"
#include "core/shellhandler.h"
#include "seat/helper.h"
#include "xsettings/settingmanager.h"

#include <woutputrenderwindow.h>
#include <wsocket.h>
#include <wxwayland.h>

#include <pwd.h>

#define _DEEPIN_NO_TITLEBAR "_DEEPIN_NO_TITLEBAR"

static xcb_atom_t internAtom(xcb_connection_t *connection, const char *name, bool onlyIfExists)
{
    if (!name || *name == 0)
        return XCB_NONE;

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(connection, onlyIfExists, strlen(name), name);
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(connection, cookie, 0);

    if (!reply)
        return XCB_NONE;

    xcb_atom_t atom = reply->atom;
    free(reply);

    return atom;
}

Session::~Session()
{
    qCDebug(treelandCore) << "Deleting session for uid:" << uid << socket;
    Q_EMIT aboutToBeDestroyed();

    if (settingManagerThread) {
        settingManagerThread->quit();
        settingManagerThread->wait(QDeadlineTimer(25000));
    }

    if (settingManager) {
        delete settingManager;
        settingManager = nullptr;
    }
    if (xwayland)
        Helper::instance()->shellHandler()->removeXWayland(xwayland);
    if (socket)
        delete socket;
}

SessionManager *SessionManager::m_instance = nullptr;

SessionManager::SessionManager(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!m_instance);
    m_instance = this;
}

SessionManager *SessionManager::instance()
{
    return m_instance;
}

SessionManager::~SessionManager()
{
    for (auto session : std::as_const(m_sessions)) {
        Q_ASSERT(session);
        if (session->xwayland) {
            delete session->xwayland;
            session->xwayland = nullptr;
        }
        if (session->socket) {
            delete session->socket;
            session->socket = nullptr;
        }
    }
    m_sessions.clear();
}

QList<std::shared_ptr<Session>> SessionManager::sessions() const
{
    return m_sessions;
}

/**
 * Get the currently active session
 *
 * @returns weak_ptr to the active session
 */
std::weak_ptr<Session> SessionManager::activeSession() const
{
    return m_activeSession;
}

/**
 * Check if the active session's WSocket is enabled
 *
 * @returns true if enabled, false otherwise
 */
bool SessionManager::activeSocketEnabled() const
{
    auto ptr = m_activeSession.lock();
    if (ptr && ptr->socket)
        return ptr->socket->isEnabled();
    return false;
}

/**
 * Set the active session's WSocket enabled state
 *
 * @param newEnabled New enabled state
 */
void SessionManager::setActiveSocketEnabled(bool newEnabled)
{
    auto ptr = m_activeSession.lock();
    if (ptr && ptr->socket)
        ptr->socket->setEnabled(newEnabled);
    else
        qCWarning(treelandCore) << "Can't set enabled for empty socket!";
}

/**
 * Remove a session from the session list
 *
 * @param session The session to remove
 */
void SessionManager::removeSession(std::shared_ptr<Session> session)
{
    if (!session)
        return;

    if (m_activeSession.lock() == session) {
        m_activeSession.reset();
        Helper::instance()->activateSurface(nullptr);
    }

    for (auto s : std::as_const(m_sessions)) {
        if (s.get() == session.get()) {
            m_sessions.removeOne(s);
            break;
        }
    }

    session.reset();
}

/**
 * Ensure a session exists for the given username, creating it if necessary
 *
 * @param id An existing logind session ID
 * @param uid Username to ensure session for
 * @returns Session for the given username, or nullptr on failure
 */
std::shared_ptr<Session> SessionManager::ensureSession(int id, QString username)
{
    // Helper lambda to create WSocket and WXWayland
    auto createWSocket = [this]() {
        // Create WSocket
        auto socket = new WSocket(true, this);
        if (!socket->autoCreate()) {
            qCCritical(treelandCore) << "Failed to create Wayland socket";
            delete socket;
            return static_cast<WSocket *>(nullptr);
        }
        // Connect signals
        connect(socket, &WSocket::fullServerNameChanged, this, [this] {
            if (m_activeSession.lock())
                Q_EMIT socketFileChanged();
        });
        // Add socket to server
        Helper::instance()->addSocket(socket);
        return socket;
    };
    auto createXWayland = [this](WSocket *socket) {
        // Create xwayland
        auto *xwayland = Helper::instance()->createXWayland();
        if (!xwayland) {
            qCCritical(treelandCore) << "Failed to create XWayland instance";
            return static_cast<WXWayland *>(nullptr);
        }
        // Bind xwayland to socket
        xwayland->setOwnsSocket(socket);
        // Connect signals
        connect(xwayland, &WXWayland::ready, this, [this, xwayland] {
            if (auto session = sessionForXWayland(xwayland)) {
                session->noTitlebarAtom =
                    internAtom(session->xwayland->xcbConnection(), _DEEPIN_NO_TITLEBAR, false);
                if (!session->noTitlebarAtom) {
                    qCWarning(treelandInput) << "Failed to intern atom:" << _DEEPIN_NO_TITLEBAR;
                }
                session->settingManager = new SettingManager(session->xwayland->xcbConnection(),
                                                             session->xwayland);
                session->settingManagerThread = new QThread(session->xwayland);

                session->settingManager->moveToThread(session->settingManagerThread);

                const qreal scale = Helper::instance()->rootSurfaceContainer()->window()->effectiveDevicePixelRatio();
                const auto renderWindow = Helper::instance()->window();
                connect(session->settingManagerThread, &QThread::started, this, [session, scale, renderWindow]{
                    QMetaObject::invokeMethod(session->settingManager, [session, scale]() {
                            session->settingManager->setGlobalScale(scale);
                            session->settingManager->apply();
                        }, Qt::QueuedConnection);

                    QObject::connect(renderWindow,
                        &WOutputRenderWindow::effectiveDevicePixelRatioChanged,
                        session->settingManager,
                        [session](qreal dpr) {
                            session->settingManager->setGlobalScale(dpr);
                            session->settingManager->apply();
                        }, Qt::QueuedConnection);
                });

                connect(session->settingManagerThread, &QThread::finished, session->settingManagerThread, &QThread::deleteLater);
                session->settingManagerThread->start();
            }
        });
        return xwayland;
    };
    // Check if session already exists for user
    if (auto session = sessionForUser(username)) {
        // Ensure it has a socket and xwayland
        if (!session->socket) {
            auto *socket = createWSocket();
            if (!socket) {
                m_sessions.removeOne(session);
                return nullptr;
            }
            session->socket = socket;
        }
        if (!session->xwayland) {
            auto *xwayland = createXWayland(session->socket);
            if (!xwayland) {
                delete session->socket;
                m_sessions.removeOne(session);
                return nullptr;
            }

            session->xwayland = xwayland;
        }

        return session;
    }
    // Session does not exist, create new session with deleter
    auto session = std::make_shared<Session>();
    session->id = id;
    session->username = username;
    session->uid = getpwnam(username.toLocal8Bit().data())->pw_uid;

    session->socket = createWSocket();
    if (!session->socket) {
        session.reset();
        return nullptr;
    }

    session->xwayland = createXWayland(session->socket);
    if (!session->xwayland) {
        session.reset();
        return nullptr;
    }

    // Add session to list
    m_sessions.append(session);
    return session;
}


/**
 * Find the session for the given logind session id
 *
 * @param uid Session ID to find session for
 * @returns Session for the given id, or nullptr if not found
 */
std::shared_ptr<Session> SessionManager::sessionForId(int id) const
{
    for (auto session : m_sessions) {
        if (session && session->id == id)
            return session;
    }
    return nullptr;
}

/**
 * Find the session for the given uid
 *
 * @param uid User ID to find session for
 * @returns Session for the given uid, or nullptr if not found
 */
std::shared_ptr<Session> SessionManager::sessionForUid(uid_t uid) const
{
    for (auto session : m_sessions) {
        if (session && session->uid == uid)
            return session;
    }
    return nullptr;
}

/**
 * Find the session for the given username
 *
 * @param uid Username to find session for
 * @returns Session for the given username, or nullptr if not found
 */
std::shared_ptr<Session> SessionManager::sessionForUser(const QString &username) const
{
    for (auto session : m_sessions) {
        if (session && session->username == username)
            return session;
    }
    return nullptr;
}

/**
 * Find the session for the given WXWayland
 *
 * @param xwayland WXWayland to find session for
 * @returns Session for the given xwayland, or nullptr if not found
 */
std::shared_ptr<Session> SessionManager::sessionForXWayland(WXWayland *xwayland) const
{
    for (auto session : m_sessions) {
        if (session && session->xwayland == xwayland)
            return session;
    }
    return nullptr;
}

/**
 * Find the session for the given WSocket
 *
 * @param socket WSocket to find session for
 * @returns Session for the given socket, or nullptr if not found
 */
std::shared_ptr<Session> SessionManager::sessionForSocket(WSocket *socket) const
{
    for (auto session : m_sessions) {
        if (session && session->socket == socket)
            return session;
    }
    return nullptr;
}

/**
 * Get the WXWayland for the given uid
 *
 * @param uid User ID to get WXWayland for
 * @returns WXWayland for the given uid, or nullptr if not found/created
 */
WXWayland *SessionManager::xwaylandForUid(uid_t uid) const
{
    auto session = sessionForUid(uid);
    return session ? session->xwayland : nullptr;
}

/**
 * Get the WSocket for the given uid
 *
 * @param uid User ID to get WSocket for
 * @returns WSocket for the given uid, or nullptr if not found/created
 */
WSocket *SessionManager::waylandSocketForUid(uid_t uid) const
{
    auto session = sessionForUid(uid);
    return session ? session->socket : nullptr;
}

/**
 * Get the global WSocket, which is not relative with any session and
 * always available.
 *
 * @returns The global WSocket, or nullptr if it's not created yet.
 */
WSocket *SessionManager::globalWaylandSocket() const
{
    return waylandSocketForUid(getpwnam("dde")->pw_uid);
}

/**
 * Get the global WXWayland, which is not relative with any session and
 * always available.
 *
 * @returns The global WXWayland, or nullptr if none active
 */
WXWayland *SessionManager::globalXWayland() const
{
    return xwaylandForUid(getpwnam("dde")->pw_uid);
}

/**
 * Update the active session to the given uid, creating it if necessary.
 * This will update XWayland visibility and emit socketFileChanged if the
 * active session changed.
 *
 * @param username Username to set as active session
 */
void SessionManager::updateActiveUserSession(const QString &username, int id)
{
    // Get previous active session
    auto previous = m_activeSession.lock();
    // Get new session for uid, creating if necessary
    auto session = ensureSession(id, username);
    if (!session) {
        qCWarning(treelandInput) << "Failed to ensure session for user" << username;
        return;
    }
    if (previous != session) {
        // Update active session
        m_activeSession = session;
        // Clear activated surface
        Helper::instance()->activateSurface(nullptr);
        // Emit signal and update socket enabled state
        if (previous && previous->socket)
            previous->socket->setEnabled(false);
        session->socket->setEnabled(true);
        Q_EMIT socketFileChanged();
        // Notify session changed through DBus, treeland-sd will listen it to update envs
        Q_EMIT sessionChanged();
    }
    qCInfo(treelandCore) << "Listing on:" << session->socket->fullServerName();
}

