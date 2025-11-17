// Copyright (C) 2023 Dingyuan Zhang <zhangdingyuan@uniontech.com>.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

// #include <wayland-server-core.h>
#include "treeland-shortcut-manager-protocol.h"
#include "input/gestures.h"

#include <qwdisplay.h>

#include <QList>
#include <QObject>

#include <functional>
#include <memory>
#include <unordered_map>

struct treeland_shortcut_manager_v1 : public QObject
{
    Q_OBJECT
public:
    ~treeland_shortcut_manager_v1();

    static treeland_shortcut_manager_v1 *create(QW_NAMESPACE::qw_display *display);
    
    wl_global *global{ nullptr };
    QMap<uid_t, QMap<QKeySequence, treeland_shortcut_v1 *>> keyMap;
    QMap<uid_t, QSet<std::pair<SwipeGesture::Direction, uint>>> bindedGestures;
Q_SIGNALS:
    void newShortcut(treeland_shortcut_v1 *shortcut);
    void before_destroy();
};

struct treeland_shortcut_v1 : public QObject
{
    Q_OBJECT
public:
    ~treeland_shortcut_v1();
    static treeland_shortcut_v1 *from_resource(struct wl_resource *resource);

    treeland_shortcut_manager_v1 *manager{ nullptr };

    wl_resource *resource{ nullptr };

    uid_t uid = 0;

    QList<treeland_shortcut_v1_action> actions;

    std::unordered_map<uint, std::unique_ptr<void, std::function<void(void*)>>> bindings;

    uint newId();

    void sendBindSuccess(uint binding_id);

Q_SIGNALS:
    void before_destroy();
    void gestureActivated();
public Q_SLOTS:
    void sendActivated();
private:
};
