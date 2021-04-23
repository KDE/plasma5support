/*
    SPDX-FileCopyrightText: 2008 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA5SUPPORTVERSION_H
#define PLASMA5SUPPORTVERSION_H

/** @file plasma5support/version.h <Plasma5Support/Version> */

#include <plasma5support/plasma5support_export.h>
#include <plasma5support_version.h>

#define PLASMA5SUPPORT_MAKE_VERSION(a, b, c) (((a) << 16) | ((b) << 8) | (c))

/**
 * Compile-time macro for checking the plasma version. Not useful for
 * detecting the version of libplasma at runtime.
 */
#define PLASMA5SUPPORT_IS_VERSION(a, b, c) (PLASMA5SUPPORT_VERSION >= PLASMA5SUPPORT_MAKE_VERSION(a, b, c))

/**
 * Namespace for everything in libplasma
 */
namespace Plasma5Support
{
/**
 * The runtime version of libplasma
 */
PLASMA5SUPPORT_EXPORT unsigned int version();

/**
 * The runtime major version of libplasma
 */
PLASMA5SUPPORT_EXPORT unsigned int versionMajor();

/**
 * The runtime major version of libplasma
 */
PLASMA5SUPPORT_EXPORT unsigned int versionMinor();

/**
 * The runtime major version of libplasma
 */
PLASMA5SUPPORT_EXPORT unsigned int versionRelease();

/**
 * The runtime version string of libplasma
 */
PLASMA5SUPPORT_EXPORT const char *versionString();

/**
 * Verifies that a plugin is compatible with plasma
 */
PLASMA5SUPPORT_EXPORT bool isPluginVersionCompatible(unsigned int version);

} // Plasma5Support namespace

#endif // multiple inclusion guard
