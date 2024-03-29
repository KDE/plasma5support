/*
    SPDX-FileCopyrightText: 2024 Nicolas Fella <nicolas.fella@gmx.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <QQmlEngine>
#include <QQmlPropertyMap>

#include "plasma5support.h"

struct PropertyMapForeign {
  Q_GADGET
  QML_ANONYMOUS
  QML_FOREIGN(QQmlPropertyMap)
};

struct TypesForeign {
  Q_GADGET
  QML_NAMED_ELEMENT(Types)
  QML_UNCREATABLE("")
  QML_FOREIGN(Plasma5Support::Types)
};
