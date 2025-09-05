#pragma once

#include "wglobal.h"
#include "wserver.h"
#include "wsessionlock.h"
#include <qwsessionlockv1.h>
WAYLIB_SERVER_BEGIN_NAMESPACE

class WSessionLockManagerPrivate;
class WAYLIB_SERVER_EXPORT WSessionLockManager : public WWrapObject, public WServerInterface
{
    Q_OBJECT
    W_DECLARE_PRIVATE(WSessionLockManager)
public:
    explicit WSessionLockManager(QObject *parent = nullptr);
    void *create();

    QByteArrayView interfaceName() const override;
    
    QList<WSessionLock*> lockList() const;
Q_SIGNALS:
    void lockCreated(WSessionLock *lock);
    void lockDestroyed(WSessionLock *lock);
protected:
    void create(WServer *server) override;
    void destroy(WServer *server) override;
    wl_global *global() const override;
};

WAYLIB_SERVER_END_NAMESPACE