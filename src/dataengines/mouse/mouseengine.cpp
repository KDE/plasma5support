/*
    SPDX-FileCopyrightText: 2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "mouseengine.h"

#include <QCursor>

MouseEngine::MouseEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
    , timerId(0)
{
    init();
}

MouseEngine::~MouseEngine()
{
    if (timerId)
        killTimer(timerId);
}

QStringList MouseEngine::sources() const
{
    QStringList list;

    list << QLatin1String("Position");

    return list;
}

void MouseEngine::init()
{
    if (!timerId)
        timerId = startTimer(40);

    // Init cursor position
    QPoint pos = QCursor::pos();
    setData(QLatin1String("Position"), QVariant(pos));
    lastPosition = pos;
}

void MouseEngine::timerEvent(QTimerEvent *)
{
    QPoint pos = QCursor::pos();

    if (pos != lastPosition) {
        setData(QLatin1String("Position"), QVariant(pos));
        lastPosition = pos;
    }
}

void MouseEngine::updateCursorName(const QString &name)
{
    setData(QLatin1String("Name"), QVariant(name));
}

K_PLUGIN_CLASS_WITH_JSON(MouseEngine, "plasma-dataengine-mouse.json")

#include "mouseengine.moc"
