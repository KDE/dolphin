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
#include <kio/paste.h>
#include <klibloader.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <konq_bgnddlg.h>
#include <konqdirlister.h>
#include <konqdrag.h>
#include <konqfileitem.h>
#include <kparts/mainwindow.h>
#include <kparts/partmanager.h>
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
  return m_listView->listViewWidget()->contentsX();
}

int ListViewBrowserExtension::yOffset()
{
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
  emit enableAction( "trash", del && !bInTrash );
  emit enableAction( "del", del );
  emit enableAction( "shred", del );

  bool bKIOClipboard = !KIO::isClipboardEmpty();
  QMimeSource *data = QApplication::clipboard()->data();
  bool paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 ) &&
    (selection.count() == 1); // Let's allow pasting only on an item, not on the background

  emit enableAction( "paste", paste );

  KFileItemList lstItems;
  if ( firstSelectedItem )
      lstItems.append( firstSelectedItem );
  emit enableAction( "properties", ( selection.count() == 1 ) &&
                     KPropertiesDialog::canDisplay( lstItems ) );
  emit enableAction( "editMimeType", ( selection.count() == 1 ) );
}

void ListViewBrowserExtension::copySelection( bool move )
{
  QValueList<KonqBaseListViewItem*> selection;

  m_listView->listViewWidget()->selectedItems( selection );

  QStrList lstURLs;

  QValueList<KonqBaseListViewItem*>::ConstIterator it = selection.begin();
  QValueList<KonqBaseListViewItem*>::ConstIterator end = selection.end();
  for (; it != end; ++it )
  {
    lstURLs.append( (*it)->item()->url().url().local8Bit().data() );
  }

  KonqDrag *urlData = new KonqDrag;
  urlData->setUris( lstURLs );
  urlData->setMoveSelection( move );
  QApplication::clipboard()->setData( urlData );
}

void ListViewBrowserExtension::paste()
{
  QValueList<KonqBaseListViewItem*> selection;
  m_listView->listViewWidget()->selectedItems( selection );
  assert ( selection.count() <= 1 );
  KURL pasteURL;
  if ( selection.count() == 1 )
    pasteURL = selection.first()->item()->url();
  else
    pasteURL = m_listView->url();

  // move or not move ?
  bool move = false;
  QMimeSource *data = QApplication::clipboard()->data();
  if ( data->provides( "application/x-kde-cutselection" ) ) {
    move = KonqDrag::decodeIsCutSelection( data );
    kdDebug() << "move (from clipboard data) = " << move << endl;
  }
  KIO::Job * undoJob = KIO::pasteClipboard( pasteURL, move );

  if ( undoJob )
    (void) new KonqCommandRecorder( move ? KonqCommand::MOVE : KonqCommand::COPY, KURL::List(), pasteURL, undoJob );
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
      kdDebug(1202) << "Creating KonqTextViewWidget\n" << endl;
      setXMLFile( "konq_textview.rc" );
      m_pListView=new KonqTextViewWidget(this, parentWidget);
   }
   else if (mode=="MixedTree")
   {
      kdDebug(1202) << "Creating KonqTreeViewWidget\n" << endl;
      setXMLFile( "konq_treeview.rc" );
      m_pListView=new KonqTreeViewWidget(this,parentWidget);
   }
   else
   {
      kdDebug(1202) << "Creating KonqDetailedListViewWidget\n" << endl;
      setXMLFile( "konq_detailedlistview.rc" );
      m_pListView = new KonqBaseListViewWidget( this, parentWidget);
   }
   setWidget( m_pListView );

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
}

KonqListView::~KonqListView()
{
  delete m_pProps;
}

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
  return true;
}

void KonqListView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
  KParts::ReadOnlyPart::guiActivateEvent( event );
  if ( event->activated() )
    static_cast<ListViewBrowserExtension*>(m_extension)->updateActions();
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
      m_pListView->repaintContents(0,0,m_pListView->width(),m_pListView->height());
      //m_pListView->updateSelectedFilesInfo();

      // do this once, not for each item
      //m_pListView.slotSelectionChanged();
      //slotDisplayFileSelectionInfo();
   }
   emit m_pListView->selectionChanged();
   m_pListView->updateSelectedFilesInfo();
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
      m_pListView->repaintContents(0,0,m_pListView->width(),m_pListView->height());

      // do this once, not for each item
      //m_pListView.slotSelectionChanged();
      //slotDisplayFileSelectionInfo();
      //m_pListView->updateSelectedFilesInfo();
   }
   emit m_pListView->selectionChanged();
   m_pListView->updateSelectedFilesInfo();
}

void KonqListView::slotSelectAll()
{
   m_pListView->selectAll(TRUE);
   emit m_pListView->selectionChanged();
   m_pListView->updateSelectedFilesInfo();
}

void KonqListView::slotUnselectAll()
{
    m_pListView->selectAll(FALSE);
    emit m_pListView->selectionChanged();
    m_pListView->updateSelectedFilesInfo();
}


void KonqListView::slotInvertSelection()
{
    m_pListView->invertSelection();
    m_pListView->repaintContents(0,0,m_pListView->width(),m_pListView->height());
    emit m_pListView->selectionChanged();
    m_pListView->updateSelectedFilesInfo();
}

void KonqListView::slotIconSizeToggled( bool)
{
   if (m_paLargeIcons->isChecked()) m_pProps->setIconSize(KIcon::SizeLarge);
   else if (m_paMediumIcons->isChecked()) m_pProps->setIconSize(KIcon::SizeMedium);
   else m_pProps->setIconSize(KIcon::SizeSmall);
   m_pListView->updateListContents();
}

void KonqListView::slotShowDot()
{
   m_pProps->setShowingDotFiles( m_paShowDot->isChecked() );
   m_pListView->m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );
}

void KonqListView::slotColumnToggled()
{
   kdDebug(1202) << "::slotColumnToggled\n" << endl;
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
   int clickedColumn(0);
   for (int i=0; i<KonqBaseListViewWidget::NumberOfAtoms; i++)
      if (m_pListView->confColumns[i].displayInColumn==sec) clickedColumn=i;

   if (m_pListView->confColumns[clickedColumn].desktopFileName!=m_pListView->sortedByColumn)
   {
      m_pListView->sortedByColumn=m_pListView->confColumns[clickedColumn].desktopFileName;
      m_pListView->ascending=TRUE;
   }
   else
      m_pListView->ascending=!m_pListView->ascending;

   KConfig * config = KGlobal::config();
   QString groupName="ListView_" + m_pListView->url().protocol();
   config->setGroup( groupName );
   config->writeEntry("SortBy",m_pListView->confColumns[clickedColumn].desktopFileName);
   config->writeEntry("SortOrder",m_pListView->ascending);
   config->sync();
};

void KonqListView::headerDragged(int sec, int from, int to)
{
   kdDebug(1202)<<"section: "<<sec<<" fromIndex: "<<from<<" toIndex "<<to<<endl;
   //at this point the columns aren't moved yet, so I let the listview
   //rearrange the stuff and use a single shot timer
   QTimer::singleShot(200,this,SLOT(slotSaveAfterHeaderDrag()));
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
   m_paShowPermissions=new KToggleAction(i18n("Show permissions"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_permissions" );
   m_paShowURL=new KToggleAction(i18n("Show URL"), 0, this, SLOT(slotColumnToggled()),actionCollection(), "show_url" );

   m_paSelect = new KAction( i18n( "&Select..." ), CTRL+Key_Plus, this, SLOT( slotSelect() ), actionCollection(), "select" );
  m_paUnselect = new KAction( i18n( "&Unselect..." ), CTRL+Key_Minus, this, SLOT( slotUnselect() ), actionCollection(), "unselect" );
  m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), this, "selectall" );
  m_paUnselectAll = new KAction( i18n( "U&nselect All" ), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
  m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), CTRL+Key_Asterisk, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );

  m_paLargeIcons = new KRadioAction( i18n( "&Large" ), 0, actionCollection(), "modelarge" );
  m_paMediumIcons = new KRadioAction( i18n( "&Medium" ), 0, actionCollection(), "modemedium" );
  m_paSmallIcons = new KRadioAction( i18n( "&Small" ), 0, actionCollection(), "modesmall" );

  m_paShowDot = new KToggleAction( i18n( "Show &Hidden Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );
  /*KAction * m_paBackgroundColor =*/ new KAction( i18n( "Background Color..." ), 0, this, SLOT( slotBackgroundColor() ), actionCollection(), "bgcolor" );
  /*KAction * m_paBackgroundImage =*/ new KAction( i18n( "Background Image..." ), 0, this, SLOT( slotBackgroundImage() ), actionCollection(), "bgimage" );

  m_paLargeIcons->setExclusiveGroup( "ViewMode" );
  m_paMediumIcons->setExclusiveGroup( "ViewMode" );
  m_paSmallIcons->setExclusiveGroup( "ViewMode" );

  m_paLargeIcons->setChecked( false );
  m_paMediumIcons->setChecked( false );
  m_paSmallIcons->setChecked( true );

  connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
  connect( m_paMediumIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
  connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );

  slotIconSizeToggled( true );
}

#include "konq_listview.moc"


