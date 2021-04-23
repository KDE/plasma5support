/*
    SPDX-FileCopyrightText: 2005 Aaron Seigo <aseigo@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#ifndef PLASMA5SUPPORT_DEFS_H
#define PLASMA5SUPPORT_DEFS_H

/** @header plasma/plasma5support.h <Plasma/Plasma> */

#include <QObject>

#include <plasma/plasma5support_export.h>

class QAction;

/**
 * Namespace for everything in libplasma
 */
namespace Plasma5Support
{
/**
 * @class Types plasma/plasma5support.h <Plasma/Plasma>
 * @short Enums and constants used in Plasma
 *
 */
class PLASMA5SUPPORT_EXPORT Types : public QObject
{
    Q_OBJECT

public:
    ~Types();


    /**
     * Possible timing alignments
     **/
    enum IntervalAlignment {
        NoAlignment = 0, /**< No alignment **/
        AlignToMinute, /**< Align to the minute **/
        AlignToHour, /**< Align to the hour **/
    };
    Q_ENUM(IntervalAlignment)

private:
    Types(QObject *parent = nullptr);
};

} // Plasma5Support namespace

#endif // multiple inclusion guard
