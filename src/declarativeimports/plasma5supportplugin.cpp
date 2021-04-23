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

    // KF6: check if it makes sense to call qmlRegisterInterface for any of these
    // as they seem currently not used as properties and are only used from JavaScript engine
    // due to being return types of Q_INVOKABLE methods,
    // so registering the pointers to the qobject meta-object system would be enough:
    // Plasma5Support::Service, Plasma5Support::ServiceJob
    // So this here would become just
    // qRegisterMetaType<Plasma5Support::Service *>();
    // qRegisterMetaType<Plasma5Support::ServiceJob *>();
    // For that also change all usages with those methods to use the fully namespaced type name
    // in the method signature.
    QT_WARNING_PUSH
    QT_WARNING_DISABLE_CLANG("-Wdeprecated-declarations")
    QT_WARNING_DISABLE_GCC("-Wdeprecated-declarations")
    // Do not to port these two for KF5 to
    // qmlRegisterInterface<Plasma5Support::T>(uri, 1);
    // as this will incompatibly register with the fully namespaced name "Plasma5Support::T",
    // not just the now explicitly passed alias name "T"
    qmlRegisterInterface<Plasma5Support::Service>("Service");
    qmlRegisterInterface<Plasma5Support::ServiceJob>("ServiceJob");
    QT_WARNING_POP

    qmlRegisterType<ServiceOperationStatus>(uri, 2, 0, "ServiceOperationStatus");
    qmlRegisterAnonymousType<QAbstractItemModel>(uri, 1);

    qmlRegisterAnonymousType<QQmlPropertyMap>(uri, 1);
}
