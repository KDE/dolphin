/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_listview.h"
#include "konq_listviewitems.h"
#include "konq_listviewwidget.h"
#include "konq_textviewwidget.h"
#include "konq_treeviewwidget.h"
#include <konq_undo.h>

#include <kaction.h>
#include <kcolordlg.h>
#include <kcursor.h>
#include <kdebug.h>
#include <kio/job.h>
#include <klibloader.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <konq_bgnddlg.h>
#include <konq_dirlister.h>
#include <konq_drag.h>
#include <konq_fileitem.h>
#include <kpropsdlg.h>
#include <kprotocolmanager.h>
#include <kstdaction.h>

#include <qapplication.h>
#include <qclipboard.h>
#include <qdragobject.h>
#include <qheader.h>
#include <qkeycode.h>
#include <qlist.h>
#include <qregexp.h>
#include <qstringlist.h>
#include <qtimer.h>

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

KonqListViewFactory::KonqListViewFactory()
{
  s_instance = 0;
  s_defaultViewProps = 0;
}

KonqListViewFactory::~KonqListViewFactory()
{
  if ( s_instance )
    delete s_instance;
  if ( s_defaultViewProps )
    delete s_defaultViewProps;

  s_instance = 0;
  s_defaultViewProps = 0;
}

KParts::Part* KonqListViewFactory::createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList &args )
{
  if( args.count() < 1 )
    kdWarning() << "KonqListView: Missing Parameter" << endl;

  KParts::Part *obj = new KonqListView( parentWidget, parent, name, args.first() );
  emit objectCreated( obj );
  return obj;
}

KInstance *KonqListViewFactory::instance()
{
  if ( !s_instance )
    s_instance = new KInstance( "konqlistview" );
  return s_instance;
}

KonqPropsView *KonqListViewFactory::defaultViewProps()
{
  if ( !s_defaultViewProps )
    s_defaultViewProps = new KonqPropsView( instance(),0L );
    //s_defaultViewProps = KonqPropsView::defaultProps( instance() );

  return s_defaultViewProps;
}

KInstance *KonqListViewFactory::s_instance = 0;
KonqPropsView *KonqListViewFactory::s_defaultViewProps = 0;

extern "C"
{
  void *init_libkonqlistview()
  {
    return new KonqListViewFactory;
  }
}

ListViewBrowserExtension::ListViewBrowserExtension( KonqListView *listView )
 : KParts::BrowserExtension( listView )
{
  m_listView = listView;
}

int ListViewBrowserExtension::xOffset()
{
  kdDebug() << "ListViewBrowserExtension::xOffset" << endl;
  return m_listView->listViewWidget()->contentsX();
}

int ListViewBrowserExtension::yOffset()
{
  kdDebug() << "ListViewBrowserExtension::yOffset" << endl;
  return m_listView->listViewWidget()->contentsY();
}

void ListViewBrowserExtension::updateActions()
{
  // This code is very related to KonqIconViewWidget::slotSelectionChanged

  QValueList<KonqBaseListViewItem*> selection;
  m_listView->listViewWidget()->selectedItems( selection );

  bool cutcopy, del;
  bool bInTrash = false;
  QValueList<KonqBaseListViewItem*>::ConstIterator it = selection.begin();
  KFileItem * firstSelectedItem = 0L;
  for (; it != selection.end(); ++it )
  {
    if ( (*it)->item()->url().directory(false) == KGlobalSettings::trashPath() )
      bInTrash = true;
    if ( ! firstSelectedItem )
        firstSelectedItem = (*it)->item();
  }
  cutcopy = del = ( selection.count() > 0 );

  emit enableAction( "copy", cutcopy );
  emit enableAction( "cut", cutcopy );
  emit enableAction( "trash", del && !bInTrash && m_listView->url().isLocalFile() );
  emit enableAction( "del", del );
  emit enableAction( "shred", del );

  KFileItemList lstItems;
  if ( firstSelectedItem )
      lstItems.append( firstSelectedItem );
  emit enableAction( "properties", ( selection.count() == 1 ) &&
                     KPropertiesDialog::canDisplay( lstItems ) );
  emit enableAction( "editMimeType", ( selection.count() == 1 ) );
  emit enableAction( "rename", ( selection.count() == 1 ) );
}

void ListViewBrowserExtension::copySelection( bool move )
{
  QValueList<KonqBaseListViewItem*> selection;

  m_listView->listViewWidget()->selectedItems( selection );

  KURL::List lstURLs;

  QValueList<KonqBaseListViewItem*>::ConstIterator it = selection.begin();
  QValueList<KonqBaseListViewItem*>::ConstIterator end = selection.end();
  for (; it != end; ++it )
  {
    lstURLs.append( (*it)->item()->url() );
  }

  KonqDrag *urlData = KonqDrag::newDrag( lstURLs, move );
  QApplication::clipboard()->setData( urlData );
}

void ListViewBrowserExtension::paste()
{
  QValueList<KonqBaseListViewItem*> selection;
  /*m_listView->listViewWidget()->selectedItems( selection );
  assert ( selection.count() <= 1 );
  KURL pasteURL;
  if ( selection.count() == 1 )
    pasteURL = selection.first()->item()->url();
  else
    pasteURL = m_listView->url();*/

  KonqOperations::doPaste( m_listView->listViewWidget(), /*pasteURL*/m_listView->url() );
}

void ListViewBrowserExtension::rename()
{
  QValueList<KonqBaseListViewItem*> selection;
  m_listView->listViewWidget()->selectedItems( selection );
  ASSERT ( selection.count() == 1 );
  m_listView->listViewWidget()->rename( selection.first(), 0 );
}

void ListViewBrowserExtension::reparseConfiguration()
{
  // m_pProps is a problem here (what is local, what is global ?)
  // but settings is easy :
  m_listView->listViewWidget()->initConfig();
}

void ListViewBrowserExtension::setSaveViewPropertiesLocally(bool value)
{
   m_listView->props()->setSaveViewPropertiesLocally( value );
}

void ListViewBrowserExtension::setNameFilter( QString nameFilter )
{
  m_listView->setNameFilter( nameFilter );
}

void ListViewBrowserExtension::properties()
{
    QValueList<KonqBaseListViewItem*> selection;
    m_listView->listViewWidget()->selectedItems( selection );
    assert ( selection.count() == 1 );
    (void) new KPropertiesDialog( selection.first()->item() );
}

void ListViewBrowserExtension::editMimeType()
{
    QValueList<KonqBaseListViewItem*> selection;
    m_listView->listViewWidget()->selectedItems( selection );
    assert ( selection.count() == 1 );
    KonqOperations::editMimeType( selection.first()->item()->mimetype() );
}

KonqListView::KonqListView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode )
 : KonqDirPart( parent, name )
{
   setInstance( KonqListViewFactory::instance() );

   // Create a properties instance for this view
   // All the listview view modes inherit the same properties defaults...
   m_pProps = new KonqPropsView( KonqListViewFactory::instance(), KonqListViewFactory::defaultViewProps() );

   setBrowserExtension( new ListViewBrowserExtension( this ) );

   if (mode=="TextView")
   {
      kdDebug(1202) << "Creating KonqTextViewWidget" << endl;
      setXMLFile( "konq_textview.rc" );
      m_pListView=new KonqTextViewWidget(this, parentWidget);
   }
   else if (mode=="MixedTree")
   {
      kdDebug(1202) << "Creating KonqTreeViewWidget" << endl;
      setXMLFile( "konq_treeview.rc" );
      m_pListView=new KonqTreeViewWidget(this,parentWidget);
   }
   else
   {
      kdDebug(1202) << "Creating KonqDetailedListViewWidget" << endl;
      setXMLFile( "konq_detailedlistview.rc" );
      m_pListView = new KonqBaseListViewWidget( this, parentWidget);
   }
   setWidget( m_pListView );

   QTimer * timer = new QTimer( this );
   connect( timer, SIGNAL( timeout() ), this, SLOT( slotProcessMimeIcons() ) );
   m_mimeTypeResolver = new KonqMimeTypeResolver<KonqBaseListViewItem,KonqListView>(this,timer);
   // When our viewport is adjusted (resized or scrolled) we need
   // to get the mime types for any newly visible icons. (Rikkus)
   connect( m_pListView, SIGNAL( viewportAdjusted() ),
            SLOT( slotViewportAdjusted() ) );

   setupActions();

   m_pListView->confColumns[0].setData(I18N_NOOP("MimeType"),"Type",KIO::UDS_MIME_TYPE,-1,FALSE,m_paShowMimeType);
   m_pListView->confColumns[1].setData(I18N_NOOP("Size"),"Size",KIO::UDS_SIZE,-1,FALSE,m_paShowSize);
   m_pListView->confColumns[2].setData(I18N_NOOP("Modified"),"Date",KIO::UDS_MODIFICATION_TIME,-1,FALSE,m_paShowTime);
   m_pListView->confColumns[3].setData(I18N_NOOP("Accessed"),"AccessDate",KIO::UDS_ACCESS_TIME,-1,FALSE,m_paShowAccessTime);
   m_pListView->confColumns[4].setData(I18N_NOOP("Created"),"CreationDate",KIO::UDS_CREATION_TIME,-1,FALSE,m_paShowCreateTime);
   m_pListView->confColumns[5].setData(I18N_NOOP("Permissions"),"Access",KIO::UDS_ACCESS,-1,FALSE,m_paShowPermissions);
   m_pListView->confColumns[6].setData(I18N_NOOP("Owner"),"Owner",KIO::UDS_USER,-1,FALSE,m_paShowOwner);
   m_pListView->confColumns[7].setData(I18N_NOOP("Group"),"Group",KIO::UDS_GROUP,-1,FALSE,m_paShowGroup);
   m_pListView->confColumns[8].setData(I18N_NOOP("Link"),"Link",KIO::UDS_LINK_DEST,-1,FALSE,m_paShowLinkDest);
   m_pListView->confColumns[9].setData(I18N_NOOP("URL"),"URL",KIO::UDS_URL,-1,FALSE,m_paShowURL);
   // Note: File Type is in fact the mimetype comment. We use UDS_FILE_TYPE but that's not what we show in fact :/
   m_pListView->confColumns[10].setData(I18N_NOOP("File Type"),"Type",KIO::UDS_FILE_TYPE,-1,FALSE,m_paShowType);

   QObject::connect( m_pListView, SIGNAL( selectionChanged() ),
                     m_extension, SLOT( updateActions() ) );
   connect(m_pListView->header(),SIGNAL(indexChange(int,int,int)),this,SLOT(headerDragged(int,int,int)));
   connect(m_pListView->header(),SIGNAL(clicked(int)),this,SLOT(slotHeaderClicked(int)));

   // signals from konqdirpart (for BC reasons)
   connect( this, SIGNAL( findOpened( KonqDirPart * ) ), SLOT( slotKFindOpened() ) );
   connect( this, SIGNAL( findClosed( KonqDirPart * ) ), SLOT( slotKFindClosed() ) );
}

KonqListView::~KonqListView()
{
  delete m_mimeTypeResolver;
  delete m_pProps;
}

void KonqListView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
   KonqDirPart::guiActivateEvent(event );
   //ReadOnlyPart::guiActivateEvent(event );
   ((ListViewBrowserExtension*)m_extension)->updateActions();
};

bool KonqListView::openURL( const KURL &url )
{
  m_url = url;

  KURL u( url );

  emit setWindowCaption( u.prettyURL() );

  return m_pListView->openURL( url );
}

bool KonqListView::closeURL()
{
  m_pListView->stop();
  m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
  return true;
}

void KonqListView::listingComplete()
{
  m_mimeTypeResolver->start( /*10*/ 0 );
}

void KonqListView::determineIcon( KonqBaseListViewItem * item )
{
  //int oldSerial = item->pixmap(0)->serialNumber();

  (void) item->item()->determineMimeType();

  //QPixmap newIcon = item->item()->pixmap( m_parent->iconSize(),
  //                                        item->state() );
  //if ( oldSerial != newIcon.serialNumber() )
  //  item->setPixmap( 0, newIcon );

  // We also have columns to update, not only the icon
  item->updateContents();
}

void KonqListView::saveState( QDataStream &stream )
{
    kdDebug() << "KonqListView::saveState" << endl;
    KonqDirPart::saveState( stream );
    m_pListView->saveState( stream );
}

void KonqListView::restoreState( QDataStream &stream )
{
    KonqDirPart::restoreState( stream );
    m_pListView->restoreState( stream );
}

void KonqListView::disableIcons( const KURL::List &lst )
{
    m_pListView->disableIcons( lst );
}

void KonqListView::slotSelect()
{
   KLineEditDlg l( i18n("Select files:"), "*", m_pListView );
   if ( l.exec() )
   {
      QString pattern = l.text();
      if ( pattern.isEmpty() )
         return;

      QRegExp re( pattern, true, true );

      m_pListView->blockSignals( true );

      for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ )
         if ( re.match( it->text(0) ) != -1 )
            it->setSelected( TRUE);

      m_pListView->blockSignals( false );
   }
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotUnselect()
{
   KLineEditDlg l( i18n("Unselect files:"), "*", m_pListView );
   if ( l.exec() )
   {
      QString pattern = l.text();
      if ( pattern.isEmpty() )
         return;

      QRegExp re( pattern, TRUE, TRUE );

      m_pListView->blockSignals(TRUE);

      for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ )
         if ( re.match( it->text(0) ) != -1 )
            it->setSelected(FALSE);

      m_pListView->blockSignals(FALSE);
   }
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotSelectAll()
{
   m_pListView->selectAll(TRUE);
   emit m_pListView->selectionChanged();
}

void KonqListView::slotUnselectAll()
{
    m_pListView->selectAll(FALSE);
    emit m_pListView->selectionChanged();
}


void KonqListView::slotInvertSelection()
{
    m_pListView->invertSelection();
    emit m_pListView->selectionChanged();
    m_pListView->viewport()->update();
}

void KonqListView::newIconSize( int size )
{
    KonqDirPart::newIconSize( size );
    m_pListView->updateListContents();
}

void KonqListView::slotShowDot()
{
   m_pProps->setShowingDotFiles( m_paShowDot->isChecked() );
   m_pListView->m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );
}

void KonqListView::slotCaseInsensitive()
{
   m_pListView->setCaseInsensitiveSort( m_paCaseInsensitive->isChecked() );
   m_pListView->sort();
}

void KonqListView::slotColumnToggled()
{
   kdDebug(1202) << "::slotColumnToggled" << endl;
   for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
   {
      m_pListView->confColumns[i].displayThisOne=m_pListView->confColumns[i].toggleThisOne->isChecked()&&m_pListView->confColumns[i].toggleThisOne->isEnabled();
      //this column has been enabled, the columns after it slide one column back
      if ((m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn==-1))
      {
         int maxColumn(0);
         for (unsigned int j=0; j<KonqBaseListViewWidget::NumberOfAtoms; j++)
            if ((m_pListView->confColumns[j].displayInColumn>maxColumn) && (m_pListView->confColumns[j].displayThisOne))
               maxColumn=m_pListView->confColumns[j].displayInColumn;
         m_pListView->confColumns[i].displayInColumn=maxColumn+1;
      }
      //this column has been disabled, the columns after it slide one column
      if ((!m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn!=-1))
      {
         for (unsigned int j=0; j<KonqBaseListViewWidget::NumberOfAtoms; j++)
            if (m_pListView->confColumns[j].displayInColumn>m_pListView->confColumns[i].displayInColumn)
               m_pListView->confColumns[j].displayInColumn--;
         m_pListView->confColumns[i].displayInColumn=-1;
      }
   }

   //create the new list contents
   m_pListView->createColumns();
   m_pListView->updateListContents();

   //save the config
   KConfig * config = KGlobal::config();
   QString groupName="ListView_" + m_pListView->url().protocol();
   config->setGroup( groupName );
   QStringList lstColumns;
   int currentColumn(m_pListView->m_filenameColumn+1);
   for (int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
   {
      kdDebug(1202)<<"checking: -"<<m_pListView->confColumns[i].name<<"-"<<endl;
      if ((m_pListView->confColumns[i].displayThisOne) && (currentColumn==m_pListView->confColumns[i].displayInColumn))
      {
          lstColumns.append(m_pListView->confColumns[i].name);
          kdDebug(1202)<<" adding"<<endl;
          currentColumn++;
          i=-1;
      }
   }
   config->writeEntry("Columns",lstColumns);
   config->sync();
}

void KonqListView::slotHeaderClicked(int sec)
{
   kdDebug(1202)<<"section: "<<sec<<" clicked"<<endl;
   int clickedColumn(-1);
   for (int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
      if (m_pListView->confColumns[i].displayInColumn==sec) clickedColumn=i;
   kdDebug(1202)<<"atom index "<<clickedColumn<<endl;
   QString nameOfSortColumn;
   //we clicked the file name column
   if (clickedColumn==-1)
      nameOfSortColumn="FileName";
   else
      nameOfSortColumn=m_pListView->confColumns[clickedColumn].desktopFileName;

   if (nameOfSortColumn!=m_pListView->sortedByColumn)
   {
      m_pListView->sortedByColumn=nameOfSortColumn;
      m_pListView->setAscending(TRUE);
   }
   else
      m_pListView->setAscending(!m_pListView->ascending());

   KConfig * config = KGlobal::config();
   QString groupName="ListView_" + m_pListView->url().protocol();
   config->setGroup( groupName );
   config->writeEntry("SortBy",nameOfSortColumn);
   config->writeEntry("SortOrder",m_pListView->ascending());
   config->sync();
};

void KonqListView::headerDragged(int sec, int from, int to)
{
   kdDebug(1202)<<"section: "<<sec<<" fromIndex: "<<from<<" toIndex "<<to<<endl;
   //at this point the columns aren't moved yet, so I let the listview
   //rearrange the stuff and use a single shot timer
   QTimer::singleShot(200,this,SLOT(slotSaveAfterHeaderDrag()));
}

const KFileItem * KonqListView::currentItem()
{
   if (m_pListView==0 || m_pListView->currentItem()==0)
      return 0L;
   return static_cast<KonqListViewItem *>(m_pListView->currentItem())->item();
}

void KonqListView::slotSaveAfterHeaderDrag()
{
   KConfig * config = KGlobal::config();
   QString groupName="ListView_" + m_pListView->url().protocol();
   config->setGroup( groupName );
   QStringList lstColumns;

   int oldCurrentColumn(-1);
   for (unsigned int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
   {
      int currentColumn(1000);
      for (unsigned int j=0; j<KonqBaseListViewWidget::NumberOfAtoms; j++)
      {
         int tmp=m_pListView->header()->mapToIndex(m_pListView->confColumns[j].displayInColumn);
         if ((tmp>oldCurrentColumn) && (tmp<currentColumn))
            currentColumn=tmp;
      };
      kdDebug(1202)<<"currentColumn: "<<currentColumn<<endl;
      //everything done
      if (currentColumn==1000) break;
      for (unsigned int j=0; j<KonqBaseListViewWidget::NumberOfAtoms; j++)
      {
         int tmp=m_pListView->header()->mapToIndex(m_pListView->confColumns[j].displayInColumn);
         if (tmp==currentColumn)
         {
            oldCurrentColumn=currentColumn;
            lstColumns.append(m_pListView->confColumns[j].name);
            kdDebug(1202)<<"appending: "<<m_pListView->confColumns[j].name<<endl;
         }
      }

   }
   config->writeEntry("Columns",lstColumns);
   config->sync();
}

void KonqListView::slotKFindOpened()
{
    if ( m_pListView->m_dirLister )
        m_pListView->m_dirLister->setAutoUpdate( false );
}

void KonqListView::slotKFindClosed()
{
    if ( m_pListView->m_dirLister )
        m_pListView->m_dirLister->setAutoUpdate( true );
}

void KonqListView::setupActions()
{
   m_paShowTime=new KToggleAction(i18n("Show &Modification Time"), 0,this, SLOT(slotColumnToggled()), actionCollection(), "show_time" );
   m_paShowType=new KToggleAction(i18n("Show &File Type"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_type" );
   m_paShowMimeType=new KToggleAction(i18n("Show MimeType"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_mimetype" );
   m_paShowAccessTime=new KToggleAction(i18n("Show &Access Time"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_access_time" );
   m_paShowCreateTime=new KToggleAction(i18n("Show &Creation Time"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_creation_time" );
   m_paShowLinkDest=new KToggleAction(i18n("Show &Link Destination"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_link_dest" );
   m_paShowSize=new KToggleAction(i18n("Show Filesize"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_size" );
   m_paShowOwner=new KToggleAction(i18n("Show Owner"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_owner" );
   m_paShowGroup=new KToggleAction(i18n("Show Group"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_group" );
   m_paShowPermissions=new KToggleAction(i18n("Show Permissions"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_permissions" );
   m_paShowURL=new KToggleAction(i18n("Show URL"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_url" );

   m_paSelect = new KAction( i18n( "&Select..." ), CTRL+Key_Plus, this, SLOT( slotSelect() ), actionCollection(), "select" );
  m_paUnselect = new KAction( i18n( "&Unselect..." ), CTRL+Key_Minus, this, SLOT( slotUnselect() ), actionCollection(), "unselect" );
  m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), this, "selectall" );
  m_paUnselectAll = new KAction( i18n( "U&nselect All" ), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
  m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), CTRL+Key_Asterisk, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );

  m_paShowDot = new KToggleAction( i18n( "Show &Hidden Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );
  m_paCaseInsensitive = new KToggleAction(i18n("Case Insensitive Sort"), 0, this, SLOT(slotCaseInsensitive()),actionCollection(), "sort_caseinsensitive" );
  m_paCaseInsensitive->setChecked( m_pListView->caseInsensitiveSort() );

  newIconSize( KIcon::SizeSmall /* default size */ );
}

#include "konq_listview.moc"


