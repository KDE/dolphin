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

#include "toplevel.h"
#include "commands.h"
#include <kaction.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>
#include <kdebug.h>
#include <kstdaction.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <klistview.h>
#include <krun.h>
#include <kiconloader.h>
#include <kapp.h>
#include <qfile.h>
#include <dcopclient.h>
#include <assert.h>
#include <stdlib.h>

// #define DEBUG_ADDRESSES

// toplevel item (there should be only one!)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmark & group )
    : QListViewItem(parent, i18n("Bookmarks") ), m_bookmark(group)
{
    setPixmap(0, SmallIcon("bookmark"));
    setExpandable(true); // Didn't know this was necessary :)
#ifdef DEBUG_ADDRESSES
    setText(2, bk.address());
#endif
}

// bookmark (first of its group)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, const KBookmark & bk )
    : QListViewItem(parent, bk.fullText(), bk.url()), m_bookmark(bk)
{
    setPixmap(0, SmallIcon( bk.icon() ) );
#ifdef DEBUG_ADDRESSES
    setText(2, bk.address());
#endif
}

// bookmark (after another)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmark & bk )
    : QListViewItem(parent, after, bk.fullText(), bk.url()), m_bookmark(bk)
{
    setPixmap(0, SmallIcon( bk.icon() ) );
#ifdef DEBUG_ADDRESSES
    setText(2, bk.address());
#endif
}

// group
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmarkGroup & gp )
    : QListViewItem(parent, after, gp.fullText()), m_bookmark(gp)
{
    setPixmap(0, SmallIcon( gp.icon() ) );
    setExpandable(true);
#ifdef DEBUG_ADDRESSES
    setText(2, gp.address());
#endif
}

void KEBListViewItem::setOpen( bool open )
{
    m_bookmark.internalElement().setAttribute( "OPEN", open ? 1 : 0 );
    QListViewItem::setOpen( open );
}

class KEBListView : public KListView
{
public:
    KEBListView( QWidget * parent ) : KListView( parent ) {}
    virtual ~KEBListView() {}

protected:
    // KListView is no good for undoable operations. It moves stuff before telling us.
    virtual void contentsDropEvent(QDropEvent* e)
    {
        cleanDropVisualizer();

        if (acceptDrag (e))
        {
            e->accept();
            QListViewItem *afterme;
            QListViewItem *parent;
            findDrop(e->pos(), parent, afterme);

            // Now a simplified version of movableDropEvent
            //movableDropEvent (parent, afterme);
            QListViewItem * i = selectedItem();
            ASSERT(i);
            if (i != afterme)
            {

                // sanity check - don't move a item into it's own child structure
                QListViewItem *chk = parent;
                while(chk)
                {
                    if(chk == i)
                        return;
                    chk = chk->parent();
                }


                // Note the abuse of the 2nd argument ! Instead of "old after",
                // I'm using it for passing the "new parent". Well, KListView
                // should really pass both (KDE 3.0).
                emit moved(i, parent, afterme);
            }
        }
    }
};

KEBTopLevel * KEBTopLevel::s_topLevel = 0L;

KEBTopLevel::KEBTopLevel( const QString & bookmarksFile )
    : KMainWindow(), m_commandHistory( actionCollection() )
{
    // Create the bookmark manager.
    // It will be available in KBookmarkManager::self() from now.
    (void) new KBookmarkManager( bookmarksFile, false );

    // Create the list view
    m_pListView = new KEBListView( this );
    m_pListView->setDragEnabled( true );
    m_pListView->addColumn( i18n("Bookmark"), 300 );
    m_pListView->addColumn( i18n("URL"), 300 );
#ifdef DEBUG_ADDRESSES
    m_pListView->addColumn( "Address", 100 );
#endif

    m_pListView->setRootIsDecorated( true );
    m_pListView->setRenameable( 0 );
    m_pListView->setRenameable( 1 );
    m_pListView->setItemsRenameable( true );
    m_pListView->setItemsMovable( true );
    m_pListView->setAcceptDrops( true );
    m_pListView->setAllColumnsShowFocus( true );
    m_pListView->setSorting(-1, false);
    setCentralWidget( m_pListView );
    resize( m_pListView->sizeHint().width(), 400 );

    connect( m_pListView, SIGNAL(itemRenamed(QListViewItem *, const QString &, int)),
             SLOT(slotItemRenamed(QListViewItem *, const QString &, int)) );
    connect( m_pListView, SIGNAL(moved (QListViewItem *, QListViewItem *, QListViewItem *)),
             SLOT(slotMoved(QListViewItem *, QListViewItem *, QListViewItem *)) );
    connect( m_pListView, SIGNAL(selectionChanged() ),
             SLOT(slotSelectionChanged() ) );
    connect( m_pListView, SIGNAL(contextMenu( KListView *, QListViewItem *, const QPoint & )),
             SLOT(slotContextMenu( KListView *, QListViewItem *, const QPoint & )) );
    // If someone plays with konq's bookmarks while we're open, update.
    connect( KBookmarkManager::self(), SIGNAL(changed(const QString &) ),
             SLOT( slotBookmarksChanged() ) );
    // Update GUI after executing command
    connect (&m_commandHistory, SIGNAL( commandExecuted() ), SLOT( slotCommandExecuted() ) );

    fillListView();

    // Create the actions

    KAction * act = new KAction( i18n( "Import Netscape Bookmarks" ), "netscape", 0, this, SLOT( slotImportNS() ), actionCollection(), "file_importNS" );
    act->setEnabled( QFile::exists( KBookmarkImporter::netscapeBookmarksFile() ) );
    act = new KAction( i18n( "Import Mozilla Bookmarks" ), "mozilla", 0, this, SLOT( slotImportMoz() ), actionCollection(), "file_importMoz" );
    act->setEnabled( QFile::exists( KBookmarkImporter::mozillaBookmarksFile() ) );
    // TODO export to NS and Moz
    (void) KStdAction::save( this, SLOT( slotSave() ), actionCollection() );
    (void) KStdAction::close( this, SLOT( close() ), actionCollection() );
    (void) new KAction( i18n( "&Delete" ), "editdelete", SHIFT+Key_Delete, this, SLOT( slotDelete() ), actionCollection(), "edit_delete" );
    (void) new KAction( i18n( "&Rename" ), "text", Key_F2, this, SLOT( slotRename() ), actionCollection(), "edit_rename" );
    (void) new KAction( i18n( "&Create New Folder" ), "folder_new", CTRL+Key_N, this, SLOT( slotNewFolder() ), actionCollection(), "edit_newfolder" );
    (void) new KAction( i18n( "&Insert separator" ), CTRL+Key_I, this, SLOT( slotInsertSeparator() ), actionCollection(), "edit_insertseparator" );
    (void) new KAction( i18n( "&Sort alphabetically" ), 0, this, SLOT( slotSort() ), actionCollection(), "edit_sort" );
    (void) new KAction( i18n( "Set As &Toolbar Folder" ), 0, this, SLOT( slotSetAsToolbar() ), actionCollection(), "edit_setastoolbar" );
    (void) new KAction( i18n( "&Open Link" ), "fileopen", 0, this, SLOT( slotOpenLink() ), actionCollection(), "edit_openlink" );
    (void) new KAction( i18n( "Test &Link" ), "bookmark", 0, this, SLOT( slotTestLink() ), actionCollection(), "edit_testlink" );
    // TODO change icon (calls KIconDialog) ?
    m_taShowNS = new KToggleAction( i18n( "Show Netscape Bookmarks in Konqueror windows" ), 0, this, SLOT( slotShowNS() ), actionCollection(), "settings_showNS" );

    m_taShowNS->setChecked( KBookmarkManager::self()->showNSBookmarks() );

    actionCollection()->action("edit_sort")->setEnabled(false); // not implemented
    actionCollection()->action("edit_testlink")->setEnabled(false); // not implemented

    slotSelectionChanged();

    createGUI();

    setModified(false); // for a nice caption

    s_topLevel = this;
}

KEBTopLevel::~KEBTopLevel()
{
    s_topLevel = 0L;
}

void KEBTopLevel::slotSave()
{
    (void)save();
}

bool KEBTopLevel::save()
{
    bool ok = KBookmarkManager::self()->save();
    if (ok)
    {
        QByteArray data;
        // We don't want to notify ourselves (keditbookmarks), because this would
        // call slotBookmarksChanged, which clears the history.
        // There's probably a better solution than hardcoding konq/kdesktop, but not at 4:47am.
        kapp->dcopClient()->send( "konqueror*", "KBookmarkManager", "notifyCompleteChange()", data );
        kapp->dcopClient()->send( "kdesktop", "KBookmarkManager", "notifyCompleteChange()", data );
        setModified( false );
    }
    return ok;
}

QString KEBTopLevel::insertionAddress() const
{
    KBookmark current = selectedBookmark();
    if (current.isGroup())
        // In a group, we insert as first child
        return current.address() + "/0";
    else
        // Otherwise, as next sibling
        return KBookmark::nextAddress( current.address() );
}

KEBListViewItem * KEBTopLevel::findByAddress( const QString & address ) const
{
    kdDebug() << "KEBTopLevel::findByAddress " << address << endl;
    QListViewItem * item = m_pListView->firstChild();
    // The address is something like /5/10/2
    QStringList addresses = QStringList::split('/',address);
    for ( QStringList::Iterator it = addresses.begin() ; it != addresses.end() ; ++it )
    {
        uint number = (*it).toUInt();
        //kdDebug() << "KBookmarkManager::findByAddress " << number << endl;
        assert(item);
        item = item->firstChild();
        for ( uint i = 0 ; i < number ; ++i )
        {
            assert(item);
            item = item->nextSibling();
        }
    }
    ASSERT(item);
    return static_cast<KEBListViewItem *>(item);
}

void KEBTopLevel::slotRename()
{
    ASSERT( m_pListView->selectedItem() );
    m_pListView->rename( m_pListView->selectedItem(), 0 );
}

void KEBTopLevel::slotDelete()
{
    if( !m_pListView->selectedItem() )
    {
        kdWarning() << "KEBTopLevel::slotDelete no selected item !" << endl;
        return;
    }
    KBookmark bk = selectedBookmark();
    kdDebug() << "KEBTopLevel::slotDelete child count=" << bk.internalElement().childNodes().count() << endl;
    if ( bk.isGroup() && bk.internalElement().childNodes().count() > 1 /*there's always "TEXT"*/ )
    {
        if ( KMessageBox::questionYesNo( this, i18n("This is a bookmark folder. Are you sure you want to delete it ?\nThis operation can't be undone."),
                                         i18n("Confirmation required") ) == KMessageBox::No )
            return;
    }

    DeleteCommand * cmd = new DeleteCommand( i18n("Delete item"), bk.address() );
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotNewFolder()
{
    CreateCommand * cmd = new CreateCommand( i18n("Create Folder"), insertionAddress(), QString::null );
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotInsertSeparator()
{
    CreateCommand * cmd = new CreateCommand( i18n("Insert separator"), insertionAddress() );
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotImportNS()
{
    ImportCommand * cmd = new ImportCommand( i18n("Import Netscape Bookmarks"), KBookmarkImporter::netscapeBookmarksFile(),
                                             i18n("Netscape Bookmarks"), "netscape");
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotImportMoz()
{
    ImportCommand * cmd = new ImportCommand( i18n("Import Mozilla Bookmarks"), KBookmarkImporter::mozillaBookmarksFile(),
                                             i18n("Mozilla Bookmarks"), "mozilla");
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotSort()
{

}

void KEBTopLevel::slotSetAsToolbar()
{
    KMacroCommand * cmd = new KMacroCommand("Set as Bookmark Toolbar");

    KBookmarkGroup oldToolbar = KBookmarkManager::self()->toolbar();
    if (!oldToolbar.isNull())
    {
        QValueList<EditCommand::Edition> lst;
        lst.append(EditCommand::Edition( "TOOLBAR", "0" ));
        lst.append(EditCommand::Edition( "ICON", "" ));
        EditCommand * cmd1 = new EditCommand("", oldToolbar.address(), lst);
        cmd->addCommand(cmd1);
    }

    KBookmark bk = selectedBookmark();
    ASSERT( bk.isGroup() );
    QValueList<EditCommand::Edition> lst;
    lst.append(EditCommand::Edition( "TOOLBAR", "1" ));
    lst.append(EditCommand::Edition( "ICON", "bookmark_toolbar" ));
    EditCommand * cmd2 = new EditCommand("", bk.address(), lst);
    cmd->addCommand(cmd2);

    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotOpenLink()
{
    KBookmark bk = selectedBookmark();
    ASSERT( !bk.isGroup() );
    (void) new KRun( bk.url() );
}

void KEBTopLevel::slotTestLink()
{

}

void KEBTopLevel::slotShowNS()
{
    QDomElement rootElem = KBookmarkManager::self()->root().internalElement();
    QString attr = "HIDE_NSBK";
    rootElem.setAttribute(attr, rootElem.attribute(attr) == "1" ? "0" : "1");
    setModified(); // one will need to save, to get konq to notice the change
    // If that's bad, then we need to put this flag in a KConfig.
}

void KEBTopLevel::setModified( bool modified )
{
    m_bModified = modified;
    setCaption( i18n("Bookmark Editor"), m_bModified );
    actionCollection()->action("file_save")->setEnabled( m_bModified );
}

KBookmark KEBTopLevel::selectedBookmark() const
{
    QListViewItem * item = m_pListView->selectedItem();
    ASSERT(item);
    KEBListViewItem * kebItem = static_cast<KEBListViewItem *>(item);
    return kebItem->bookmark();
}

void KEBTopLevel::slotItemRenamed(QListViewItem * item, const QString & newText, int column)
{
    ASSERT(item);
    KEBListViewItem * kebItem = static_cast<KEBListViewItem *>(item);
    KBookmark bk = kebItem->bookmark();
    switch (column) {
        case 0:
            if ( bk.fullText() != newText )
            {
                RenameCommand * cmd = new RenameCommand( i18n("Renaming"), bk.address(), newText );
                m_commandHistory.addCommand( cmd );
            }
            break;
        case 1:
            if ( bk.url() != newText )
            {
                EditCommand * cmd = new EditCommand( i18n("URL change"), bk.address(),
                                                     EditCommand::Edition("URL", newText) );
                m_commandHistory.addCommand( cmd );
            }
            break;
        default:
            kdWarning() << "No such column " << column << endl;
            break;
    }
}

void KEBTopLevel::slotMoved(QListViewItem *_item, QListViewItem * _newParent, QListViewItem *_afterNow)
{
    // The whole moving-items thing is screwed up in KListView
    // Try moving something as the first item of a group -> will be inserted as
    // the sibling instead.
    // It should be like netscape, where you can drop between items or _on_ a
    // group, which sets as first item of the group. I'm afraid it means a
    // "new parent" item in the signal

    kdDebug() << "KEBTopLevel::slotMoved _item=" << _item << " _afterNow=" << _afterNow << endl;
    KEBListViewItem * item = static_cast<KEBListViewItem *>(_item);
    KEBListViewItem * newParent = static_cast<KEBListViewItem *>(_newParent);
    KEBListViewItem * afterNow = static_cast<KEBListViewItem *>(_afterNow);
    if (!_newParent) // Not allowed to drop something before the root item !
    {
        return;
    }

    QString newAddress;
#if 0
    if (newParent.isNull())
    {
        // newParent can be null, if afterNow is the root item. In this case, set as first child
        // This is a very special case, in fact. The more generic solution would be to
        // allow insertions into a group. See above.
        /*m_pListView->takeItem( item );
        m_pListView->firstChild()->insertItem( item );
        afterNow->bookmark().internalElement().insertBefore( item->bookmark().internalElement(), QDomNode() );
        */
        newAddress="/0";
    }
    else
#endif

    if ( afterNow )
        // We move as the next child of afterNow
        newAddress = KBookmark::nextAddress( afterNow->bookmark().address() );
    else
        // We move as first child of newParent
        newAddress = newParent->bookmark().address() + "/0";

    QString oldAddress = item->bookmark().address();
    if ( oldAddress != newAddress )
    {
        kdDebug() << "KEBTopLevel::slotMoved moving " << oldAddress << " to " << newAddress << endl;

        MoveCommand * cmd = new MoveCommand( i18n("Move %1").arg(item->bookmark().text()),
                                             oldAddress, newAddress );
        m_commandHistory.addCommand( cmd );
    }
}

void KEBTopLevel::slotSelectionChanged()
{
    QListViewItem * item = m_pListView->selectedItem();
    kdDebug() << "KEBTopLevel::slotSelectionChanged " << item << endl;
    bool itemSelected = (item != 0L);
    bool group = false;
    bool root = false;
    bool separator = false;
    if ( itemSelected )
    {
        KEBListViewItem * kebItem = static_cast<KEBListViewItem *>(item);
        group = kebItem->bookmark().isGroup();
        separator = kebItem->bookmark().isSeparator();
        root = (m_pListView->firstChild() == item);
    }

    KActionCollection * coll = actionCollection();

    coll->action("edit_rename")->setEnabled(itemSelected && !separator && !root);
    coll->action("edit_delete")->setEnabled(itemSelected && !root);
    coll->action("edit_newfolder")->setEnabled(itemSelected);
    coll->action("edit_insertseparator")->setEnabled(itemSelected);
    //coll->action("edit_sort")->setEnabled(group); // not implemented
    coll->action("edit_setastoolbar")->setEnabled(group);
    coll->action("edit_openlink")->setEnabled(itemSelected && !group && !separator);
    //coll->action("edit_testlink")->setEnabled(itemSelected && !group && !separator); // not implemented
}

void KEBTopLevel::slotContextMenu( KListView *, QListViewItem * _item, const QPoint &p )
{
    if (_item)
    {
        KEBListViewItem * item = static_cast<KEBListViewItem *>(_item);
        if ( item->bookmark().isGroup() )
        {
            QWidget* popup = factory()->container("popup_folder", this);
            if ( popup )
                static_cast<QPopupMenu*>(popup)->popup(p);
        }
        else
        {
            QWidget* popup = factory()->container("popup_bookmark", this);
            if ( popup )
                static_cast<QPopupMenu*>(popup)->popup(p);
        }
    }
}

void KEBTopLevel::slotBookmarksChanged()
{
    kdDebug() << "KEBTopLevel::slotBookmarksChanged" << endl;
    // This is called when someone changes bookmarks in konqueror....
    m_commandHistory.clear();
    fillListView();
}

void KEBTopLevel::update()
{
    QListViewItem * item = m_pListView->selectedItem();
    if (item)
    {
        kdDebug() << "KEBTopLevel::update item=" << item << endl;
        QString address = static_cast<KEBListViewItem*>(item)->bookmark().address();
        fillListView();
        KEBListViewItem * newItem = findByAddress( address );
        ASSERT(newItem);
        if (newItem)
        {
            m_pListView->setCurrentItem(newItem);
            m_pListView->setSelected(newItem,true);
        }
    }
    else
    {
        fillListView();
        slotSelectionChanged();
    }
}

void KEBTopLevel::fillListView()
{
    m_pListView->clear();
    KBookmarkGroup root = KBookmarkManager::self()->root();
    // Create root item
    KEBListViewItem * rootItem = new KEBListViewItem( m_pListView, root );
    fillGroup( rootItem, root );
    rootItem->QListViewItem::setOpen(true);
}

void KEBTopLevel::fillGroup( KEBListViewItem * parentItem, KBookmarkGroup group )
{
    KEBListViewItem * lastItem = 0L;
    for ( KBookmark bk = group.first() ; !bk.isNull() ; bk = group.next(bk) )
    {
        //kdDebug() << "KEBTopLevel::fillGroup group=" << group.text() << " bk=" << bk.text() << endl;
        if ( bk.isGroup() )
        {
            KBookmarkGroup grp = bk.toGroup();
            KEBListViewItem * item = new KEBListViewItem( parentItem, lastItem, grp );
            fillGroup( item, grp );
            if (grp.internalElement().attribute("OPEN") == "1")
                item->QListViewItem::setOpen(true); // no need to save it again :)
            lastItem = item;
        }
        else
        {
            lastItem = new KEBListViewItem( parentItem, lastItem, bk );
        }
    }
}

bool KEBTopLevel::queryClose()
{
    if (m_bModified)
    {
        switch ( KMessageBox::warningYesNoCancel( this,
                                                  i18n("Save changes ?")) ) {
            case KMessageBox::Yes :
                return save();
            case KMessageBox::No :
                return true;
            default: // cancel
                return false;
        }
    }
    return true;
}

///////////////////

void KEBTopLevel::slotCommandExecuted()
{
    KEBTopLevel::self()->setModified();
    KEBTopLevel::self()->update();     // Update GUI
}

#include "toplevel.moc"
