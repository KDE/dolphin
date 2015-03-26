/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

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
    static void scale(QPixmap& pixmap, const QSize& scaledSize);

    /**
     * Resize and paint a frame round an icon
     * @arg scaledSize is assumed to be the scaled to the same device pixel ratio as the source icon
     */
    static void applyFrame(QPixmap& icon, const QSize& scaledSize);

    /**
     * return and paint a frame round an icon
     * @arg framesize is in device-independent pixels
     * @return is in device-indepent pixels
     */

    static QSize sizeInsideFrame(const QSize& frameSize);
};

#endif


