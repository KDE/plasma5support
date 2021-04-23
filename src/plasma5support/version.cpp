/*
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "debug_p.h"
#include <QDebug>
#include <plasma5support/version.h>

namespace Plasma5Support
{
unsigned int version()
{
    return PLASMA5SUPPORT_VERSION;
}

unsigned int versionMajor()
{
    return PLASMA5SUPPORT_VERSION_MAJOR;
}

unsigned int versionMinor()
{
    return PLASMA5SUPPORT_VERSION_MINOR;
}

unsigned int versionRelease()
{
    return PLASMA5SUPPORT_VERSION_PATCH;
}

const char *versionString()
{
    return PLASMA5SUPPORT_VERSION_STRING;
}

bool isPluginVersionCompatible(unsigned int version)
{
    if (version == quint32(-1)) {
        // unversioned, just let it through
        qCWarning(LOG_PLASMA5SUPPORT) << "unversioned plugin detected, may result in instability";
        return true;
    }

    // we require PLASMA5SUPPORT_VERSION_MAJOR and PLASMA5SUPPORT_VERSION_MINOR
    const quint32 minVersion = PLASMA5SUPPORT_MAKE_VERSION(PLASMA5SUPPORT_VERSION_MAJOR, 0, 0);
    const quint32 maxVersion = PLASMA5SUPPORT_MAKE_VERSION(PLASMA5SUPPORT_VERSION_MAJOR, PLASMA5SUPPORT_VERSION_MINOR, 60);

    if (version < minVersion || version > maxVersion) {
#ifndef NDEBUG
        // qCDebug(LOG_PLASMA5SUPPORT) << "plugin is compiled against incompatible Plasma version  " << version
        //         << "This build is compatible with" << PLASMA5SUPPORT_VERSION_MAJOR << ".0.0 (" << minVersion
        //         << ") to" << PLASMA5SUPPORT_VERSION_STRING << "(" << maxVersion << ")";
#endif
        return false;
    }

    return true;
}

} // Plasma namespace
