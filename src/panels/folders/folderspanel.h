/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef FOLDERSPANEL_H
#define FOLDERSPANEL_H

#include <KUrl>
#include <panels/panel.h>

class KFileItemModel;
class KItemListController;
class QGraphicsSceneDragDropEvent;

/**
 * @brief Shows a tree view of the directories starting from
 *        the currently selected place.
 *
 * The tree view is always synchronized with the currently active view
 * from the main window.
 */
class FoldersPanel : public Panel
{
    Q_OBJECT

public:
    FoldersPanel(QWidget* parent = 0);
    virtual ~FoldersPanel();

    void setShowHiddenFiles(bool show);
    bool showHiddenFiles() const;

    void setAutoScrolling(bool enable);
    bool autoScrolling() const;

    void rename(const KFileItem& item);

signals:
    /**
     * Is emitted if the an URL change is requested.
     */
    void changeUrl(const KUrl& url, Qt::MouseButtons buttons);

protected:
    /** @see Panel::urlChanged() */
    virtual bool urlChanged();

    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

    /** @see QWidget::keyPressEvent() */
    virtual void keyPressEvent(QKeyEvent* event);

private slots:
    void slotItemActivated(int index);
    void slotItemMiddleClicked(int index);
    void slotItemContextMenuRequested(int index, const QPointF& pos);
    void slotViewContextMenuRequested(const QPointF& pos);
    void slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event);
    void slotRoleEditingFinished(int index, const QByteArray& role, const QVariant& value);

    void slotLoadingCompleted();

    /**
     * Increases the opacity of the view step by step until it is fully
     * opaque.
     */
    void startFadeInAnimation();

private:
    /**
     * Initializes the base URL of the tree and expands all
     * directories until \a url.
     * @param url  URL of the leaf directory that should get expanded.
     */
    void loadTree(const KUrl& url);

    /**
     * Sets the item with the index \a index as current item, selects
     * the item and assures that the item will be visible.
     */
    void updateCurrentItem(int index);

private:
    bool m_updateCurrentItem;
    KItemListController* m_controller;
    KFileItemModel* m_model;
};

#endif // FOLDERSPANEL_H
