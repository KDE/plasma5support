/*
    SPDX-FileCopyrightText: 2007-2009 Shawn Starr <shawn.starr@rogers.com>
    SPDX-FileCopyrightText: 2009 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "weatherengine.h"

#include <KPluginMetaData>
#include <KSycoca>

#include <Plasma5Support/DataContainer>
#include <Plasma5Support/PluginLoader>

#include "weatherenginedebug.h"

// Constructor
WeatherEngine::WeatherEngine(QObject *parent)
    : Plasma5Support::DataEngine(parent)
{
    m_reconnectTimer.setSingleShot(true);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &WeatherEngine::startReconnect);

    // Globally notify all plugins to remove their sources (and unload plugin)
    connect(this, &Plasma5Support::DataEngine::sourceRemoved, this, &WeatherEngine::removeIonSource);

    QNetworkInformation::load(QNetworkInformation::Feature::Reachability);
    connect(QNetworkInformation::instance(), &QNetworkInformation::reachabilityChanged, this, &WeatherEngine::onOnlineStateChanged);

    // Get the list of available plugins but don't load them
    connect(KSycoca::self(), &KSycoca::databaseChanged, this, &WeatherEngine::updateIonList);

    updateIonList();
}

// Destructor
WeatherEngine::~WeatherEngine()
{
}

/* FIXME: Q_PROPERTY functions to update the list of available plugins */

void WeatherEngine::updateIonList()
{
    removeAllData(QStringLiteral("ions"));
    const auto infos = Plasma5Support::PluginLoader::listDataEngineMetaData(QStringLiteral("weatherengine"));
    for (const KPluginMetaData &info : infos) {
        // We want to provide the short ion name, but pluginId is the full plugin library name
        const QString ionName = info.pluginId().split(QLatin1Char('_')).last();
        const QString data = QStringLiteral("%1|%2").arg(info.name()).arg(ionName);
        setData(QStringLiteral("ions"), info.pluginId(), data);
    }
}

/**
 * SLOT: Remove the datasource from the ion and unload plugin if needed
 */
void WeatherEngine::removeIonSource(const QString &source)
{
    QString ionName;
    IonInterface *ion = ionForSource(source, &ionName);
    if (ion) {
        ion->removeSource(source);

        // track used ions
        QHash<QString, int>::Iterator it = m_ionUsage.find(ionName);

        if (it == m_ionUsage.end()) {
            qCWarning(WEATHER) << "Removing ion source without being added before:" << source;
        } else {
            // no longer used?
            if (it.value() <= 1) {
                // forget about it
                m_ionUsage.erase(it);
                disconnect(ion, &IonInterface::forceUpdate, this, &WeatherEngine::forceUpdate);
                disconnect(ion, &IonInterface::cleanUpData, this, &WeatherEngine::removeAllData);

                qCDebug(WEATHER) << "Ion no longer used as source:" << ionName;
            } else {
                --(it.value());
            }
        }
    } else {
        qCWarning(WEATHER) << "Could not find ion to remove source for:" << source;
    }
}

/**
 * SLOT: Push out new data to applet
 */
void WeatherEngine::dataUpdated(const QString &source, const Plasma5Support::DataEngine::Data &data)
{
    qCDebug(WEATHER) << "dataUpdated() for:" << source;
    setData(source, data);
}

/**
 * SLOT: Set up each Ion for the first time and get any data
 */
bool WeatherEngine::sourceRequestEvent(const QString &source)
{
    QString ionName;
    IonInterface *ion = ionForSource(source, &ionName);

    if (!ion) {
        qCWarning(WEATHER) << "Could not find ion to request source for:" << source;
        return false;
    }

    // track used ions
    QHash<QString, int>::Iterator it = m_ionUsage.find(ionName);
    if (it == m_ionUsage.end()) {
        m_ionUsage.insert(ionName, 1);
        connect(ion, &IonInterface::forceUpdate, this, &WeatherEngine::forceUpdate);
        connect(ion, &IonInterface::cleanUpData, this, &WeatherEngine::removeAllData);

        qCDebug(WEATHER) << "Ion now used as source:" << ionName;
    } else {
        ++(*it);
    }

    // we should connect to the ion anyway, even if the network
    // is down. when it comes up again, then it will be refreshed
    ion->connectSource(source, this);

    qCDebug(WEATHER) << "sourceRequestEvent(): Network is: " << QNetworkInformation::instance()->reachability();
    if (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Online) {
        setData(source, Data());
        return true;
    }

    if (!containerForSource(source)) {
        // it is an async reply, we need to set up the data anyways
        setData(source, Data());
    }

    return true;
}

/**
 * SLOT: update the Applet with new data from all ions loaded.
 */
bool WeatherEngine::updateSourceEvent(const QString &source)
{
    qCDebug(WEATHER) << "updateSourceEvent(): Network is: " << QNetworkInformation::instance()->reachability();
    if (QNetworkInformation::instance()->reachability() != QNetworkInformation::Reachability::Online) {
        return false;
    }

    IonInterface *ion = ionForSource(source);
    if (!ion) {
        qCWarning(WEATHER) << "Could not find ion to update source for:" << source;
        return false;
    }

    return ion->updateSourceEvent(source);
}

void WeatherEngine::onOnlineStateChanged(QNetworkInformation::Reachability reachability)
{
    if (reachability == QNetworkInformation::Reachability::Online) {
        qCDebug(WEATHER) << "starting m_reconnectTimer";
        // allow the network to settle down and actually come up
        m_reconnectTimer.start(1000);
    } else {
        m_reconnectTimer.stop();
    }
}

void WeatherEngine::startReconnect()
{
    for (QHash<QString, int>::ConstIterator it = m_ionUsage.constBegin(); it != m_ionUsage.constEnd(); ++it) {
        const QString &ionName = it.key();
        IonInterface *ion = qobject_cast<IonInterface *>(dataEngine(ionName));

        if (ion) {
            qCDebug(WEATHER) << "Resetting ion" << ion;
            ion->reset();
        } else {
            qCWarning(WEATHER) << "Could not find ion to reset:" << ionName;
        }
    }
}

void WeatherEngine::forceUpdate(IonInterface *ion, const QString &source)
{
    Q_UNUSED(ion);
    Plasma5Support::DataContainer *container = containerForSource(source);
    if (container) {
        qCDebug(WEATHER) << "immediate update of" << source;
        container->forceImmediateUpdate();
    } else {
        qCWarning(WEATHER) << "inexplicable failure of" << source;
    }
}

IonInterface *WeatherEngine::ionForSource(const QString &source, QString *ionName)
{
    const int offset = source.indexOf(QLatin1Char('|'));

    if (offset < 1) {
        return nullptr;
    }

    const QString name = source.left(offset);

    IonInterface *result = qobject_cast<IonInterface *>(dataEngine(name));

    if (result && ionName) {
        *ionName = name.split(QLatin1Char('_')).last();
    }

    return result;
}

K_PLUGIN_CLASS_WITH_JSON(WeatherEngine, "plasma-dataengine-weather.json")

#include "weatherengine.moc"

#include "moc_weatherengine.cpp"
