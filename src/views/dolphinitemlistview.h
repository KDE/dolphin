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

#ifndef DOLPHINITEMLISTVIEW_H
#define DOLPHINITEMLISTVIEW_H

#include <kitemviews/kfileitemlistview.h>
#include <settings/viewmodes/viewmodesettings.h>

#include <libdolphin_export.h>

class KFileItemListView;

/**
 * @brief Dolphin specific view-implementation.
 *
 * Offers zoom-level support and takes care for translating
 * the view-properties into the corresponding KItemListView
 * properties.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinItemListView : public KFileItemListView
{
    Q_OBJECT

public:
    explicit DolphinItemListView(QGraphicsWidget* parent = 0);
    virtual ~DolphinItemListView();

    void setZoomLevel(int level);
    int zoomLevel() const;

    void readSettings();
    void writeSettings();

protected:
    virtual KItemListWidgetCreatorBase* defaultWidgetCreator() const;
    virtual void onItemLayoutChanged(ItemLayout current, ItemLayout previous);
    virtual void onPreviewsShownChanged(bool shown);
    virtual void onVisibleRolesChanged(const QList<QByteArray>& current,
                                       const QList<QByteArray>& previous);

private:
    void updateGridSize();
    void updateFont();

    ViewModeSettings::ViewMode viewMode() const;

private:
    int m_zoomLevel;
};

#endif
