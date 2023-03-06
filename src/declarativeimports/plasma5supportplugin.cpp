/*
    SPDX-FileCopyrightText: 2009 Alan Alpert <alan.alpert@nokia.com>
    SPDX-FileCopyrightText: 2010 Ménard Alexis <menard@kde.org>
    SPDX-FileCopyrightText: 2010 Marco Martin <mart@kde.org>
    SPDX-FileCopyrightText: 2013 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "plasma5supportplugin.h"

#include <QQmlContext>

#include "datamodel.h"
#include "datasource.h"
#include "serviceoperationstatus.h"

#include <plasma5support/servicejob.h>

// #include "dataenginebindings_p.h"

#include <QDebug>
#include <QWindow>

void Plasma5SupportPlugin::registerTypes(const char *uri)
{
    Q_ASSERT(uri == QByteArray("org.kde.plasma.plasma5support"));

    qmlRegisterUncreatableType<Plasma5Support::Types>(uri, 2, 0, "Types", {});

    qmlRegisterType<Plasma5Support::DataSource>(uri, 2, 0, "DataSource");
    qmlRegisterType<Plasma5Support::DataModel>(uri, 2, 0, "DataModel");
    qmlRegisterType<Plasma5Support::SortFilterModel, 0>(uri, 2, 0, "SortFilterModel");
    qmlRegisterType<Plasma5Support::SortFilterModel, 1>(uri, 2, 1, "SortFilterModel");

    qmlRegisterType<ServiceOperationStatus>(uri, 2, 0, "ServiceOperationStatus");
    qmlRegisterAnonymousType<QAbstractItemModel>(uri, 1);

    qmlRegisterAnonymousType<QQmlPropertyMap>(uri, 1);
}
