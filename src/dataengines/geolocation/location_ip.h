/*
    SPDX-FileCopyrightText: 2009 Petri Damstén <damu@iki.fi>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#pragma once

#include "geolocationprovider.h"

class Ip : public GeolocationProvider
{
    Q_OBJECT
public:
    explicit Ip(QObject *parent);
    ~Ip() override;

    void update() override;

private:
    class Private;
    Private *const d;
};
