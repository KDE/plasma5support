/*
    SPDX-FileCopyrightText: 2010 Ryan Rix <ry@n.rix.si>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pluginloader.h"

#include <QPluginLoader>
#include <QPointer>
#include <QStandardPaths>

#include <KLazyLocalizedString>
#include <KRuntimePlatform>
#include <KService>
#include <QDebug>
#include <QRegularExpression>
#include <kcoreaddons_export.h>
#include <kpackage/packageloader.h>

#include "config-plasma5support.h"

#include "dataengine.h"
#include "debug_p.h"
#include "private/service_p.h" // for NullService
#include "private/storage_p.h"

namespace Plasma5Support
{
inline bool isContainmentMetaData(const KPluginMetaData &md)
{
    return md.rawData().contains(QStringLiteral("X-Plasma-ContainmentType"));
}
static PluginLoader *s_pluginLoader = nullptr;

class PluginLoaderPrivate
{
public:
    PluginLoaderPrivate()
        : isDefaultLoader(false)
    {
    }

    static QSet<QString> s_customCategories;
    bool isDefaultLoader;

    static QString s_dataEnginePluginDir;
    static QString s_packageStructurePluginDir;
    static QString s_plasmoidsPluginDir;
    static QString s_servicesPluginDir;

    class Cache
    {
        // We only use this cache during start of the process to speed up many consecutive calls
        // After that, we're too afraid to produce race conditions and it's not that time-critical anyway
        // the 20 seconds here means that the cache is only used within 20sec during startup, after that,
        // complexity goes up and we'd have to update the cache in order to avoid subtle bugs
        // just not using the cache is way easier then, since it doesn't make *that* much of a difference,
        // anyway
        int maxCacheAge = 20;
        qint64 pluginCacheAge = 0;
        QHash<QString, KPluginMetaData> plugins;

    public:
        KPluginMetaData findPluginById(const QString &name, const QString &pluginNamespace);
    };
    Cache plasmoidCache;
    Cache dataengineCache;
};

QSet<QString> PluginLoaderPrivate::s_customCategories;

QString PluginLoaderPrivate::s_dataEnginePluginDir = QStringLiteral("plasma5support/dataengine");
QString PluginLoaderPrivate::s_packageStructurePluginDir = QStringLiteral("plasma5support/packagestructure");
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
    if (!s_pluginLoader) {
        // we have been called before any PluginLoader was set, so just use the default
        // implementation. this prevents plugins from nefariously injecting their own
        // plugin loader if the app doesn't
        s_pluginLoader = new PluginLoader;
        s_pluginLoader->d->isDefaultLoader = true;
    }

    return s_pluginLoader;
}

Service *PluginLoader::loadService(const QString &name, const QVariantList &args, QObject *parent)
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
        service = KPluginFactory::instantiatePlugin<Plasma5Support::Service>(plugin, parent, args).plugin;
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
    auto filter = [&parentApp](const KPluginMetaData &md) -> bool {
        return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp;
    };

    QVector<KPluginMetaData> plugins;
    if (parentApp.isEmpty()) {
        plugins = KPluginMetaData::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir);
    } else {
        plugins = KPluginMetaData::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir, filter);
    }

    return plugins;
}

KPluginMetaData PluginLoaderPrivate::Cache::findPluginById(const QString &name, const QString &pluginNamespace)
{
    const qint64 now = qRound64(QDateTime::currentMSecsSinceEpoch() / 1000.0);
    bool useRuntimeCache = true;

    if (pluginCacheAge == 0) {
        // Find all the plugins now, but only once
        pluginCacheAge = now;

        const auto metaDataList = KPluginMetaData::findPlugins(pluginNamespace, {}, KPluginMetaData::AllowEmptyMetaData);
        for (const KPluginMetaData &metadata : metaDataList) {
            plugins.insert(metadata.pluginId(), metadata);
        }
    } else if (now - pluginCacheAge > maxCacheAge) {
        // cache is old and we're not within a few seconds of startup anymore
        useRuntimeCache = false;
        plugins.clear();
    }

    // if name wasn't a path, pluginName == name
    const QString pluginName = name.section(QLatin1Char('/'), -1);

    if (useRuntimeCache) {
        KPluginMetaData data = plugins.value(name);
        qCDebug(LOG_PLASMA5SUPPORT) << "loading dataengnine by name" << name << useRuntimeCache << data.isValid();
        return data;
    } else {
        const QVector<KPluginMetaData> offers = KPluginMetaData::findPlugins(
            pluginNamespace,
            [&pluginName](const KPluginMetaData &data) {
                return data.pluginId() == pluginName;
            },
            KPluginMetaData::AllowEmptyMetaData);
        return offers.isEmpty() ? KPluginMetaData() : offers.first();
    }
}

} // Plasma Namespace
