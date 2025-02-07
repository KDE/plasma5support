/*
    SPDX-FileCopyrightText: 2007 Fredrik Höglund <fredrik@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#include "mouseengine.h"

#include <QCursor>

#if WITH_X11
#include "cursornotificationhandler.h"
#endif

MouseEngine::MouseEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
    , timerId(0)
#if WITH_X11
    , handler(nullptr)
#endif
{
    init();
}

MouseEngine::~MouseEngine()
{
    if (timerId)
        killTimer(timerId);
#if WITH_X11
    delete handler;
#endif
}

QStringList MouseEngine::sources() const
{
    QStringList list;

    list << QLatin1String("Position");
#if WITH_X11
    list << QLatin1String("Name");
#endif

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

#if WITH_X11
    handler = new CursorNotificationHandler;
    connect(handler, &CursorNotificationHandler::cursorNameChanged, this, &MouseEngine::updateCursorName);

    setData(QLatin1String("Name"), QVariant(handler->cursorName()));
#endif
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
