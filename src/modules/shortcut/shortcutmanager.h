// Copyright (C) 2023 Dingyuan Zhang <zhangdingyuan@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <wserver.h>
#include "modules/shortcut/impl/shortcut_manager_impl.h"
#include <QObject>
#include <QQmlEngine>

class QAction;

WAYLIB_SERVER_BEGIN_NAMESPACE
class WServer;
WAYLIB_SERVER_END_NAMESPACE

WAYLIB_SERVER_USE_NAMESPACE

class ShortcutV1
    : public QObject
    , public WAYLIB_SERVER_NAMESPACE::WServerInterface
{
    Q_OBJECT
public:
    explicit ShortcutV1(QObject *parent = nullptr);
    QByteArrayView interfaceName() const override;
    bool handleKeySequence(uid_t uid, const QKeySequence &sequence);

protected:
    void create(WServer *server) override;
    void destroy(WServer *server) override;
    wl_global *global() const override;

Q_SIGNALS:
    void requestCompositorAction(treeland_shortcut_v1_action action);

private Q_SLOTS:
    void onNewShortcut(treeland_shortcut_v1 *shortcut);

private:
    treeland_shortcut_manager_v1 *m_manager = nullptr;
    void activateShortcut(treeland_shortcut_v1 *shortcut);
};

// Q_DECLARE_FLAGS(MetaKeyChecks, ShortcutV1::MetaKeyCheck)
// Q_DECLARE_OPERATORS_FOR_FLAGS(MetaKeyChecks)
