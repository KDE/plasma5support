/*
    SPDX-FileCopyrightText: 2010 Ryan Rix <ry@n.rix.si>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pluginloader.h"

#include <QPluginLoader>
#include <QPointer>
#include <QStandardPaths>

#include <KRuntimePlatform>
#include <QDebug>
#include <QRegularExpression>

#include "dataengine.h"
#include "private/service_p.h" // for NullService
#include "private/storage_p.h"

namespace Plasma5Support
{
PluginLoader::~PluginLoader() = default;

Service *PluginLoader::loadService(const QString &name, QObject *parent)
{
    const static QString s_servicesPluginDir = QStringLiteral("plasma5support/services");
    Service *service = nullptr;

    // TODO: scripting API support
    if (name.isEmpty()) {
        return new NullService(QString(), parent);
    } else if (name == QLatin1String("org.kde.servicestorage")) {
        return new Storage(parent);
    }

    // Look for C++ plugins first
    KPluginMetaData plugin = KPluginMetaData::findPluginById(s_servicesPluginDir, name);
    if (plugin.isValid()) {
        service = KPluginFactory::instantiatePlugin<Plasma5Support::Service>(plugin, parent).plugin;
    }

    if (service) {
        if (service->name().isEmpty()) {
            service->setName(name);
        }
        return service;
    } else {
        return new NullService(name, parent);
    }
}

QList<KPluginMetaData> PluginLoader::listDataEngineMetaData(const QString &parentApp)
{
    const static QString s_dataEnginePluginDir = QStringLiteral("plasma5support/dataengine");
    if (parentApp.isEmpty()) {
        return KPluginMetaData::findPlugins(s_dataEnginePluginDir);
    } else {
        return KPluginMetaData::findPlugins(s_dataEnginePluginDir, [&parentApp](const KPluginMetaData &md) -> bool {
            return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp;
        });
    }
}

} // Plasma Namespace
