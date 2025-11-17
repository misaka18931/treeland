// Copyright (C) 2023-2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "shortcutmanager.h"
#include "common/treelandlogging.h"
#include "shortcutcontroller.h"
#include "seat/helper.h"
#include "usermodel.h"
#include "input/gestures.h"

#include "modules/shortcut/impl/shortcut_manager_impl.h"

#include "treeland-shortcut-manager-protocol.h"

#include <qwdisplay.h>
#include <optional>

struct KeyShortcut {
    QString name;
    QKeySequence keySequence;
    ShortcutAction action;
};

struct SwipeShortcut {
    QString name;
    uint finger;
    SwipeGesture::Direction direction;
    ShortcutAction action;
};

struct HoldShortcut {
    QString name;
    uint finger;
    ShortcutAction action;
};

class UserShortcuts {
public:
    QList<KeyShortcut> keys;
    QList<SwipeShortcut> swipes;
    QList<HoldShortcut> holds;

    void append(const UserShortcuts &other)
    {
        keys.append(other.keys);
        swipes.append(other.swipes);
        holds.append(other.holds);
    }
};

static SwipeGesture::Direction toSwipeDirection(uint32_t direction)
{
    switch (direction) {
    case TREELAND_SHORTCUT_MANAGER_V2_DIRECTION_DOWN:
        return SwipeGesture::Direction::Down;
    case TREELAND_SHORTCUT_MANAGER_V2_DIRECTION_LEFT:
        return SwipeGesture::Direction::Left;
    case TREELAND_SHORTCUT_MANAGER_V2_DIRECTION_UP:
        return SwipeGesture::Direction::Up;
    case TREELAND_SHORTCUT_MANAGER_V2_DIRECTION_RIGHT:
        return SwipeGesture::Direction::Right;
    default:
        return SwipeGesture::Direction::Invalid;
    }
}

class ShortcutManagerV2Private {
public:
    ShortcutManagerV2Private(ShortcutManagerV2 *q) : q_ptr(q) {}

    bool updateShortcuts(const UserShortcuts& shortcuts);

    treeland_shortcut_manager_v2 *m_manager = nullptr;
    ShortcutController *m_controller = nullptr;

    QMap<uid_t, UserShortcuts> m_shortcuts;
    QMap<uid_t, UserShortcuts> m_pendingShortcuts;
    QMap<uid_t, UserShortcuts> m_pendingCommittedShortcuts;
    QMap<uid_t, QList<QString>> m_pendingDeletes;

    uid_t m_activeUid = -1;

    ShortcutManagerV2 *q_ptr;
    Q_DECLARE_PUBLIC(ShortcutManagerV2)
};

bool ShortcutManagerV2Private::updateShortcuts(const UserShortcuts& shortcuts)
{
    QList<QString> names;

    auto tryRegisterAll = [&]() {
        for (const auto& [name, keySequence, action] : shortcuts.keys) {
            if (!m_controller->registerKeySequence(name, keySequence, action))
                return false;
            names.append(name);
        }
        for (const auto& [name, finger, direction, action] : shortcuts.swipes) {
            if (!m_controller->registerSwipeGesture(name, finger, direction, action))
                return false;
            names.append(name);
        }
        for (const auto& [name, finger, action] : shortcuts.holds) {
            if (!m_controller->registerHoldGesture(name, finger, action))
                return false;
            names.append(name);
        }
        return true;
    };

    if (!tryRegisterAll()) {
        for (const auto& name : names) {
            m_controller->unregisterShortcut(name);
        }
        return false;
    }
    return true;
}

ShortcutManagerV2::ShortcutManagerV2(QObject *parent)
    : QObject(parent)
    , d_ptr(new ShortcutManagerV2Private(this))
{
    Q_D(ShortcutManagerV2);
    d->m_controller = new ShortcutController(this);
}

ShortcutManagerV2::~ShortcutManagerV2() = default;

void ShortcutManagerV2::create(WServer *server)
{
    Q_D(ShortcutManagerV2);
    d->m_manager = treeland_shortcut_manager_v2::create(server->handle());
    Q_ASSERT(d->m_manager);

    connect(d->m_manager, &treeland_shortcut_manager_v2::requestUnregisterShortcut,
            this, &ShortcutManagerV2::handleUnregisterShortcut);
    connect(d->m_manager, &treeland_shortcut_manager_v2::requestBindKeySequence,
            this, &ShortcutManagerV2::handleBindKeySequence);
    connect(d->m_manager, &treeland_shortcut_manager_v2::requestBindSwipeGesture,
            this, &ShortcutManagerV2::handleBindSwipeGesture);
    connect(d->m_manager, &treeland_shortcut_manager_v2::requestBindHoldGesture,
            this, &ShortcutManagerV2::handleBindHoldGesture);
    connect(d->m_manager, &treeland_shortcut_manager_v2::requestCommit,
            this, &ShortcutManagerV2::handleCommit);
    
}

void ShortcutManagerV2::destroy(WServer *server)
{
    Q_UNUSED(server);
    Q_EMIT before_destroy();
}

wl_global *ShortcutManagerV2::global() const
{
    Q_D(const ShortcutManagerV2);
    return d->m_manager->global;
}

QByteArrayView ShortcutManagerV2::interfaceName() const
{
    return "treeland_shortcut_manager_v2";
}

ShortcutController* ShortcutManagerV2::controller()
{
    Q_D(ShortcutManagerV2);
    return d->m_controller;
}

void ShortcutManagerV2::sendActivated(const QString& name)
{
    Q_D(ShortcutManagerV2);
    d->m_manager->sendActivated(d->m_activeUid, name);
}

void ShortcutManagerV2::onSessionChanged()
{
    Q_D(ShortcutManagerV2);
    uid_t uid = Helper::instance()->userModel()->currentUser()->UID();
    
    if (d->m_activeUid == uid)
        return;

    d->m_controller->clear();
    d->m_activeUid = uid;
    
    if (d->m_shortcuts.contains(uid)) {
        bool ok = d->updateShortcuts(d->m_shortcuts[uid]);
        if (!ok) {
            qCWarning(treelandShortcut) << "Failed to restore shortcuts for user" << uid;
        }
        return;
    }

    if (d->m_pendingDeletes.contains(uid)) {
        const auto names = d->m_pendingDeletes.take(uid);
        for (const auto& name : names) {
            d->m_controller->unregisterShortcut(name);
        }
    }

    if (d->m_pendingCommittedShortcuts.contains(uid)) {
        const auto pendingShortcuts = d->m_pendingCommittedShortcuts.take(uid);
        bool ok = d->updateShortcuts(pendingShortcuts);
        if (ok) {
            d->m_shortcuts[uid].append(pendingShortcuts);
        }
        d->m_manager->sendCommitStatus(uid, ok);
    }
}

void ShortcutManagerV2::handleCommit(uid_t uid)
{
    Q_D(ShortcutManagerV2);
    if (!d->m_pendingShortcuts.contains(uid)) {
        d->m_manager->sendCommitStatus(uid, true);
        return;
    }

    if (uid != d->m_activeUid) {
        if (d->m_pendingCommittedShortcuts.contains(uid)) {
            d->m_manager->sendInvalidCommit(uid);
            return;
        }
        d->m_pendingCommittedShortcuts[uid] = d->m_pendingShortcuts.take(uid);
        return;
    }

    const auto pendingShortcuts = d->m_pendingShortcuts.take(uid);
    bool ok = d->updateShortcuts(pendingShortcuts);
    if (ok) {
        d->m_shortcuts[uid].append(pendingShortcuts);
    }
    d->m_manager->sendCommitStatus(uid, ok);
}

void ShortcutManagerV2::handleBindHoldGesture(uid_t uid, const QString& name, uint finger, uint action)
{
    Q_D(ShortcutManagerV2);
    d->m_pendingShortcuts[uid].holds.append(HoldShortcut{
        .name = name,
        .finger = finger,
        .action = static_cast<ShortcutAction>(action),
    });
}

void ShortcutManagerV2::handleBindSwipeGesture(uid_t uid, const QString& name, uint finger, uint direction, uint action)
{
    Q_D(ShortcutManagerV2);

    d->m_pendingShortcuts[uid].swipes.append(SwipeShortcut{
        .name = name,
        .finger = finger,
        .direction = toSwipeDirection(direction),
        .action = static_cast<ShortcutAction>(action),
    });
}

void ShortcutManagerV2::handleBindKeySequence(uid_t uid, const QString& name, const QKeySequence& keySequence, uint action)
{
    Q_D(ShortcutManagerV2);
    d->m_pendingShortcuts[uid].keys.append(KeyShortcut{
        .name = name,
        .keySequence = keySequence,
        .action = static_cast<ShortcutAction>(action),
    });
}

void ShortcutManagerV2::handleUnregisterShortcut(uid_t uid, const QString& name)
{
    Q_D(ShortcutManagerV2);
    if (uid != d->m_activeUid) {
        d->m_pendingDeletes[uid].append(name);
        return;
    }

    d->m_controller->unregisterShortcut(name);
}
