// Copyright (C) 2025 UnionTech Software Technology Co., Ltd.
// SPDX-License-Identifier: Apache-2.0 OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "shortcutcontroller.h"
#include "input/inputdevice.h"
#include "input/gestures.h"

ShortcutController::ShortcutController(QObject *parent)
    : QObject(parent)
{
}

ShortcutController::~ShortcutController()
{
    clear();
}

bool ShortcutController::registerKeySequence(const QString &name, const QKeySequence &sequence, ShortcutAction action)
{
    if (m_deleters.contains(name)) {
        return false;
    }

    if (sequence.count() != 1) {
        return false;
    }
    
    auto &entry = m_keymap[sequence];

    if (entry.contains(action)) {
        return false;
    }

    entry.insert(action, name);

    m_deleters[name] = [this, sequence, action]() {
        m_keymap[sequence].remove(action);
    };

    return true;
}

bool ShortcutController::registerSwipeGesture(const QString &name, uint finger, SwipeGesture::Direction direction, ShortcutAction action)
{
    if (direction == SwipeGesture::Direction::Invalid) {
        return false;
    }

    auto gestureKey = std::make_pair(finger, direction);

    if (!m_gestures.contains(gestureKey)) {
        auto gesture = InputDevice::instance()->registerTouchpadSwipe(
            SwipeFeedBack {
                direction,
                finger,
                [this, gestureKey, name]() {
                    for (const auto& [act, nm] : m_gesturemap[gestureKey].asKeyValueRange()) {
                        emit actionFinished(act, nm);
                    }
                },
                [this, gestureKey, name](qreal progress) {
                    for (const auto& [act, nm] : m_gesturemap[gestureKey].asKeyValueRange()) {
                        emit actionProgress(act, progress, nm);
                    }
                }
        });

        if (!gesture) {
            return false;
        }

        m_gestures[gestureKey] = gesture;
    }

    auto &entry = m_gesturemap[gestureKey];

    if (entry.contains(action)) {
        return false;
    }

    entry.insert(action, name);
    m_deleters[name] = [this, gestureKey, action]() {
        auto &entry = m_gesturemap[gestureKey];
        entry.remove(action);
        if (entry.isEmpty()) {
            auto gesture = qobject_cast<SwipeGesture*>(m_gestures.take(gestureKey));
            if (gesture) {
                InputDevice::instance()->unregisterTouchpadSwipe(gesture);
            }
        }
    };

    return true;
}

bool ShortcutController::registerHoldGesture(const QString &name, uint finger, ShortcutAction action)
{
    auto gestureKey = std::make_pair(finger, SwipeGesture::Direction::Invalid);

    if (!m_gestures.contains(gestureKey)) {
        auto gesture = InputDevice::instance()->registerTouchpadHold(
            HoldFeedBack {
                finger,
                nullptr,
                [this, gestureKey, name]() {
                    for (const auto& [act, nm] : m_gesturemap[gestureKey].asKeyValueRange()) {
                        emit actionTriggered(act, nm);
                    }
                }
        });

        if (!gesture) {
            return false;
        }

        m_gestures[gestureKey] = gesture;
    }

    auto &entry = m_gesturemap[gestureKey];

    if (entry.contains(action)) {
        return false;
    }

    entry.insert(action, name);
    m_deleters[name] = [this, gestureKey, action]() {
        auto &entry = m_gesturemap[gestureKey];
        entry.remove(action);
        if (entry.isEmpty()) {
            auto gesture = qobject_cast<HoldGesture*>(m_gestures.take(gestureKey));
            if (gesture) {
                InputDevice::instance()->unregisterTouchpadHold(gesture);
            }
        }
    };

    return true;
}

void ShortcutController::unregisterShortcut(const QString &name)
{
    if (m_deleters.contains(name)) {
        m_deleters.take(name)();
    }
}

bool ShortcutController::dispatchKeySequence(const QKeySequence &sequence)
{
    if (m_keymap.contains(sequence)) {
        for (const auto &[action, name] : m_keymap[sequence].asKeyValueRange()) {
            emit actionTriggered(action, name);
        }
        return true;
    }
    return false;
}

void ShortcutController::clear()
{
    for (const auto& deleter : m_deleters) {
        deleter();
    }
    m_deleters.clear();
}
