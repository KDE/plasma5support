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
class PluginLoaderPrivate
{
public:
    static QString s_dataEnginePluginDir;
    static QString s_servicesPluginDir;
};

QString PluginLoaderPrivate::s_dataEnginePluginDir = QStringLiteral("plasma5support/dataengine");
QString PluginLoaderPrivate::s_servicesPluginDir = QStringLiteral("plasma5support/services");

PluginLoader::PluginLoader()
    : d(new PluginLoaderPrivate)
{
}

PluginLoader::~PluginLoader()
{
    delete d;
}

PluginLoader *PluginLoader::self()
{
    static PluginLoader *s_pluginLoader = new PluginLoader();
    return s_pluginLoader;
}

Service *PluginLoader::loadService(const QString &name, QObject *parent)
{
    Service *service = nullptr;

    // TODO: scripting API support
    if (name.isEmpty()) {
        return new NullService(QString(), parent);
    } else if (name == QLatin1String("org.kde.servicestorage")) {
        return new Storage(parent);
    }

    // Look for C++ plugins first
    KPluginMetaData plugin = KPluginMetaData::findPluginById(PluginLoaderPrivate::s_servicesPluginDir, name);
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

QVector<KPluginMetaData> PluginLoader::listDataEngineMetaData(const QString &parentApp)
{
    if (parentApp.isEmpty()) {
        return KPluginMetaData::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir);
    } else {
        return KPluginMetaData::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir, [&parentApp](const KPluginMetaData &md) -> bool {
            return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp;
        });
    }
}

} // Plasma Namespace
