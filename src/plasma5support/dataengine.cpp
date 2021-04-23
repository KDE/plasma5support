/*
    SPDX-FileCopyrightText: 2006-2007 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dataengine.h"
#include "private/datacontainer_p.h"
#include "private/dataengine_p.h"

#include <QAbstractItemModel>
#include <QQueue>
#include <QTime>
#include <QTimer>
#include <QTimerEvent>
#include <QVariant>

#include <QDebug>
#include <QStandardPaths>

#include <KLocalizedString>

#include "datacontainer.h"
#include "pluginloader.h"
#include "service.h"

#include "config-plasma5support.h"
#include "private/service_p.h"
#include "private/storage_p.h"

namespace Plasma5Support
{
DataEngine::DataEngine(const KPluginInfo &plugin, QObject *parent)
    : DataEngine(plugin.toMetaData(), parent)
{
}

DataEngine::DataEngine(const KPluginMetaData &plugin, QObject *parent)
    : QObject(parent)
    , d(new DataEnginePrivate(this, plugin))
{
}

DataEngine::DataEngine(QObject *parent, const QVariantList &args)
    : QObject(parent)
    , d(new DataEnginePrivate(this, KPluginInfo(args).toMetaData(), args))
{
}

DataEngine::~DataEngine()
{
    // qCDebug(LOG_PLASMA5SUPPORT) << objectName() << ": bye bye birdy! ";
    delete d;
}

QStringList DataEngine::sources() const
{
    return d->sources.keys();
}

Service *DataEngine::serviceForSource(const QString &source)
{
    return new NullService(source, this);
}

KPluginInfo DataEngine::pluginInfo() const
{
    return KPluginInfo(d->dataEngineDescription);
}

KPluginMetaData DataEngine::metadata() const
{
    return d->dataEngineDescription;
}

void DataEngine::connectSource(const QString &source, QObject *visualization, uint pollingInterval, Plasma5Support::Types::IntervalAlignment intervalAlignment) const
{
    // qCDebug(LOG_PLASMA5SUPPORT) << "connectSource" << source;
    bool newSource;
    DataContainer *s = d->requestSource(source, &newSource);

    if (s) {
        // we suppress the immediate invocation of dataUpdated here if the
        // source was prexisting and they don't request delayed updates
        // (we want to do an immediate update in that case so they don't
        // have to wait for the first time out)
        if (newSource && !s->data().isEmpty()) {
            newSource = false;
        }
        d->connectSource(s, visualization, pollingInterval, intervalAlignment, !newSource || pollingInterval > 0);
        // qCDebug(LOG_PLASMA5SUPPORT) << " ==> source connected";
    }
}

void DataEngine::connectAllSources(QObject *visualization, uint pollingInterval, Plasma5Support::Types::IntervalAlignment intervalAlignment) const
{
    for (DataContainer *s : qAsConst(d->sources)) {
        d->connectSource(s, visualization, pollingInterval, intervalAlignment);
    }
}

void DataEngine::disconnectSource(const QString &source, QObject *visualization) const
{
    DataContainer *s = d->source(source, false);

    if (s) {
        s->disconnectVisualization(visualization);
    }
}

DataContainer *DataEngine::containerForSource(const QString &source)
{
    return d->source(source, false);
}

bool DataEngine::sourceRequestEvent(const QString &name)
{
    return false;
}

bool DataEngine::updateSourceEvent(const QString &source)
{
    return false; // TODO: should this be true to trigger, even needless, updates on every tick?
}

void DataEngine::setData(const QString &source, const QVariant &value)
{
    setData(source, source, value);
}

void DataEngine::setData(const QString &source, const QString &key, const QVariant &value)
{
    DataContainer *s = d->source(source, false);
    bool isNew = !s;

    if (isNew) {
        s = d->source(source);
    }

    s->setData(key, value);

    if (isNew && source != d->waitingSourceRequest) {
        Q_EMIT sourceAdded(source);
    }

    d->scheduleSourcesUpdated();
}

void DataEngine::setData(const QString &source, const QVariantMap &data)
{
    DataContainer *s = d->source(source, false);
    bool isNew = !s;

    if (isNew) {
        s = d->source(source);
    }

    Data::const_iterator it = data.constBegin();
    while (it != data.constEnd()) {
        s->setData(it.key(), it.value());
        ++it;
    }

    if (isNew && source != d->waitingSourceRequest) {
        Q_EMIT sourceAdded(source);
    }

    d->scheduleSourcesUpdated();
}

void DataEngine::removeAllData(const QString &source)
{
    DataContainer *s = d->source(source, false);
    if (s) {
        s->removeAllData();
        d->scheduleSourcesUpdated();
    }
}

void DataEngine::removeData(const QString &source, const QString &key)
{
    DataContainer *s = d->source(source, false);
    if (s) {
        s->setData(key, QVariant());
        d->scheduleSourcesUpdated();
    }
}

void DataEngine::setModel(const QString &source, QAbstractItemModel *model)
{
    if (model) {
        setData(source, QStringLiteral("HasModel"), true);
    } else {
        removeData(source, QStringLiteral("HasModel"));
    }

    Plasma5Support::DataContainer *s = containerForSource(source);

    if (s) {
        s->setModel(model);
    }
}

QAbstractItemModel *DataEngine::modelForSource(const QString &source)
{
    Plasma5Support::DataContainer *s = containerForSource(source);

    if (s) {
        return s->model();
    } else {
        return nullptr;
    }
}

void DataEngine::addSource(DataContainer *source)
{
    if (d->sources.contains(source->objectName())) {
#ifndef NDEBUG
        // qCDebug(LOG_PLASMA5SUPPORT) << "source named \"" << source->objectName() << "\" already exists.";
#endif
        return;
    }

    QObject::connect(source, SIGNAL(updateRequested(DataContainer *)), this, SLOT(internalUpdateSource(DataContainer *)));
    QObject::connect(source, SIGNAL(destroyed(QObject *)), this, SLOT(sourceDestroyed(QObject *)));
    d->sources.insert(source->objectName(), source);
    Q_EMIT sourceAdded(source->objectName());
    d->scheduleSourcesUpdated();
}

void DataEngine::setMinimumPollingInterval(int minimumMs)
{
    d->minPollingInterval = minimumMs;
}

int DataEngine::minimumPollingInterval() const
{
    return d->minPollingInterval;
}

void DataEngine::setPollingInterval(uint frequency)
{
    killTimer(d->updateTimerId);
    d->updateTimerId = 0;

    if (frequency > 0) {
        d->updateTimerId = startTimer(frequency);
    }
}

void DataEngine::removeSource(const QString &source)
{
    QHash<QString, DataContainer *>::iterator it = d->sources.find(source);
    if (it != d->sources.end()) {
        DataContainer *s = it.value();
        s->d->store();
        d->sources.erase(it);
        s->disconnect(this);
        s->deleteLater();
        Q_EMIT sourceRemoved(source);
    }
}

void DataEngine::removeAllSources()
{
    QMutableHashIterator<QString, Plasma5Support::DataContainer *> it(d->sources);
    while (it.hasNext()) {
        it.next();
        Plasma5Support::DataContainer *s = it.value();
        const QString source = it.key();
        it.remove();
        s->disconnect(this);
        s->deleteLater();
        Q_EMIT sourceRemoved(source);
    }
}

bool DataEngine::isValid() const
{
    return d->valid;
}

bool DataEngine::isEmpty() const
{
    return d->sources.isEmpty();
}

void DataEngine::setValid(bool valid)
{
    d->valid = valid;
}

QHash<QString, DataContainer *> DataEngine::containerDict() const
{
    return d->sources;
}

void DataEngine::timerEvent(QTimerEvent *event)
{
    // qCDebug(LOG_PLASMA5SUPPORT);
    if (event->timerId() == d->updateTimerId) {
        // if the freq update is less than 0, don't bother
        if (d->minPollingInterval < 0) {
            // qCDebug(LOG_PLASMA5SUPPORT) << "uh oh.. no polling allowed!";
            return;
        }

        // minPollingInterval
        if (d->updateTimer.elapsed() < d->minPollingInterval) {
            // qCDebug(LOG_PLASMA5SUPPORT) << "hey now.. slow down!";
            return;
        }

        d->updateTimer.start();
        updateAllSources();
    } else if (event->timerId() == d->checkSourcesTimerId) {
        killTimer(d->checkSourcesTimerId);
        d->checkSourcesTimerId = 0;

        QHashIterator<QString, Plasma5Support::DataContainer *> it(d->sources);
        while (it.hasNext()) {
            it.next();
            it.value()->checkForUpdate();
        }
    } else {
        QObject::timerEvent(event);
    }
}

void DataEngine::updateAllSources()
{
    QHashIterator<QString, Plasma5Support::DataContainer *> it(d->sources);
    while (it.hasNext()) {
        it.next();
        // qCDebug(LOG_PLASMA5SUPPORT) << "updating" << it.key();
        if (it.value()->isUsed()) {
            updateSourceEvent(it.key());
        }
    }

    d->scheduleSourcesUpdated();
}

void DataEngine::forceImmediateUpdateOfAllVisualizations()
{
    for (DataContainer *source : qAsConst(d->sources)) {
        if (source->isUsed()) {
            source->forceImmediateUpdate();
        }
    }
}

void DataEngine::setStorageEnabled(const QString &source, bool store)
{
    DataContainer *s = d->source(source, false);
    if (s) {
        s->setStorageEnabled(store);
    }
}

// Private class implementations
DataEnginePrivate::DataEnginePrivate(DataEngine *e, const KPluginMetaData &md, const QVariantList &args)
    : q(e)
    , dataEngineDescription(md)
    , refCount(-1)
    , checkSourcesTimerId(0) // first ref
    , updateTimerId(0)
    , minPollingInterval(-1)
    , valid(false)
{
    updateTimer.start();

    if (dataEngineDescription.isValid()) {
        valid = true;
        e->setObjectName(dataEngineDescription.name());
    }
}

DataEnginePrivate::~DataEnginePrivate()
{
}

void DataEnginePrivate::internalUpdateSource(DataContainer *source)
{
    if (minPollingInterval > 0 && source->timeSinceLastUpdate() < (uint)minPollingInterval) {
        // skip updating this source; it's been too soon
        // qCDebug(LOG_PLASMA5SUPPORT) << "internal update source is delaying" << source->timeSinceLastUpdate() << minPollingInterval;
        // but fake an update so that the signalrelay that triggered this gets the data from the
        // recent update. this way we don't have to worry about queuing - the relay will send a
        // signal immediately and everyone else is undisturbed.
        source->setNeedsUpdate();
        return;
    }

    if (q->updateSourceEvent(source->objectName())) {
        // qCDebug(LOG_PLASMA5SUPPORT) << "queuing an update";
        scheduleSourcesUpdated();
    } /* else {
 #ifndef NDEBUG
         // qCDebug(LOG_PLASMA5SUPPORT) << "no update";
 #endif
     }*/
}

void DataEnginePrivate::ref()
{
    --refCount;
}

void DataEnginePrivate::deref()
{
    ++refCount;
}

bool DataEnginePrivate::isUsed() const
{
    return refCount != 0;
}

DataContainer *DataEnginePrivate::source(const QString &sourceName, bool createWhenMissing)
{
    QHash<QString, DataContainer *>::const_iterator it = sources.constFind(sourceName);
    if (it != sources.constEnd()) {
        DataContainer *s = it.value();
        return s;
    }

    if (!createWhenMissing) {
        return nullptr;
    }

    // qCDebug(LOG_PLASMA5SUPPORT) << "DataEngine " << q->objectName() << ": could not find DataContainer " << sourceName << ", creating";
    DataContainer *s = new DataContainer(q);
    s->setObjectName(sourceName);
    sources.insert(sourceName, s);
    QObject::connect(s, SIGNAL(destroyed(QObject *)), q, SLOT(sourceDestroyed(QObject *)));
    QObject::connect(s, SIGNAL(updateRequested(DataContainer *)), q, SLOT(internalUpdateSource(DataContainer *)));

    return s;
}

void DataEnginePrivate::connectSource(DataContainer *s,
                                      QObject *visualization,
                                      uint pollingInterval,
                                      Plasma5Support::Types::IntervalAlignment align,
                                      bool immediateCall)
{
    // qCDebug(LOG_PLASMA5SUPPORT) << "connect source called" << s->objectName() << "with interval" << pollingInterval;

    if (pollingInterval > 0) {
        // never more frequently than allowed, never more than 20 times per second
        uint min = qMax(50, minPollingInterval); // for qMax below
        pollingInterval = qMax(min, pollingInterval);

        // align on the 50ms
        pollingInterval = pollingInterval - (pollingInterval % 50);
    }

    if (immediateCall) {
        // we don't want to do an immediate call if we are simply
        // reconnecting
        // qCDebug(LOG_PLASMA5SUPPORT) << "immediate call requested, we have:" << s->visualizationIsConnected(visualization);
        immediateCall = !s->data().isEmpty() && !s->visualizationIsConnected(visualization);
    }

    s->connectVisualization(visualization, pollingInterval, align);

    if (immediateCall) {
        QMetaObject::invokeMethod(visualization, "dataUpdated", Q_ARG(QString, s->objectName()), Q_ARG(Plasma5Support::DataEngine::Data, s->data()));
        if (s->d->model) {
            QMetaObject::invokeMethod(visualization, "modelChanged", Q_ARG(QString, s->objectName()), Q_ARG(QAbstractItemModel *, s->d->model.data()));
        }
        s->d->dirty = false;
    }
}

void DataEnginePrivate::sourceDestroyed(QObject *object)
{
    QHash<QString, DataContainer *>::iterator it = sources.begin();
    while (it != sources.end()) {
        if (it.value() == object) {
            sources.erase(it);
            Q_EMIT q->sourceRemoved(object->objectName());
            break;
        }
        ++it;
    }
}

DataContainer *DataEnginePrivate::requestSource(const QString &sourceName, bool *newSource)
{
    if (newSource) {
        *newSource = false;
    }

    // qCDebug(LOG_PLASMA5SUPPORT) << "requesting source " << sourceName;
    DataContainer *s = source(sourceName, false);

    if (!s) {
        // we didn't find a data source, so give the engine an opportunity to make one
        /*// qCDebug(LOG_PLASMA5SUPPORT) << "DataEngine " << q->objectName()
            << ": could not find DataContainer " << sourceName
            << " will create on request" << endl;*/
        waitingSourceRequest = sourceName;
        if (q->sourceRequestEvent(sourceName)) {
            s = source(sourceName, false);
            if (s) {
                // now we have a source; since it was created on demand, assume
                // it should be removed when not used
                if (newSource) {
                    *newSource = true;
                }
                QObject::connect(s, &DataContainer::becameUnused, q, &DataEngine::removeSource);
                Q_EMIT q->sourceAdded(sourceName);
            }
        }
        waitingSourceRequest.clear();
    }

    return s;
}

void DataEnginePrivate::scheduleSourcesUpdated()
{
    if (checkSourcesTimerId) {
        return;
    }

    checkSourcesTimerId = q->startTimer(0);
}

}

#include "moc_dataengine.cpp"
