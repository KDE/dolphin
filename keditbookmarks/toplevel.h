/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __toplevel_h
#define __toplevel_h

#include <kmainwindow.h>
#include <kbookmark.h>

class KToggleAction;
class KListView;
class QListViewItem;
class KEBListViewItem;

class KEBTopLevel : public KMainWindow
{
    Q_OBJECT
public:
    KEBTopLevel( const QString & bookmarksFile );
    virtual ~KEBTopLevel();

    bool save();

    void setModified( bool modified = true );

    KBookmark selectedBookmark() const;

public slots:
    void slotSave();
    void slotUndo();
    void slotRename();
    void slotDelete();
    void slotNewFolder();
    void slotInsertSeparator();
    void slotSort();
    void slotSetAsToolbar();
    void slotOpenLink();
    void slotTestLink();
    void slotShowNS();

protected slots:
    void slotItemRenamed(QListViewItem *, const QString &, int);
    void slotMoved(QListViewItem *, QListViewItem *, QListViewItem *);
    void slotSelectionChanged( QListViewItem * );
    void slotContextMenu( KListView *, QListViewItem *, const QPoint & );
    void fillListView();

protected:
    void fillGroup( KEBListViewItem * parentItem, KBookmarkGroup group );
    virtual bool queryClose();

    bool m_bModified;
    KToggleAction * m_taShowNS;
    KListView * m_pListView;
};

#endif
