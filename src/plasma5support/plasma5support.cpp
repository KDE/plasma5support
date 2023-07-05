/*
    SPDX-FileCopyrightText: 2005 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <plasma5support/plasma5support.h>

#include <QAction>
#include <QMenu>

namespace Plasma5Support
{
Types::Types(QObject *parent)
    : QObject(parent)
{
}

Types::~Types()
{
}

} // Plasma5Support namespace

#include "moc_plasma5support.cpp"
