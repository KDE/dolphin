/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KPIXMAPMODIFIER_H
#define KPIXMAPMODIFIER_H

#include "dolphin_export.h"

class QPixmap;
class QSize;

class DOLPHIN_EXPORT KPixmapModifier
{
public:
    /**
     * Scale a pixmap to a given size.
     * @arg scaledSize is assumed to be the scaled to the same device pixel ratio as the source pixmap
     * @arg scaledSize is in device pixels
     */
    static void scale(QPixmap &pixmap, const QSize &scaledSize);

    /**
     * Resize and paint a frame round an icon
     * @arg scaledSize is in device-independent pixels
     * The returned image will be scaled by the application devicePixelRatio
     */
    static void applyFrame(QPixmap &icon, const QSize &scaledSize);

    /**
     * return and paint a frame round an icon
     * @arg framesize is in device-independent pixels
     * @return is in device-independent pixels
     */

    static QSize sizeInsideFrame(const QSize &frameSize);
};

#endif
