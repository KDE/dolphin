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

#define DEBUG_ADDRESSES

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
void KEBListViewItem::paintCell(QPainter *p, const QColorGroup &cg, int column, int width, int alignment)
{
  QColorGroup col(cg);

  int h, s, v;

  if (column == 2) {
    if (render == 0) {
      col.background().hsv(&h,&s,&v);
      if (v >180 && v < 220) {
	col.setColor(QColorGroup::Text, Qt::darkGray);
      } else
	col.setColor(QColorGroup::Text, Qt::gray);
    } else if (render == 2) {
      QFont font=p->font();
      font.setBold( true );
      p->setFont(font);
    }
  }

  QListViewItem::paintCell( p, col, column, width, alignment );
}


void KEBListViewItem::init( const KBookmark & bk )
{
    setPixmap(0, SmallIcon( bk.icon() ) );
#ifdef DEBUG_ADDRESSES
    setText(3, bk.address());
#endif
    modUpdate();
}

// AK - TODO abstract this away, possibly split this rather large file up?

void internal_nsGet(QString nsinfo, QString & nCreate, QString & nAccess, QString & nModify) {
  QStringList sl = QStringList::split(' ', nsinfo);
  for ( QStringList::Iterator it = sl.begin(); it != sl.end(); ++it ) {
    QStringList spl = QStringList::split('"', *it);
    // kdDebug() << spl[0] << "+" << spl[1] << "\n";
    if (spl[0] == "LAST_MODIFIED=") {
      nModify = spl[1];
    } else if (spl[0] == "ADD_DATE=") {
      nCreate = spl[1];
    } else if (spl[0] == "LAST_VISIT=") {
      nAccess = spl[1];
    }
  }
}

void KEBListViewItem::nsGet(QString & nCreate, QString & nAccess, QString & nModify )
{
  QString nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
  internal_nsGet(nsinfo, nCreate, nAccess, nModify);
}

void KEBListViewItem::nsGet(QString & nModify )
{
  QString c, a;
  nsGet(c, a, nModify);
}

QString internal_nsPut(QString _nsinfo, QString nm) {

  QString nCreate, nAccess, nModify;

  internal_nsGet(_nsinfo, nCreate, nAccess, nModify);

  QString nsinfo;

  nsinfo  = "ADD_DATE=\"";
  nsinfo += (nCreate.isEmpty()) ? QString::number(time(0)) : nCreate;

  nsinfo += "\" LAST_VISIT=\"";
  nsinfo += (nAccess.isEmpty()) ? "0" : nAccess;

  nsinfo += "\" LAST_MODIFIED=\"";

  bool okNum = false;
  nm.toInt(&okNum);
  nsinfo += (okNum) ? nm : "1";

  nsinfo += "\"";

  return nsinfo;

}

void KEBListViewItem::nsPut( QString nm )
{
   QString _nsinfo = m_bookmark.internalElement().attribute("netscapeinfo");
   QString nsinfo = internal_nsPut(_nsinfo,nm);
   m_bookmark.internalElement().setAttribute("netscapeinfo",nsinfo);
   KEBTopLevel::self()->setModified(true);
   KEBTopLevel::self()->Modify[m_bookmark.url().url()] = nm;
   setText(2, nm);
}

// */ of nsinfo stuff

void KEBListViewItem::modUpdate( )
{

  QString url = m_bookmark.url().url();

  KEBTopLevel *top = KEBTopLevel::self();

  if (top) {
    QString nModify, oModify;
    bool ois = false, nis = false, nMod = false;
    int nM = 0, oM = 0;

    // get new mod date if there is one
    if ( top->Modify.contains(url)) {
      nModify = top->Modify[url];
      nMod = true;
      bool ok = false;
      nM = nModify.toInt(&ok);
      if (!ok)
	nis = true;
    }

    if (top->oldModify.contains(url)) {
      if (nMod) {
	oModify = top->oldModify[url];
      } else { // may be reading a second bookmark with same url
	QString oom;
	nsGet(oom);
	int ood = oom.toInt();

	oModify = top->oldModify[url];
	int ond = oModify.toInt();

	if (ood > ond) {
	  top->oldModify[url] = oom;
	  oModify = oom;
	}
      }
    } else { // first time
      nsGet(oModify);
      top->oldModify[url] = oModify;
    }
    oM = oModify.toInt();
    if (oM == 1) {
      ois = true;
    }

    //    kdDebug() << "nMod=" << nMod << " nis=" << nis << " nM=" << nM << " oM=" << nM << "\n";
    QString sn;
    QDateTime dt;

    if (nMod && nis) { // Eror in current check
      sn = nModify;
      if (ois)
	render = 1;
      else
	render = 2;
    } else if (nMod && nM == 0) { // No modify time returned
      sn = i18n(".");
    } else if (nMod && nM >= oM) { // Info from current check
      dt.setTime_t(nM);
      if (dt.daysTo(QDateTime::currentDateTime()) > 31) {
	sn =  KGlobal::locale()->formatDate(dt.date(), false);
      } else {
	sn =  KGlobal::locale()->formatDateTime(dt, false);
      }
      if (nM > oM)
	render = 2;
      else {
	render = 1;
      }
    } else if (ois) { // Error in previous check
      sn = i18n("...Error...");
      render = 0;
    } else if (oM) { // Info from previous check
      dt.setTime_t(oM);
      if (dt.daysTo(QDateTime::currentDateTime()) > 31) {
	sn =  KGlobal::locale()->formatDate(dt.date(), false);
      } else {
	sn =  KGlobal::locale()->formatDateTime(dt, false);
      }
      render = 0;
    }
    setText(2,  sn);
  }
}

void KEBListViewItem::setTmpStatus(QString status, QString &oldStatus) {
  KEBTopLevel *top = KEBTopLevel::self();
  QString url = m_bookmark.url().url();

  render = 2;
  setText(2,status);
  if (top->Modify.contains(url)) {
    oldStatus = top->Modify[url];
  } else {
    oldStatus = "";
  }
  //  kdDebug() << "setStatus " << status << " old=" << oldStatus << "\n";
  top->Modify[url] = status;
}

void KEBListViewItem::restoreStatus( QString oldStatus)
{
  KEBTopLevel *top = KEBTopLevel::self();
  QString url = m_bookmark.url().url();

  if (! oldStatus.isEmpty()) {
    top->Modify[url] = oldStatus;
  } else {
    top->Modify.remove(url);
  }
  modUpdate();
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

QDragObject *KEBListView::dragObject()
{
    if( KEBTopLevel::self()->numSelected() == 0 )
        return (QDragObject*)0;

    /* viewport() - not sure why klistview does it this way*/
    QValueList<KBookmark> bookmarks = KEBTopLevel::self()->getBookmarkSelection();
    KBookmarkDrag * drag = KBookmarkDrag::newDrag( bookmarks, viewport() );
    drag->setPixmap( SmallIcon( (bookmarks.size() > 1) ? ("bookmark") : (bookmarks.first().icon()) ) );
    return drag;
}

KBookmarkEditorIface::KBookmarkEditorIface()
 : QObject(), DCOPObject("KBookmarkEditor")
{
    connectDCOPSignal(0, "KBookmarkNotifier", "addedBookmark(QString,QString,QString,QString,QString)", "slotAddedBookmark(QString,QString,QString,QString,QString)", false);
    connectDCOPSignal(0, "KBookmarkNotifier", "createdNewFolder(QString,QString,QString)", "slotCreatedNewFolder(QString,QString,QString)", false);
}

void KBookmarkEditorIface::slotAddedBookmark( QString filename, QString url, QString text, QString address, QString icon )
{
    if ( filename == KEBTopLevel::bookmarkManager()->path() )
        emit addedBookmark( url, text, address, icon );
}

void KBookmarkEditorIface::slotCreatedNewFolder( QString filename, QString text, QString address )
{
    if ( filename == KEBTopLevel::bookmarkManager()->path() )
        emit createdNewFolder( text, address );
}

KEBTopLevel * KEBTopLevel::s_topLevel = 0L;
KBookmarkManager * KEBTopLevel::s_pManager = 0L;

// AK - note, possible ugly solution to remove code dup, a "init" bool

KEBTopLevel::KEBTopLevel( const QString & bookmarksFile, bool readonly )
    : KMainWindow(), m_commandHistory( actionCollection() ), m_dcopIface(NULL)
{
    // Create the bookmark manager.
    s_pManager = KBookmarkManager::managerForFile( bookmarksFile, false );
    m_bReadOnly = readonly;

    if (!m_bReadOnly) {
       // Create the DCOP interface object
       m_dcopIface = new KBookmarkEditorIface();
    }

    m_pListView = new KEBListView( this );

    initListView();
    connectSignals();

    s_topLevel = this;
    fillListView();

    setCentralWidget( m_pListView );
    resize( m_pListView->sizeHint().width(), 400 );

    // Create the actions

    KAction * act = new KAction( i18n( "Import Netscape Bookmarks" ), "netscape", 0, this, SLOT( slotImportNS() ), actionCollection(), "importNS" );
    act->setEnabled( QFile::exists( KNSBookmarkImporter::netscapeBookmarksFile() ) );
    (void) new KAction( i18n( "Export to Netscape Bookmarks" ), "netscape", 0, this, SLOT( slotExportNS() ), actionCollection(), "exportNS" );
    act = new KAction( i18n( "Import Mozilla Bookmarks" ), "mozilla", 0, this, SLOT( slotImportMoz() ), actionCollection(), "importMoz" );
    (void) new KAction( i18n( "Export to Mozilla Bookmarks" ), "mozilla", 0, this, SLOT( slotExportMoz() ), actionCollection(), "exportMoz" );
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
    (void) new KAction( i18n( "&Rename" ), "text", Key_F2, this, SLOT( slotRename() ), actionCollection(), "rename" );
    (void) new KAction( i18n( "Change &URL" ), "text", Key_F3, this, SLOT( slotChangeURL() ), actionCollection(), "changeurl" );
    (void) new KAction( i18n( "Chan&ge Icon" ), 0, this, SLOT( slotChangeIcon() ), actionCollection(), "changeicon" );
    (void) new KAction( i18n( "&Create New Folder" ), "folder_new", CTRL+Key_N, this, SLOT( slotNewFolder() ), actionCollection(), "newfolder" );
    (void) new KAction( i18n( "&Create New Bookmark" ), "www", 0, this, SLOT( slotNewBookmark() ), actionCollection(), "newbookmark" );
    (void) new KAction( i18n( "&Insert Separator" ), CTRL+Key_I, this, SLOT( slotInsertSeparator() ), actionCollection(), "insertseparator" );
    (void) new KAction( i18n( "&Sort Alphabetically" ), 0, this, SLOT( slotSort() ), actionCollection(), "sort" );
    (void) new KAction( i18n( "Set as &Toolbar Folder" ), "bookmark_toolbar", 0, this, SLOT( slotSetAsToolbar() ), actionCollection(), "setastoolbar" );
    (void) new KAction( i18n( "&Expand All Folders" ), 0, this, SLOT( slotExpandAll() ), actionCollection(), "expandall" );
    (void) new KAction( i18n( "&Collapse All Folders" ), 0, this, SLOT( slotCollapseAll() ), actionCollection(), "collapseall" );
    (void) new KAction( i18n( "&Open in Konqueror" ), "fileopen", 0, this, SLOT( slotOpenLink() ), actionCollection(), "openlink" );
    (void) new KAction( i18n( "Check &Status" ), "bookmark", 0, this, SLOT( slotTestLink() ), actionCollection(), "testlink" );
    (void) new KAction( i18n( "Check Status: &All" ), 0, this, SLOT( slotTestAllLinks() ), actionCollection(), "testall" );
    (void) new KAction( i18n( "Cancel &Checks" ), 0, this, SLOT( slotCancelAllTests() ), actionCollection(), "canceltests" );
    m_taShowNS = new KToggleAction( i18n( "Show Netscape Bookmarks in Konqueror Windows" ), 0, this, SLOT( slotShowNS() ), actionCollection(), "settings_showNS" );

    resetActions();

    slotSelectionChanged();
    slotClipboardDataChanged();

    createGUI();

    setAutoSaveSettings();
    setModified(false); // for a very nice caption
    m_commandHistory.documentSaved();

    KGlobal::locale()->insertCatalogue("libkonq");
}

void KEBTopLevel::slotLoad()
{
    if (!queryClose()) return;
    QString bookmarksFile = KFileDialog::getLoadFileName( QString::null, "*.xml", this );
    // add a few default place to the file dialog somehow?, e.g kfile bookmarks +  normal bookmarks file dir
    KBookmarkManager::managerForFile( bookmarksFile, false )->slotEditBookmarks();
    close();

    return;

    // AK - for the moment this can only be a readonly!

    m_pListView->clear();
    disconnectSignals();

    delete m_dcopIface;
    delete s_pManager;

    // recreate the bookmark manager.
    s_pManager = KBookmarkManager::managerForFile( bookmarksFile, false );
    m_bReadOnly = true; // readonly;
 
    if (!m_bReadOnly) {
       // Create the DCOP interface object
       m_dcopIface = new KBookmarkEditorIface();
    }

    initListView();
    connectSignals();

    fillListView();

    // resize( m_pListView->sizeHint().width(), 400 );

    resetActions();
    slotSelectionChanged();

    setAutoSaveSettings(); // NEEDED???
    setModified(false); // for a very nice caption
    m_commandHistory.documentSaved();
}

void KEBTopLevel::resetActions() 
{
    m_taShowNS->setChecked( s_pManager->showNSBookmarks() );

    // first disable all actions

    QValueList<KAction *> actions = actionCollection()->actions();
    QValueList<KAction *>::Iterator it = actions.begin();
    QValueList<KAction *>::Iterator end = actions.end();
    for (; it != end; ++it )
    {
       KAction *act = *it;
       /* do not touch the configureblah actions */
       if ( strncmp( act->name(), "configure", 9 ) )
          act->setEnabled( false );
    }

    // then reenable needed ones

    actionCollection()->action("file_open")->setEnabled(true);
    actionCollection()->action("file_save")->setEnabled(true); // setModified
    actionCollection()->action("file_save_as")->setEnabled(true);
    actionCollection()->action("file_quit")->setEnabled(true);

    actionCollection()->action("importNS")->setEnabled(true);

    if (!m_bReadOnly) {
       actionCollection()->action("exportNS")->setEnabled(true);
       actionCollection()->action("exportMoz")->setEnabled(true);
       actionCollection()->action("importMoz")->setEnabled(true);
       actionCollection()->action("settings_showNS")->setEnabled(true);
    }

}

void KEBTopLevel::initListView()
{
    // Create the list view
    m_pListView->setDragEnabled( true );
    m_pListView->addColumn( i18n("Bookmark"), 300 );
    m_pListView->addColumn( i18n("URL"), 300 );
    m_pListView->addColumn( i18n("Status/Last Modified"), 300 );
#ifdef DEBUG_ADDRESSES
    m_pListView->addColumn( i18n("Address"), 100 );
#endif

    m_pListView->setRootIsDecorated( true );
    m_pListView->setRenameable( 0 );
    m_pListView->setRenameable( 1 );

    m_pListView->setItemsRenameable( !m_bReadOnly );
    m_pListView->setItemsMovable( m_bReadOnly ); // We move items ourselves (for undo)
    m_pListView->setAcceptDrops( !m_bReadOnly );
    m_pListView->setDropVisualizer( !m_bReadOnly );

    m_pListView->setSelectionModeExt( KListView::Extended );
    m_pListView->setDragEnabled( true );
    m_pListView->setAllColumnsShowFocus( true );
    m_pListView->setSorting(-1, false);

}

void KEBTopLevel::disconnectSignals() {

    // NEW, evil method :)

    kdWarning() << disconnect( m_pListView, 0, 0, 0 ) << endl;
    kdWarning() << disconnect( s_pManager, 0, 0, 0 ) << endl;
    kdWarning() << disconnect( &m_commandHistory, 0, 0, 0 ) << endl;
    kdWarning() << disconnect( m_dcopIface, 0, 0, 0 ) << endl;

    // OLD not so evil way

    return;

    disconnect( m_pListView, SIGNAL( selectionChanged()), 0, 0 );
    disconnect( m_pListView, SIGNAL( contextMenu( KListView *, QListViewItem *, const QPoint & )), 0, 0 );

    disconnect( s_pManager, SIGNAL( changed(const QString &, const QString &) ), 0, 0 );

    if (!m_bReadOnly) {

        disconnect( m_pListView, SIGNAL( itemRenamed(QListViewItem *, const QString &, int) ), 0, 0 );
        disconnect( m_pListView, SIGNAL( dropped(QDropEvent* , QListViewItem* , QListViewItem* ) ), 0, 0 );
        disconnect( m_pListView, SIGNAL( dataChanged() ), 0, 0 );
       
        disconnect( &m_commandHistory, SIGNAL( commandExecuted() ), 0, 0 );
        disconnect( &m_commandHistory, SIGNAL( documentRestored() ), 0, 0 );
       
        disconnect( m_dcopIface, SIGNAL( addedBookmark(QString,QString,QString,QString) ), 0, 0 );
        disconnect( m_dcopIface, SIGNAL( createdNewFolder(QString,QString) ), 0, 0 );
    }
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
       connect( m_pListView, SIGNAL( itemRenamed(QListViewItem *, const QString &, int) ),
                SLOT( slotItemRenamed(QListViewItem *, const QString &, int) ) );
       connect( m_pListView, SIGNAL( dropped (QDropEvent* , QListViewItem* , QListViewItem* ) ),
                SLOT( slotDropped(QDropEvent* , QListViewItem* , QListViewItem* ) ) );
       connect( kapp->clipboard(), SIGNAL( dataChanged() ),
                SLOT( slotClipboardDataChanged() ) );

       // Update GUI after executing command
       connect( &m_commandHistory, SIGNAL( commandExecuted() ), SLOT( slotCommandExecuted() ) );
       connect( &m_commandHistory, SIGNAL( documentRestored() ), SLOT( slotDocumentRestored() ) );

       connect(m_dcopIface, SIGNAL( addedBookmark(QString,QString,QString,QString) ),
               SLOT( slotAddedBookmark(QString,QString,QString,QString) ));
       connect(m_dcopIface, SIGNAL( createdNewFolder(QString,QString) ),
               SLOT( slotCreatedNewFolder(QString,QString) ));
    }
}

KEBTopLevel::~KEBTopLevel()
{
    s_topLevel = 0L;
    if (m_dcopIface)
        delete m_dcopIface;
}

void KEBTopLevel::slotConfigureKeyBindings()
{
    KKeyDialog::configure(actionCollection());
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
   return *(selectedBookmarks()->first());
}

QPtrList<QListViewItem> * KEBTopLevel::selectedItems()
{
   // selection helper
   QPtrList<QListViewItem> *items = new QPtrList<QListViewItem>();
   for( QListViewItemIterator it(KEBTopLevel::self()->m_pListView); it.current(); it++ ) {
      if ( it.current()->isSelected()
        && it.current()->parent()
        && !it.current()->parent()->isSelected()
      ) {
         items->append(it.current());
      }
   }
   return items;
}

QPtrList<KBookmark>* KEBTopLevel::selectedBookmarks() const
{
   // selection helper
   QPtrList<KBookmark> *bookmarks = new QPtrList<KBookmark>();
   for( QListViewItemIterator it(m_pListView); it.current(); it++ ) {
      if ( it.current()->isSelected()
        && it.current()->parent()
        && !it.current()->parent()->isSelected()
      ) {
         KEBListViewItem * kebItem = static_cast<KEBListViewItem *>(it.current());
         bookmarks->append(&kebItem->bookmark());
      }
   }
   return bookmarks;
}

KBookmark KEBTopLevel::rootBookmark() const
{
   return ITEM_TO_BK(m_pListView->firstChild());
}

QValueList<KBookmark> KEBTopLevel::getBookmarkSelection() 
{
    QValueList<KBookmark> bookmarks;
    QPtrList<QListViewItem>* items = KEBTopLevel::self()->selectedItems();
    QPtrListIterator<QListViewItem> it(*items);
    for ( ; it.current() != 0; ++it ) {
       QListViewItem* item = it.current();
       bookmarks.append( KBookmark( ITEM_TO_BK(item) ) );
    }
    return bookmarks;
}

void KEBTopLevel::updateSelection()
{
    // AK - TODO - optimisation, make a selectedItems "cache"
    QListViewItem *lastItem = NULL;
    for( QListViewItemIterator it(KEBTopLevel::self()->m_pListView); it.current(); it++ ) {
       if ( it.current()->isSelected()
         && it.current()->parent()
         && !it.current()->parent()->isSelected()
       ) {
          lastItem = it.current();
       }
    }
    if (lastItem) {
       m_last_selection_address = ITEM_TO_BK(lastItem).address();
    }
}

void KEBTopLevel::slotSelectionChanged()
{
    QListViewItem * item = selectedItems()->first();
    if (item) {
        kdDebug() << "KEBTopLevel::slotSelectionChanged " << (static_cast<KEBListViewItem *>(item))->bookmark().address() << endl;
    }
    bool itemSelected = false;
    bool group = false;
    bool root = false;
    bool separator = false;
    bool urlIsEmpty = false;
    bool multiSelect = false;

    if (item)
    {
        itemSelected = true;
        KEBListViewItem * kebItem = static_cast<KEBListViewItem *>(item);
        group = kebItem->bookmark().isGroup();
        separator = kebItem->bookmark().isSeparator();
        root = (m_pListView->firstChild() == item);
        urlIsEmpty= kebItem->bookmark().url().isEmpty();
        multiSelect = numSelected() > 1;
    }

    updateSelection();

    KActionCollection * coll = actionCollection();

    coll->action("edit_copy")          ->setEnabled(itemSelected && !root);
    coll->action("openlink")           ->setEnabled(itemSelected && !group && !separator && !urlIsEmpty);

    // AK - note if these do eventually save there state for every drawer than need to move this down
    coll->action("expandall")      ->setEnabled(!multiSelect && !(root && m_pListView->childCount()==1));
    coll->action("collapseall")    ->setEnabled(!multiSelect && !(root && m_pListView->childCount()==1));

    if (!m_bReadOnly) {
        coll->action("exportNS")       ->setEnabled(true);
        coll->action("exportMoz")      ->setEnabled(true);
        coll->action("importMoz")      ->setEnabled(true);
        coll->action("settings_showNS")->setEnabled(true);
        coll->action("edit_cut")       ->setEnabled(itemSelected && !root);
        coll->action("edit_paste")     ->setEnabled(itemSelected && !root && m_bCanPaste);
        coll->action("rename")         ->setEnabled(!multiSelect && itemSelected && !separator && !root);
        coll->action("changeurl")      ->setEnabled(!multiSelect && itemSelected && !group && !separator && !root);
        coll->action("delete")         ->setEnabled(itemSelected && !root); // AK
        coll->action("newfolder")      ->setEnabled(!multiSelect);
        coll->action("changeicon")     ->setEnabled(!multiSelect && itemSelected && !root && !separator);
        coll->action("insertseparator")->setEnabled(!multiSelect && itemSelected);
        coll->action("newbookmark")    ->setEnabled(!multiSelect);
        coll->action("sort")           ->setEnabled(!multiSelect && group);
        coll->action("setastoolbar")   ->setEnabled(!multiSelect && group);
        coll->action("testlink")       ->setEnabled(!root && itemSelected && !separator); // AK
        coll->action("testall")        ->setEnabled(!multiSelect && !(root && m_pListView->childCount()==1));
    }
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

void KEBTopLevel::slotSaveAs()
{
	QString saveFilename=
		KFileDialog::getSaveFileName( QString::null, "*.xml", this );
        if(!saveFilename.isEmpty())
            s_pManager->saveAs( saveFilename );
}

bool KEBTopLevel::save()
{
    bool ok = s_pManager->save();
    if (ok)
    {
        QString data( kapp->name() );
        QCString objId( "KBookmarkManager-" );
        objId += s_pManager->path().utf8();
        kapp->dcopClient()->send( "*", objId, "notifyCompleteChange(QString)", data );
        setModified( false );
        m_commandHistory.documentSaved();
    }
    return ok;
}

QString KEBTopLevel::insertionAddress() const
{
    if( numSelected() == 0 )
        return "/0";

    KBookmark current = *(selectedBookmarks()->first());
    if (current.isGroup())
        // In a group, we insert as first child
        return current.address() + "/0";
    else
        // Otherwise, as next sibling
        return KBookmark::nextAddress( current.address() );
}

KEBListViewItem * KEBTopLevel::findByAddress( const QString & address ) const
{
    // AK - this completely assumed perfection in the address.. is that okay?
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
    Q_ASSERT(item);
    return static_cast<KEBListViewItem *>(item);
}

void KEBTopLevel::slotRename()
{
    QListViewItem* item = selectedItem();
    Q_ASSERT( item );
    if ( item )
      m_pListView->rename( item, 0 );
}

void KEBTopLevel::slotChangeURL()
{
    QListViewItem* item = selectedItem();
    Q_ASSERT( item );
    if ( item )
      m_pListView->rename( item, 1 );
}

void KEBTopLevel::deleteSelection(QString commandName) 
{
    QPtrList<QListViewItem>* items = selectedItems();
    QPtrListIterator<QListViewItem> it(*items);
    KMacroCommand * mcmd = new KMacroCommand(commandName);
    for ( ; it.current() != 0; ++it ) {
       // AK - umm.. "" in i18n field ???
       QListViewItem* item = it.current();
       DeleteCommand * dcmd = new DeleteCommand( "", ITEM_TO_BK(item).address() );
       dcmd->execute();
       mcmd->addCommand( dcmd );
    }
    m_commandHistory.addCommand( mcmd, false );
    slotCommandExecuted();
}

void KEBTopLevel::slotDelete()
{
    if( numSelected() == 0 )
    {
        kdWarning() << "KEBTopLevel::slotDelete no selected item !" << endl;
        return;
    }
    deleteSelection(i18n("Delete Items"));
}

void KEBTopLevel::slotNewFolder()
{
    // AK - fix this
    // EVIL HACK
    // We need to ask for the folder name before creating the command, in case of "Cancel".
    // But in message-freeze time, impossible to add i18n()s. So... we have to call the existing code :
    QDomDocument doc("xbel"); // Dummy document
    QDomElement elem = doc.createElement("xbel");
    doc.appendChild( elem );
    KBookmarkGroup grp( elem ); // Dummy group
    KBookmark bk = grp.createNewFolder( s_pManager, QString::null, false ); // Asks for the name
    if ( !bk.fullText().isEmpty() ) // Not canceled
    {
        CreateCommand * cmd = new CreateCommand( i18n("Create Folder"), insertionAddress(), bk.fullText(),bk.icon() , true /*open*/ );
        m_commandHistory.addCommand( cmd );
    }
}

QString KEBTopLevel::correctAddress(QString address)
{
   // AK - move to kbookmark
   return s_pManager->findByAddress(address,true).address();
}

// Handling a DCOP-originated event
void KEBTopLevel::slotCreatedNewFolder(QString text, QString address)
{
   kdWarning() << "slotCreatedNewFolder(" << text << "," << address << ")" << endl;
   if (!m_bModified) return;
   CreateCommand * cmd = new CreateCommand( i18n("Create Folder in Konqueror"), correctAddress(address), text, QString :: null, true );
   m_commandHistory.addCommand( cmd );
}

// Handling a DCOP-originated event
void KEBTopLevel::slotAddedBookmark(QString url, QString text, QString address, QString icon)
{
   kdDebug() << "slotAddedBookmark(" << url << "," << text << "," << address << "," << icon << ")" << endl;
   if (!m_bModified) return;
   CreateCommand * cmd = new CreateCommand( i18n("Add Bookmark in Konqueror"), correctAddress(address), text, icon, KURL(url) );
   m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotNewBookmark()
{
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
    int answer = KMessageBox::questionYesNo( this, i18n("Import as a new subfolder or replace all the current bookmarks?"),
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
    int answer = KMessageBox::questionYesNo( this, i18n("Import as a new subfolder or replace all the current bookmarks?"),
                                             i18n("Mozilla Import"), i18n("As New Folder"), i18n("Replace") );
    bool subFolder = (answer==KMessageBox::Yes);
    QString mozFile=KNSBookmarkImporter::mozillaBookmarksFile();
    if(!mozFile.isEmpty())
    {
        ImportCommand * cmd = new ImportCommand( i18n("Import Mozilla Bookmarks"), mozFile,
                                                 subFolder ? i18n("Mozilla Bookmarks") : QString::null, "mozilla", true);
        m_commandHistory.addCommand( cmd );
    }
}

void KEBTopLevel::slotExportNS()
{
    QString path = KNSBookmarkImporter::netscapeBookmarksFile(true);
    if (!path.isEmpty())
    {
        KNSBookmarkExporter exporter( s_pManager, path );
        exporter.write( false );
    }
}

void KEBTopLevel::slotExportMoz()
{
    QString path = KNSBookmarkImporter::mozillaBookmarksFile(true);
    if (!path.isEmpty())
    {
        KNSBookmarkExporter exporter( s_pManager, path );
        exporter.write( true );
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
    Q_ASSERT( numSelected() != 0 );
    QValueList<KBookmark> bookmarks = getBookmarkSelection();
    KBookmarkDrag* data = KBookmarkDrag::newDrag( bookmarks, 0L /* not this ! */ );
    QApplication::clipboard()->setData( data );
    slotClipboardDataChanged(); // don't ask
}

void KEBTopLevel::slotPaste()
{
    pasteData( i18n("Paste"), QApplication::clipboard()->data(), insertionAddress() );
}

void KEBTopLevel::pasteData( const QString & cmdName,  QMimeSource * data, const QString & insertionAddress )
{
    QString currentAddress = insertionAddress;
    if ( KBookmarkDrag::canDecode( data ) )
    {
        KMacroCommand * mcmd = new KMacroCommand( i18n("Add a number of bookmarks") );
        QValueList<KBookmark> bookmarks = KBookmarkDrag::decode( data );
        for ( QValueListConstIterator<KBookmark> it = bookmarks.begin(); it != bookmarks.end(); ++it ) {
           CreateCommand * cmd = new CreateCommand( cmdName, currentAddress, (*it) );
           mcmd->addCommand( cmd );
           kdDebug() << "KEBTopLevel::slotPaste url=" << (*it).url().prettyURL() << currentAddress << endl;
           currentAddress = KBookmark::nextAddress( currentAddress );
        }
        m_commandHistory.addCommand( mcmd );
    }
}

void KEBTopLevel::slotSort()
{
    KBookmark bk = selectedBookmark();
    Q_ASSERT(bk.isGroup());
    SortCommand * cmd = new SortCommand(i18n("Sort alphabetically"), bk.address());
    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotSetAsToolbar()
{
    KMacroCommand * cmd = new KMacroCommand(i18n("Set as Bookmark Toolbar"));

    KBookmarkGroup oldToolbar = s_pManager->toolbar();
    if (!oldToolbar.isNull())
    {
        QValueList<EditCommand::Edition> lst;
        lst.append(EditCommand::Edition( "toolbar", "no" ));
        lst.append(EditCommand::Edition( "icon", "" ));
        EditCommand * cmd1 = new EditCommand("", oldToolbar.address(), lst);
        cmd->addCommand(cmd1);
    }

    KBookmark bk = selectedBookmark();
    Q_ASSERT( bk.isGroup() );
    QValueList<EditCommand::Edition> lst;
    lst.append(EditCommand::Edition( "toolbar", "yes" ));
    lst.append(EditCommand::Edition( "icon", "bookmark_toolbar" ));
    EditCommand * cmd2 = new EditCommand("", bk.address(), lst);
    cmd->addCommand(cmd2);

    m_commandHistory.addCommand( cmd );
}

void KEBTopLevel::slotOpenLink()
{
    QPtrList<KBookmark>* bks = selectedBookmarks();
    QPtrListIterator<KBookmark> it(*bks);
    for ( ; it.current() != 0; ++it ) {
       KBookmark *bk = it.current();
       Q_ASSERT( !bk->isGroup() );
       (void) new KRun( bk->url() );
    }
}

void KEBTopLevel::slotTestAllLinks()
{
    QPtrList<KBookmark> bookmarks;
    KBookmark rootBm = rootBookmark();
    bookmarks.append(&rootBm);
    testBookmarks(&bookmarks);
}

void KEBTopLevel::slotTestLink()
{
    testBookmarks(selectedBookmarks());
}

void KEBTopLevel::testBookmarks(QPtrList<KBookmark>* bks) 
{
    QPtrListIterator<KBookmark> it(*bks);
    for ( ; it.current() != 0; ++it ) {
       KBookmark *bk = it.current();
       tests.insert(0, new TestLink(*bk));
       actionCollection()->action("canceltests")->setEnabled( true );
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

void KEBTopLevel::slotCancelTest(TestLink *t)
{
  tests.removeRef(t);
  delete t;
  if (tests.count() == 0)
    actionCollection()->action("canceltests")->setEnabled( false );

}

void KEBTopLevel::setAllOpen(bool open) {
   for( QListViewItemIterator it(m_pListView); it.current(); it++ ) {
      if (it.current()->parent() )
         it.current()->setOpen( open );
   }
}

void KEBTopLevel::slotExpandAll()
{
   setAllOpen(true);
}

void KEBTopLevel::slotCollapseAll()
{
   setAllOpen(false);
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
       setCaption( i18n("Bookmark Editor"), m_bModified );
    } else {
       m_bModified = false;
       setCaption( QString("%1 [%2]").arg(i18n("Bookmark Editor")).arg(i18n("Read Only")) );
    }
    actionCollection()->action("file_save")->setEnabled( m_bModified );
    s_pManager->setUpdate( !m_bModified ); // only update when non-modified
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
        QPtrList<QListViewItem>* selection = selectedItems();
        QListViewItem * i = selection->first();
        Q_ASSERT(i);
        if (i && i != _afterNow)
        {
            // sanity check - don't move a item into it's own child structure
            // AK - check if this is still a posssiblity or not, as this
            //      seems like quite unneeded complexity as itemMoved in
            //      its multiple reincarnation should skip such items...
            QListViewItem *chk = _newParent;
            while(chk)
            {
                if(chk == i)
                    return;
                chk = chk->parent();
            }

            itemMoved(selection, newAddress, e->action() == QDropEvent::Copy);
        }
    } else
    {
        // Drop from the outside
        pasteData( i18n("Drop items"), e, newAddress );
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
       if ( copy )
       {
           CreateCommand * cmd = new CreateCommand( i18n("Copy %1").arg(item->bookmark().text()), destAddress,
                                                    item->bookmark().internalElement().cloneNode( true ).toElement() );
           cmd->execute();
           finalAddress = cmd->finalAddress();
           mcmd->addCommand( cmd );
       }
       else
       {
           QString oldAddress = item->bookmark().address();
           if ( oldAddress == destAddress
             || destAddress.startsWith(oldAddress) )  // duplicate code??? // duplicate code???
           {
               continue;
           } 
           else 
           {
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
    QPoint pos(m_pListView->contentsX(), m_pListView->contentsY());
    QPtrList<QListViewItem>* items = selectedItems();
    if (items->count() != 0)
    {
        QPtrListIterator<QListViewItem> it(*items);
        QStringList addressList;
        for ( ; it.current() != 0; ++it ) {
            KEBListViewItem* item = static_cast<KEBListViewItem*>(it.current());
            QString address = ITEM_TO_BK(item).address();
            // AK - hacky, FIXME
            if ( address != "ERROR" )  
                addressList << address; 
        }
        fillListView();
        KEBListViewItem * newItem = NULL;
        for ( QStringList::Iterator ait = addressList.begin(); ait != addressList.end(); ++ait ) {
            newItem = findByAddress( *ait );
            kdDebug() << "KEBTopLevel::update item=" << *ait << endl;
            Q_ASSERT(newItem);
            if (newItem)
               m_pListView->setSelected(newItem,true);
        }
        if (newItem) {
            m_pListView->setCurrentItem(newItem);
        } else {
            newItem = findByAddress(correctAddress(m_last_selection_address));
            m_pListView->setCurrentItem(newItem);
            m_pListView->setSelected(newItem,true);
        }
    }
    else
    {
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
                                                  i18n("The bookmarks have been modified.\nSave changes?")) ) {
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
    slotSelectionChanged();
}


#include "toplevel.moc"
