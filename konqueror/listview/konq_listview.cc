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
#include "konq_textviewwidget.h"
#include "konq_treeviewwidget.h"
#include "konq_infolistviewwidget.h"

#include <kaction.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <konq_drag.h>
#include <kpropertiesdialog.h>
#include <kstdaction.h>
#include <kprotocolinfo.h>

#include <qapplication.h>
#include <qclipboard.h>
#include <qheader.h>
#include <qregexp.h>

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <kinstance.h>

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

KParts::Part* KonqListViewFactory::createPartObject( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList &args )
{
  if( args.count() < 1 )
    kdWarning() << "KonqListView: Missing Parameter" << endl;

  KParts::Part *obj = new KonqListView( parentWidget, parent, name, args.first() );
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
  void *init_konq_listview()
  {
    return new KonqListViewFactory;
  }
}

ListViewBrowserExtension::ListViewBrowserExtension( KonqListView *listView )
 : KonqDirPartBrowserExtension( listView )
{
  m_listView = listView;
}

int ListViewBrowserExtension::xOffset()
{
  //kdDebug(1202) << k_funcinfo << endl;
  return m_listView->listViewWidget()->contentsX();
}

int ListViewBrowserExtension::yOffset()
{
  //kdDebug(1202) << k_funcinfo << endl;
  return m_listView->listViewWidget()->contentsY();
}

void ListViewBrowserExtension::updateActions()
{
  // This code is very related to KonqIconViewWidget::slotSelectionChanged
  int canCopy = 0;
  int canDel = 0;
  bool bInTrash = false;
  KFileItemList lstItems = m_listView->selectedFileItems();

  for (KFileItem *item = lstItems.first(); item; item = lstItems.next())
  {
    canCopy++;
    KURL url = item->url();
    if ( url.directory(false) == KGlobalSettings::trashPath() )
      bInTrash = true;
    if (  KProtocolInfo::supportsDeleting(  url ) )
      canDel++;
  }

  emit enableAction( "copy", canCopy > 0 );
  emit enableAction( "cut", canDel > 0 );
  emit enableAction( "trash", canDel > 0 && !bInTrash && m_listView->url().isLocalFile() );
  emit enableAction( "del", canDel > 0 );
  emit enableAction( "properties", lstItems.count() > 0 && KPropertiesDialog::canDisplay( lstItems ) );
  emit enableAction( "editMimeType", ( lstItems.count() == 1 ) );
  emit enableAction( "rename", ( m_listView->listViewWidget()->currentItem() != 0 )&& !bInTrash );
}

void ListViewBrowserExtension::copySelection( bool move )
{
  KonqDrag *urlData = KonqDrag::newDrag( m_listView->listViewWidget()->selectedUrls(), move );
  QApplication::clipboard()->setData( urlData );
}

void ListViewBrowserExtension::paste()
{
  KonqOperations::doPaste( m_listView->listViewWidget(), m_listView->url() );
}

void ListViewBrowserExtension::pasteTo( const KURL& url )
{
  KonqOperations::doPaste( m_listView->listViewWidget(), url );
}

void ListViewBrowserExtension::rename()
{
  QListViewItem* item = m_listView->listViewWidget()->currentItem();
  Q_ASSERT ( item );
  m_listView->listViewWidget()->rename( item, 0 );
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

void ListViewBrowserExtension::setNameFilter( const QString &nameFilter )
{
  m_listView->setNameFilter( nameFilter );
}

void ListViewBrowserExtension::properties()
{
  (void) new KPropertiesDialog( m_listView->selectedFileItems() );
}

void ListViewBrowserExtension::editMimeType()
{
  KFileItemList items = m_listView->selectedFileItems();
  assert ( items.count() == 1 );
  KonqOperations::editMimeType( items.first()->mimetype() );
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
   else if (mode=="InfoListView")
   {
      kdDebug(1202) << "Creating KonqInfoListViewWidget" << endl;
      setXMLFile( "konq_infolistview.rc" );
      m_pListView=new KonqInfoListViewWidget(this,parentWidget);
   }
   else
   {
      kdDebug(1202) << "Creating KonqDetailedListViewWidget" << endl;
      setXMLFile( "konq_detailedlistview.rc" );
      m_pListView = new KonqBaseListViewWidget( this, parentWidget);
   }
   setWidget( m_pListView );

   m_mimeTypeResolver = new KMimeTypeResolver<KonqBaseListViewItem,KonqListView>(this);

   setupActions();

   m_pListView->confColumns.resize( 11 );
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


   connect( m_pListView, SIGNAL( selectionChanged() ),
            m_extension, SLOT( updateActions() ) );
   connect( m_pListView, SIGNAL( selectionChanged() ),
            this, SLOT( slotSelectionChanged() ) );

   connect( m_pListView, SIGNAL( currentChanged(QListViewItem*) ),
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
}

bool KonqListView::doOpenURL( const KURL &url )
{
  KURL u( url );
  emit setWindowCaption( u.prettyURL() );
  return m_pListView->openURL( url );
}

bool KonqListView::doCloseURL()
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
    //kdDebug(1202) << k_funcinfo << endl;
    KonqDirPart::saveState( stream );
    m_pListView->saveState( stream );
}

void KonqListView::restoreState( QDataStream &stream )
{
    //kdDebug(1202) << k_funcinfo << endl;
    KonqDirPart::restoreState( stream );
    m_pListView->restoreState( stream );
}

void KonqListView::disableIcons( const KURL::List &lst )
{
    m_pListView->disableIcons( lst );
}

void KonqListView::slotSelect()
{
   bool ok;
   QString pattern = KInputDialog::getText( QString::null,
      i18n( "Select files:" ), "*", &ok, m_pListView );
   if ( !ok )
      return;

   QRegExp re( pattern, true, true );

   m_pListView->blockSignals( true );

   for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ )
   {
      if ((m_pListView->automaticSelection()) && (it->isSelected()))
      {
         it->setSelected(FALSE);
         //the following line is to prevent that more than one item were selected
         //and now get deselected and automaticSelection() was true, this shouldn't happen
         //but who knows, aleXXX
         m_pListView->deactivateAutomaticSelection();
      };
      if ( re.exactMatch( it->text(0) ) )
         it->setSelected( TRUE);
   };
   m_pListView->blockSignals( false );
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotUnselect()
{
   bool ok;
   QString pattern = KInputDialog::getText( QString::null,
      i18n( "Unselect files:" ), "*", &ok, m_pListView );
   if ( !ok )
      return;

   QRegExp re( pattern, TRUE, TRUE );

   m_pListView->blockSignals(TRUE);

   for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ )
      if ( re.exactMatch( it->text(0) ) )
         it->setSelected(FALSE);

   m_pListView->blockSignals(FALSE);
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotSelectAll()
{
   m_pListView->selectAll(TRUE);
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
}

void KonqListView::slotUnselectAll()
{
    m_pListView->selectAll(FALSE);
   m_pListView->deactivateAutomaticSelection();
    emit m_pListView->selectionChanged();
}


void KonqListView::slotInvertSelection()
{
   if ((m_pListView->automaticSelection())
       && (m_pListView->currentItem()!=0)
       && (m_pListView->currentItem()->isSelected()))
      m_pListView->currentItem()->setSelected(FALSE);

   m_pListView->invertSelection();
    m_pListView->deactivateAutomaticSelection();
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
   m_pListView->m_dirLister->emitChanges();
}

void KonqListView::slotCaseInsensitive()
{
   m_pProps->setCaseInsensitiveSort( m_paCaseInsensitive->isChecked() );
   m_pListView->sort();
}

void KonqListView::slotColumnToggled()
{
   kdDebug(1202) << "::slotColumnToggled" << endl;
   for (unsigned int i=0; i<(uint)m_pListView->NumberOfAtoms; i++)
   {
      m_pListView->confColumns[i].displayThisOne=m_pListView->confColumns[i].toggleThisOne->isChecked()&&m_pListView->confColumns[i].toggleThisOne->isEnabled();
      //this column has been enabled, the columns after it slide one column back
      if ((m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn==-1))
      {
         int maxColumn(0);
         for (unsigned int j=0; j<m_pListView->NumberOfAtoms; j++)
            if ((m_pListView->confColumns[j].displayInColumn>maxColumn) && (m_pListView->confColumns[j].displayThisOne))
               maxColumn=m_pListView->confColumns[j].displayInColumn;
         m_pListView->confColumns[i].displayInColumn=maxColumn+1;
      }
      //this column has been disabled, the columns after it slide one column
      if ((!m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn!=-1))
      {
         for (unsigned int j=0; j<m_pListView->NumberOfAtoms; j++)
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
   for (int i=0; i<m_pListView->NumberOfAtoms; i++)
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
   for (int i=0; i<m_pListView->NumberOfAtoms; i++)
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
}

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
   for (unsigned int i=0; i<m_pListView->NumberOfAtoms; i++)
   {
      int currentColumn(1000);
      for (unsigned int j=0; j<m_pListView->NumberOfAtoms; j++)
      {
         int tmp=m_pListView->header()->mapToIndex(m_pListView->confColumns[j].displayInColumn);
         if ((tmp>oldCurrentColumn) && (tmp<currentColumn))
            currentColumn=tmp;
      };
      kdDebug(1202)<<"currentColumn: "<<currentColumn<<endl;
      //everything done
      if (currentColumn==1000) break;
      for (unsigned int j=0; j<m_pListView->NumberOfAtoms; j++)
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

   m_paSelect = new KAction( i18n( "Se&lect..." ), CTRL+Key_Plus, this, SLOT( slotSelect() ), actionCollection(), "select" );
  m_paUnselect = new KAction( i18n( "Unselect..." ), CTRL+Key_Minus, this, SLOT( slotUnselect() ), actionCollection(), "unselect" );
  m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), actionCollection(), "selectall" );
  m_paUnselectAll = new KAction( i18n( "Unselect All" ), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
  m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), CTRL+Key_Asterisk, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );

  m_paShowDot = new KToggleAction( i18n( "Show &Hidden Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );
  m_paCaseInsensitive = new KToggleAction(i18n("Case Insensitive Sort"), 0, this, SLOT(slotCaseInsensitive()),actionCollection(), "sort_caseinsensitive" );

  newIconSize( KIcon::SizeSmall /* default size */ );
}

void KonqListView::slotSelectionChanged()
{
  bool itemSelected = selectedFileItems().count()>0;
  m_paUnselect->setEnabled( itemSelected );
  m_paUnselectAll->setEnabled( itemSelected );
  m_paInvertSelection->setEnabled( itemSelected );
}

#include "konq_listview.moc"


