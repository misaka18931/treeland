// Copyright (C) 2023 Dingyuan Zhang <zhangdingyuan@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "shortcutmanager.h"
#include "seat/helper.h"

#include "modules/shortcut/impl/shortcut_manager_impl.h"

#include <wayland-server-core.h>
#include <wayland-util.h>

#include <qwdisplay.h>
#include <QAction>

#include <pwd.h>
#include <sys/socket.h>
#include <unistd.h>

ShortcutV1::ShortcutV1(QObject *parent)
    : QObject(parent)
{
}

void ShortcutV1::onNewShortcut(treeland_shortcut_v1 *shortcut)
{
    connect(shortcut, &treeland_shortcut_v1::gestureActivated, this, [this, shortcut]() {
        activateShortcut(shortcut);
    });
}

void ShortcutV1::create(WServer *server)
{
    m_manager = treeland_shortcut_manager_v1::create(server->handle());
    connect(m_manager, &treeland_shortcut_manager_v1::newShortcut, this, &ShortcutV1::onNewShortcut);
}

void ShortcutV1::destroy([[maybe_unused]] WServer *server)
{

}

wl_global *ShortcutV1::global() const
{
    return m_manager->global;
}

QByteArrayView ShortcutV1::interfaceName() const
{
    return "treeland_shortcut_manager_v1";
}

bool ShortcutV1::handleKeySequence(uid_t uid, const QKeySequence &sequence)
{
    if (m_manager->keyMap.contains(uid) && m_manager->keyMap[uid].contains(sequence)) {
        activateShortcut(m_manager->keyMap[uid][sequence]);
        return true;
    }
    return false;
}

void ShortcutV1::activateShortcut(treeland_shortcut_v1 *shortcut)
{
    for (auto action : shortcut->actions)
        Q_EMIT requestCompositorAction(action);
    shortcut->sendActivated();
}
