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
#include <qlistview.h>
#include <gcommand.h>

class KToggleAction;
class KListView;

class KEBListViewItem : public QListViewItem
{
public:
    // toplevel item (there should be only one!)
    KEBListViewItem(QListView *parent, const KBookmark & group );
    // bookmark (first of its group)
    KEBListViewItem(KEBListViewItem *parent, const KBookmark & bk );
    // bookmark (after another)
    KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmark & bk );
    // group
    KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmarkGroup & gp );

    virtual void setOpen( bool );

    const KBookmark & bookmark() { return m_bookmark; }
private:
    KBookmark m_bookmark;
};

class KEBTopLevel : public KMainWindow
{
    Q_OBJECT
public:
    static KEBTopLevel * self() { return s_topLevel; }

    KEBTopLevel( const QString & bookmarksFile );
    virtual ~KEBTopLevel();

    bool save();

    void setModified( bool modified = true );

    KBookmark selectedBookmark() const;

    // @return where to insert a new item - depending on the selected item
    QString insertionAddress() const;

    KEBListViewItem * findByAddress( const QString & address ) const;

    /**
     * Rebuild the whole list view from the dom document
     * Openness of folders is saved, as well as current item.
     * Call this every time the underlying qdom document is modified.
     */
    void update();

    KListView * listView() const { return m_pListView; }

public slots:
    void slotImportNS();
    void slotImportMoz();
    void slotSave();
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
    void slotSelectionChanged();
    void slotContextMenu( KListView *, QListViewItem *, const QPoint & );
    void slotBookmarksChanged();
    void slotCommandExecuted();

protected:
    void fillGroup( KEBListViewItem * parentItem, KBookmarkGroup group );
    virtual bool queryClose();
    void fillListView();

    bool m_bModified;
    KToggleAction * m_taShowNS;
    KListView * m_pListView;
    KCommandHistory m_commandHistory;

    static KEBTopLevel * s_topLevel;
};

#endif

