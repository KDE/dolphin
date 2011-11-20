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

class KDirLister;
class KFileItemModel;
class KItemListController;

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

    void setHiddenFilesShown(bool show);
    bool hiddenFilesShown() const;

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

    /** @see QWidget::contextMenuEvent() */
    virtual void contextMenuEvent(QContextMenuEvent* event);

    /** @see QWidget::keyPressEvent() */
    virtual void keyPressEvent(QKeyEvent* event);

private slots:
    /**
     * Updates the active view to the URL
     * which is given by the item with the index \a index.
     */
    //void updateActiveView(const QModelIndex& index);

    /**
     * Is emitted if URLs have been dropped
     * to the index \a index.
     */
    //void dropUrls(const QModelIndex& index, QDropEvent* event);

    /**
     * Expands the treeview to show the directory
     * specified by \a index.
     */
    //void expandToDir(const QModelIndex& index);

    /**
     * Assures that the leaf folder gets visible.
     */
    //void scrollToLeaf();

    void updateMouseButtons();

    void slotLoadingCompleted();

    void slotHorizontalScrollBarMoved(int value);

    void slotVerticalScrollBarMoved(int value);

private:
    /**
     * Initializes the base URL of the tree and expands all
     * directories until \a url.
     * @param url  URL of the leaf directory that should get expanded.
     */
    void loadTree(const KUrl& url);

    /**
     * Selects the current leaf directory m_leafDir and assures
     * that the directory is visible if the leaf has been set by
     * FoldersPanel::setUrl().
     */
    void selectLeafDirectory();

    KFileItemModel* fileItemModel() const;

private:
    bool m_setLeafVisible;
    Qt::MouseButtons m_mouseButtons;
    KDirLister* m_dirLister;
    KItemListController* m_controller;
    KUrl m_leafDir;
};

#endif // FOLDERSPANEL_H
