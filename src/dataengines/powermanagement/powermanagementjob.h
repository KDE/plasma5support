/*
    SPDX-FileCopyrightText: 2011 Sebastian Kügler <sebas@kde.org>

    SPDX-License-Identifier: LGPL-2.0-only
*/

#pragma once

// plasma
#include <Plasma5Support/ServiceJob>

class SessionManagement;
class QDBusPendingCall;

class PowerManagementJob : public Plasma5Support::ServiceJob
{
    Q_OBJECT

public:
    PowerManagementJob(const QString &operation, QMap<QString, QVariant> &parameters, QObject *parent = nullptr);
    ~PowerManagementJob() override;

protected:
    void start() override;

private:
    QDBusPendingCall setPowerProfile(const QString &value);
    SessionManagement *m_session;
    inline static uint m_sleepInhibitionCookie = -1;
    inline static uint m_lockInhibitionCookie = -1;
};
