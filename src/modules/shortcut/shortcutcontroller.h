// Copyright (C) 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#pragma once

#include "shortcutmanager.h"
#include "input/gestures.h"

#include <QMap>
#include <QKeySequence>
class Gesture;

class ShortcutController : public QObject
{
    Q_OBJECT
public:
    explicit ShortcutController(QObject *parent = nullptr);
    ~ShortcutController() override;

    bool registerKeySequence(const QString &name, const QKeySequence &sequence, ShortcutAction action);
    bool registerSwipeGesture(const QString &name, uint finger, SwipeGesture::Direction direction, ShortcutAction action);
    bool registerHoldGesture(const QString &name, uint finger, ShortcutAction action);
    void unregisterShortcut(const QString &name);

    void clear();
    bool dispatchKeySequence(const QKeySequence &sequence);

Q_SIGNALS:
    void actionTriggered(ShortcutAction action, const QString &name);
    void actionProgress(ShortcutAction action, qreal progress, const QString &name);
    void actionFinished(ShortcutAction action, const QString &name);

private:
    QMap<QKeySequence, QMap<ShortcutAction, QString>> m_keymap;
    QMap<std::pair<uint, SwipeGesture::Direction>, QMap<ShortcutAction, QString>> m_gesturemap;
    QMap<std::pair<uint, SwipeGesture::Direction>, QObject*> m_gestures;
    QMap<QString, std::function<void()>> m_deleters;
};
