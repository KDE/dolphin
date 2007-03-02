/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>
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

#ifndef TREEVIEWSIDEBARPAGE_H
#define TREEVIEWSIDEBARPAGE_H

#include <sidebarpage.h>

class KDirLister;
class KDirModel;
class KUrl;
class QTreeView;

/**
 * @brief
 */
class TreeViewSidebarPage : public SidebarPage
{
    Q_OBJECT

public:
    TreeViewSidebarPage(DolphinMainWindow* mainWindow, QWidget* parent = 0);
    virtual ~TreeViewSidebarPage();

protected:
    /** @see SidebarPage::activeViewChanged() */
    virtual void activeViewChanged();

private slots:
    /**
     * Updates the current position inside the tree to
     * \a url.
     */
    void updatePosition(const KUrl& url);

private:
    /**
     * Connects to signals from the currently active Dolphin view to get
     * informed about highlighting changes.
     */
    void connectToActiveView();

private:
    KDirLister* m_dirLister;
    KDirModel* m_dirModel;
    QTreeView* m_treeView;
};

#endif // BOOKMARKSSIDEBARPAGE_H
