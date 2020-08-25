/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

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
