#pragma once

#include "wglobal.h"
#include "wsessionlocksurface.h"
#include <qwsessionlockv1.h>
#include <QtCore/qtmetamacros.h>
#include <QtQmlIntegration/qqmlintegration.h>

WAYLIB_SERVER_BEGIN_NAMESPACE

class WSessionLockPrivate;
class WAYLIB_SERVER_EXPORT WSessionLock : public WWrapObject
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WSessionLock)

    Q_PROPERTY(bool locked READ locked NOTIFY lockedChanged);

    QML_NAMED_ELEMENT(WaylandSessionLock)
    QML_UNCREATABLE("Only create in C++")
public:
    explicit WSessionLock(qw_session_lock_v1 *handle, QObject *parent = nullptr);
    ~WSessionLock();
    static WSessionLock *fromHandle(qw_session_lock_v1 *handle);

    Q_INVOKABLE void lock();
    Q_INVOKABLE void finish();
    bool locked() const;
    qw_session_lock_v1 *handle() const;
    
    QList<WSessionLockSurface*> surfaceList() const;
Q_SIGNALS:
    void surfaceCreated(WSessionLockSurface *surface);
    void surfaceRemoved(WSessionLockSurface *surface);
    void unlocked();
    void lockedChanged();
};

WAYLIB_SERVER_END_NAMESPACE
