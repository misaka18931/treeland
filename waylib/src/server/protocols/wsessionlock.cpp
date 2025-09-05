#include "wsessionlock.h"

#include "wglobal.h"
#include "private/wglobal_p.h"
#include <qassert.h>
#include <qwsessionlockv1.h>
#include "qobject.h"
#include "wsessionlocksurface.h"

WAYLIB_SERVER_BEGIN_NAMESPACE

class Q_DECL_HIDDEN WSessionLockPrivate : public WWrapObjectPrivate
{
public:
    WWRAP_HANDLE_FUNCTIONS(qw_session_lock_v1, wlr_session_lock_v1)

    WSessionLockPrivate(WSessionLock *qq, qw_session_lock_v1 *handle)
        : WWrapObjectPrivate(qq)
    {
        initHandle(handle);
    }
    void instantRelease() override;
    // begin slot function
    void onNewSurface(qw_session_lock_surface_v1 *surface);
    void onSurfaceDestroy(qw_session_lock_surface_v1 *surface);
    // end slot function
    
    void init();
    void lock();

    W_DECLARE_PUBLIC(WSessionLock)

    QList<WSessionLockSurface*> surfaceList;
    bool m_locked = false;
};

void WSessionLockPrivate::instantRelease()
{
    W_Q(WSessionLock);
    auto list = surfaceList;
    surfaceList.clear();
    for (auto surface : list) {
        q->surfaceRemoved(surface);
        surface->safeDeleteLater();
    }
}

void WSessionLockPrivate::onNewSurface(qw_session_lock_surface_v1 *surface)
{
    W_Q(WSessionLock);
    WSessionLockSurface *lockSurface = new WSessionLockSurface(surface, q);
    lockSurface->setParent(q);
    Q_ASSERT(lockSurface->parent() == q);
    
    lockSurface->safeConnect(&qw_session_lock_surface_v1::before_destroy, q, [this, surface] {
        onSurfaceDestroy(surface);
    });

    surfaceList.append(lockSurface);
    emit q->surfaceCreated(lockSurface);
}

void WSessionLockPrivate::onSurfaceDestroy(qw_session_lock_surface_v1 *surface)
{
    W_Q(WSessionLock);
    WSessionLockSurface *lockSurface = WSessionLockSurface::fromHandle(surface);
    
    bool ok = surfaceList.removeOne(lockSurface);
    Q_ASSERT(ok);
    emit q->surfaceRemoved(lockSurface);
    lockSurface->safeDeleteLater();
}

void WSessionLockPrivate::init()
{
    W_Q(WSessionLock);
    handle()->set_data(this, q);
}

void WSessionLockPrivate::lock() {
    W_Q(WSessionLock);
    Q_ASSERT(!m_locked);
    handle()->send_locked();
    m_locked = true;
}

WSessionLock::WSessionLock(qw_session_lock_v1 *handle, QObject *parent)
    : WWrapObject(*new WSessionLockPrivate(this, handle), parent)
{
    W_D(WSessionLock);
    d->init();
    // connect new_surface and unlock signals in
    connect(handle, &qw_session_lock_v1::notify_new_surface, this, [d](wlr_session_lock_surface_v1 *surface) {
        d->onNewSurface(qw_session_lock_surface_v1::from(surface));
    });
    connect(handle, &qw_session_lock_v1::notify_unlock, this, [d, this]() {
        d->m_locked = false;
        emit unlocked();
    });
}

WSessionLock::~WSessionLock()
{

}

WSessionLock *WSessionLock::fromHandle(qw_session_lock_v1 *handle)
{
    return handle->get_data<WSessionLock>();
}

qw_session_lock_v1 *WSessionLock::handle() const
{
    W_DC(WSessionLock);
    return d->handle();
}

QList<WSessionLockSurface*> WSessionLock::surfaceList() const
{
    W_DC(WSessionLock);
    return d->surfaceList;
}

bool WSessionLock::locked() const
{
    W_DC(WSessionLock);
    return d->m_locked;
}

void WSessionLock::lock()
{
    W_D(WSessionLock);
    Q_ASSERT(!locked());
    d->lock();
}

// Finish the lock (deny locking request)
void WSessionLock::finish()
{
    Q_ASSERT(!locked());
    handle()->destroy();
}

WAYLIB_SERVER_END_NAMESPACE

// TODO: handle lock destruction
