/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
   Copyright (C) 2002 Alexander Kellett <lypanov@kde.org>

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
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>
#include <kbookmarkexporter.h>
#include <kdebug.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kstdaction.h>
#include <kmessagebox.h>
#include <krun.h>
#include <kicondialog.h>
#include <kapplication.h>
#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>
#include <dcopclient.h>
#include <assert.h>
#include <stdlib.h>
#include <klocale.h>
#include <kiconloader.h>

#include <favicons.h>

#define COL_NAME 0
#define COL_URL  1 

KEBTopLevel * KEBTopLevel::s_topLevel = 0;
KBookmarkManager * KEBTopLevel::s_pManager = 0;

KEBTopLevel::KEBTopLevel(const QString & bookmarksFile, bool readonly)
   : KMainWindow(), m_commandHistory(actionCollection()), m_dcopIface(0) 
{
   m_bookmarksFilename = bookmarksFile;
   m_bReadOnly = readonly;
   construct();
}

void KEBTopLevel::construct(bool firstTime) 
{
   s_pManager = KBookmarkManager::managerForFile(m_bookmarksFilename, false);

   if (!m_bReadOnly) {
      m_dcopIface = new KBookmarkEditorIface();
   }

   if (firstTime) {
      m_pListView = new KEBListView(this);
   }

   initListView(firstTime);
   connectSignals();

   s_topLevel = this;
   fillListView();

   if (firstTime) {
      setCentralWidget(m_pListView);
      resize(m_pListView->sizeHint().width(), 400);
      createActions();
   }

   resetActions();

   slotSelectionChanged();
   slotClipboardDataChanged();

   if (firstTime) {
      createGUI();
   }

   setAutoSaveSettings();
   setModified(false); // for a very nice caption
   m_commandHistory.documentSaved();

   if (firstTime) {
      KGlobal::locale()->insertCatalogue("libkonq");
   }

}

void KEBTopLevel::createActions() 
{
   KAction * act = new KAction( i18n( "Import &Netscape Bookmarks" ), "netscape", 0, this, SLOT( slotImportNS() ), actionCollection(), "importNS" );
   (void) new KAction( i18n( "Import &Opera Bookmarks..." ), "opera", 0, this, SLOT( slotImportOpera() ), actionCollection(), "importOpera" );
   (void) new KAction( i18n( "Import &Galeon Bookmarks..." ), 0, this, SLOT( slotImportGaleon() ), actionCollection(), "importGaleon" );
   (void) new KAction( i18n( "Import &KDE Bookmarks..." ), 0, this, SLOT( slotImportKDE() ), actionCollection(), "importKDE" );
   (void) new KAction( i18n( "&Import IE Bookmarks..." ), 0, this, SLOT( slotImportIE() ), actionCollection(), "importIE" );
   (void) new KAction( i18n( "&Export to Netscape Bookmarks" ), "netscape", 0, this, SLOT( slotExportNS() ), actionCollection(), "exportNS" );
   act = new KAction( i18n( "Import &Mozilla Bookmarks..." ), "mozilla", 0, this, SLOT( slotImportMoz() ), actionCollection(), "importMoz" );
   (void) new KAction( i18n( "Export to &Mozilla Bookmarks..." ), "mozilla", 0, this, SLOT( slotExportMoz() ), actionCollection(), "exportMoz" );
   (void) KStdAction::open( this, SLOT( slotLoad() ), actionCollection() );
   (void) KStdAction::save( this, SLOT( slotSave() ), actionCollection() );
   (void) KStdAction::saveAs( this, SLOT( slotSaveAs() ), actionCollection() );
   (void) KStdAction::quit( this, SLOT( close() ), actionCollection() );
   (void) KStdAction::cut( this, SLOT( slotCut() ), actionCollection() );
   (void) KStdAction::copy( this, SLOT( slotCopy() ), actionCollection() );
   (void) KStdAction::paste( this, SLOT( slotPaste() ), actionCollection() );
   (void) KStdAction::keyBindings( this, SLOT( slotConfigureKeyBindings() ), actionCollection() );
   (void) KStdAction::configureToolbars( this, SLOT( slotConfigureToolbars() ), actionCollection() );
   (void) new KAction( i18n( "&Delete" ), "editdelete", Key_Delete, this, SLOT( slotDelete() ), actionCollection(), "delete" );
   (void) new KAction( i18n( "Rename" ), "text", Key_F2, this, SLOT( slotRename() ), actionCollection(), "rename" );
   (void) new KAction( i18n( "C&hange URL" ), "text", Key_F3, this, SLOT( slotChangeURL() ), actionCollection(), "changeurl" );
   (void) new KAction( i18n( "Chan&ge Icon..." ), 0, this, SLOT( slotChangeIcon() ), actionCollection(), "changeicon" );
#ifdef FAVICON_UPDATE
   (void) new KAction( i18n( "Update Favicon" ), 0, this, SLOT( slotUpdateFavicon() ), actionCollection(), "updatefavicon" );
#endif
   (void) new KAction( i18n( "&Create New Folder..." ), "folder_new", CTRL+Key_N, this, SLOT( slotNewFolder() ), actionCollection(), "newfolder" );
   (void) new KAction( i18n( "Create &New Bookmark" ), "www", 0, this, SLOT( slotNewBookmark() ), actionCollection(), "newbookmark" );
   (void) new KAction( i18n( "&Insert Separator" ), CTRL+Key_I, this, SLOT( slotInsertSeparator() ), actionCollection(), "insertseparator" );
   (void) new KAction( i18n( "&Sort Alphabetically" ), 0, this, SLOT( slotSort() ), actionCollection(), "sort" );
   (void) new KAction( i18n( "Set as T&oolbar Folder" ), "bookmark_toolbar", 0, this, SLOT( slotSetAsToolbar() ), actionCollection(), "setastoolbar" );
   (void) new KAction( i18n( "&Expand All Folders" ), 0, this, SLOT( slotExpandAll() ), actionCollection(), "expandall" );
   (void) new KAction( i18n( "Collapse &All Folders" ), 0, this, SLOT( slotCollapseAll() ), actionCollection(), "collapseall" );
   (void) new KAction( i18n( "&Open in Konqueror" ), "fileopen", 0, this, SLOT( slotOpenLink() ), actionCollection(), "openlink" );
   (void) new KAction( i18n( "Check &Status" ), "bookmark", 0, this, SLOT( slotTestLink() ), actionCollection(), "testlink" );
   (void) new KAction( i18n( "Check Status: &All" ), 0, this, SLOT( slotTestAllLinks() ), actionCollection(), "testall" );
   (void) new KAction( i18n( "Cancel &Checks" ), 0, this, SLOT( slotCancelAllTests() ), actionCollection(), "canceltests" );
   m_taShowNS = new KToggleAction( i18n( "&Show Netscape Bookmarks in Konqueror Windows" ), 0, this, SLOT( slotShowNS() ), actionCollection(), "settings_showNS" );
}

void KEBTopLevel::slotLoad()
{
   // TODO - add a few default place to the file dialog somehow?,
   //      - e.g kfile bookmarks +  normal bookmarks file dir
   if (!queryClose()) {
      return;
   }
   QString bookmarksFile = KFileDialog::getOpenFileName(QString::null, "*.xml", this);
   m_bookmarksFilename = bookmarksFile;
   if (bookmarksFile == QString::null) {
      return;
   }
   construct(false);
}

void KEBTopLevel::resetActions()
{
   m_taShowNS->setChecked(s_pManager->showNSBookmarks());

   // first disable all actions

   QValueList<KAction *> actions = actionCollection()->actions();
   QValueList<KAction *>::Iterator it = actions.begin();
   QValueList<KAction *>::Iterator end = actions.end();
   for (; it != end; ++it) {
      KAction *act = *it;
      // do not touch the configureblah actions
      if ( strncmp(act->name(), "configure", 9) )
         act->setEnabled(false);
   }

   // then reenable needed ones

   actionCollection()->action("file_open")->setEnabled(true);
   actionCollection()->action("file_save")->setEnabled(true); // setModified
   actionCollection()->action("file_save_as")->setEnabled(true);
   actionCollection()->action("file_quit")->setEnabled(true);

   actionCollection()->action("exportNS")->setEnabled(true);
   actionCollection()->action("exportMoz")->setEnabled(true);

   if (!m_bReadOnly) {
      actionCollection()->action("importGaleon")->setEnabled(true);
      actionCollection()->action("importKDE")->setEnabled(true);
      actionCollection()->action("importOpera")->setEnabled(true);
      actionCollection()->action("importIE")->setEnabled(true);
      // TODO for 3.2 - this feels wrong, someone should be allowed to import
      //                from anywhere but get this one by default...
      bool nsExists = QFile::exists( KNSBookmarkImporter::netscapeBookmarksFile() );
      actionCollection()->action("importNS")->setEnabled(nsExists);
      actionCollection()->action("importMoz")->setEnabled(true);
      actionCollection()->action("settings_showNS")->setEnabled(true);
   }
}

void KEBTopLevel::initListView(bool firstTime)
{
   if (firstTime) {
      m_pListView->setDragEnabled( true );
      m_pListView->addColumn( i18n("Bookmark"), 300 );
      m_pListView->addColumn( i18n("URL"), 300 );
      m_pListView->addColumn( i18n("Status/Last Modified"), 300 );
#ifdef DEBUG_ADDRESSES
      m_pListView->addColumn( i18n("Address"), 100 );
#endif

      m_pListView->setRootIsDecorated( true );
      m_pListView->setRenameable( COL_NAME );
      m_pListView->setRenameable( COL_URL );

      m_pListView->setSelectionModeExt( KListView::Extended );
      m_pListView->setDragEnabled( true );
      m_pListView->setAllColumnsShowFocus( true );
      m_pListView->setSorting(-1, false);
   }

   m_pListView->setItemsRenameable( !m_bReadOnly );
   m_pListView->setItemsMovable( m_bReadOnly ); // We move items ourselves (for undo)
   m_pListView->setAcceptDrops( !m_bReadOnly );
   m_pListView->setDropVisualizer( !m_bReadOnly );

}

void KEBTopLevel::disconnectSignals() {
   kdWarning() 
      << "disconnectSignals()'s returned " 
      << disconnect( m_pListView,       0, 0, 0 ) << ", "
      << disconnect( s_pManager,        0, 0, 0 ) << ", "
      << disconnect( &m_commandHistory, 0, 0, 0 ) << ", "
      << disconnect( m_dcopIface,       0, 0, 0 ) << endl;
}

void KEBTopLevel::connectSignals() {

   connect( m_pListView, SIGNAL( selectionChanged() ),
            SLOT( slotSelectionChanged() ) );
   connect( m_pListView, SIGNAL( contextMenu( KListView *, QListViewItem*, const QPoint & ) ),
            SLOT( slotContextMenu( KListView *, QListViewItem *, const QPoint & ) ) );

   // If someone plays with konq's bookmarks while we're open, update. (when applicable)
   connect( s_pManager, SIGNAL( changed(const QString &, const QString &) ),
            SLOT( slotBookmarksChanged(const QString &, const QString &) ) );

   if (!m_bReadOnly) {
      connect(m_pListView, SIGNAL( itemRenamed(QListViewItem *, const QString &, int) ),
              SLOT( slotItemRenamed(QListViewItem *, const QString &, int) ) );
      connect(m_pListView, SIGNAL( dropped (QDropEvent* , QListViewItem* , QListViewItem* ) ),
              SLOT( slotDropped(QDropEvent* , QListViewItem* , QListViewItem* ) ) );
      connect(kapp->clipboard(), SIGNAL( dataChanged() ),
              SLOT( slotClipboardDataChanged() ) );

      // Update GUI after executing command
      connect(&m_commandHistory, SIGNAL( commandExecuted() ), SLOT( slotCommandExecuted() ) );
      connect(&m_commandHistory, SIGNAL( documentRestored() ), SLOT( slotDocumentRestored() ) );

      connect(m_dcopIface, SIGNAL( addedBookmark(QString,QString,QString,QString) ),
              SLOT( slotAddedBookmark(QString,QString,QString,QString) ));
      connect(m_dcopIface, SIGNAL( createdNewFolder(QString,QString) ),
              SLOT( slotCreatedNewFolder(QString,QString) ));
   }
}

KEBTopLevel::~KEBTopLevel()
{
   s_topLevel = 0;
   if (m_dcopIface) {
      delete m_dcopIface;
   }
}

void KEBTopLevel::slotConfigureKeyBindings()
{
    KKeyDialog::configure(actionCollection());
}

void KEBTopLevel::slotConfigureToolbars()
{
   saveMainWindowSettings( KGlobal::config(), "MainWindow" );
   KEditToolbar dlg(actionCollection());
   connect(&dlg,SIGNAL( newToolbarConfig() ), this, SLOT( slotNewToolbarConfig() ));
   if (dlg.exec()) {
      createGUI();
   }
}

// This is called when OK or Apply is clicked
void KEBTopLevel::slotNewToolbarConfig()
{
   applyMainWindowSettings( KGlobal::config(), "MainWindow" );
}

#define ITEM_TO_BK(item) static_cast<KEBListViewItem *>(item)->bookmark()

int KEBTopLevel::numSelected()
{
   return KEBTopLevel::self()->selectedItems()->count();
}

QListViewItem* KEBTopLevel::selectedItem()
{
   Q_ASSERT( (numSelected() == 1) );
   return (selectedItems()->first());
}

KBookmark KEBTopLevel::selectedBookmark() const
{
   Q_ASSERT( (numSelected() == 1) );
   return (selectedBookmarks().first());
}

// AK - TODO
// this all needs to be rethought
// pruning should be a layer of processing
// done after the selectedItems call, the
// selected items should be cached based
// on use of slotSelectionChanged, 
// 
// simple motivation for this:
// currently you can't ctrl-v paste onto
// a empty folder item!!!

// if ( (parent is selected) or (has no parent) )
#define IS_REAL(it)                                                                       \
               (                                                                          \
                   (    (it.current()->parent() && !it.current()->parent()->isSelected()) \
                    || !(it.current()->parent())                                          \
                   )                                                                      \
                   && ( it.current() != KEBTopLevel::self()->m_pListView->firstChild())   \
                   && ( !static_cast<KEBListViewItem *>(it.current())->m_emptyFolder )    \
               )

#define IS_REAL_SEL(it) ( (it.current()->isSelected()) && IS_REAL(it) )

QPtrList<QListViewItem> * KEBTopLevel::selectedItems()
{
   QPtrList<QListViewItem> *items = new QPtrList<QListViewItem>();
   for( QListViewItemIterator it(KEBTopLevel::self()->m_pListView); it.current(); it++ ) {
      if (IS_REAL_SEL(it)) {
         items->append(it.current());
      }
   }
   return items;
}

QValueList<KBookmark> KEBTopLevel::allBookmarks() const
{
   QValueList<KBookmark> bookmarks;
   for( QListViewItemIterator it(m_pListView); it.current(); it++ ) {
      if (IS_REAL(it) && (it.current()->childCount() == 0)) {
         bookmarks.append(ITEM_TO_BK(it.current()));
      }
   }
   return bookmarks;
}

QValueList<KBookmark> KEBTopLevel::selectedBookmarksExpanded() const
{
   QValueList<KBookmark> bookmarks;
   QStringList addresses;
   for(QListViewItemIterator it(m_pListView); it.current(); it++) {
      if (IS_REAL_SEL(it)) {
         // is not an empty folder && childCount() == 0 
         // therefore MUST be a single bookmark. good logic???
         // no!, this is what you would think, but actually
         // the "empty folder" is only the empty folder item
         // itself and not the wrapping folder. therefore we
         // need yet another check in the childCount > 0 path!!!
         if (it.current()->childCount() > 0) {
            for(QListViewItemIterator it2(it.current()); it2.current(); it2++) {
               if (!static_cast<KEBListViewItem *>(it2.current())->m_emptyFolder) {
                  const KBookmark bk = ITEM_TO_BK(it2.current());
                  if (!addresses.contains(bk.address())) {
                     bookmarks.append(bk);
                     addresses.append(bk.address());
                  }
               }
               if (it.current()->nextSibling() 
                && it2.current() == it.current()->nextSibling()->itemAbove()) {
                  break;
               }
            }
         } else {
            if (!addresses.contains(ITEM_TO_BK(it.current()).address())) {
               bookmarks.append(ITEM_TO_BK(it.current()));
               addresses.append(ITEM_TO_BK(it.current()).address());
            }
         }
      }
   }
   return bookmarks;
}

QValueList<KBookmark> KEBTopLevel::selectedBookmarks() const
{
   QValueList<KBookmark> bookmarks;
   for( QListViewItemIterator it(m_pListView); it.current(); it++ ) {
      if (IS_REAL_SEL(it)) {
         bookmarks.append(ITEM_TO_BK(it.current()));
      }
   }
   return bookmarks;
}

QValueList<KBookmark> KEBTopLevel::getBookmarkSelection()
{
    QValueList<KBookmark> bookmarks;
    QPtrList<QListViewItem>* items = KEBTopLevel::self()->selectedItems();
    QPtrListIterator<QListViewItem> it(*items);
    for ( ; it.current() != 0; ++it ) {
       QListViewItem* item = it.current();
       bookmarks.append(KBookmark( ITEM_TO_BK(item) ));
    }
    return bookmarks;
}

void KEBTopLevel::updateSelection()
{
   QListViewItem *lastItem = 0;
   for( QListViewItemIterator it(KEBTopLevel::self()->m_pListView); it.current(); it++ ) {
      if (IS_REAL_SEL(it)) {
         lastItem = it.current();
      }
   }

   if (lastItem) {
      m_last_selection_address = ITEM_TO_BK(lastItem).address();
   }
}

void KEBTopLevel::slotSelectionChanged()
{
   bool itemSelected = false;
   bool group = false;
   bool root = false;
   bool separator = false;
   bool urlIsEmpty = false;
   bool multiSelect = false;
   bool singleSelect = false; // for simplification, not a real pulled value

   QListViewItem * item = selectedItems()->first();
   if (item) {
      kdDebug() << "KEBTopLevel::slotSelectionChanged " << ITEM_TO_BK(item).address() << endl;
      itemSelected = true;
      KBookmark nbk = ITEM_TO_BK(item);
      group = nbk.isGroup();
      separator = nbk.isSeparator();
      root = (m_pListView->firstChild() == item);
      urlIsEmpty= nbk.url().isEmpty();
      multiSelect = numSelected() > 1;
      singleSelect = !multiSelect && itemSelected;
   }

   updateSelection();

   KActionCollection * coll = actionCollection();

   coll->action("edit_copy")          ->setEnabled(itemSelected && !root);
   coll->action("openlink")           ->setEnabled(itemSelected && !group && !separator && !urlIsEmpty);

   // AK - note if these do eventually save there state for every drawer than need to move this down
   coll->action("expandall")      ->setEnabled(!multiSelect && !(root && m_pListView->childCount()==1));
   coll->action("collapseall")    ->setEnabled(!multiSelect && !(root && m_pListView->childCount()==1));

   if (!m_bReadOnly) {
      coll->action("edit_cut")       ->setEnabled(itemSelected && !root);
      coll->action("edit_paste")     ->setEnabled(itemSelected && !root && m_bCanPaste);
      coll->action("rename")         ->setEnabled(singleSelect && !separator && !root);
      coll->action("changeurl")      ->setEnabled(singleSelect && !group && !separator && !root);
      coll->action("delete")         ->setEnabled(itemSelected && !root);
      coll->action("newfolder")      ->setEnabled(!multiSelect);
#ifdef FAVICON_UPDATE
      coll->action("updatefavicon")  ->setEnabled(singleSelect && !root && !separator);
#endif
      coll->action("changeicon")     ->setEnabled(singleSelect && !root && !separator);
      coll->action("insertseparator")->setEnabled(singleSelect);
      coll->action("newbookmark")    ->setEnabled(!multiSelect);
      coll->action("sort")           ->setEnabled(!multiSelect && group);
      coll->action("setastoolbar")   ->setEnabled(!multiSelect && group);
      // AK - root should work.. does it?, FIXME TEST!
      coll->action("testlink")       ->setEnabled(!root && itemSelected && !separator); 
      coll->action("testall")        ->setEnabled(!multiSelect && !(root && m_pListView->childCount()==1));
   }
}

void KEBTopLevel::slotClipboardDataChanged()
{
   kdDebug() << "KEBTopLevel::slotClipboardDataChanged" << endl;
   QClipboard *clipboard = QApplication::clipboard();
   bool oldMode = clipboard->selectionModeEnabled();
   clipboard->setSelectionMode( false );
   QMimeSource *data = clipboard->data();
   clipboard->setSelectionMode( oldMode );
   m_bCanPaste = KBookmarkDrag::canDecode( data );
   slotSelectionChanged();
}

void KEBTopLevel::slotSave()
{
   (void)save();
}

void KEBTopLevel::slotSaveAs()
{
   QString saveFilename = KFileDialog::getSaveFileName(QString::null, "*.xml", this);
   if(!saveFilename.isEmpty()) {
      s_pManager->saveAs(saveFilename);
   }
}

bool KEBTopLevel::save()
{
   if (s_pManager->save()) {
      QString data( kapp->name() );
      QCString objId( "KBookmarkManager-" );
      objId += s_pManager->path().utf8();
      kapp->dcopClient()->send( "*", objId, "notifyCompleteChange(QString)", data );
      setModified( false );
      m_commandHistory.documentSaved();
      return true;
   }
   return false;
}

QString KEBTopLevel::insertionAddress() const
{
   if(numSelected() == 0) {
      return "/0";
   }

   KBookmark current = selectedBookmarks().first();
   if (current.isGroup()) {
      // in a group, we insert as first child
      return current.address() + "/0";

   } else {
      // otherwise, as next sibling
      return KBookmark::nextAddress(current.address());
   }
}

KEBListViewItem * KEBTopLevel::findByAddress( const QString & address ) const
{
   // AK - this completely assumed perfection in the address.. is that okay?
   kdDebug() << "KEBTopLevel::findByAddress " << address << endl;
   QListViewItem * item = m_pListView->firstChild();

   // The address is something like /5/10/2
   QStringList addresses = QStringList::split('/',address);

   // AK - TODO - why assert rather than Q_ASSERT???

   for (QStringList::Iterator it = addresses.begin(); it != addresses.end(); ++it) {
      uint number = (*it).toUInt();
      //kdDebug() << "KBookmarkManager::findByAddress " << number << endl;
      assert(item);
      item = item->firstChild();
      for (uint i = 0; i < number; ++i) {
         assert(item);
         item = item->nextSibling();
      }
   }
   Q_ASSERT(item);
   return static_cast<KEBListViewItem *>(item);
}

void KEBTopLevel::slotRename()
{
   QListViewItem* item = selectedItem();
   Q_ASSERT(item);
   if (item) {
      m_pListView->rename(item, COL_NAME);
   }
}

void KEBTopLevel::slotChangeURL()
{
   QListViewItem* item = selectedItem();
   Q_ASSERT(item);
   if (item) {
      m_pListView->rename(item, COL_URL);
   }
}

void KEBTopLevel::deleteSelection(QString commandName)
{
   QPtrList<QListViewItem>* items = selectedItems();
   QPtrListIterator<QListViewItem> it(*items);
   KMacroCommand * mcmd = new KMacroCommand(commandName);
   for (; it.current() != 0; ++it) {
      QListViewItem* item = it.current();
      DeleteCommand * dcmd = new DeleteCommand("", ITEM_TO_BK(item).address());
      dcmd->execute();
      mcmd->addCommand( dcmd );
   }
   m_commandHistory.addCommand(mcmd, false);
   slotCommandExecuted();
}

void KEBTopLevel::slotDelete()
{
   if(numSelected() == 0) {
      kdWarning() << "KEBTopLevel::slotDelete no selected item !" << endl;
      return;
   }
   deleteSelection(i18n("Delete Items"));
}

void KEBTopLevel::slotNewFolder()
{
   // AK - TODO for 3.2
   // EVIL HACK
   // We need to ask for the folder name before creating the command, in case of "Cancel".
   // But in message-freeze time, impossible to add i18n()s. So... we have to call the existing code :
   QDomDocument doc("xbel"); // dummy
   QDomElement elem = doc.createElement("xbel");
   doc.appendChild( elem );
   KBookmarkGroup grp( elem ); // dummy
   KBookmark bk = grp.createNewFolder( s_pManager, QString::null, false ); // Asks for the name
   if (!bk.fullText().isEmpty()) {
      // not canceled
      CreateCommand * cmd 
         = new CreateCommand(i18n("Create Folder"), insertionAddress(), 
                             bk.fullText(),bk.icon(), true /*open*/ );
      m_commandHistory.addCommand( cmd );
   }
}

QString KEBTopLevel::correctAddress(QString address)
{
   // AK - 3.2 TODO - move to kbookmark ?
   return s_pManager->findByAddress(address, true).address();
}

// DCOP event handling

KBookmarkEditorIface::KBookmarkEditorIface()
 : QObject(), DCOPObject("KBookmarkEditor")
{
    connectDCOPSignal(0, "KBookmarkNotifier", "addedBookmark(QString,QString,QString,QString,QString)", "slotAddedBookmark(QString,QString,QString,QString,QString)", false);
    connectDCOPSignal(0, "KBookmarkNotifier", "createdNewFolder(QString,QString,QString)", "slotCreatedNewFolder(QString,QString,QString)", false);
}

void KBookmarkEditorIface::slotAddedBookmark( QString filename, QString url, QString text, QString address, QString icon )
{
    if (filename == KEBTopLevel::bookmarkManager()->path()) {
        emit addedBookmark( url, text, address, icon );
    }
}

void KBookmarkEditorIface::slotCreatedNewFolder( QString filename, QString text, QString address )
{
    if (filename == KEBTopLevel::bookmarkManager()->path()) {
        emit createdNewFolder( text, address );
    }
}

void KEBTopLevel::slotCreatedNewFolder(QString text, QString address)
{
   kdWarning() << "slotCreatedNewFolder(" << text << "," << address << ")" << endl;
   if (!m_bModified) { 
      return;
   }
   CreateCommand * cmd = new CreateCommand( 
                                 i18n("Create Folder in Konqueror"), 
                                 correctAddress(address), 
                                 text, QString::null, true );
   m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotAddedBookmark(QString url, QString text, QString address, QString icon)
{
   kdDebug() << "slotAddedBookmark(" << url << "," << text << "," << address << "," << icon << ")" << endl;
   if (!m_bModified) {
      return;
   }
   CreateCommand * cmd = new CreateCommand(
                                 i18n("Add Bookmark in Konqueror"), 
                                 correctAddress(address), 
                                 text, icon, KURL(url) );
   m_commandHistory.addCommand(cmd);
}


void KEBTopLevel::slotNewBookmark()
{
   CreateCommand * cmd = new CreateCommand( 
                                i18n("Create Bookmark" ), 
                                insertionAddress(), 
                                QString::null, QString::null, KURL()
                             );
   m_commandHistory.addCommand(cmd);
}

void KEBTopLevel::slotInsertSeparator()
{
   CreateCommand * cmd = new CreateCommand( 
                                i18n("Insert Separator"), 
                                insertionAddress() 
                             );
   m_commandHistory.addCommand(cmd);
}

void KEBTopLevel::selectImport(ImportCommand *cmd)
{
   KEBListViewItem *item = findByAddress(cmd->groupAddress());
   if (item) {
      m_pListView->setCurrentItem(item);
      m_pListView->ensureItemVisible(item);
   }
}

// TODO for 3.2 - move the following two functinos into new kbookmarkimporter files in libkonq

QString kdeBookmarksFile() {
   // locateLocal on the bookmarks file and get dir?
   return KFileDialog::getOpenFileName( 
               QDir::homeDirPath() + "/.kde",
               i18n("*.xml|KDE bookmark files (*.xml)") );
}

QString galeonBookmarksFile() {
   return KFileDialog::getOpenFileName( 
               QDir::homeDirPath() + "/.galeon",
               i18n("*.xbel|Galeon bookmark files (*.xbel)") );
}

// TODO - THIS IS INSANELY UGLY!
void KEBTopLevel::doImport(
   QString imp, QString imp_bks, QString bks, 
   QString dirname, QString icon, bool dabool, int type
) {
   if (!dirname.isEmpty()) {
      int answer = KMessageBox::questionYesNoCancel(
                     this, i18n("Import as a new subfolder or replace all the current bookmarks?"),
                     imp, i18n("As New Folder"), i18n("Replace"));

      if (answer == KMessageBox::Cancel) { 
         return;
      }

      // update the gui
      slotCommandExecuted();

      ImportCommand * cmd = new ImportCommand(
                                    imp_bks, dirname,
                                    (answer==KMessageBox::Yes) ? bks : QString::null,
                                    icon, dabool, type);
      m_commandHistory.addCommand( cmd );
      selectImport(cmd);

   } else {
      // AK - note
      // do something, yet this case is ambigious,
      // for a cancel from the file dialog it should
      // do nothing, yet for a import without "..."
      // it shouldn't even be called + thus should be
      // a assert, or possible not... 
      // need to think more about this...
   }
}

// (imp, imp_bks, bks), dirname, icon, bool, type
#define BK_STRS(a) i18n(##a" Import"), i18n("Import "##a" Bookmarks"), i18n(##a" Bookmarks")

// TODO - maybe requests for ie + galeon icons should be made?, just a "windows" icon would do for ie

void KEBTopLevel::slotImportIE()
{ doImport( BK_STRS("IE"), KIEBookmarkImporter::IEBookmarksDir(), "", false, BK_IE); }

void KEBTopLevel::slotImportGaleon()
{ doImport( BK_STRS("Galeon"), galeonBookmarksFile(), "", false, BK_XBEL); }

void KEBTopLevel::slotImportKDE()
{ doImport( BK_STRS("KDE"), kdeBookmarksFile(), "", false, BK_XBEL); }

void KEBTopLevel::slotImportOpera()
{ doImport( BK_STRS("Opera"), KOperaBookmarkImporter::operaBookmarksFile(), "opera", false, BK_OPERA); }

void KEBTopLevel::slotImportMoz()
{ doImport( BK_STRS("Mozilla"), KNSBookmarkImporter::mozillaBookmarksFile(), "mozilla", true, BK_NS); }

void KEBTopLevel::slotImportNS()
{
   doImport( BK_STRS("Netscape"), KNSBookmarkImporter::netscapeBookmarksFile(), "netscape", false, BK_NS);

   // Ok, we don't need the dynamic menu anymore
   if (m_taShowNS->isChecked()) {
      m_taShowNS->activate();
   }

   // AK - are you sure about the above?, this is a bit
   // "automated" and intrusive and unlike the menu the
   // bookmarks don't get automatically updated...
}

void KEBTopLevel::slotExportNS()
{
   QString path = KNSBookmarkImporter::netscapeBookmarksFile(true);
   if (!path.isEmpty()) {
      KNSBookmarkExporter exporter(s_pManager, path);
      exporter.write(false);
   }
}

void KEBTopLevel::slotExportMoz()
{
   QString path = KNSBookmarkImporter::mozillaBookmarksFile(true);
   if (!path.isEmpty()) {
      KNSBookmarkExporter exporter(s_pManager, path);
      exporter.write(true);
   }
}

void KEBTopLevel::slotCut()
{
   slotCopy();
   deleteSelection(i18n("Cut items"));
}

void KEBTopLevel::slotCopy()
{
   // This is not a command, because it can't be undone
   Q_ASSERT( numSelected() != 0 ); // really an ASSERT?
   QValueList<KBookmark> bookmarks = getBookmarkSelection();
   KBookmarkDrag* data = KBookmarkDrag::newDrag( bookmarks, 0 /* not this ! */ );

   QClipboard *clipboard = QApplication::clipboard();
   bool oldMode = clipboard->selectionModeEnabled();
   clipboard->setSelectionMode(false);
   clipboard->setData(data);
   clipboard->setSelectionMode(oldMode);

   // AK - TODO - research this
   // slotClipboardDataChanged(); 
   // dfaure: don't ask 
   // ak: umm.. okay - but i'm commenting out for 3.1 :)
}

void KEBTopLevel::slotPaste()
{
   QClipboard *clipboard = QApplication::clipboard();
   bool oldMode = clipboard->selectionModeEnabled();
   clipboard->setSelectionMode(false);
   pasteData(i18n("Paste"), clipboard->data(), insertionAddress());
   clipboard->setSelectionMode(oldMode);
}

void KEBTopLevel::pasteData( const QString & cmdName,  QMimeSource * data, const QString & insertionAddress )
{
   QString currentAddress = insertionAddress;
   if (KBookmarkDrag::canDecode( data )) {
      KMacroCommand * mcmd = new KMacroCommand( i18n("Add a Number of Bookmarks") );
      QValueList<KBookmark> bookmarks = KBookmarkDrag::decode( data );
      for (QValueListConstIterator<KBookmark> it = bookmarks.begin(); it != bookmarks.end(); ++it) {
         CreateCommand * cmd = new CreateCommand(cmdName, currentAddress, (*it));
         mcmd->addCommand(cmd);
         kdDebug() << "KEBTopLevel::slotPaste url=" << (*it).url().prettyURL() << currentAddress << endl;
         currentAddress = KBookmark::nextAddress(currentAddress);
      }
      m_commandHistory.addCommand(mcmd);
   }
}

void KEBTopLevel::slotSort()
{
   KBookmark bk = selectedBookmark();
   Q_ASSERT(bk.isGroup());
   SortCommand * cmd = new SortCommand(i18n("Sort Alphabetically"), bk.address());
   m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotSetAsToolbar()
{
   KMacroCommand * cmd = new KMacroCommand(i18n("Set as Bookmark Toolbar"));

   KBookmarkGroup oldToolbar = s_pManager->toolbar();
   if (!oldToolbar.isNull()) {
      QValueList<EditCommand::Edition> lst;
      lst.append(EditCommand::Edition("toolbar", "no"));
      lst.append(EditCommand::Edition("icon", ""));
      EditCommand * cmd1 = new EditCommand("", oldToolbar.address(), lst);
      cmd->addCommand(cmd1);
   }

   KBookmark bk = selectedBookmark();
   Q_ASSERT( bk.isGroup() );
   QValueList<EditCommand::Edition> lst;
   lst.append(EditCommand::Edition("toolbar", "yes"));
   lst.append(EditCommand::Edition("icon", "bookmark_toolbar"));
   EditCommand * cmd2 = new EditCommand("", bk.address(), lst);
   cmd->addCommand(cmd2);

   m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotOpenLink()
{
   QValueList<KBookmark> bks = selectedBookmarks();
   QValueListIterator<KBookmark> it;
   for (it = bks.begin(); it != bks.end(); ++it) {
      Q_ASSERT(!(*it).isGroup());
      (void)new KRun((*it).url());
   }
}

void KEBTopLevel::slotTestAllLinks()
{
   testBookmarks(allBookmarks());
}

void KEBTopLevel::slotTestLink()
{
   testBookmarks(selectedBookmarksExpanded());
}

void KEBTopLevel::testBookmarks(QValueList<KBookmark> bks)
{
   tests.insert(0, new TestLink(bks));
   actionCollection()->action("canceltests")->setEnabled( true );
}

// needed by the TestLink stuff
void KEBTopLevel::slotCancelTest(TestLink *t)
{
   tests.removeRef(t);
   delete t;
   if (tests.count() == 0) {
      actionCollection()->action("canceltests")->setEnabled( false );
   }
}

void KEBTopLevel::slotCancelAllTests()
{
   TestLink *t, *p;
   for (t = tests.first(); t != 0; t=p) {
      p = tests.next();
      slotCancelTest(t);
   }
}

void KEBTopLevel::setAllOpen(bool open) {
   for (QListViewItemIterator it(m_pListView); it.current(); it++) {
      if (it.current()->parent()) {
         it.current()->setOpen(open);
      }
   }
}

void KEBTopLevel::slotShowNS()
{
   QDomElement rootElem = s_pManager->root().internalElement();
   QString attr = "hide_nsbk";
   rootElem.setAttribute(attr, rootElem.attribute(attr) == "yes" ? "no" : "yes");

   // one will need to save, to get konq to notice the change
   // if that's bad, then we need to put this flag in a KConfig.
   setModified();
}

void KEBTopLevel::setModified( bool modified )
{
   if (!m_bReadOnly) {
      m_bModified = modified;
      setCaption(i18n("Bookmark Editor"), m_bModified);
   } else {
      m_bModified = false;
      setCaption(QString("%1 [%2]").arg(i18n("Bookmark Editor")).arg(i18n("Read Only")));
   }
   actionCollection()->action("file_save")->setEnabled(m_bModified);
   s_pManager->setUpdate(!m_bModified); // only update when non-modified
}

void KEBTopLevel::slotDocumentRestored()
{
   // Called when undoing the very first action - or the first one after
   // saving. The "document" is set to "non modified" in that case.
   setModified( false );
}

void KEBTopLevel::slotItemRenamed(QListViewItem * item, const QString & newText, int column)
{
   Q_ASSERT(item);
   KBookmark bk = ITEM_TO_BK(item);
   switch (column) {
      case COL_NAME:
         if ( (bk.fullText() != newText) && !newText.isEmpty()) {
            RenameCommand * cmd = new RenameCommand( i18n("Renaming"), bk.address(), newText );
            m_commandHistory.addCommand( cmd );

         } else if(newText.isEmpty()) {
            item->setText(COL_NAME, bk.fullText() );
         }
         break;
      case COL_URL:
         if (bk.url() != newText) {
            EditCommand * cmd = new EditCommand( i18n("URL Change"), bk.address(),
                                                 EditCommand::Edition("href", newText) );
            m_commandHistory.addCommand( cmd );
         }
         break;
      default:
         kdWarning() << "No such column " << column << endl;
         break;
   }
}

void KEBTopLevel::slotUpdateFavicon()
{
   // if folder then recursive
   KBookmark bk = selectedBookmark();
   FavIconUpdater::self()->queueIcon(bk);
}

void KEBTopLevel::slotChangeIcon()
{
   KBookmark bk = selectedBookmark();
   KIconDialog dlg(this);
   QString newIcon = dlg.selectIcon(KIcon::Small, KIcon::FileSystem);
   if (!newIcon.isEmpty()) {
      EditCommand * cmd = new EditCommand( i18n("Icon Change"), bk.address(),
                                           EditCommand::Edition("icon", newIcon) );
      m_commandHistory.addCommand( cmd );
   }
}

void KEBTopLevel::slotDropped (QDropEvent* e, QListViewItem * _newParent, QListViewItem * _afterNow)
{
   // calculate the address given by parent+afterme
   KEBListViewItem * newParent = static_cast<KEBListViewItem *>(_newParent);
   KEBListViewItem * afterNow = static_cast<KEBListViewItem *>(_afterNow);

   if (!_newParent) {
      // not allowed to drop something before the root item !
      return;
   }

   // skip empty folders
   if (afterNow && afterNow->m_emptyFolder) {
      afterNow = 0;
   }

   QString newAddress =
      (afterNow)
      ? (KBookmark::nextAddress(afterNow->bookmark().address())) // the next child of afterNow
      : (newParent->bookmark().address() + "/0"); // first child of newParent

   if (e->source() == m_pListView->viewport()) {
      // simplified version of movableDropEvent (parent, afterme);
      QPtrList<QListViewItem>* selection = selectedItems();
      QListViewItem * i = selection->first();
      Q_ASSERT(i);
      if (i && i != _afterNow) {
         // sanity check - don't move a item into it's own child structure
         // AK - check if this is still a posssiblity or not, as this
         //      seems like quite unneeded complexity as itemMoved in
         //      its multiple reincarnation should skip such items...
         QListViewItem *chk = _newParent;
         while(chk) {
            if(chk == i) {
               return;
            }
            chk = chk->parent();
         }
         itemMoved(selection, newAddress, e->action() == QDropEvent::Copy);
      }

   } else {
      // Drop from the outside
      pasteData(i18n("Drop items"), e, newAddress);
   }
}

void KEBTopLevel::itemMoved(QPtrList<QListViewItem> *_items, const QString & newAddress, bool copy)
{
   KMacroCommand * mcmd = new KMacroCommand( copy ? i18n("Copy Items") : i18n("Move Items") );

   QString destAddress = newAddress;

   QPtrList<QListViewItem>* items = _items;
   QPtrListIterator<QListViewItem> it(*items);

   for ( ; it.current() != 0; ++it ) {
      KEBListViewItem * item = static_cast<KEBListViewItem *>(it.current());
      QString finalAddress;
      if (copy) {
         CreateCommand * cmd = new CreateCommand( i18n("Copy %1").arg(item->bookmark().text()), destAddress,
               item->bookmark().internalElement().cloneNode( true ).toElement() );
         cmd->execute();
         finalAddress = cmd->finalAddress();
         mcmd->addCommand( cmd );

      } else {
         QString oldAddress = item->bookmark().address();
         if ( oldAddress == destAddress
               || destAddress.startsWith(oldAddress) 
            ) {
            // AK - old comment "duplicate code???", whats that mean?
            continue;

         } else {
            MoveCommand * cmd = new MoveCommand( i18n("Move %1").arg(item->bookmark().text()),
                  oldAddress, destAddress );
            cmd->execute();
            finalAddress = cmd->finalAddress();
            mcmd->addCommand( cmd );
         }
      }
      destAddress = KBookmark::nextAddress(finalAddress);
   }

   m_commandHistory.addCommand( mcmd, false );
   slotCommandExecuted();
}

void KEBTopLevel::slotContextMenu( KListView *, QListViewItem * _item, const QPoint &p )
{
   if (_item) {
      if (ITEM_TO_BK(_item).isGroup()) {
         QWidget* popup = factory()->container("popup_folder", this);
         if (popup) {
            static_cast<QPopupMenu*>(popup)->popup(p);
         }

      } else {
         QWidget* popup = factory()->container("popup_bookmark", this);
         if (popup) {
            static_cast<QPopupMenu*>(popup)->popup(p);
         }
      }
   }
}

void KEBTopLevel::slotBookmarksChanged( const QString &, const QString & caller )
{
   // This is called when someone changes bookmarks in konqueror....
   if (caller != kapp->name()) {
      kdDebug() << "KEBTopLevel::slotBookmarksChanged" << endl;
      m_commandHistory.clear();
      fillListView();
      slotSelectionChanged(); // to disable everything, because no more current item
   }
}

void KEBTopLevel::update()
{
   QPoint pos(m_pListView->contentsX(), m_pListView->contentsY());
   QPtrList<QListViewItem>* items = selectedItems();

   if (items->count() != 0) {
      QPtrListIterator<QListViewItem> it(*items);
      QStringList addressList;

      for ( ; it.current() != 0; ++it ) {
         KEBListViewItem* item = static_cast<KEBListViewItem*>(it.current());
         QString address = ITEM_TO_BK(item).address();
         // AK - use of string ERROR is very hacky
         if (address != "ERROR") {
            addressList << address;
         }
      }

      fillListView();
      KEBListViewItem * newItem = 0;

      for ( QStringList::Iterator ait = addressList.begin(); ait != addressList.end(); ++ait ) {
         newItem = findByAddress( *ait );
         kdDebug() << "KEBTopLevel::update item=" << *ait << endl;
         Q_ASSERT(newItem);
         if (newItem) {
            m_pListView->setSelected(newItem, true);
         }
      }
      if (!newItem) {
         newItem = findByAddress(correctAddress(m_last_selection_address));
         m_pListView->setSelected(newItem, true);
      }
      m_pListView->setCurrentItem(newItem);

   } else {
      fillListView();
      slotSelectionChanged();
   }

   m_pListView->setContentsPos(pos.x(), pos.y());
}

void KEBTopLevel::fillListView()
{
   m_pListView->clear();
   // (re)create root item
   KBookmarkGroup root = s_pManager->root();
   KEBListViewItem * rootItem = new KEBListViewItem(m_pListView, root);
   fillGroup(rootItem, root);
   rootItem->QListViewItem::setOpen(true);
}

void KEBTopLevel::fillGroup(KEBListViewItem * parentItem, KBookmarkGroup group)
{
   KEBListViewItem * lastItem = 0;
   for (KBookmark bk = group.first() ; !bk.isNull() ; bk = group.next(bk)) {
      //kdDebug() << "KEBTopLevel::fillGroup group=" << group.text() << " bk=" << bk.text() << endl;
      if (bk.isGroup()) {
         KBookmarkGroup grp = bk.toGroup();
         KEBListViewItem *item = new KEBListViewItem(parentItem, lastItem, grp);
         fillGroup(item, grp);
         if (grp.isOpen()) {
            // no need to save it again :)
            item->QListViewItem::setOpen(true);
         }
         if (grp.first().isNull()) {
            // kdDebug() << "cool, an empty group!!!" << endl;
            new KEBListViewItem( item, item );
         }
         lastItem = item;

      } else {
         lastItem = new KEBListViewItem(parentItem, lastItem, bk);
      }
   }
}

bool KEBTopLevel::queryClose()
{
   if (m_bModified) {
      switch ( KMessageBox::warningYesNoCancel( this,
               i18n("The bookmarks have been modified.\nSave changes?")) ) {
         case KMessageBox::Yes :
            return save();
         case KMessageBox::No :
            return true;
            // case KMessageBox::Cancel :
         default:
            return false;
      }
   }
   return true;
}

void KEBTopLevel::slotCommandExecuted()
{
   kdDebug() << "KEBTopLevel::slotCommandExecuted" << endl;
   KEBTopLevel::self()->setModified();
   KEBTopLevel::self()->update();     // Update GUI
   slotSelectionChanged();
}

#include "toplevel.moc"
