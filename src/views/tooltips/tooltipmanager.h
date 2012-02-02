/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#ifndef TOOLTIPMANAGER_H
#define TOOLTIPMANAGER_H

#include <QObject>
#include <QRect>

#include <KFileItem>

class DolphinModel;
class DolphinSortFilterProxyModel;
class FileMetaDataToolTip;
class QAbstractItemView;
class QModelIndex;
class QTimer;

/**
 * @brief Manages the tooltips for an item view.
 *
 * When hovering an item, a tooltip is shown after
 * a short timeout. The tooltip is hidden again when the
 * viewport is hovered or the item view has been left.
 */
class ToolTipManager : public QObject
{
    Q_OBJECT

public:
    explicit ToolTipManager(QWidget* parent);
    virtual ~ToolTipManager();

    /**
     * Triggers the showing of the tooltip for the item \p item
     * where the item has the maximum boundaries of \p itemRect.
     * The tooltip manager takes care that the tooltip is shown
     * slightly delayed.
     */
    void showToolTip(const KFileItem& item, const QRectF& itemRect);

    /**
     * Hides the currently shown tooltip.
     */
    void hideToolTip();

private slots:
    void startContentRetrieval();
    void setPreviewPix(const KFileItem& item, const QPixmap& pix);
    void previewFailed();
    void slotMetaDataRequestFinished();
    void showToolTip();

private:
    /// Timeout from requesting a tooltip until the tooltip
    /// should be shown
    QTimer* m_showToolTipTimer;

    /// Timeout from requesting a tooltip until the retrieving of
    /// the tooltip content like preview and meta data gets started.
    QTimer* m_contentRetrievalTimer;

    FileMetaDataToolTip* m_fileMetaDataToolTip;

    bool m_toolTipRequested;
    bool m_metaDataRequested;
    bool m_appliedWaitCursor;
    int m_margin;
    KFileItem m_item;
    QRect m_itemRect;
};

#endif
