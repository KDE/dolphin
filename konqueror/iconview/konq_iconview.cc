/* This file is part of the KDE projects
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

#include "konq_iconview.h"
#include "konq_bgnddlg.h"
#include "konq_propsview.h"
#include "konq_childview.h"
#include "konq_frame.h"
#include "konq_factory.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kaccel.h>
#include <kcolordlg.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfileivi.h>
#include <kfileitem.h>
#include <kio_error.h>
#include <kio_job.h>
#include <klibloader.h>
#include <klineeditdlg.h>
#include <kmimetype.h>
#include <konqiconviewwidget.h>
#include <konqsettings.h>
#include <krun.h>
#include <kurl.h>

#include <qmessagebox.h>
#include <qfile.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <klocale.h>
#include <qregexp.h>

class KonqIconViewFactory : public KLibFactory
{
public:
  KonqIconViewFactory() 
  {
    KonqFactory::instanceRef();
  }
  virtual ~KonqIconViewFactory()
  {
    KonqFactory::instanceUnref();
  }

  virtual QObject* create( QObject*, const char*, const char*, const QStringList &args )
  {
    KonqKfmIconView *obj = new KonqKfmIconView;
    emit objectCreated( obj );

    QStringList::ConstIterator it = args.begin();
    QStringList::ConstIterator end = args.end();
    uint i = 1;
    for (; it != end; ++it, ++i )
      if ( *it == "-viewMode" && i <= args.count() )
      {
        ++it;
	
	QIconView *iconView = obj->iconViewWidget();
	
        // TODO : use setSize instead of setViewMode
	if ( *it == "LargeIcons" )
	{
	  iconView->setViewMode( QIconSet::Large );
	  iconView->setItemTextPos( QIconView::Bottom );
	}
	else if ( *it == "SmallIcons" )
	{
	  iconView->setViewMode( QIconSet::Small );
	  iconView->setItemTextPos( QIconView::Bottom );
	}
	else if ( *it == "SmallVerticalIcons" )
	{
	  iconView->setViewMode( QIconSet::Small );
	  iconView->setItemTextPos( QIconView::Right );
	}

        break;
      }

    return obj;
  }

};

extern "C"
{
  void *init_libkonqiconview()
  {
    return new KonqIconViewFactory;
  }
};

IconViewPropertiesExtension::IconViewPropertiesExtension( KonqKfmIconView *iconView )
  : ViewPropertiesExtension( iconView, "ViewPropertiesExtension" )
{
  m_iconView = iconView;
}

void IconViewPropertiesExtension::reparseConfiguration()
{
 KonqFMSettings::reparseConfiguration();
  // m_pProps is a problem here (what is local, what is global ?)
  // but settings is easy : 
  m_iconView->iconViewWidget()->initConfig();
}

void IconViewPropertiesExtension::saveLocalProperties()
{
  m_iconView->m_pProps->saveLocal( KURL( m_iconView->url() ) );
}

void IconViewPropertiesExtension::savePropertiesAsDefault()
{
  m_iconView->m_pProps->saveAsDefault();
}

KonqKfmIconView::KonqKfmIconView()
{
  kdebug(0, 1202, "+KonqKfmIconView");

  // Create a properties instance for this view
  // (copying the default values)
  m_pProps = new KonqPropsView( * KonqPropsView::defaultProps() );

  m_pIconView = new KonqIconViewWidget( this, "qiconview" );

  m_extension = new IconEditExtension( m_pIconView );

  (void)new IconViewPropertiesExtension( this );

  m_ulTotalFiles = 0;

  // Don't repaint on configuration changes during construction
  m_bInit = true;

  m_paDotFiles = new KToggleAction( i18n( "Show &Dot Files" ), 0, this, SLOT( slotShowDot() ), this );
  m_pamSort = new KActionMenu( i18n( "Sort..." ), this );

  KToggleAction *aSortByNameCS = new KToggleAction( i18n( "by Name (Case Sensitive)" ), 0, this );
  KToggleAction *aSortByNameCI = new KToggleAction( i18n( "by Name (Case Insensitive)" ), 0, this );
  KToggleAction *aSortBySize = new KToggleAction( i18n( "By Size" ), 0, this );

  aSortByNameCS->setExclusiveGroup( "sorting" );
  aSortByNameCI->setExclusiveGroup( "sorting" );
  aSortBySize->setExclusiveGroup( "sorting" );

  aSortByNameCS->setChecked( true );
  aSortByNameCI->setChecked( false );
  aSortBySize->setChecked( false );

  connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseSensitive( bool ) ) );
  connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseInsensitive( bool ) ) );
  connect( aSortBySize, SIGNAL( toggled( bool ) ), this, SLOT( slotSortBySize( bool ) ) );

  KToggleAction *aSortDescending = new KToggleAction( i18n( "Descending" ), 0, this );

  connect( aSortDescending, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDescending( bool ) ) );

  m_pamSort->insert( aSortByNameCS );
  m_pamSort->insert( aSortByNameCI );
  m_pamSort->insert( aSortBySize );

  m_pamSort->popupMenu()->insertSeparator();

  m_pamSort->insert( aSortDescending );

  m_paSelect = new KAction( i18n( "&Select" ), 0, this, SLOT( slotSelect() ), this );
  m_paUnselect = new KAction( i18n( "&Unselect" ), 0, this, SLOT( slotUnselect() ), this );
  m_paSelectAll = new KAction( i18n( "Select &All" ), CTRL+Key_A, this, SLOT( slotSelectAll() ), this );
  m_paUnselectAll = new KAction( i18n( "U&nselect All" ), 0, this, SLOT( slotUnselectAll() ), this );

  m_paLargeIcons = new KToggleAction( i18n( "&Large View" ), 0, this );
  m_paNormalIcons = new KToggleAction( i18n( "&Normal View" ), 0, this );
  m_paSmallIcons = new KToggleAction( i18n( "&Small View" ), 0, this );
  m_paKOfficeMode = new KToggleAction( i18n( "&KOffice mode" ), 0, this );

  m_paLargeIcons->setExclusiveGroup( "ViewMode" );
  m_paNormalIcons->setExclusiveGroup( "ViewMode" );
  m_paSmallIcons->setExclusiveGroup( "ViewMode" );
  m_paKOfficeMode->setExclusiveGroup( "ViewMode" );

  m_paLargeIcons->setChecked( false );
  m_paNormalIcons->setChecked( true );
  m_paSmallIcons->setChecked( false );
  m_paKOfficeMode->setChecked( false );
  m_paKOfficeMode->setEnabled( false );

  m_paBottomText = new KToggleAction( i18n( "Text at the &bottom" ), 0, this );
  m_paRightText = new KToggleAction( i18n( "Text at the &right" ), 0, this );

  m_paBottomText->setExclusiveGroup( "TextPos" );
  m_paRightText->setExclusiveGroup( "TextPos" );

  m_paBottomText->setChecked( true );
  m_paRightText->setChecked( false );

  KAction * paBackgroundColor = new KAction( i18n( "Background Color..." ), 0, this, SLOT( slotBackgroundColor() ), this );
  KAction * paBackgroundImage = new KAction( i18n( "Background Image..." ), 0, this, SLOT( slotBackgroundImage() ), this );

  //
  
  connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewLarge( bool ) ) );
  connect( m_paNormalIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewNormal( bool ) ) );
  connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewSmall( bool ) ) );
  connect( m_paKOfficeMode, SIGNAL( toggled( bool ) ), this, SLOT( slotKofficeMode( bool ) ) );

  connect( m_paBottomText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextBottom( bool ) ) );
  connect( m_paRightText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextRight( bool ) ) );

  //

  actions()->append( BrowserView::ViewAction( m_paDotFiles, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_pamSort, BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( m_paBottomText, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paRightText, BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( paBackgroundColor, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( paBackgroundImage, BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( m_paLargeIcons, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paNormalIcons, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paSmallIcons, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paKOfficeMode, BrowserView::MenuView ) );


  actions()->append( BrowserView::ViewAction( m_paSelect, BrowserView::MenuEdit ) );
  actions()->append( BrowserView::ViewAction( m_paUnselect, BrowserView::MenuEdit ) );
  actions()->append( BrowserView::ViewAction( m_paSelectAll, BrowserView::MenuEdit ) );
  actions()->append( BrowserView::ViewAction( m_paUnselectAll, BrowserView::MenuEdit ) );

  QObject::connect( m_pIconView, SIGNAL( doubleClicked( QIconViewItem * ) ),
                    this, SLOT( slotReturnPressed( QIconViewItem * ) ) );
  QObject::connect( m_pIconView, SIGNAL( returnPressed( QIconViewItem * ) ),
                    this, SLOT( slotReturnPressed( QIconViewItem * ) ) );
		
  QObject::connect( m_pIconView, SIGNAL( onItem( QIconViewItem * ) ),
                    this, SLOT( slotOnItem( QIconViewItem * ) ) );
		
  QObject::connect( m_pIconView, SIGNAL( onViewport() ),
                    this, SLOT( slotOnViewport() ) );
		
  QObject::connect( m_pIconView, SIGNAL( mouseButtonPressed(int, QIconViewItem*, const QPoint&)),
                    this, SLOT( slotMouseButtonPressed(int, QIconViewItem*, const QPoint&)) );
  QObject::connect( m_pIconView, SIGNAL( viewportRightPressed() ),
                    this, SLOT( slotViewportRightClicked() ) );

  // Now we may react to configuration changes
  m_bInit = false;

  m_dirLister = 0L;
  m_bLoading = false;
  m_bNeedAlign = false;

  m_pIconView->setResizeMode( QIconView::Adjust );
  // KDE extension : KIconLoader size
  m_pIconView->setSize( KIconLoader::Medium ); // TODO : part of KonqPropsView

  m_eSortCriterion = NameCaseInsensitive;
}

KonqKfmIconView::~KonqKfmIconView()
{
  kdebug(0, 1202, "-KonqKfmIconView");
  if ( m_dirLister ) delete m_dirLister;
  delete m_pProps;
  delete m_pIconView;
}

void KonqKfmIconView::slotShowDot()
{
  kdebug(0, 1202, "KonqKfmIconView::slotShowDot()");
  m_pProps->m_bShowDot = !m_pProps->m_bShowDot;
  m_dirLister->setShowingDotFiles( m_pProps->m_bShowDot );
  //we don't want the non-dot files to remain where they are
  m_bNeedAlign = true;
}

void KonqKfmIconView::slotSelect()
{
  KLineEditDlg l( i18n("Select files:"), "", this );
  if ( l.exec() )
  {
    QString pattern = l.text();
    if ( pattern.isEmpty() )
      return;

    QRegExp re( pattern, true, true );

    QIconViewItem *it = m_pIconView->firstItem();
    while ( it )
    {
      if ( re.match( it->text() ) != -1 )
        it->setSelected( true );
      it = it->nextItem();
    }

    // Why this ? Doesn't QIconView emit it on setSelected ?
    // emit m_extension->selectionChanged();
  }
}

void KonqKfmIconView::slotUnselect()
{
  KLineEditDlg l( i18n("Unselect files:"), "", this );
  if ( l.exec() )
  {
    QString pattern = l.text();
    if ( pattern.isEmpty() )
      return;

    QRegExp re( pattern, true, true );

    QIconViewItem *it = m_pIconView->firstItem();
    while ( it )
    {
      if ( re.match( it->text() ) != -1 )
        it->setSelected( false );
      it = it->nextItem();
    }

    // Why this ? Doesn't QIconView emit it on setSelected ?
    //emit m_extension->selectionChanged();
  }
}

void KonqKfmIconView::slotSelectAll()
{
  m_pIconView->selectAll( true );
}

void KonqKfmIconView::slotUnselectAll()
{
  m_pIconView->selectAll( false );
}

void KonqKfmIconView::slotSortByNameCaseSensitive( bool toggle )
{
  if ( !toggle )
    return;

  setupSorting( NameCaseSensitive );
}

void KonqKfmIconView::slotSortByNameCaseInsensitive( bool toggle )
{
  if ( !toggle )
    return;

  setupSorting( NameCaseInsensitive );
}

void KonqKfmIconView::slotSortBySize( bool toggle )
{
  if ( !toggle )
    return;

  setupSorting( Size );
}

void KonqKfmIconView::setupSorting( SortCriterion criterion )
{
  m_eSortCriterion = criterion;

  setupSortKeys();

  m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::resizeEvent( QResizeEvent * )
{
  m_pIconView->setGeometry( 0, 0, width(), height() );
}

void KonqKfmIconView::slotSortDescending( bool toggle )
{
  if ( !toggle )
    return;

  if ( m_pIconView->sortDirection() )
    m_pIconView->setSorting( true, false );
  else
    m_pIconView->setSorting( true, true );

  m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::slotKofficeMode( bool b )
{
  if ( b )
  {
    QObject *obj = parent();
    while ( obj )
    {
      if ( obj->inherits( "KonqFrame" ) )
        break;
      obj = obj->parent();
    }

    if ( obj && obj->inherits( "KonqFrame" ) )
    {
      KonqChildView *childView = ((KonqFrame *)obj)->childView();
      // TODO switch to koffice view mode here
      childView->changeViewMode( "inode/directory", url(), false, "KonqTreeView" );
    }
  }
}

void KonqKfmIconView::slotViewLarge( bool b )
{
    if ( b )
        m_pIconView->setSize( KIconLoader::Large );
}

void KonqKfmIconView::slotViewNormal( bool b )
{
    if ( b )
        m_pIconView->setSize( KIconLoader::Medium );
}

void KonqKfmIconView::slotViewSmall( bool b )
{
    if ( b )
        m_pIconView->setSize( KIconLoader::Small );
}

void KonqKfmIconView::slotTextBottom( bool b )
{
    if ( b ) {
	m_pIconView->setGridX( 70 );
	m_pIconView->setItemTextPos( QIconView::Bottom );
    }
}

void KonqKfmIconView::slotTextRight( bool b )
{
    if ( b ) {
	m_pIconView->setGridX( 120 );
	m_pIconView->setItemTextPos( QIconView::Right );
    }
}

void KonqKfmIconView::slotBackgroundColor()
{
  QColor bgndColor;
  if ( KColorDialog::getColor( bgndColor ) == KColorDialog::Accepted )
  {
    m_pProps->m_bgColor = bgndColor;
    m_pProps->m_bgPixmap = QPixmap();
    m_pIconView->viewport()->setBackgroundColor( m_pProps->m_bgColor );
    m_pIconView->viewport()->setBackgroundPixmap( m_pProps->m_bgPixmap );
    m_pProps->saveLocal( m_dirLister->url() );
    m_pIconView->updateContents();
  }
}

void KonqKfmIconView::slotBackgroundImage()
{
  KonqBgndDialog dlg( m_dirLister->url() );
  if ( dlg.exec() == KonqBgndDialog::Accepted )
  {
    m_pProps->m_bgPixmap = dlg.pixmap();
    m_pIconView->viewport()->setBackgroundColor( m_pProps->m_bgColor );
    m_pIconView->viewport()->setBackgroundPixmap( m_pProps->m_bgPixmap );
    // no need to savelocal, the dialog does it
    m_pIconView->updateContents();
  }
}

void KonqKfmIconView::stop()
{
  debug("KonqKfmIconView::stop()");
  if ( m_dirLister ) m_dirLister->stop();
}

void KonqKfmIconView::saveState( QDataStream &stream )
{
  BrowserView::saveState( stream );

  stream << (Q_INT32)m_pIconView->size() << (Q_INT32)m_pIconView->itemTextPos();
}

void KonqKfmIconView::restoreState( QDataStream &stream )
{
  BrowserView::restoreState( stream );

  Q_INT32 iIconSize, iTextPos;

  stream >> iIconSize >> iTextPos;

  KIconLoader::Size iconSize = (KIconLoader::Size)iIconSize;
  QIconView::ItemTextPos textPos = (QIconView::ItemTextPos)iTextPos;

  switch ( iconSize )
  {
    case KIconLoader::Large: m_paLargeIcons->setChecked( true ); break;
    case KIconLoader::Medium: m_paNormalIcons->setChecked( true ); break;
    case KIconLoader::Small: m_paSmallIcons->setChecked( true ); break;
  }

  if ( textPos == QIconView::Bottom )
    m_paBottomText->setChecked( true );
  else
    m_paRightText->setChecked( true );
}

QString KonqKfmIconView::url()
{
  return m_dirLister ? m_dirLister->url() : QString();
}

int KonqKfmIconView::xOffset()
{
  return m_pIconView->contentsX();
}

int KonqKfmIconView::yOffset()
{
  return m_pIconView->contentsY();
}

void KonqKfmIconView::slotReturnPressed( QIconViewItem *item )
{
  KFileItem *fileItem = ((KFileIVI*)item)->item();
  emit openURLRequest( fileItem->url().url(), false, 0, 0 );
}

void KonqKfmIconView::slotMouseButtonPressed(int _button, QIconViewItem* _item, const QPoint& _global)
{
  if(_item) {
    switch(_button) {
      case RightButton:
        ((KFileIVI*)_item)->setSelected( true );
        emit popupMenu( _global, m_pIconView->selectedFileItems() );
        break;
      case MidButton:
        // New view
        ((KFileIVI*)_item)->item()->run();
        break;
    }
  }
}

void KonqKfmIconView::slotViewportRightClicked()
{
  KURL bgUrl( m_dirLister->url() );

  // This is a directory. Always.
  mode_t mode = S_IFDIR;

  KFileItem item( mode, bgUrl );
  KFileItemList items;
  items.append( &item );
  emit popupMenu( QCursor::pos(), items );
}

void KonqKfmIconView::slotStarted( const QString & /*url*/ )
{
  m_pIconView->selectAll( false );
  if ( m_bLoading )
    emit started();
}

void KonqKfmIconView::slotCompleted()
{
  if ( m_bLoading )
  {
    emit completed();
    m_bLoading = false;
  }
  m_pIconView->setContentsPos( m_iXOffset, m_iYOffset );
  m_paKOfficeMode->setEnabled( m_dirLister->kofficeDocsFound() );

  if ( m_bNeedAlign )
  {
    m_pIconView->alignItemsInGrid();
  }
}

void KonqKfmIconView::slotNewItem( KFileItem * _fileitem )
{
//  kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotNewItem(...)");
  KFileIVI* item = new KFileIVI( m_pIconView, _fileitem, m_pIconView->size() );
  item->setRenameEnabled( false );

  QObject::connect( item, SIGNAL( dropMe( KFileIVI *, QDropEvent * ) ),
                    m_pIconView, SLOT( slotDropItem( KFileIVI *, QDropEvent * ) ) );

  QString key;

  switch ( m_eSortCriterion )
  {
    case NameCaseSensitive: key = item->text(); break;
    case NameCaseInsensitive: key = item->text().lower(); break;
    case Size: key = makeSizeKey( item ); break;
  }

  item->setKey( key );

  if ( m_ulTotalFiles > 0 )
    emit loadingProgress( ( m_pIconView->count() * 100 ) / m_ulTotalFiles );
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
  //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotDeleteItem(...)");
  // we need to find out the iconcontainer item containing the fileitem
  QIconViewItem *it = m_pIconView->firstItem();
  while ( it )
  {
    if ( ((KFileIVI*)it)->item() == _fileitem ) // compare the pointers
    {
      m_pIconView->removeItem( it );
      break;
    }
    it = it->nextItem();
  }
}

void KonqKfmIconView::slotClear()
{
  m_pIconView->clear();
}

void KonqKfmIconView::slotTotalFiles( int, unsigned long files )
{
  m_ulTotalFiles = files;
}

void KonqKfmIconView::openURL( const QString &_url, bool /*reload*/, int xOffset, int yOffset )
{
  if ( !m_dirLister )
  {
    // Create the directory lister
    m_dirLister = new KDirLister();

    QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ),
                      this, SLOT( slotStarted( const QString & ) ) );
    QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
    QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SIGNAL( canceled() ) );
    QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
    QObject::connect( m_dirLister, SIGNAL( newItem( KFileItem * ) ),
                      this, SLOT( slotNewItem( KFileItem * ) ) );
    QObject::connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
                      this, SLOT( slotDeleteItem( KFileItem * ) ) );
  }

  m_iXOffset = xOffset;
  m_iYOffset = yOffset;
  m_bLoading = true;

  KURL u( _url );
  // Start the directory lister !
  m_dirLister->openURL( u, m_pProps->m_bShowDot );
  // Note : we don't store the url. KDirLister does it for us.

  m_pIconView->setURL( _url );

  KIOJob *job = KIOJob::find( m_dirLister->jobId() );
  if ( job )
  {
    connect( job, SIGNAL( sigTotalFiles( int, unsigned long ) ),
             this, SLOT( slotTotalFiles( int, unsigned long ) ) );
  }

  m_ulTotalFiles = 0;
  m_bNeedAlign = false;

  // do it after starting the dir lister to avoid changing bgcolor of the
  // old view
  if ( m_pProps->enterDir( u ) )
  {
    m_pIconView->viewport()->setBackgroundColor( m_pProps->m_bgColor );
    m_pIconView->viewport()->setBackgroundPixmap( m_pProps->m_bgPixmap );
  }

#warning FIXME (Simon)
//  setCaptionFromURL( _url );
  m_pIconView->show(); // ?
}

void KonqKfmIconView::slotOnItem( QIconViewItem *item )
{
  emit setStatusBarText( ((KFileIVI *)item)->item()->getStatusBarInfo() );
}

void KonqKfmIconView::slotOnViewport()
{
  emit setStatusBarText( QString::null );
}

void KonqKfmIconView::setupSortKeys()
{

  switch ( m_eSortCriterion )
  {
    case NameCaseSensitive:
         for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
           it->setKey( it->text() );
         break;
    case NameCaseInsensitive:
         for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
           it->setKey( it->text().lower() );
         break;
    case Size:
         for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
           it->setKey( makeSizeKey( (KFileIVI *)it ) );
         break;
  }
}

QString KonqKfmIconView::makeSizeKey( KFileIVI *item )
{
  return QString::number( item->item()->size() ).rightJustify( 20, '0' );
}

#include "konq_iconview.moc"
