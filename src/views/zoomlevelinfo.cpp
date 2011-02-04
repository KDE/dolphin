/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "zoomlevelinfo.h"
#include <KIconLoader>
#include <QSize>

int ZoomLevelInfo::minimumLevel()
{
    return 0;
}

int ZoomLevelInfo::maximumLevel()
{
    return 16;
}

int ZoomLevelInfo::iconSizeForZoomLevel(int level)
{
    int size = KIconLoader::SizeMedium;
    switch (level) {
    case 0:  size = KIconLoader::SizeSmall; break;
    case 1:  size = KIconLoader::SizeSmallMedium; break;
    case 2:  size = KIconLoader::SizeMedium; break;
    case 3:  size = KIconLoader::SizeLarge; break;
    case 4:  size = KIconLoader::SizeHuge; break;
    default: size = KIconLoader::SizeHuge + ((level - 4) << 4);
    }
    return size;
}

int ZoomLevelInfo::zoomLevelForIconSize(const QSize& size)
{
    int level = 0;
    switch (size.height()) {
    case KIconLoader::SizeSmall:       level = 0; break;
    case KIconLoader::SizeSmallMedium: level = 1; break;
    case KIconLoader::SizeMedium:      level = 2; break;
    case KIconLoader::SizeLarge:       level = 3; break;
    case KIconLoader::SizeHuge:        level = 4; break;
    default: level = 4 + ((size.height() - KIconLoader::SizeHuge) >> 4);
    }
    return level;
}
