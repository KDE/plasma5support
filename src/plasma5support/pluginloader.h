/*
    SPDX-FileCopyrightText: 2010 Ryan Rix <ry@n.rix.si>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLUGIN_LOADER_H
#define PLUGIN_LOADER_H

#include <QString>
#include <plasma5support/plasma5support_export.h>

class KPluginMetaData;
namespace Plasma5Support
{
class DataEngine;
class Service;

class PluginLoaderPrivate;

/**
 * @class PluginLoader plasma/pluginloader.h <Plasma/PluginLoader>
 *
 * This is an abstract base class which defines an interface to which Plasma's
 * DataEngine and Service Loading logic can communicate with a parent application.
 * This is deprecated api in KF6, use C++ based QML imports instead
 */
class PLASMA5SUPPORT_EXPORT PluginLoader
{
public:
    /**
     * Load a Service plugin.
     *
     * @param name the plugin name of the service to load
     * @param args a list of arguments to supply to the service plugin when loading it
     * @param parent the parent object, if any, for the service
     *
     * @return a Service object, unlike Plasma5Support::Service::loadService, this can return null.
     **/
    static Service *loadService(const QString &name, QObject *parent = nullptr);

    /**
     * Returns a list of all known dataengines.
     *
     * @param parentApp the application to filter dataengines on. Uses the
     *                  X-KDE-ParentApp entry (if any) in the plugin info.
     *                  The default value of QString() will result in a
     *                  list of all dataengines
     * @return list of dataengines
     **/
    static QList<KPluginMetaData> listDataEngineMetaData(const QString &parentApp = QString());

private:
    PluginLoader() = default;
    virtual ~PluginLoader();
    void *d;
};

}

#endif
