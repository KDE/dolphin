/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef VIEWEXTENSIONSFACTORY_H
#define VIEWEXTENSIONSFACTORY_H

#include <QObject>

#include "dolphinview.h"

class DolphinController;
class DolphinFileItemDelegate;
class DolphinSortFilterProxyModel;
class DolphinViewAutoScroller;
class KFilePreviewGenerator;
class FolderExpander;
class QModelIndex;
class SelectionManager;
class ToolTipManager;
class QAbstractItemView;
class VersionControlObserver;

/**
 * @brief Responsible for creating extensions like tooltips and previews
 *        that are available in all view implementations.
 *
 * Each view implementation (iconsview, detailsview, columnview) must
 * instantiate an instance of this class to assure having
 * a common behavior that is independent from the custom functionality of
 * a view implementation.
 */
class ViewExtensionsFactory : public QObject
{
    Q_OBJECT

public:
    explicit ViewExtensionsFactory(QAbstractItemView* view,
                                   DolphinController* controller);
    virtual ~ViewExtensionsFactory();

    /**
     * Must be invoked by the item view, when QAbstractItemView::currentChanged()
     * has been called. Assures that the current item stays visible when it has been
     * changed by the keyboard.
     */
    void handleCurrentIndexChange(const QModelIndex& current, const QModelIndex& previous);

    DolphinFileItemDelegate* fileItemDelegate() const;

    /**
     * Enables the automatically expanding of a folder when dragging
     * items above the folder.
     */
    void setAutoFolderExpandingEnabled(bool enabled);
    bool autoFolderExpandingEnabled() const;

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    void slotZoomLevelChanged();
    void cancelPreviews();
    void slotShowPreviewChanged();
    void slotShowHiddenFilesChanged();
    void slotSortingChanged(DolphinView::Sorting sorting);
    void slotSortOrderChanged(Qt::SortOrder order);
    void slotSortFoldersFirstChanged(bool foldersFirst);
    void slotNameFilterChanged(const QString& nameFilter);
    void slotRequestVersionControlActions(const KFileItemList& items);
    void requestActivation();

private:
    DolphinSortFilterProxyModel* proxyModel() const;

private:
    QAbstractItemView* m_view;
    DolphinController* m_controller;
    ToolTipManager* m_toolTipManager;
    KFilePreviewGenerator* m_previewGenerator;
    SelectionManager* m_selectionManager;
    DolphinViewAutoScroller* m_autoScroller;
    DolphinFileItemDelegate* m_fileItemDelegate;   
    VersionControlObserver* m_versionControlObserver;
    FolderExpander* m_folderExpander;
};

#endif

