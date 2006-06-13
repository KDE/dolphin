/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef bookmark_module_h
#define bookmark_module_h

#include <QDropEvent>
#include <QMap>
#include <QObject>

#include <konq_sidebartreemodule.h>
#include <kbookmark.h>
#include <kdialog.h>

class KActionCollection;
class KLineEdit;
class KonqSidebarBookmarkItem;

/**
 * This module displays bookmarks in the tree
 */
class KonqSidebarBookmarkModule : public QObject, public KonqSidebarTreeModule
{
    Q_OBJECT
public:
    KonqSidebarBookmarkModule( KonqSidebarTree * parentTree );
    virtual ~KonqSidebarBookmarkModule();

    // Handle this new toplevel item [can only be called once currently]
    virtual void addTopLevelItem( KonqSidebarTreeTopLevelItem * item );
    virtual bool handleTopLevelContextMenu( KonqSidebarTreeTopLevelItem *, const QPoint& );

    void showPopupMenu();


protected Q_SLOTS:
    void slotBookmarksChanged( const QString & );
    void slotMoved(Q3ListViewItem*,Q3ListViewItem*,Q3ListViewItem*);
    void slotDropped(K3ListView*,QDropEvent*,Q3ListViewItem*,Q3ListViewItem*);
    void slotCreateFolder();
    void slotDelete();
    void slotProperties(KonqSidebarBookmarkItem *bi = 0);
    void slotOpenNewWindow();
    void slotOpenTab();
    void slotCopyLocation();

protected:
    void fillListView();
    void fillGroup( KonqSidebarTreeItem * parentItem, KBookmarkGroup group );
    KonqSidebarBookmarkItem * findByAddress( const QString & address ) const;

private Q_SLOTS:
    void slotOpenChange(Q3ListViewItem*);

private:
    KonqSidebarTreeTopLevelItem * m_topLevelItem;
    KonqSidebarBookmarkItem * m_rootItem;

    KActionCollection *m_collection;

    bool m_ignoreOpenChange;
    QMap<QString, bool> m_folderOpenState;
};

class BookmarkEditDialog : public KDialog
{
    Q_OBJECT

public:
    BookmarkEditDialog( const QString& title, const QString& url,
                        QWidget * = 0, const char * = 0,
                        const QString& caption = i18n( "Add Bookmark" ) );

    QString finalUrl() const;
    QString finalTitle() const;

private:
    KLineEdit *m_title, *m_location;
};

#endif
