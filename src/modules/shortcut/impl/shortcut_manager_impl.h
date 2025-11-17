// Copyright (C) 2023-2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include <QObject>
#include <QMap>
#include <QString>
#include <QKeySequence>

#include "qwglobal.h"

QW_BEGIN_NAMESPACE
class qw_display;
QW_END_NAMESPACE

struct wl_global;
struct wl_resource;

class treeland_shortcut_manager_v2 : public QObject
{
    Q_OBJECT
public:
    ~treeland_shortcut_manager_v2();
    static treeland_shortcut_manager_v2* create(qw_display *display);

    void sendActivated(uid_t uid, const QString& name);
    void sendCommitStatus(uid_t uid, bool success);
    void sendInvalidCommit(uid_t uid);

    wl_global *global = nullptr;

    QMap<uid_t, wl_resource*> ownerClients;

Q_SIGNALS:
    void requestUnregisterShortcut(uid_t uid, const QString& name);
    void requestBindKeySequence(uid_t uid, const QString& name, const QKeySequence& keySequence, uint action);
    void requestBindSwipeGesture(uid_t uid, const QString& name, uint finger, uint direction, uint action);
    void requestBindHoldGesture(uid_t uid, const QString& name, uint finger, uint action);
    void requestCommit(uid_t uid);
    void before_destroy();
};
