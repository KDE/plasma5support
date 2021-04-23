/*
    SPDX-FileCopyrightText: 2005 Aaron Seigo <aseigo@kde.org>
    SPDX-FileCopyrightText: 2007 Riccardo Iaconelli <riccardo@kde.org>
    SPDX-FileCopyrightText: 2008 Ménard Alexis <darktears31@gmail.com>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA5SUPPORT_DATAENGINECONSUMER_P_H
#define PLASMA5SUPPORT_DATAENGINECONSUMER_P_H

#include <QMap>
#include <QObject>
#include <QSet>
#include <QUrl>

namespace Plasma5Support
{
class DataEngineConsumer;
class Service;
class ServiceMonitor;
class ServiceJob;

class DataEngineConsumerPrivate : public QObject
{
    Q_OBJECT

public:
    QSet<QString> loadedEngines;
    QMap<Service *, QString> engineNameForService;

public Q_SLOTS:
    void slotJobFinished(Plasma5Support::ServiceJob *job);
    void slotServiceReady(Plasma5Support::Service *service);
};

} // namespace Plasma5Support

#endif
