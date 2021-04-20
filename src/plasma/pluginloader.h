/*
    SPDX-FileCopyrightText: 2010 Ryan Rix <ry@n.rix.si>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <KPluginInfo>
#include <plasma/plasma5support.h>

namespace Plasma5Support
{
class Applet;
class Containment;
class ContainmentActions;
class DataEngine;
class Service;

class PluginLoaderPrivate;

// TODO:
// * add loadWallpaper
// * add KPluginInfo listing support for Containments (already loaded via the applet loading code)

/**
 * @class PluginLoader plasma/pluginloader.h <Plasma/PluginLoader>
 *
 * This is an abstract base class which defines an interface to which Plasma's
 * Applet Loading logic can communicate with a parent application. The plugin loader
 * must be set before any plugins are loaded, otherwise (for safety reasons), the
 * default PluginLoader implementation will be used. The reimplemented version should
 * not do more than simply returning a loaded plugin. It should not init() it, and it should not
 * hang on to it. The associated methods will be called only when a component of Plasma
 * needs to load a _new_ plugin. (e.g. DataEngine does its own caching).
 *
 * @author Ryan Rix <ry@n.rix.si>
 * @since 4.6
 **/
class PLASMA_EXPORT PluginLoader
{
public:
    /**
     * Load a dataengine plugin.
     *
     * @param name the name of the engine
     * @return the dataengine that was loaded, or the NullEngine on failure.
     **/
    DataEngine *loadDataEngine(const QString &name);

    /**
     * @return a listing of all known dataengines by name
     *
     * @param parentApp the application to filter dataengines on. Uses the
     *                  X-KDE-ParentApp entry (if any) in the plugin info.
     *                  The default value of QString() will result in a
     *                  list of all dataengines.
     */
    static QStringList listAllEngines(const QString &parentApp = QString());

#if PLASMA_ENABLE_DEPRECATED_SINCE(5, 77)
    /**
     * Returns a list of all known dataengines.
     *
     * @param parentApp the application to filter dataengines on. Uses the
     *                  X-KDE-ParentApp entry (if any) in the plugin info.
     *                  The default value of QString() will result in a
     *                  list of all dataengines.
     * @return list of dataengines
     * @deprecated since 5.77, use listDataEngineMetaData instead.
     **/
    PLASMA_DEPRECATED_VERSION(5, 77, "Use listDataEngineMetaData instead")
    static KPluginInfo::List listEngineInfo(const QString &parentApp = QString());
#endif

#if PLASMA_ENABLE_DEPRECATED_SINCE(5, 81)
    /**
     * Returns a list of all known dataengines filtering by category.
     *
     * @param category the category to filter dataengines on. Uses the
     *                  X-KDE-PluginInfo-Category entry (if any) in the
     *                  plugin info. The value of QString() will
     *                  result in a list of dataengines with an empty category.
     *
     * @param parentApp the application to filter dataengines on. Uses the
     *                  X-KDE-ParentApp entry (if any) in the plugin info.
     *                  The default value of QString() will result in a
     *                  list of all dataengines in specified categories.
     * @return list of dataengines
     * @deprecated since 5.81, use listDataEngineMetaData() instead.
     * @since 4.3
     **/
    PLASMA_DEPRECATED_VERSION(5, 81, "Use listDataEngineMetaData instead")
    static KPluginInfo::List listEngineInfoByCategory(const QString &category, const QString &parentApp = QString());
#endif

    /**
     * Load a Service plugin.
     *
     * @param name the plugin name of the service to load
     * @param args a list of arguments to supply to the service plugin when loading it
     * @param parent the parent object, if any, for the service
     *
     * @return a Service object, unlike Plasma::Service::loadService, this can return null.
     **/
    Service *loadService(const QString &name, const QVariantList &args, QObject *parent = nullptr);

#if PLASMA_ENABLE_DEPRECATED_SINCE(5, 77)
    /**
     * Returns a list of all known dataengines.
     *
     * @param parentApp the application to filter dataengines on. Uses the
     *                  X-KDE-ParentApp entry (if any) in the plugin info.
     *                  The default value of QString() will result in a
     *                  list of all dataengines
     * @return list of dataengines
     * @deprecated since 5.77, use listDataEngineMetaData()
     **/
    PLASMA_DEPRECATED_VERSION(5, 77, "Use listDataEngineMetaData()")
    KPluginInfo::List listDataEngineInfo(const QString &parentApp = QString());
#endif

    /**
     * Returns a list of all known dataengines.
     *
     * @param parentApp the application to filter dataengines on. Uses the
     *                  X-KDE-ParentApp entry (if any) in the plugin info.
     *                  The default value of QString() will result in a
     *                  list of all dataengines
     * @return list of dataengines
     * @since 5.77
     **/
    QVector<KPluginMetaData> listDataEngineMetaData(const QString &parentApp = QString());

    /**
     * Set the plugin loader which will be queried for all loads.
     *
     * @param loader A subclass of PluginLoader which will be supplied
     * by the application
     **/
    static void setPluginLoader(PluginLoader *loader);

    /**
     * Return the active plugin loader
     **/
    static PluginLoader *self();

protected:

    /**
     * A re-implementable method that allows subclasses to override
     * the default behaviour of loadDataEngine. If the engine requested is not recognized,
     * then the implementation should return a NULL pointer. This method is called
     * by loadDataEngine prior to attempting to load a DataEgine using the standard Plasma
     * plugin mechanisms.
     *
     * @param name the name of the engine
     * @return the data engine that was loaded, or the NullEngine on failure.
     **/
    virtual DataEngine *internalLoadDataEngine(const QString &name);

    /**
     * A re-implementable method that allows subclasses to override
     * the default behaviour of loadService. If the service requested is not recognized,
     * then the implementation should return a NULL pointer. This method is called
     * by loadService prior to attempting to load a Service using the standard Plasma
     * plugin mechanisms.
     *
     * @param name the plugin name of the service to load
     * @param args a list of arguments to supply to the service plugin when loading it
     * @param parent the parent object, if any, for the service
     *
     * @return a Service object, unlike Plasma::Service::loadService, this can return null.
     **/
    virtual Service *internalLoadService(const QString &name, const QVariantList &args, QObject *parent = nullptr);

    /**
     * A re-implementable method that allows subclasses to provide additional dataengines
     * for DataEngine::listDataEngines.
     *
     * @return list of dataengine info, or an empty list if none
     **/
    virtual KPluginInfo::List internalDataEngineInfo() const;

    /**
     * Returns a list of all known Service implementations
     *
     * @return list of Service info, or an empty list if none
     */
    virtual KPluginInfo::List internalServiceInfo() const;

    /**
     * Standardized mechanism for providing internal dataengines by install .desktop files
     * in $APPPDATA/plasma/internal/dataengines/
     *
     * For applications that do this, internalDataEngineInfo can be implemented as a one-liner
     * call to this method.
     *
     * @return list of dataengines
     */
    KPluginInfo::List standardInternalDataEngineInfo() const;

    /**
     * Standardized mechanism for providing internal services by install .desktop files
     * in $APPPDATA/plasma/internal/services/
     *
     * For applications that do this, internalServiceInfo can be implemented as a one-liner
     * call to this method.
     *
     * @return list of services
     */
    KPluginInfo::List standardInternalServiceInfo() const;

    PluginLoader();
    virtual ~PluginLoader();

private:
    bool isPluginVersionCompatible(KPluginLoader &loader);

    PluginLoaderPrivate *const d;
};

}

Q_DECLARE_METATYPE(Plasma5Support::PluginLoader *)

#endif
