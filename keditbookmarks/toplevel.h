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
#include <klistview.h>
#include <kcommand.h>

class KToggleAction;

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
    void init( const KBookmark & bk );
    KBookmark m_bookmark;
};

class KEBListView : public KListView
{
    Q_OBJECT
public:
    KEBListView( QWidget * parent ) : KListView( parent ) {}
    virtual ~KEBListView() {}

public slots:
    virtual void rename( QListViewItem *item, int c );

protected:
    virtual bool acceptDrag( QDropEvent * e ) const;
    virtual QDragObject *dragObject() const;
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
    void slotExportNS();
    void slotImportMoz();
    void slotExportMoz();
    void slotSave();
    void slotDocumentRestored();
    void slotCut();
    void slotCopy();
    void slotPaste();
    void slotRename();
    void slotChangeURL();
    void slotChangeIcon();
    void slotDelete();
    void slotNewFolder();
    void slotNewBookmark();
    void slotInsertSeparator();
    void slotSort();
    void slotSetAsToolbar();
    void slotOpenLink();
    void slotTestLink();
    void slotShowNS();
    void slotConfigureKeyBindings();
    void slotConfigureToolbars();

protected slots:
    void slotItemRenamed(QListViewItem *, const QString &, int);
    void slotDropped(QDropEvent* , QListViewItem* , QListViewItem* );
    void slotSelectionChanged();
    void slotClipboardDataChanged();
    void slotContextMenu( KListView *, QListViewItem *, const QPoint & );
    void slotBookmarksChanged( const QString &, const QString & );
    void slotCommandExecuted();
    void slotNewToolbarConfig();

protected:
    void fillGroup( KEBListViewItem * parentItem, KBookmarkGroup group );
    virtual bool queryClose();
    void fillListView();
    void pasteData( const QString & cmdName, QMimeSource * data, const QString & insertionAddress );
    void itemMoved(QListViewItem * item, const QString & newAddress, bool copy);

    bool m_bModified;
    bool m_bCanPaste;
    KToggleAction * m_taShowNS;
    KListView * m_pListView;
    KCommandHistory m_commandHistory;

    static KEBTopLevel * s_topLevel;
};

#endif

