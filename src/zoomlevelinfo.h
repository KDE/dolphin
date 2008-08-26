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

#ifndef ZOOMLEVELINFO_H
#define ZOOMLEVELINFO_H

class QSize;

/**
 * @short Helper class for getting information about the zooming
 *        capabilities.
 */
class ZoomLevelInfo {
public:
    static int minimumLevel();
    static int maximumLevel();
    
    /**
     * Helper method for the view implementation to get
     * the icon size for the zoom level \a level that
     * is between the range ZoomLevelInfo::minimumLevel() and
     * ZoomLevelInfo::maximumLevel().
     */
    static int iconSizeForZoomLevel(int level);
    
    /**
     * Helper method for the view implementation to get
     * the zoom level for the icon size \a size that
     * is between the range ZoomLevelInfo::minimumLevel() and
     * ZoomLevelInfo::maximumLevel().
     */
    static int zoomLevelForIconSize(const QSize& size);
};

#endif
