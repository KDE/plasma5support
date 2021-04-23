/*
    SPDX-FileCopyrightText: 2010 Ryan Rix <ry@n.rix.si>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "pluginloader.h"

#include <QPointer>
#include <QStandardPaths>

#include <KPluginLoader>
#include <KPluginTrader>
#include <KService>
#include <KServiceTypeTrader>
#include <QDebug>
#include <kpackage/packageloader.h>

#include "config-plasma5support.h"

#include "dataengine.h"
#include "debug_p.h"
#include "private/service_p.h" // for NullService
#include "private/storage_p.h"
#include <plasma5support/version.h>

namespace Plasma5Support
{
static PluginLoader *s_pluginLoader = nullptr;

class PluginLoaderPrivate
{
public:
    PluginLoaderPrivate()
        : isDefaultLoader(false)
    {
    }

    static QSet<QString> knownCategories();

    static QSet<QString> s_customCategories;
    bool isDefaultLoader;

    static QString s_dataEnginePluginDir;
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
        QHash<QString, QVector<KPluginMetaData>> plugins;

    public:
        QVector<KPluginMetaData> findPluginsById(const QString &name, const QStringList &dirs);
    };
    Cache plasmoidCache;
    Cache dataengineCache;
    Cache containmentactionCache;
};

QSet<QString> PluginLoaderPrivate::s_customCategories;

QString PluginLoaderPrivate::s_dataEnginePluginDir = QStringLiteral("plasma5support/dataengine");
QString PluginLoaderPrivate::s_servicesPluginDir = QStringLiteral("plasma5support/services");

QSet<QString> PluginLoaderPrivate::knownCategories()
{
    // this is to trick the translation tools into making the correct
    // strings for translation
    QSet<QString> categories = s_customCategories;
    /* clang-format off */
    categories << QStringLiteral(I18N_NOOP("Accessibility")).toLower()
               << QStringLiteral(I18N_NOOP("Application Launchers")).toLower()
               << QStringLiteral(I18N_NOOP("Astronomy")).toLower()
               << QStringLiteral(I18N_NOOP("Date and Time")).toLower()
               << QStringLiteral(I18N_NOOP("Development Tools")).toLower()
               << QStringLiteral(I18N_NOOP("Education")).toLower()
               << QStringLiteral(I18N_NOOP("Environment and Weather")).toLower()
               << QStringLiteral(I18N_NOOP("Examples")).toLower()
               << QStringLiteral(I18N_NOOP("File System")).toLower()
               << QStringLiteral(I18N_NOOP("Fun and Games")).toLower()
               << QStringLiteral(I18N_NOOP("Graphics")).toLower()
               << QStringLiteral(I18N_NOOP("Language")).toLower()
               << QStringLiteral(I18N_NOOP("Mapping")).toLower()
               << QStringLiteral(I18N_NOOP("Miscellaneous")).toLower()
               << QStringLiteral(I18N_NOOP("Multimedia")).toLower()
               << QStringLiteral(I18N_NOOP("Online Services")).toLower()
               << QStringLiteral(I18N_NOOP("Productivity")).toLower()
               << QStringLiteral(I18N_NOOP("System Information")).toLower()
               << QStringLiteral(I18N_NOOP("Utilities")).toLower()
               << QStringLiteral(I18N_NOOP("Windows and Tasks")).toLower()
               << QStringLiteral(I18N_NOOP("Clipboard")).toLower()
               << QStringLiteral(I18N_NOOP("Tasks")).toLower();
    /* clang-format on */
    return categories;
}

PluginLoader::PluginLoader()
    : d(new PluginLoaderPrivate)
{
}

PluginLoader::~PluginLoader()
{
    delete d;
}

void PluginLoader::setPluginLoader(PluginLoader *loader)
{
    if (!s_pluginLoader) {
        s_pluginLoader = loader;
    } else {
#ifndef NDEBUG
        // qCDebug(LOG_PLASMA5SUPPORT) << "Cannot set pluginLoader, already set!" << s_pluginLoader;
#endif
    }
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

DataEngine *PluginLoader::loadDataEngine(const QString &name)
{
    DataEngine *engine = d->isDefaultLoader ? nullptr : internalLoadDataEngine(name);
    if (engine) {
        return engine;
    }

    // Look for C++ plugins first
    const QVector<KPluginMetaData> plugins = d->dataengineCache.findPluginsById(name, {PluginLoaderPrivate::s_dataEnginePluginDir});
    if (!plugins.isEmpty()) {
        KPluginLoader loader(plugins.constFirst().fileName());
        const QVariantList argsWithMetaData = QVariantList() << loader.metaData().toVariantMap();
        KPluginFactory *factory = loader.factory();
        return factory ? factory->create<Plasma5Support::DataEngine>(nullptr, argsWithMetaData) : nullptr;
    }
    if (engine) {
        return engine;
    }

    const KPackage::Package p = KPackage::PackageLoader::self()->loadPackage(QStringLiteral("Plasma5Support/DataEngine"), name);
    if (!p.isValid()) {
        return nullptr;
    }

    return new DataEngine(KPluginInfo(p.metadata().fileName()), nullptr);
}

QStringList PluginLoader::listAllEngines(const QString &parentApp)
{
    QStringList engines;
    // Look for C++ plugins first
    auto filter = [&parentApp](const KPluginMetaData &md) -> bool {
        return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp;
    };
    QVector<KPluginMetaData> plugins;
    if (parentApp.isEmpty()) {
        plugins = KPluginLoader::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir);
    } else {
        plugins = KPluginLoader::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir, filter);
    }

    for (auto &plugin : qAsConst(plugins)) {
        engines << plugin.pluginId();
    }

    const QList<KPluginMetaData> packagePlugins = KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma5Support/DataEngine"));
    for (const auto &plugin : packagePlugins) {
        engines << plugin.pluginId();
    }

    return engines;
}

#if PLASMA5SUPPORT_BUILD_DEPRECATED_SINCE(5, 77)
KPluginInfo::List PluginLoader::listEngineInfo(const QString &parentApp)
{
    return PluginLoader::self()->listDataEngineInfo(parentApp);
}
#endif

#if PLASMA5SUPPORT_BUILD_DEPRECATED_SINCE(5, 81)
KPluginInfo::List PluginLoader::listEngineInfoByCategory(const QString &category, const QString &parentApp)
{
    KPluginInfo::List list;

    // Look for C++ plugins first
    auto filterNormal = [&category](const KPluginMetaData &md) -> bool {
        return md.value(QStringLiteral("X-KDE-PluginInfo-Category")) == category;
    };
    auto filterParentApp = [&category, &parentApp](const KPluginMetaData &md) -> bool {
        return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp //
            && md.value(QStringLiteral("X-KDE-PluginInfo-Category")) == category;
    };
    QVector<KPluginMetaData> plugins;
    if (parentApp.isEmpty()) {
        plugins = KPluginLoader::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir, filterNormal);
    } else {
        plugins = KPluginLoader::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir, filterParentApp);
    }

    list = KPluginInfo::fromMetaData(plugins);

    // TODO FIXME: PackageLoader needs to have a function to inject packageStructures
    const QList<KPluginMetaData> packagePlugins = KPackage::PackageLoader::self()->listPackages(QStringLiteral("Plasma5Support/DataEngine"));
    list << KPluginInfo::fromMetaData(packagePlugins.toVector());

    return list;
}
#endif

Service *PluginLoader::loadService(const QString &name, const QVariantList &args, QObject *parent)
{
    Service *service = d->isDefaultLoader ? nullptr : internalLoadService(name, args, parent);
    if (service) {
        return service;
    }

    // TODO: scripting API support
    if (name.isEmpty()) {
        return new NullService(QString(), parent);
    } else if (name == QLatin1String("org.kde.servicestorage")) {
        return new Storage(parent);
    }

    // Look for C++ plugins first
    auto filter = [&name](const KPluginMetaData &md) -> bool {
        return md.pluginId() == name;
    };
    QVector<KPluginMetaData> plugins = KPluginLoader::findPlugins(PluginLoaderPrivate::s_servicesPluginDir, filter);

    if (!plugins.isEmpty()) {
        KPluginLoader loader(plugins.first().fileName());
        if (!isPluginVersionCompatible(loader)) {
            return nullptr;
        }
        KPluginFactory *factory = loader.factory();
        if (factory) {
            service = factory->create<Plasma5Support::Service>(nullptr, args);
        }
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

#if PLASMA5SUPPORT_BUILD_DEPRECATED_SINCE(5, 77)
KPluginInfo::List PluginLoader::listDataEngineInfo(const QString &parentApp)
{
    return KPluginInfo::fromMetaData(listDataEngineMetaData(parentApp));
}
#endif

QVector<KPluginMetaData> PluginLoader::listDataEngineMetaData(const QString &parentApp)
{
    auto filter = [&parentApp](const KPluginMetaData &md) -> bool {
        return md.value(QStringLiteral("X-KDE-ParentApp")) == parentApp;
    };

    QVector<KPluginMetaData> plugins;
    if (parentApp.isEmpty()) {
        plugins = KPluginLoader::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir);
    } else {
        plugins = KPluginLoader::findPlugins(PluginLoaderPrivate::s_dataEnginePluginDir, filter);
    }

    return plugins;
}

DataEngine *PluginLoader::internalLoadDataEngine(const QString &name)
{
    Q_UNUSED(name)
    return nullptr;
}

Service *PluginLoader::internalLoadService(const QString &name, const QVariantList &args, QObject *parent)
{
    Q_UNUSED(name)
    Q_UNUSED(args)
    Q_UNUSED(parent)
    return nullptr;
}

KPluginInfo::List PluginLoader::internalDataEngineInfo() const
{
    return KPluginInfo::List();
}

KPluginInfo::List PluginLoader::internalServiceInfo() const
{
    return KPluginInfo::List();
}

static KPluginInfo::List standardInternalInfo(const QString &type, const QString &category = QString())
{
    QStringList files = QStandardPaths::locateAll(QStandardPaths::GenericDataLocation,
                                                  QLatin1String(PLASMA5SUPPORT_RELATIVE_DATA_INSTALL_DIR "/internal/") + type + QLatin1String("/*.desktop"),
                                                  QStandardPaths::LocateFile);

    const KPluginInfo::List allInfo = KPluginInfo::fromFiles(files);

    if (category.isEmpty() || allInfo.isEmpty()) {
        return allInfo;
    }

    KPluginInfo::List matchingInfo;
    for (const KPluginInfo &info : allInfo) {
        if (info.category().compare(category, Qt::CaseInsensitive) == 0) {
            matchingInfo << info;
        }
    }

    return matchingInfo;
}

KPluginInfo::List PluginLoader::standardInternalDataEngineInfo() const
{
    return standardInternalInfo(QStringLiteral("dataengines"));
}

KPluginInfo::List PluginLoader::standardInternalServiceInfo() const
{
    return standardInternalInfo(QStringLiteral("services"));
}

bool PluginLoader::isPluginVersionCompatible(KPluginLoader &loader)
{
    const quint32 version = loader.pluginVersion();
    if (version == quint32(-1)) {
        // unversioned, just let it through
        qCWarning(LOG_PLASMA5SUPPORT) << loader.fileName() << "unversioned plugin detected, may result in instability";
        return true;
    }

    // we require PLASMA5SUPPORT_VERSION_MAJOR and PLASMA5SUPPORT_VERSION_MINOR
    const quint32 minVersion = PLASMA5SUPPORT_MAKE_VERSION(PLASMA5SUPPORT_VERSION_MAJOR, 0, 0);
    const quint32 maxVersion = PLASMA5SUPPORT_MAKE_VERSION(PLASMA5SUPPORT_VERSION_MAJOR, PLASMA5SUPPORT_VERSION_MINOR, 60);

    if (version < minVersion || version > maxVersion) {
        qCWarning(LOG_PLASMA5SUPPORT) << loader.fileName() << ": this plugin is compiled against incompatible Plasma version" << version
                              << "This build is compatible with" << PLASMA5SUPPORT_VERSION_MAJOR << ".0.0 (" << minVersion << ") to" << PLASMA5SUPPORT_VERSION_STRING << "("
                              << maxVersion << ")";
        return false;
    }

    return true;
}

QVector<KPluginMetaData> PluginLoaderPrivate::Cache::findPluginsById(const QString &name, const QStringList &dirs)
{
    const qint64 now = qRound64(QDateTime::currentMSecsSinceEpoch() / 1000.0);
    bool useRuntimeCache = true;

    if (pluginCacheAge == 0) {
        // Find all the plugins now, but only once
        pluginCacheAge = now;

        auto insertIntoCache = [this](const QString &pluginPath) {
            KPluginMetaData metadata(pluginPath);
            if (!metadata.isValid()) {
                qCDebug(LOG_PLASMA5SUPPORT) << "invalid metadata" << pluginPath;
                return;
            }

            plugins[metadata.pluginId()].append(metadata);
        };

        for (const QString &dir : dirs)
            KPluginLoader::forEachPlugin(dir, insertIntoCache);
    } else if (now - pluginCacheAge > maxCacheAge) {
        // cache is old and we're not within a few seconds of startup anymore
        useRuntimeCache = false;
        plugins.clear();
    }

    // if name wasn't a path, pluginName == name
    const QString pluginName = name.section(QLatin1Char('/'), -1);

    QVector<KPluginMetaData> ret;

    if (useRuntimeCache) {
        auto it = plugins.constFind(pluginName);
        if (it != plugins.constEnd()) {
            ret = *it;
        }
        qCDebug(LOG_PLASMA5SUPPORT) << "loading applet by name" << name << useRuntimeCache << ret.size();
    } else {
        for (const auto &dir : dirs) {
            ret = KPluginLoader::findPluginsById(dir, pluginName);
            if (!ret.isEmpty())
                break;
        }
    }
    return ret;
}

} // Plasma Namespace
