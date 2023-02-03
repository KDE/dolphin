/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
    case 0:
        size = KIconLoader::SizeSmall;
        break;
    case 1:
        size = KIconLoader::SizeSmallMedium;
        break;
    case 2:
        size = KIconLoader::SizeMedium;
        break;
    case 3:
        size = KIconLoader::SizeLarge;
        break;
    case 4:
        size = KIconLoader::SizeHuge;
        break;
    default:
        size = KIconLoader::SizeHuge + ((level - 4) << 4);
    }
    return size;
}

int ZoomLevelInfo::zoomLevelForIconSize(const QSize &size)
{
    int level = 0;
    switch (size.height()) {
    case KIconLoader::SizeSmall:
        level = 0;
        break;
    case KIconLoader::SizeSmallMedium:
        level = 1;
        break;
    case KIconLoader::SizeMedium:
        level = 2;
        break;
    case KIconLoader::SizeLarge:
        level = 3;
        break;
    case KIconLoader::SizeHuge:
        level = 4;
        break;
    default:
        level = 4 + ((size.height() - KIconLoader::SizeHuge) >> 4);
    }
    return level;
}
