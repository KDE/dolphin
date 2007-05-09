/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef SIDEBARTREEVIEW_H
#define SIDEBARTREEVIEW_H

#include <kurl.h>
#include <QtGui/QTreeView>

/**
 * @brief Tree view widget which is used for the sidebar panel.
 *
 * @see TreeViewSidebarPage
 */
class SidebarTreeView : public QTreeView
{
    Q_OBJECT

public:
    explicit SidebarTreeView(QWidget* parent = 0);
    virtual ~SidebarTreeView();

signals:
    /**
      * Is emitted if the URLs \a urls have been dropped to
      * the index \a index.
      */
    void urlsDropped(const KUrl::List& urls,
                     const QModelIndex& index);

protected:
    virtual bool event(QEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dropEvent(QDropEvent* event);

};

#endif
