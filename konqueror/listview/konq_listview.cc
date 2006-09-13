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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_listview.h"
#include "konq_textviewwidget.h"
#include "konq_treeviewwidget.h"
#include "konq_infolistviewwidget.h"
#include "konq_listviewsettings.h"

#include <konqmimedata.h>

#include <kaction.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kinputdialog.h>
#include <klineedit.h>
#include <klocale.h>
#include <kmimetype.h>
#include <kpropertiesdialog.h>
#include <kstdaction.h>
#include <ktoggleaction.h>
#include <kprotocolmanager.h>

#include <QApplication>
#include <QClipboard>
#include <q3header.h>
#include <QRegExp>

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
  delete s_instance;
  delete s_defaultViewProps;

  s_instance = 0;
  s_defaultViewProps = 0;
}

KParts::Part* KonqListViewFactory::createPartObject( QWidget *parentWidget, QObject *parent, const char*, const QStringList &args )
{
  if( args.count() < 1 )
    kWarning() << "KonqListView: Missing Parameter" << endl;

  KParts::Part *obj = new KonqListView( parentWidget, parent, args.first() );
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

K_EXPORT_COMPONENT_FACTORY( konq_listview, KonqListViewFactory )

ListViewBrowserExtension::ListViewBrowserExtension( KonqListView *listView )
 : KonqDirPartBrowserExtension( listView )
{
  m_listView = listView;
}

int ListViewBrowserExtension::xOffset()
{
  //kDebug(1202) << k_funcinfo << endl;
  return m_listView->listViewWidget()->contentsX();
}

int ListViewBrowserExtension::yOffset()
{
  //kDebug(1202) << k_funcinfo << endl;
  return m_listView->listViewWidget()->contentsY();
}

void ListViewBrowserExtension::updateActions()
{
  // This code is very related to KonqIconViewWidget::slotSelectionChanged
  int canCopy = 0;
  int canDel = 0;
  int canTrash = 0;
  bool bInTrash = false;
  const KFileItemList lstItems = m_listView->selectedFileItems();

  KFileItemList::const_iterator kit = lstItems.begin();
  const KFileItemList::const_iterator kend = lstItems.end();
  for (; kit != kend; ++kit )
  {
    KFileItem* item = *kit;
    canCopy++;
    KUrl url = item->url();
#warning hardcoded protocol: find a better way to determine if a url is a trash url.
    if ( url.protocol() == "trash" )
      bInTrash = true;
    if (  KProtocolManager::supportsDeleting(  url ) )
      canDel++;
    if ( !item->localPath().isEmpty() )
      canTrash++;
  }

  emit enableAction( "copy", canCopy > 0 );
  emit enableAction( "cut", canDel > 0 );
  emit enableAction( "trash", canDel > 0 && !bInTrash && canDel == canTrash );
  emit enableAction( "del", canDel > 0 );
  emit enableAction( "properties", lstItems.count() > 0 && KPropertiesDialog::canDisplay( lstItems ) );
  emit enableAction( "editMimeType", ( lstItems.count() == 1 ) );
  emit enableAction( "rename", ( m_listView->listViewWidget()->currentItem() != 0 )&& !bInTrash );
}

void ListViewBrowserExtension::copySelection( bool move )
{
  QMimeData* mimeData = new QMimeData;
  KonqMimeData::populateMimeData( mimeData, m_listView->listViewWidget()->selectedUrls(false),
                                  m_listView->listViewWidget()->selectedUrls(true), move );
  QApplication::clipboard()->setMimeData( mimeData );
}

void ListViewBrowserExtension::paste()
{
  KonqOperations::doPaste( m_listView->listViewWidget(), m_listView->url() );
}

void ListViewBrowserExtension::pasteTo( const KUrl& url )
{
  KonqOperations::doPaste( m_listView->listViewWidget(), url );
}

void ListViewBrowserExtension::rename()
{
  Q3ListViewItem* item = m_listView->listViewWidget()->currentItem();
  Q_ASSERT ( item );
  m_listView->listViewWidget()->rename( item, 0 );

  // Enhanced rename: Don't highlight the file extension.
  KLineEdit* le = m_listView->listViewWidget()->renameLineEdit();
  if ( le ) {
     const QString txt = le->text();
     QString fileExtension = KMimeType::extractKnownExtension( txt );
     if (!fileExtension.isEmpty())
         le->setSelection(0, txt.length()-fileExtension.length()+1);
     else
     {
         int lastDot = txt.lastIndexOf('.');
         if (lastDot > 0)
             le->setSelection(0, lastDot);
     }
  }
}

void ListViewBrowserExtension::trash()
{
  KonqOperations::del(m_listView->listViewWidget(),
                      KonqOperations::TRASH,
                      m_listView->listViewWidget()->selectedUrls( true ));
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

KonqListView::KonqListView( QWidget *parentWidget, QObject *parent, const QString& mode )
 : KonqDirPart( parent )
,m_headerTimer(0)
{
   setInstance( KonqListViewFactory::instance(), false );

   // Create a properties instance for this view
   // All the listview view modes inherit the same properties defaults...
   m_pProps = new KonqPropsView( KonqListViewFactory::instance(), KonqListViewFactory::defaultViewProps() );

   setBrowserExtension( new ListViewBrowserExtension( this ) );

   QString xmlFile;

   if (mode=="TextView")
   {
      kDebug(1202) << "Creating KonqTextViewWidget" << endl;
      xmlFile = "konq_textview.rc";
      m_pListView=new KonqTextViewWidget(this, parentWidget);
   }
   else if (mode=="MixedTree")
   {
      kDebug(1202) << "Creating KonqTreeViewWidget" << endl;
      xmlFile = "konq_treeview.rc";
      m_pListView=new KonqTreeViewWidget(this,parentWidget);
   }
   else if (mode=="InfoListView")
   {
      kDebug(1202) << "Creating KonqInfoListViewWidget" << endl;
      xmlFile = "konq_infolistview.rc";
      m_pListView=new KonqInfoListViewWidget(this,parentWidget);
   }
   else
   {
      kDebug(1202) << "Creating KonqDetailedListViewWidget" << endl;
      xmlFile = "konq_detailedlistview.rc";
      m_pListView = new KonqBaseListViewWidget( this, parentWidget);
   }
   setWidget( m_pListView );
   setDirLister( m_pListView->m_dirLister );

   m_mimeTypeResolver = new KMimeTypeResolver<KonqBaseListViewItem,KonqListView>(this);

   setXMLFile( xmlFile );

   setupActions();

   m_pListView->confColumns.resize( 11 );
   m_pListView->confColumns[0].setData(I18N_NOOP("MimeType"),"Type",KIO::UDS_MIME_TYPE,m_paShowMimeType);
   m_pListView->confColumns[1].setData(I18N_NOOP("Size"),"Size",KIO::UDS_SIZE,m_paShowSize);
   m_pListView->confColumns[2].setData(I18N_NOOP("Modified"),"Date",KIO::UDS_MODIFICATION_TIME,m_paShowTime);
   m_pListView->confColumns[3].setData(I18N_NOOP("Accessed"),"AccessDate",KIO::UDS_ACCESS_TIME,m_paShowAccessTime);
   m_pListView->confColumns[4].setData(I18N_NOOP("Created"),"CreationDate",KIO::UDS_CREATION_TIME,m_paShowCreateTime);
   m_pListView->confColumns[5].setData(I18N_NOOP("Permissions"),"Access",KIO::UDS_ACCESS,m_paShowPermissions);
   m_pListView->confColumns[6].setData(I18N_NOOP("Owner"),"Owner",KIO::UDS_USER,m_paShowOwner);
   m_pListView->confColumns[7].setData(I18N_NOOP("Group"),"Group",KIO::UDS_GROUP,m_paShowGroup);
   m_pListView->confColumns[8].setData(I18N_NOOP("Link"),"Link",KIO::UDS_LINK_DEST,m_paShowLinkDest);
   m_pListView->confColumns[9].setData(I18N_NOOP("URL"),"URL",KIO::UDS_URL,m_paShowURL);
   // Note: File Type is in fact the mimetype comment. We use UDS_FILE_TYPE but that's not what we show in fact :/
   m_pListView->confColumns[10].setData(I18N_NOOP("File Type"),"Type",KIO::UDS_FILE_TYPE,m_paShowType);


   connect( m_pListView, SIGNAL( selectionChanged() ),
            m_extension, SLOT( updateActions() ) );
   connect( m_pListView, SIGNAL( selectionChanged() ),
            this, SLOT( slotSelectionChanged() ) );

   connect( m_pListView, SIGNAL( currentChanged(Q3ListViewItem*) ),
            m_extension, SLOT( updateActions() ) );
   connect(m_pListView->header(),SIGNAL(indexChange(int,int,int)),this,SLOT(headerDragged(int,int,int)));
   connect(m_pListView->header(),SIGNAL(clicked(int)),this,SLOT(slotHeaderClicked(int)));
   connect(m_pListView->header(),SIGNAL(sizeChange(int,int,int)),SLOT(slotHeaderSizeChanged()));

   // signals from konqdirpart (for BC reasons)
   connect( this, SIGNAL( findOpened( KonqDirPart * ) ), SLOT( slotKFindOpened() ) );
   connect( this, SIGNAL( findClosed( KonqDirPart * ) ), SLOT( slotKFindClosed() ) );

   loadPlugins( this, this, instance() );
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

bool KonqListView::doOpenURL( const KUrl &url )
{
  KUrl u( url );
  const QString prettyUrl = url.pathOrUrl();
  emit setWindowCaption( prettyUrl );
  return m_pListView->openUrl( url );
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
    //kDebug(1202) << k_funcinfo << endl;
    KonqDirPart::saveState( stream );
    m_pListView->saveState( stream );
}

void KonqListView::restoreState( QDataStream &stream )
{
    //kDebug(1202) << k_funcinfo << endl;
    KonqDirPart::restoreState( stream );
    m_pListView->restoreState( stream );
}

void KonqListView::disableIcons( const KUrl::List &lst )
{
    m_pListView->disableIcons( lst );
}

void KonqListView::slotSelect()
{
   bool ok;
   QString pattern = KInputDialog::getText( QString(),
      i18n( "Select files:" ), "*", &ok, m_pListView );
   if ( !ok )
      return;

   QRegExp re( pattern, Qt::CaseSensitive, QRegExp::Wildcard );

   m_pListView->blockSignals( true );

   for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ )
   {
      if ((m_pListView->automaticSelection()) && (it->isSelected()))
      {
         it->setSelected(false);
         //the following line is to prevent that more than one item were selected
         //and now get deselected and automaticSelection() was true, this shouldn't happen
         //but who knows, aleXXX
         m_pListView->deactivateAutomaticSelection();
      };
      if ( re.exactMatch( it->text(0) ) )
         it->setSelected( true );
   };
   m_pListView->blockSignals( false );
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotUnselect()
{
   bool ok;
   QString pattern = KInputDialog::getText( QString(),
      i18n( "Unselect files:" ), "*", &ok, m_pListView );
   if ( !ok )
      return;

   QRegExp re( pattern, Qt::CaseSensitive, QRegExp::Wildcard );

   m_pListView->blockSignals(true);

   for (KonqBaseListViewWidget::iterator it = m_pListView->begin(); it != m_pListView->end(); it++ )
      if ( re.exactMatch( it->text(0) ) )
         it->setSelected(false);

   m_pListView->blockSignals(false);
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
   m_pListView->viewport()->update();
}

void KonqListView::slotSelectAll()
{
   m_pListView->selectAll(true);
   m_pListView->deactivateAutomaticSelection();
   emit m_pListView->selectionChanged();
}

void KonqListView::slotUnselectAll()
{
    m_pListView->selectAll(false);
   m_pListView->deactivateAutomaticSelection();
    emit m_pListView->selectionChanged();
}


void KonqListView::slotInvertSelection()
{
   if ((m_pListView->automaticSelection())
       && (m_pListView->currentItem()!=0)
       && (m_pListView->currentItem()->isSelected()))
      m_pListView->currentItem()->setSelected(false);

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
   kDebug(1202) << "::slotColumnToggled" << endl;
   for (uint i=0; i<m_pListView->NumberOfAtoms; i++)
   {
      m_pListView->confColumns[i].displayThisOne=!m_pListView->confColumns[i].toggleThisOne
        || (m_pListView->confColumns[i].toggleThisOne->isChecked()&&m_pListView->confColumns[i].toggleThisOne->isEnabled());
      //this column has been enabled, the columns after it slide one column back
      if ((m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn==-1))
      {
         int maxColumn(0);
         for (uint j=0; j<m_pListView->NumberOfAtoms; j++)
            if ((m_pListView->confColumns[j].displayInColumn>maxColumn) && (m_pListView->confColumns[j].displayThisOne))
               maxColumn=m_pListView->confColumns[j].displayInColumn;
         m_pListView->confColumns[i].displayInColumn=maxColumn+1;
      }
      //this column has been disabled, the columns after it slide one column
      if ((!m_pListView->confColumns[i].displayThisOne) && (m_pListView->confColumns[i].displayInColumn!=-1))
      {
         for (uint j=0; j<m_pListView->NumberOfAtoms; j++)
            if (m_pListView->confColumns[j].displayInColumn>m_pListView->confColumns[i].displayInColumn)
               m_pListView->confColumns[j].displayInColumn--;
         m_pListView->confColumns[i].displayInColumn=-1;
      }
   }

   //create the new list contents
   m_pListView->createColumns();
   m_pListView->updateListContents();

   //save the config
   QStringList lstColumns;
   int currentColumn(m_pListView->m_filenameColumn+1);
   for (int i=0; i<(int)m_pListView->NumberOfAtoms; i++)
   {
      kDebug(1202)<<"checking: -"<<m_pListView->confColumns[i].name<<"-"<<endl;
      if ((m_pListView->confColumns[i].displayThisOne) && (currentColumn==m_pListView->confColumns[i].displayInColumn))
      {
          lstColumns.append(m_pListView->confColumns[i].name);
          kDebug(1202)<<" adding"<<endl;
          currentColumn++;
          i=-1;
      }
   }
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();
   config.setColumns( lstColumns );
   config.writeConfig();

   // Update column sizes
   slotHeaderSizeChanged();
}

void KonqListView::slotHeaderClicked(int sec)
{
   kDebug(1202)<<"section: "<<sec<<" clicked"<<endl;
   int clickedColumn(-1);
   for (uint i=0; i<m_pListView->NumberOfAtoms; i++)
      if (m_pListView->confColumns[i].displayInColumn==sec) clickedColumn=i;
   kDebug(1202)<<"atom index "<<clickedColumn<<endl;
   QString nameOfSortColumn;
   //we clicked the file name column
   if (clickedColumn==-1)
      nameOfSortColumn="FileName";
   else
      nameOfSortColumn=m_pListView->confColumns[clickedColumn].desktopFileName;

   if (nameOfSortColumn!=m_pListView->sortedByColumn)
   {
      m_pListView->sortedByColumn=nameOfSortColumn;
      m_pListView->setAscending(true);
   }
   else
      m_pListView->setAscending(!m_pListView->ascending());

   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();
   config.setSortBy( nameOfSortColumn );
   config.setSortOrder( m_pListView->ascending() );
   config.writeConfig();
}

void KonqListView::headerDragged(int sec, int from, int to)
{
   kDebug(1202)<<"section: "<<sec<<" fromIndex: "<<from<<" toIndex "<<to<<endl;
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
   QStringList lstColumns;

   for ( int i=0; i < m_pListView->columns(); i++ )
   {
      int section = m_pListView->header()->mapToSection( i );

      // look for section
      for ( uint j=0; j < m_pListView->NumberOfAtoms; j++ )
      {
         if ( m_pListView->confColumns[j].displayInColumn == section )
         {
            lstColumns.append( m_pListView->confColumns[j].name );
            break;
         }
      }
   }
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();
   config.setColumns( lstColumns );
   config.writeConfig();

   // Update column sizes
   slotHeaderSizeChanged();
}

void KonqListView::slotSaveColumnWidths()
{
   QList<int> lstColumnWidths;

   for ( int i=0; i < m_pListView->columns(); i++ )
   {
      int section = m_pListView->header()->mapToSection( i );

      // look for section
      for ( uint j=0; j < m_pListView->NumberOfAtoms; j++ )
      {
         // Save size only if the column is found
         if ( m_pListView->confColumns[j].displayInColumn == section )
         {
            m_pListView->confColumns[j].width = m_pListView->columnWidth(section);
            lstColumnWidths.append( m_pListView->confColumns[j].width );
            break;
         }
      }
   }
   KonqListViewSettings config( m_pListView->url().protocol() );
   config.readConfig();
   config.setColumnWidths( lstColumnWidths );

   // size of current filename column
   config.setFileNameColumnWidth( m_pListView->columnWidth(0) );
   config.writeConfig();
}

void KonqListView::slotHeaderSizeChanged()
{
   if ( !m_headerTimer )
   {
      m_headerTimer = new QTimer( this );
      connect( m_headerTimer, SIGNAL( timeout() ), this, SLOT( slotSaveColumnWidths() ) );
   }
   else
      m_headerTimer->stop();

   m_headerTimer->setSingleShot( true );
   m_headerTimer->start( 250 );
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
   m_paShowTime = new KToggleAction(i18n("Show &Modification Time"), actionCollection(), "show_time" );
   connect(m_paShowTime, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowTime->setCheckedState(KGuiItem(i18n("Hide &Modification Time")));
   m_paShowType = new KToggleAction(i18n("Show &File Type"), actionCollection(), "show_type" );
   connect(m_paShowType, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowType->setCheckedState(KGuiItem(i18n("Hide &File Type")));
   m_paShowMimeType = new KToggleAction(i18n("Show MimeType"), actionCollection(), "show_mimetype" );
   connect(m_paShowMimeType, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowMimeType->setCheckedState(KGuiItem(i18n("Hide MimeType")));
   m_paShowAccessTime = new KToggleAction(i18n("Show &Access Time"), actionCollection(), "show_access_time" );
   connect(m_paShowAccessTime, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowAccessTime->setCheckedState(KGuiItem(i18n("Hide &Access Time")));
   m_paShowCreateTime = new KToggleAction(i18n("Show &Creation Time"), actionCollection(), "show_creation_time" );
   connect(m_paShowCreateTime, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowCreateTime->setCheckedState(KGuiItem(i18n("Hide &Creation Time")));
   m_paShowLinkDest = new KToggleAction(i18n("Show &Link Destination"), actionCollection(), "show_link_dest" );
   connect(m_paShowLinkDest, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowLinkDest->setCheckedState(KGuiItem(i18n("Hide &Link Destination")));
   m_paShowSize = new KToggleAction(i18n("Show Filesize"), actionCollection(), "show_size" );
   connect(m_paShowSize, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowSize->setCheckedState(KGuiItem(i18n("Hide Filesize")));
   m_paShowOwner = new KToggleAction(i18n("Show Owner"), actionCollection(), "show_owner" );
   connect(m_paShowOwner, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowOwner->setCheckedState(KGuiItem(i18n("Hide Owner")));
   m_paShowGroup = new KToggleAction(i18n("Show Group"), actionCollection(), "show_group" );
   connect(m_paShowGroup, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowGroup->setCheckedState(KGuiItem(i18n("Hide Group")));
   m_paShowPermissions = new KToggleAction(i18n("Show Permissions"), actionCollection(), "show_permissions" );
   connect(m_paShowPermissions, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));
   m_paShowPermissions->setCheckedState(KGuiItem(i18n("Hide Permissions")));
   m_paShowURL = new KToggleAction(i18n("Show URL"), actionCollection(), "show_url" );
   connect(m_paShowURL, SIGNAL(triggered(bool) ), SLOT(slotColumnToggled()));

   m_paSelect = new KAction( i18n( "Se&lect..." ), actionCollection(), "select" );
   connect(m_paSelect, SIGNAL(triggered(bool) ), SLOT( slotSelect() ));
   m_paSelect->setShortcut(Qt::CTRL+Qt::Key_Plus);
  m_paUnselect = new KAction( i18n( "Unselect..." ), actionCollection(), "unselect" );
  connect(m_paUnselect, SIGNAL(triggered(bool) ), SLOT( slotUnselect() ));
  m_paUnselect->setShortcut(Qt::CTRL+Qt::Key_Minus);
  m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), actionCollection(), "selectall" );
  m_paUnselectAll = new KAction( i18n( "Unselect All" ), actionCollection(), "unselectall" );
  connect(m_paUnselectAll, SIGNAL(triggered(bool) ), SLOT( slotUnselectAll() ));
  m_paUnselectAll->setShortcut(Qt::CTRL+Qt::Key_U);
  m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), actionCollection(), "invertselection" );
  connect(m_paInvertSelection, SIGNAL(triggered(bool) ), SLOT( slotInvertSelection() ));
  m_paInvertSelection->setShortcut(Qt::CTRL+Qt::Key_Asterisk);

  m_paShowDot = new KToggleAction( i18n( "Show &Hidden Files" ), actionCollection(), "show_dot" );
  connect(m_paShowDot, SIGNAL(triggered(bool) ), SLOT( slotShowDot() ));
//  m_paShowDot->setCheckedState(i18n("Hide &Hidden Files"));
  m_paCaseInsensitive = new KToggleAction(i18n("Case Insensitive Sort"), actionCollection(), "sort_caseinsensitive" );
  connect(m_paCaseInsensitive, SIGNAL(triggered(bool) ), SLOT(slotCaseInsensitive()));

  newIconSize( K3Icon::SizeSmall /* default size */ );
}

void KonqListView::slotSelectionChanged()
{
  bool itemSelected = selectedFileItems().count()>0;
  m_paUnselect->setEnabled( itemSelected );
  m_paUnselectAll->setEnabled( itemSelected );
//  m_paInvertSelection->setEnabled( itemSelected );
}

#include "konq_listview.moc"


