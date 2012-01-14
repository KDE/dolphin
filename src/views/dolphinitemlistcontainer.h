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

#ifndef DOLPHINITEMLISTCONTAINER_H
#define DOLPHINITEMLISTCONTAINER_H

#include <kitemviews/kfileitemlistview.h>
#include <kitemviews/kitemlistcontainer.h>
#include <settings/viewmodes/viewmodesettings.h>

#include <libdolphin_export.h>

class KDirLister;
class KFileItemListView;

/**
 * @brief Extends KItemListContainer by Dolphin specific properties.
 *
 * The view and model for KFileItems are created automatically when
 * instantating KItemListContainer.
 *
 * The Dolphin settings of the icons-, compact- and details-view are
 * converted internally to properties that can be used to configure e.g.
 * the item-size and visible roles of the KItemListView.
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinItemListContainer : public KItemListContainer
{
    Q_OBJECT

public:
    explicit DolphinItemListContainer(KDirLister* dirLister,
                                      QWidget* parent = 0);

    virtual ~DolphinItemListContainer();

    void setPreviewsShown(bool show);
    bool previewsShown() const;

    void setVisibleRoles(const QList<QByteArray>& roles);
    QList<QByteArray> visibleRoles() const;

    void setZoomLevel(int level);
    int zoomLevel() const;

    void setItemLayout(KFileItemListView::Layout layout);
    KFileItemListView::Layout itemLayout() const;

    void beginTransaction();
    void endTransaction();

    /**
     * Refreshs the view by reapplying the (changed) viewmode settings.
     */
    void refresh();

private:
    void updateGridSize();
    void updateFont();

    /**
     * Updates the auto activation delay of the itemlist controller
     * dependent on the 'autoExpand' setting from the general settings.
     */
    void updateAutoActivationDelay();

    ViewModeSettings::ViewMode viewMode() const;

private:
    int m_zoomLevel;
    KFileItemListView* m_fileItemListView;
};

#endif
