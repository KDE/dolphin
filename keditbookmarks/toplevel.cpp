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
#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>
#include <kbookmarkexporter.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kkeydialog.h>
#include <kstdaction.h>
#include <klocale.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kiconloader.h>
#include <kicondialog.h>
#include <kapp.h>
#include <qclipboard.h>
#include <qfile.h>
#include <dcopclient.h>
#include <assert.h>
#include <stdlib.h>

//#define DEBUG_ADDRESSES

// toplevel item (there should be only one!)
KEBListViewItem::KEBListViewItem(QListView *parent, const KBookmark & group )
    : QListViewItem(parent, i18n("Bookmarks") ), m_bookmark(group)
{
    setPixmap(0, SmallIcon("bookmark"));
    setExpandable(true); // Didn't know this was necessary :)
}

// bookmark (first of its group)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, const KBookmark & bk )
    : QListViewItem(parent, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk)
{
    init(bk);
}

// bookmark (after another)
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmark & bk )
    : QListViewItem(parent, after, bk.fullText(), bk.url().prettyURL()), m_bookmark(bk)
{
    init(bk);
}

// group
KEBListViewItem::KEBListViewItem(KEBListViewItem *parent, QListViewItem *after, const KBookmarkGroup & gp )
    : QListViewItem(parent, after, gp.fullText()), m_bookmark(gp)
{
    init(gp);
    setExpandable(true);
}

void KEBListViewItem::init( const KBookmark & bk )
{
    setPixmap(0, SmallIcon( bk.icon() ) );
#ifdef DEBUG_ADDRESSES
    setText(2, bk.address());
#endif
}

void KEBListViewItem::setOpen( bool open )
{
    m_bookmark.internalElement().setAttribute( "folded", open ? "no" : "yes" );
    QListViewItem::setOpen( open );
}

void KEBListView::rename( QListViewItem *_item, int c )
{
    KEBListViewItem * item = static_cast<KEBListViewItem *>(_item);
    if ( !(item->bookmark().isGroup() && c == 1) && !item->bookmark().isSeparator() && ( firstChild() != item) )
   	    KListView::rename( _item, c );
}

bool KEBListView::acceptDrag(QDropEvent * e) const
{
    return e->source() == viewport() || KBookmarkDrag::canDecode( e );
}

QDragObject *KEBListView::dragObject() const
{
    if (!currentItem())
        return 0;

    KBookmark bk = KEBTopLevel::self()->selectedBookmark();
    KBookmarkDrag * drag = KBookmarkDrag::newDrag( bk, viewport() /*not sure why klistview does it this way*/ );
    drag->setPixmap( SmallIcon(bk.icon()) );
    return drag;
}

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
    m_pListView->setItemsMovable( false ); // We move items ourselves (for undo)
    m_pListView->setAcceptDrops( true );
    m_pListView->setDropVisualizer( true );
    m_pListView->setDragEnabled( true );
    m_pListView->setAllColumnsShowFocus( true );
    m_pListView->setSorting(-1, false);
    setCentralWidget( m_pListView );
    resize( m_pListView->sizeHint().width(), 400 );

    connect( m_pListView, SIGNAL(itemRenamed(QListViewItem *, const QString &, int)),
             SLOT(slotItemRenamed(QListViewItem *, const QString &, int)) );
    connect( m_pListView, SIGNAL(dropped (QDropEvent* , QListViewItem* , QListViewItem* )),
             SLOT(slotDropped(QDropEvent* , QListViewItem* , QListViewItem* )) );
    connect( m_pListView, SIGNAL(contextMenu( KListView *, QListViewItem *, const QPoint & )),
             SLOT(slotContextMenu( KListView *, QListViewItem *, const QPoint & )) );
    connect( m_pListView, SIGNAL(selectionChanged() ),
             SLOT(slotSelectionChanged() ) );
    connect( kapp->clipboard(), SIGNAL(dataChanged()),
             SLOT(slotClipboardDataChanged() ) );
    // If someone plays with konq's bookmarks while we're open, update.
    connect( KBookmarkManager::self(), SIGNAL( changed(const QString &, const QString &) ),
             SLOT( slotBookmarksChanged(const QString &, const QString &) ) );
    // Update GUI after executing command
    connect( &m_commandHistory, SIGNAL( commandExecuted() ), SLOT( slotCommandExecuted() ) );
    connect( &m_commandHistory, SIGNAL( documentRestored() ), SLOT( slotDocumentRestored() ) );

    fillListView();

    // Create the actions

    KAction * act = new KAction( i18n( "Import Netscape Bookmarks" ), "netscape", 0, this, SLOT( slotImportNS() ), actionCollection(), "importNS" );
    act->setEnabled( QFile::exists( KNSBookmarkImporter::netscapeBookmarksFile() ) );
    (void) new KAction( i18n( "Export To Netscape Bookmarks" ), "netscape", 0, this, SLOT( slotExportNS() ), actionCollection(), "exportNS" );
    act = new KAction( i18n( "Import Mozilla Bookmarks" ), "mozilla", 0, this, SLOT( slotImportMoz() ), actionCollection(), "importMoz" );
    (void) new KAction( i18n( "Export To Mozilla Bookmarks" ), "mozilla", 0, this, SLOT( slotExportMoz() ), actionCollection(), "exportMoz" );
    (void) KStdAction::save( this, SLOT( slotSave() ), actionCollection() );
    (void) KStdAction::quit( this, SLOT( close() ), actionCollection() );
    (void) KStdAction::cut( this, SLOT( slotCut() ), actionCollection() );
    (void) KStdAction::copy( this, SLOT( slotCopy() ), actionCollection() );
    (void) KStdAction::paste( this, SLOT( slotPaste() ), actionCollection() );
    (void) KStdAction::keyBindings( this, SLOT( slotConfigureKeyBindings() ), actionCollection() );
    (void) KStdAction::configureToolbars( this, SLOT( slotConfigureToolbars() ), actionCollection() );
    (void) new KAction( i18n( "&Delete" ), "editdelete", Key_Delete, this, SLOT( slotDelete() ), actionCollection(), "delete" );
    (void) new KAction( i18n( "&Rename" ), "text", Key_F2, this, SLOT( slotRename() ), actionCollection(), "rename" );
    (void) new KAction( i18n( "Change &URL" ), "text", Key_F3, this, SLOT( slotChangeURL() ), actionCollection(), "changeurl" );
    (void) new KAction( i18n( "Chan&ge Icon" ), 0, this, SLOT( slotChangeIcon() ), actionCollection(), "changeicon" );
    (void) new KAction( i18n( "&Create New Folder" ), "folder_new", CTRL+Key_N, this, SLOT( slotNewFolder() ), actionCollection(), "newfolder" );
    (void) new KAction( i18n( "&Create New Bookmark" ), "www", 0, this, SLOT( slotNewBookmark() ), actionCollection(), "newbookmark" );
    (void) new KAction( i18n( "&Insert separator" ), CTRL+Key_I, this, SLOT( slotInsertSeparator() ), actionCollection(), "insertseparator" );
    (void) new KAction( i18n( "&Sort alphabetically" ), 0, this, SLOT( slotSort() ), actionCollection(), "sort" );
    (void) new KAction( i18n( "Set As &Toolbar Folder" ), "bookmark_toolbar", 0, this, SLOT( slotSetAsToolbar() ), actionCollection(), "setastoolbar" );
    (void) new KAction( i18n( "&Open In Konqueror" ), "fileopen", 0, this, SLOT( slotOpenLink() ), actionCollection(), "openlink" );
    (void) new KAction( i18n( "Test &Link" ), "bookmark", 0, this, SLOT( slotTestLink() ), actionCollection(), "testlink" );
    m_taShowNS = new KToggleAction( i18n( "Show Netscape Bookmarks in Konqueror windows" ), 0, this, SLOT( slotShowNS() ), actionCollection(), "settings_showNS" );

    m_taShowNS->setChecked( KBookmarkManager::self()->showNSBookmarks() );

    actionCollection()->action("testlink")->setEnabled(false); // not implemented

    slotSelectionChanged();
    slotClipboardDataChanged();

    createGUI();

    setAutoSaveSettings();
    setModified(false); // for a nice caption
    m_commandHistory.documentSaved();

    s_topLevel = this;
    KGlobal::locale()->insertCatalogue("libkonq");
}

KEBTopLevel::~KEBTopLevel()
{
    s_topLevel = 0L;
}

void KEBTopLevel::slotConfigureKeyBindings()
{
    KKeyDialog::configureKeys(actionCollection(), xmlFile());
}

void KEBTopLevel::slotConfigureToolbars()
{
    saveMainWindowSettings( KGlobal::config(), "MainWindow" );
    KEditToolbar dlg(actionCollection());
    connect(&dlg,SIGNAL(newToolbarConfig()),this,SLOT(slotNewToolbarConfig()));
    if (dlg.exec())
    {
        createGUI();
    }
}

void KEBTopLevel::slotNewToolbarConfig() // This is called when OK or Apply is clicked
{
    applyMainWindowSettings( KGlobal::config(), "MainWindow" );
}

void KEBTopLevel::slotSelectionChanged()
{
    QListViewItem * item = m_pListView->selectedItem();
    if (item) {
        kdDebug() << "KEBTopLevel::slotSelectionChanged " << (static_cast<KEBListViewItem *>(item))->bookmark().address() << endl;
    }
    bool itemSelected = (item != 0L);
    bool group = false;
    bool root = false;
    bool separator = false;
    bool urlIsEmpty = false;
    if ( itemSelected )
    {
        KEBListViewItem * kebItem = static_cast<KEBListViewItem *>(item);
        group = kebItem->bookmark().isGroup();
        separator = kebItem->bookmark().isSeparator();
        root = (m_pListView->firstChild() == item);
        urlIsEmpty= kebItem->bookmark().url().isEmpty();
    }

    KActionCollection * coll = actionCollection();

    coll->action("edit_cut")->setEnabled(itemSelected && !root);
    coll->action("edit_copy")->setEnabled(itemSelected && !root);
    coll->action("edit_paste")->setEnabled(itemSelected && !root && m_bCanPaste);
    coll->action("rename")->setEnabled(itemSelected && !separator && !root);
    coll->action("changeurl")->setEnabled(itemSelected && !group && !separator && !root);
    coll->action("delete")->setEnabled(itemSelected && !root);
    coll->action("newfolder")->setEnabled(itemSelected);
    coll->action("changeicon")->setEnabled(itemSelected && !root && !separator);
    coll->action("insertseparator")->setEnabled(itemSelected);
    coll->action("newbookmark")->setEnabled(itemSelected);
    coll->action("sort")->setEnabled(group);
    coll->action("setastoolbar")->setEnabled(group);
    coll->action("openlink")->setEnabled(itemSelected && !group && !separator && !urlIsEmpty);
    //coll->action("testlink")->setEnabled(itemSelected && !group && !separator); // not implemented
}

void KEBTopLevel::slotClipboardDataChanged()
{
    kdDebug() << "KEBTopLevel::slotClipboardDataChanged" << endl;
    QMimeSource *data = QApplication::clipboard()->data();
    m_bCanPaste = KBookmarkDrag::canDecode( data );
    slotSelectionChanged();
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
        QString data( kapp->name() );
        kapp->dcopClient()->send( "*", "KBookmarkManager", "notifyCompleteChange(QString)", data );
        setModified( false );
        m_commandHistory.documentSaved();
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
    if ( m_pListView->selectedItem() )
      m_pListView->rename( m_pListView->selectedItem(), 0 );
}

void KEBTopLevel::slotChangeURL()
{
    ASSERT( m_pListView->selectedItem() );
    if ( m_pListView->selectedItem() )
      m_pListView->rename( m_pListView->selectedItem(), 1 );
}

void KEBTopLevel::slotDelete()
{
    if( !m_pListView->selectedItem() )
    {
        kdWarning() << "KEBTopLevel::slotDelete no selected item !" << endl;
        return;
    }
    KBookmark bk = selectedBookmark();
    DeleteCommand * cmd = new DeleteCommand( i18n("Delete item"), bk.address() );
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotNewFolder()
{
    if( !m_pListView->selectedItem() )
    {
        kdWarning() << "KEBTopLevel::slotNewFolder no selected item !" << endl;
        return;
    }
    // EVIL HACK
    // We need to ask for the folder name before creating the command, in case of "Cancel".
    // But in message-freeze time, impossible to add i18n()s. So... we have to call the existing code :
    QDomDocument doc("xbel"); // Dummy document
    QDomElement elem = doc.createElement("xbel");
    doc.appendChild( elem );
    KBookmarkGroup grp( elem ); // Dummy group
    KBookmark bk = grp.createNewFolder( QString::null ); // Asks for the name
    if ( !bk.fullText().isEmpty() ) // Not canceled
    {
        CreateCommand * cmd = new CreateCommand( i18n("Create Folder"), insertionAddress(), bk.fullText(),bk.icon() , true /*open*/ );
        m_commandHistory.addCommand( cmd );
    }
}

void KEBTopLevel::slotNewBookmark()
{
    if( !m_pListView->selectedItem() )
    {
        kdWarning() << "KEBTopLevel::slotNewBookmark no selected item !" << endl;
        return;
    }
    CreateCommand * cmd = new CreateCommand( i18n("Create bookmark" ), insertionAddress(), QString::null, QString::null, KURL() );
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotInsertSeparator()
{
    CreateCommand * cmd = new CreateCommand( i18n("Insert separator"), insertionAddress() );
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotImportNS()
{
    // Hmm, there's no questionYesNoCancel...
    int answer = KMessageBox::questionYesNo( this, i18n("Import as a new subfolder or replace all the current bookmarks ?"),
                                             i18n("Netscape Import"), i18n("As New Folder"), i18n("Replace") );
    bool subFolder = (answer==KMessageBox::Yes);
    ImportCommand * cmd = new ImportCommand( i18n("Import Netscape Bookmarks"), KNSBookmarkImporter::netscapeBookmarksFile(),
                                             subFolder ? i18n("Netscape Bookmarks") : QString::null, "netscape", false);
    m_commandHistory.addCommand( cmd );

    // Ok, we don't need the dynamic menu anymore
    if ( m_taShowNS->isChecked() )
        m_taShowNS->activate();
}

void KEBTopLevel::slotImportMoz()
{
    // Hmm, there's no questionYesNoCancel...
    int answer = KMessageBox::questionYesNo( this, i18n("Import as a new subfolder or replace all the current bookmarks ?"),
                                             i18n("Mozilla Import"), i18n("As New Folder"), i18n("Replace") );
    bool subFolder = (answer==KMessageBox::Yes);
    ImportCommand * cmd = new ImportCommand( i18n("Import Mozilla Bookmarks"), KNSBookmarkImporter::mozillaBookmarksFile(),
                                             subFolder ? i18n("Mozilla Bookmarks") : QString::null, "mozilla", true);
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotExportNS()
{
    QString path = KNSBookmarkImporter::netscapeBookmarksFile(true);
    if (!path.isEmpty())
    {
        KNSBookmarkExporter exporter( path );
        exporter.write( false );
    }
}

void KEBTopLevel::slotExportMoz()
{
    QString path = KNSBookmarkImporter::mozillaBookmarksFile(true);
    if (!path.isEmpty())
    {
        KNSBookmarkExporter exporter( path );
        exporter.write( true );
    }
}

void KEBTopLevel::slotCut()
{
    KBookmark bk = selectedBookmark();
    ASSERT(!bk.isNull());
    slotCopy();
    // Very much like slotDelete, except for the name
    DeleteCommand * cmd = new DeleteCommand( i18n("Cut item"), bk.address() );
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotCopy()
{
    KBookmark bk = selectedBookmark();
    ASSERT(!bk.isNull());
    // This is not a command, because it can't be undone

    KBookmarkDrag* data = KBookmarkDrag::newDrag( bk, 0L /* not this ! */ );
    QApplication::clipboard()->setData( data );
    slotClipboardDataChanged(); // don't ask
}

void KEBTopLevel::slotPaste()
{
    pasteData( i18n("Paste"), QApplication::clipboard()->data(), insertionAddress() );
}

void KEBTopLevel::pasteData( const QString & cmdName,  QMimeSource * data, const QString & insertionAddress )
{
    if ( KBookmarkDrag::canDecode( data ) )
    {
        KBookmark bk = KBookmarkDrag::decode( data );
        kdDebug() << "KEBTopLevel::slotPaste url=" << bk.url().prettyURL() << endl;
        CreateCommand * cmd = new CreateCommand( cmdName, insertionAddress, bk );
        m_commandHistory.addCommand( cmd );
    }
}

void KEBTopLevel::slotSort()
{
    KBookmark bk = selectedBookmark();
    ASSERT(!bk.isNull());
    ASSERT(bk.isGroup());

    ////

    SortCommand * cmd = new SortCommand("Sort alphabetically", bk.address());
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotSetAsToolbar()
{
    KMacroCommand * cmd = new KMacroCommand("Set as Bookmark Toolbar");

    KBookmarkGroup oldToolbar = KBookmarkManager::self()->toolbar();
    if (!oldToolbar.isNull())
    {
        QValueList<EditCommand::Edition> lst;
        lst.append(EditCommand::Edition( "toolbar", "no" ));
        lst.append(EditCommand::Edition( "icon", "" ));
        EditCommand * cmd1 = new EditCommand("", oldToolbar.address(), lst);
        cmd->addCommand(cmd1);
    }

    KBookmark bk = selectedBookmark();
    ASSERT( bk.isGroup() );
    QValueList<EditCommand::Edition> lst;
    lst.append(EditCommand::Edition( "toolbar", "yes" ));
    lst.append(EditCommand::Edition( "icon", "bookmark_toolbar" ));
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
    kdDebug() << "KEBTopLevel::slotShowNS" << endl;
    QDomElement rootElem = KBookmarkManager::self()->root().internalElement();
    QString attr = "hide_nsbk";
    rootElem.setAttribute(attr, rootElem.attribute(attr) == "yes" ? "no" : "yes");
    setModified(); // one will need to save, to get konq to notice the change
    // If that's bad, then we need to put this flag in a KConfig.
}

void KEBTopLevel::setModified( bool modified )
{
    m_bModified = modified;
    setCaption( i18n("Bookmark Editor"), m_bModified );
    actionCollection()->action("file_save")->setEnabled( m_bModified );
}

void KEBTopLevel::slotDocumentRestored()
{
    setModified( false );
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
            if ( (bk.fullText() != newText) && !newText.isEmpty())
            {
                RenameCommand * cmd = new RenameCommand( i18n("Renaming"), bk.address(), newText );
                m_commandHistory.addCommand( cmd );
            }
            else if(newText.isEmpty())
            {
                item->setText ( 0, bk.fullText() );
            }
            break;
        case 1:
            if ( bk.url() != newText )
            {
                EditCommand * cmd = new EditCommand( i18n("URL change"), bk.address(),
                                                     EditCommand::Edition("href", newText) );
                m_commandHistory.addCommand( cmd );
            }
            break;
        default:
            kdWarning() << "No such column " << column << endl;
            break;
    }
}

void KEBTopLevel::slotChangeIcon()
{
    KBookmark bk = selectedBookmark();
    ASSERT(!bk.isNull());
    if (bk.isNull()) return;
    KIconDialog dlg(this);
    QString newIcon = dlg.selectIcon(KIcon::Small, KIcon::FileSystem);
    if ( !newIcon.isEmpty() ) {
        EditCommand * cmd = new EditCommand( i18n("Icon change"), bk.address(),
                                             EditCommand::Edition("icon", newIcon) );
        m_commandHistory.addCommand( cmd );
    }
}

void KEBTopLevel::slotDropped (QDropEvent* e, QListViewItem * _newParent, QListViewItem * _afterNow)
{
    // Calculate the address given by parent+afterme
    KEBListViewItem * newParent = static_cast<KEBListViewItem *>(_newParent);
    KEBListViewItem * afterNow = static_cast<KEBListViewItem *>(_afterNow);
    if (!_newParent) // Not allowed to drop something before the root item !
        return;
    QString newAddress =
        afterNow ?
        // We move as the next child of afterNow
        KBookmark::nextAddress( afterNow->bookmark().address() )
        :
        // We move as first child of newParent
        newParent->bookmark().address() + "/0";

    if (e->source() == m_pListView->viewport())
    {
        // Now a simplified version of movableDropEvent
        //movableDropEvent (parent, afterme);
        QListViewItem * i = m_pListView->selectedItem();
        ASSERT(i);
        if (i && i != _afterNow)
        {
            // sanity check - don't move a item into it's own child structure
            QListViewItem *chk = _newParent;
            while(chk)
            {
                if(chk == i)
                    return;
                chk = chk->parent();
            }

            itemMoved(i, newAddress, e->action() == QDropEvent::Copy);
        }
    } else
    {
        // Drop from the outside
        pasteData( i18n("Drop items"), e, newAddress );
    }
}

void KEBTopLevel::itemMoved(QListViewItem *_item, const QString & newAddress, bool copy)
{
    KEBListViewItem * item = static_cast<KEBListViewItem *>(_item);
    if ( copy )
    {
        CreateCommand * cmd = new CreateCommand( i18n("Copy %1").arg(item->bookmark().text()), newAddress,
                                                 item->bookmark().internalElement().cloneNode( true ).toElement() );
        m_commandHistory.addCommand( cmd );
    }
    else
    {
        QString oldAddress = item->bookmark().address();
        if ( oldAddress != newAddress )
        {
            kdDebug() << "KEBTopLevel::slotMoved moving " << oldAddress << " to " << newAddress << endl;

            MoveCommand * cmd = new MoveCommand( i18n("Move %1").arg(item->bookmark().text()),
                                                 oldAddress, newAddress );
            m_commandHistory.addCommand( cmd );
        }
    }
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

void KEBTopLevel::slotBookmarksChanged( const QString &, const QString & caller )
{
    // This is called when someone changes bookmarks in konqueror....
    if ( caller != kapp->name() )
    {
        kdDebug() << "KEBTopLevel::slotBookmarksChanged" << endl;
        m_commandHistory.clear();
        fillListView();
        slotSelectionChanged(); // to disable everything, because no more current item
    }
}

void KEBTopLevel::update()
{
    QListViewItem * item = m_pListView->selectedItem();
    if (item)
    {
        QString address = static_cast<KEBListViewItem*>(item)->bookmark().address();
        kdDebug() << "KEBTopLevel::update item=" << address << endl;
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
            if (grp.isOpen())
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
                                                  i18n("The bookmarks have been modified.\nSave changes ?")) ) {
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
    kdDebug() << "KEBTopLevel::slotCommandExecuted" << endl;
    KEBTopLevel::self()->setModified();
    KEBTopLevel::self()->update();     // Update GUI
}

#include "toplevel.moc"
