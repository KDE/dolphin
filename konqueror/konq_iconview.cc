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
#include "konq_propsview.h"
#include "konq_mainview.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kaccel.h>
#include <kapp.h>
#include <kcursor.h>
#include <kdirlister.h>
#include <kfileivi.h>
#include <kfileitem.h>
#include <kio_error.h>
#include <kio_job.h>
#include <kio_paste.h>
#include <klineeditdlg.h>
#include <kmimetypes.h>
#include <kpixmapcache.h>
#include <krun.h>
#include <kurl.h>
#include <konqsettings.h>

#include <qmsgbox.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <qdragobject.h>
#include <klocale.h>
#include <qclipboard.h>
#include <qregexp.h>

#include <opQCProxy.h>
#include <opUIUtils.h>

KonqDragItem::KonqDragItem()
    : QIconDragItem()
{
    makeKey();
}

KonqDragItem::KonqDragItem( const QRect &ir, const QRect &tr, const QString &u )
    : QIconDragItem( ir, tr ), url_( u )
{
    makeKey();
}

KonqDragItem::~KonqDragItem()
{
}

QString KonqDragItem::url() const
{
    return url_;
}

void KonqDragItem::setURL( const QString &u )
{
    url_ = u;
}

void KonqDragItem::makeKey()
{	
    QString k( "%1 %2 %3 %4 %5 %6 %7 %8 %9");
    k = k.arg( iconRect().x() ).arg( iconRect().y() ).arg( iconRect().width() ).
	arg( iconRect().height() ).arg( textRect().x() ).arg( textRect().y() ).
	arg( textRect().width() ).arg( textRect().height() ).arg( url_ );
    key_ = k;
}

KonqDrag::KonqDrag( QWidget * dragSource, const char* name )
    : QIconDrag( dragSource, name )
{
}

KonqDrag::~KonqDrag()
{
}

const char* KonqDrag::format( int i ) const
{
    if ( i == 0 )
	return "application/x-qiconlist";
    else if ( i == 1 )
	return "text/uri-iconlist";
    else if ( i == 2 )
	return "text/uri-list";
    else return 0;
}

void KonqDrag::append( const KonqDragItem &icon_ )
{
    icons.append( icon_ );
    QIconDrag::icons.append( icon_ );
}

QByteArray KonqDrag::encodedData( const char* mime ) const
{
    QByteArray a;
    if ( QString( mime ) == "application/x-qiconlist" )
	a = QIconDrag::encodedData( mime );
    else if ( QString( mime ) == "text/uri-iconlist" ) {
	int c = 0;
	KonqList::ConstIterator it = icons.begin();
	for ( ; it != icons.end(); ++it ) {
	    QString k( "%1 %2 %3 %4 %5 %6 %7 %8 %9" );
	    k = k.arg( (*it).iconRect().x() ).arg( (*it).iconRect().y() ).arg( (*it).iconRect().width() ).
		arg( (*it).iconRect().height() ).arg( (*it).textRect().x() ).arg( (*it).textRect().y() ).
		arg( (*it).textRect().width() ).arg( (*it).textRect().height() ).arg( (*it).url() );
	    int l = k.length();
	    a.resize(c + l + 1 );
	    memcpy( a.data() + c , k.latin1(), l );
	    a[ c + l ] = 0;
	    c += l + 1;
	}
	a.resize( c - 1 );
    } else if ( QString( mime ) == "text/uri-list" ) {
	int c = 0;
	KonqList::ConstIterator it = icons.begin();
	for ( ; it != icons.end(); ++it ) {
	    QString k( "%1" );
	    k = k.arg( (*it).url() );
	    int l = k.length();
	    a.resize(c + l + 2 );
	    memcpy( a.data() + c , k.latin1(), l );
	    memcpy(a.data() + c + l, "\r\n" ,2);
	    c += l + 2;
	}
	a.resize( c - 1 );
    }

    return a;
}

bool KonqDrag::canDecode( QMimeSource* e )
{
    return e->provides( "text/uri-iconlist" ) ||
	e->provides( "text/uri-list" );
}

bool KonqDrag::decode( QMimeSource* e, QValueList<KonqDragItem> &list_ )
{
    QByteArray ba = e->encodedData( "text/uri-iconlist" );
    if ( ba.size() ) {
	list_.clear();
	uint c = 0;
	
	char* d = ba.data();
	
	while ( c < ba.size() ) {
	    uint f = c;
	    while ( c < ba.size() && d[ c ] )
		c++;
	    QString s;
	    if ( c < ba.size() ) {
		s = d + f ;
		c++;
	    } else  {
		QString tmp( QString(d + f).left( c - f + 1 ) );
		s = tmp;
	    }

	    KonqDragItem icon;
	    QRect ir, tr;
	
	    ir.setX( atoi( s.latin1() ) );
	    int pos = s.find( ' ' );
	    if ( pos == -1 )
		return FALSE;
	    ir.setY( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    ir.setWidth( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    ir.setHeight( atoi( s.latin1() + pos + 1 ) );

	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setX( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setY( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setWidth( atoi( s.latin1() + pos + 1 ) );
	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    tr.setHeight( atoi( s.latin1() + pos + 1 ) );

	    pos = s.find( ' ', pos + 1 );
	    if ( pos == -1 )
		return FALSE;
	    icon.setIconRect( ir );
	    icon.setTextRect( tr );
	    icon.setURL( s.latin1() + pos + 1 );
	    list_.append( icon );
	}
	return TRUE;
    }

    return FALSE;
}

bool KonqDrag::decode( QMimeSource *e, QStringList &uris )
{
    QByteArray ba = e->encodedData( "text/uri-list" );
    if ( ba.size() ) {
	uris.clear();
	uint c = 0;
	char* d = ba.data();
	while ( c < ba.size() ) {
	    uint f = c;
	    while ( c < ba.size() && d[ c ] )
		c++;
	    if ( c < ba.size() ) {
		uris.append( d + f );
		c++;
	    } else {
		QString s( QString(d + f).left(c - f + 1) );
		uris.append( s );
	    }
	}
	return TRUE;
    }
    return FALSE;
}

KonqKfmIconView::KonqKfmIconView( KonqMainView *mainView )
{
  kdebug(0, 1202, "+KonqKfmIconView");
  ADD_INTERFACE( "IDL:Konqueror/KfmIconView:1.0" );
  ADD_INTERFACE( "IDL:Browser/EditExtension:1.0" );

  SIGNAL_IMPL( "selectionChanged" ); //part of the EditExtension

  m_pMainView = mainView;
  m_vViewMenu = 0L;
  m_vSortMenu = 0L;
  m_proxySelectAll = 0L;

  // Create a properties instance for this view
  // (copying the default values)
  m_pProps = new KonqPropsView( * KonqPropsView::defaultProps() );
  m_pSettings = KonqSettings::defaultFMSettings();

  // Dont repaint on configuration changes during construction
  m_bInit = true;

  setWidget( this );

  QWidget::setFocusPolicy( StrongFocus );
  viewport()->setFocusPolicy( StrongFocus );

  initConfig();

  QObject::connect( this, SIGNAL( doubleClicked( QIconViewItem * ) ),
                    this, SLOT( slotMousePressed( QIconViewItem * ) ) );
		    
  QObject::connect( this, SIGNAL( dropped( QDropEvent * ) ),
	            this, SLOT( slotDrop( QDropEvent* ) ) );
	   
  QObject::connect( this, SIGNAL( onItem( QIconViewItem * ) ), 
                    this, SLOT( slotOnItem( QIconViewItem * ) ) );
		    
  QObject::connect( this, SIGNAL( onViewport() ), 
                    this, SLOT( slotOnViewport() ) );
		    
  QObject::connect( this, SIGNAL( selectionChanged() ), 
                    this, SLOT( slotSelectionChanged() ) );
		    
  // TODO
  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  QObject::connect( this, SIGNAL( itemRightPressed( QIconViewItem * ) ),
                    this, SLOT( slotItemRightClicked( QIconViewItem * ) ) );
  QObject::connect( this, SIGNAL( viewportRightPressed() ),
                    this, SLOT( slotViewportRightClicked() ) );

  // Now we may react to configuration changes
  m_bInit = false;

  m_dirLister = 0L;
  m_bLoading = false;
  
  setSelectionMode( QIconView::Multi );
  QIconView::setViewMode( QIconSet::Large );
  setResizeMode( Adjust );
  setGridX( 70 );
  setGridY( 70 );
  setReorderItemsWhenInsert( true );
  setResortItemsWhenInsert( true, sortOrder() );
  
  m_eSortCriterion = NameCaseInsensitive;
}

KonqKfmIconView::~KonqKfmIconView()
{
  kdebug(0, 1202, "-KonqKfmIconView");
  if ( m_dirLister ) delete m_dirLister;
  delete m_pProps;
}

bool KonqKfmIconView::mappingOpenURL( Browser::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL(eventURL);
  openURL( eventURL.url, eventURL.xOffset, eventURL.yOffset );
  return true;
}

bool KonqKfmIconView::mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu )
{
  m_vViewMenu = OpenPartsUI::Menu::_duplicate( viewMenu );
  
  if ( !CORBA::is_nil( viewMenu ) )
  {
//    viewMenu->insertItem4( i18n("Image &Preview"), this, "slotShowSchnauzer" , 0, -1, -1 );
    m_idShowDotFiles = viewMenu->insertItem4( i18n("Show &Dot Files"), this, "slotShowDot" , 0, -1, -1 );
    viewMenu->setItemChecked( m_idShowDotFiles, m_pProps->m_bShowDot );
    
    viewMenu->insertItem8( i18n( "Sort..." ), m_vSortMenu, -1, -1 );
    
    m_vSortMenu->setCheckable( true );

    m_idSortByNameCaseSensitive = m_vSortMenu->insertItem4( i18n( "by Name ( Case Sensitive )" ), this, 
							    "slotSortByNameCaseSensitive", 0, -1, -1 );

    m_idSortByNameCaseInsensitive = m_vSortMenu->insertItem4( i18n( "by Name ( Case Insensitive )" ), this,
							      "slotSortByNameCaseInsensitive", 0, -1, -1 );
    
    m_idSortBySize = m_vSortMenu->insertItem4( i18n( "by Size" ), this, "slotSortBySize", 0, -1, -1 );

    m_vSortMenu->insertSeparator( -1 );
    
    m_idSortDescending = m_vSortMenu->insertItem4( i18n( "Descending" ), this, 
						   "slotSetSortDirectionDescending", 0, -1, -1 );
    m_vSortMenu->setItemChecked( m_idSortDescending, !sortOrder() );
    
    setupSortMenu();
  }
  else
    m_vSortMenu = 0L;
  
  return true;
}

bool KonqKfmIconView::mappingFillMenuEdit( Browser::View::EventFillMenu_ptr editMenu )
{
  if ( !CORBA::is_nil( editMenu ) )
  {
    QString text;
    text = i18n("&Select");
    editMenu->insertItem4( text, this, "slotSelect" , 0, -1, -1 );
    text = i18n("&Unselect");
    editMenu->insertItem4( text, this, "slotUnselect" , 0, -1, -1 );
    text = i18n("Select &All");
    editMenu->insertItem4( text, this, "slotSelectAll" , 0, -1, -1 );
    text = i18n("U&nselect All");
    editMenu->insertItem4( text, this, "slotUnselectAll" , 0, -1, -1 );
  }
  
  return true;
}

void KonqKfmIconView::setFocus( bool mode )
{
  if ( m_bFocus != mode )
  {
    if ( mode ) // We just got the focus
    {
      m_proxySelectAll = new Qt2CORBAProxy( this, "slotSelectAll" );
      m_pMainView->accel()->connectItem( "SelectAll", m_proxySelectAll, SLOT( callback() ) );
    }
    else // Losing the focus
    {
      if ( m_proxySelectAll ) {
        m_pMainView->accel()->disconnectItem( "SelectAll", m_proxySelectAll, SLOT( callback() ) );
        delete m_proxySelectAll;
        m_proxySelectAll = 0L;
      }
    }
  }
  
  OPPartIf::setFocus( mode );
}

void KonqKfmIconView::slotShowDot()
{
  kdebug(0, 1202, "KonqKfmIconView::slotShowDot()");
  m_pProps->m_bShowDot = !m_pProps->m_bShowDot;
  m_dirLister->setShowingDotFiles( m_pProps->m_bShowDot );
  //bSetupNeeded = true; // we don't want the non-dot files to remain where they are !

  m_vViewMenu->setItemChecked( m_idShowDotFiles, m_pProps->m_bShowDot );
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
    
    QIconViewItem *it = firstItem();
    while ( it )
    {
      if ( re.match( it->text() ) != -1 )
        it->setSelected( true );
      it = it->nextItem();
    }

    emit selectionChanged();
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

    QIconViewItem *it = firstItem();
    while ( it )
    {
      if ( re.match( it->text() ) != -1 )
        it->setSelected( false );
      it = it->nextItem();
    }
    
    emit selectionChanged();
  }
}

void KonqKfmIconView::slotSelectAll()
{
  selectAll( true );
}

void KonqKfmIconView::slotUnselectAll()
{
  selectAll( false );
}

void KonqKfmIconView::slotSortByNameCaseSensitive()
{
  setupSorting( NameCaseSensitive );
}

void KonqKfmIconView::slotSortByNameCaseInsensitive()
{
  setupSorting( NameCaseInsensitive );
}

void KonqKfmIconView::slotSortBySize()
{
  setupSorting( Size );
}

void KonqKfmIconView::setupSorting( SortCriterion criterion )
{
  m_eSortCriterion = criterion;
  
  setupSortKeys();
  
  sortItems( sortOrder() );
  
  setupSortMenu();
}

void KonqKfmIconView::slotSetSortDirectionDescending()
{
  if ( sortOrder() )
    setResortItemsWhenInsert( true, false );
  else
    setResortItemsWhenInsert( true, true );

  m_vSortMenu->setItemChecked( m_idSortDescending, !sortOrder() );
  
  sortItems( sortOrder() );
}

void KonqKfmIconView::setViewMode( Konqueror::DirectoryDisplayMode mode )
{

  switch ( mode )
  {
    case Konqueror::LargeIcons:
      QIconView::setViewMode( QIconSet::Large );
      setItemTextPos( QIconView::Bottom );
      break;
    case Konqueror::SmallIcons:
      QIconView::setViewMode( QIconSet::Small );
      setItemTextPos( QIconView::Bottom );
      break;
    case Konqueror::SmallVerticalIcons:
      QIconView::setViewMode( QIconSet::Small );
      setItemTextPos( QIconView::Right );
      break;
    default: assert( 0 );
  }
}

Konqueror::DirectoryDisplayMode KonqKfmIconView::viewMode()
{
  if ( QIconView::viewMode() == QIconSet::Large )
    return Konqueror::LargeIcons;
  else if ( itemTextPos() == QIconView::Bottom )
    return Konqueror::SmallIcons;
  else
    return Konqueror::SmallVerticalIcons;
}

void KonqKfmIconView::stop()
{
  m_dirLister->stop();
}

QCString KonqKfmIconView::url()
{
  QCString url;

  if ( m_dirLister )
    url = m_dirLister->url().ascii();

  return url;
}

long int KonqKfmIconView::xOffset()
{
  return contentsX();
}

long int KonqKfmIconView::yOffset()
{
  return contentsY();
}

void KonqKfmIconView::dropStuff( QDropEvent *ev, KFileIVI *item )
{
  QStringList lst;
  
  QStringList formats;
  
  for ( int i = 0; ev->format( i ); i++ )
    if ( *( ev->format( i ) ) )
      formats.append( ev->format( i ) );

  // Try to decode to the data you understand...
  if ( QUrlDrag::decodeToUnicodeUris( ev, lst ) )
  {
    if( lst.count() == 0 )
    {
      kdebug(KDEBUG_WARN,1202,"Oooops, no data ....");
      return;
    }
    KIOJob* job = new KIOJob;

    // Use either the root url or the item url (we stored it as the icon "name")
    KURL dest( ( item == 0L ) ? m_dirLister->url() : item->item()->url().url() );

    job->copy( lst, dest.url( 1 ) );
  }
  else if ( formats.count() >= 1 )
  {
    if ( item == 0L )
      pasteData( m_dirLister->url(), ev->data( formats.first() ) );
    else
    {
      kdebug(0,1202,"Pasting to %s", item->item()->url().url().ascii() /* item's url */);
      pasteData( item->item()->url().url()/* item's url */, ev->data( formats.first() ) );
    }
  }
}

void KonqKfmIconView::initConfig()
{
  // Color settings
  QColor bgColor           = m_pSettings->bgColor();
  QColor textColor         = m_pSettings->textColor();
  QColor linkColor         = m_pSettings->linkColor();
  QColor vLinkColor        = m_pSettings->vLinkColor();

  // Font settings
  QString stdFontName      = m_pSettings->stdFontName();
  QString fixedFontName    = m_pSettings->fixedFontName();
  int fontSize             = m_pSettings->fontSize();

  /*
  //  m_bgPixmap          = props->bgPixmap(); // !!

  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );
  */

  // bool bUnderlineLink = m_pSettings->underlineLink();

  QPalette p    = palette();
  QColorGroup c = p.normal();
  QColorGroup n( textColor, bgColor, c.light(), c.dark(), c.mid(),
		 textColor, bgColor );
  p.setNormal( n );
  c = p.active();
  QColorGroup a( textColor, bgColor, c.light(), c.dark(), c.mid(),
		 textColor, bgColor );
  p.setActive( a );
  c = p.disabled();
  QColorGroup d( textColor, bgColor, c.light(), c.dark(), c.mid(),
		 textColor, bgColor );
  p.setDisabled( d );
  setPalette( p );

  QFont font( stdFontName, fontSize );
  setFont( font );

  // Behaviour
  bool bChangeCursor = m_pSettings->changeCursor();
  QIconView::setSingleClickConfiguration( new QFont(font), new QColor(textColor), new QFont(font), new QColor(linkColor), 
                    new QCursor(bChangeCursor ? KCursor().handCursor() : KCursor().arrowCursor()), 
                    m_pSettings->autoSelect() );
  QIconView::setUseSingleClickMode( m_pSettings->singleClick() );

  // probably not necessary with QIconView
  //if ( !m_bInit )
    //refresh();
}

void KonqKfmIconView::slotMousePressed( QIconViewItem *item )
{
  KFileItem *fileItem = ((KFileIVI*)item)->item();
  openURLRequest( fileItem->url().url().ascii() );
}

void KonqKfmIconView::slotDrop( QDropEvent *e )
{
  dropStuff( e );
}

void KonqKfmIconView::slotDropItem( KFileIVI *item, QDropEvent *e )
{
  dropStuff( e, item );
}

void KonqKfmIconView::slotSelectionChanged()
{
  SIGNAL_CALL0( "selectionChanged" );
}

void KonqKfmIconView::slotItemRightClicked( QIconViewItem *item )
{
  if ( !m_pMainView )
    return;

  item->setSelected( true );

  KFileItemList lstItems;

  QIconViewItem *it = firstItem();
  for (; it; it = it->nextItem() )
    if ( it->isSelected() )
    {
      KFileItem *fItem = ((KFileIVI *)it)->item();
      lstItems.append( fItem );
    }

  m_pMainView->popupMenu( QCursor::pos(), lstItems );
}

void KonqKfmIconView::slotViewportRightClicked()
{
  if ( !m_pMainView )
    return;

  KURL bgUrl( m_dirLister->url() );

  // This is a directory. Always.
  mode_t mode = S_IFDIR;

  KFileItem item( "viewURL" /*whatever*/, mode, bgUrl );
  KFileItemList items;
  items.append( &item );
  m_pMainView->popupMenu( QCursor::pos(), items );
}

void KonqKfmIconView::slotStarted( const QString & url )
{
  selectAll( false );
  if ( m_bLoading )
    SIGNAL_CALL2( "started", id(), url.ascii() );
  //bSetupNeeded = false;
}

void KonqKfmIconView::slotCompleted()
{
  if ( m_bLoading )
  {
    SIGNAL_CALL1( "completed", id() );
    m_bLoading = false;
  }    
  setContentsPos( m_iXOffset, m_iYOffset );
}

void KonqKfmIconView::slotCanceled()
{
  SIGNAL_CALL1( "canceled", id() );
}

void KonqKfmIconView::slotClear()
{
  //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotClear()");
  clear();
}

void KonqKfmIconView::slotNewItem( KFileItem * _fileitem )
{
//  kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotNewItem(...)");
  KFileIVI* item = new KFileIVI( this, _fileitem );
  item->setRenameEnabled( false );
  
  QObject::connect( item, SIGNAL( dropMe( KFileIVI *, QDropEvent * ) ),
                    this, SLOT( slotDropItem( KFileIVI *, QDropEvent * ) ) );

  QString key;

  switch ( m_eSortCriterion )
  {
    case NameCaseSensitive: key = item->text(); break;
    case NameCaseInsensitive: key = item->text().lower(); break;
    case Size: key = makeSizeKey( item ); break;
  }

  item->setKey( key );

  //bSetupNeeded = true;
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
  //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotDeleteItem(...)");
  // we need to find out the iconcontainer item containing the fileitem
  QIconViewItem *it = firstItem();
  while ( it )
  {
    if ( ((KFileIVI*)it)->item() == _fileitem ) // compare the pointers
    {
      delete it;
      break;
    }
    it = it->nextItem();
  }    
}

void KonqKfmIconView::openURL( const char *_url, int xOffset, int yOffset )
{
  if ( !m_dirLister )
  {
    // Create the directory lister
    m_dirLister = new KDirLister();

    QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ), 
                      this, SLOT( slotStarted( const QString & ) ) );
    QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
    QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
    QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
    QObject::connect( m_dirLister, SIGNAL( newItem( KFileItem * ) ), 
                      this, SLOT( slotNewItem( KFileItem * ) ) );
    QObject::connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ), 
                      this, SLOT( slotDeleteItem( KFileItem * ) ) );
  }

  m_iXOffset = xOffset;
  m_iYOffset = yOffset;
  m_bLoading = true;

  // Start the directory lister !
  m_dirLister->openURL( KURL( _url ), m_pProps->m_bShowDot );
  // Note : we don't store the url. KDirLister does it for us.

  setCaptionFromURL( _url );
  QIconView::show();
}

void KonqKfmIconView::can( bool &copy, bool &paste, bool &move )
{
  bool bItemSelected = false;
  
  for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
    if ( it->isSelected() )
    {
      bItemSelected = true;
      break;
    }

  move = copy = bItemSelected;

  bool bKIOClipboard = !isClipboardEmpty();
  
  QMimeSource *data = QApplication::clipboard()->data();
  
  paste = ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 );
}

void KonqKfmIconView::copySelection()
{
  QDragObject * obj = dragObject();
  QApplication::clipboard()->setData( obj );
}

QDragObject * KonqKfmIconView::dragObject()
{
    if ( !currentItem() )
	return 0;

    QPoint orig = viewportToContents( viewport()->mapFromGlobal( QCursor::pos() ) );
    KonqDrag *drag = new KonqDrag( viewport() );
    drag->setPixmap( QPixmap( currentItem()->icon().pixmap( QIconView::viewMode(), QIconSet::Normal ) ),
		     QPoint( currentItem()->iconRect().width() / 2,
			     currentItem()->iconRect().height() / 2 ) ); 
    for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() ) {
	if ( it->isSelected() ) {
	    drag->append( KonqDragItem( QRect( it->iconRect( FALSE ).x() - orig.x(),
					       it->iconRect( FALSE ).y() - orig.y(),
					       it->iconRect().width(), it->iconRect().height() ),
					QRect( it->textRect( FALSE ).x() - orig.x(),
					       it->textRect( FALSE ).y() - orig.y(), 	
					       it->textRect().width(), it->textRect().height() ),
					((KFileIVI *)it)->item()->url().url() ) );
	}
    }
    return drag;

QStringList lstURLs;
 
//   for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
//     if ( it->isSelected() )
//     {
//       lstURLs.append( ((KFileIVI *)it)->item()->url().url() );
//       debug("appended %s",((KFileIVI *)it)->item()->url().url().ascii());
//     }
  
//   QUriDrag *drag = new QUriDrag( viewport() );
//   drag->setUnicodeUris( lstURLs );
//   drag->setPixmap( QPixmap( currentItem()->icon().pixmap( QIconView::viewMode(), QIconSet::Normal ) ),
//                    QPoint( currentItem()->iconRect().width() / 2,
//                            currentItem()->iconRect().height() / 2 ) ); 
//   return drag;
}

void KonqKfmIconView::pasteSelection()
{
  pasteClipboard( m_dirLister->url() );
}

void KonqKfmIconView::moveSelection( const QCString &destinationURL )
{
  QStringList lstURLs;
 
  for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
    if ( it->isSelected() )
      lstURLs.append( ( (KFileIVI *)it )->item()->url().url() );
  
  KIOJob *job = new KIOJob;
  
  if ( !destinationURL.isEmpty() )
    job->move( lstURLs, destinationURL );
  else
    job->del( lstURLs );
}

void KonqKfmIconView::slotOnItem( QIconViewItem *item )
{
  SIGNAL_CALL1( "setStatusBarText", ((KFileIVI *)item)->item()->getStatusBarInfo() );
}

void KonqKfmIconView::slotOnViewport()
{
  SIGNAL_CALL1( "setStatusBarText", QString::null );
}

void KonqKfmIconView::setupSortMenu()
{

  switch ( m_eSortCriterion )
  {
    case NameCaseSensitive:
      m_vSortMenu->setItemChecked( m_idSortByNameCaseSensitive, true );
      m_vSortMenu->setItemChecked( m_idSortByNameCaseInsensitive, false );
      m_vSortMenu->setItemChecked( m_idSortBySize, false );
      break;
    case NameCaseInsensitive:
      m_vSortMenu->setItemChecked( m_idSortByNameCaseSensitive, false );
      m_vSortMenu->setItemChecked( m_idSortByNameCaseInsensitive, true );
      m_vSortMenu->setItemChecked( m_idSortBySize, false );
      break;
    case Size:
      m_vSortMenu->setItemChecked( m_idSortByNameCaseSensitive, false );
      m_vSortMenu->setItemChecked( m_idSortByNameCaseInsensitive, false );
      m_vSortMenu->setItemChecked( m_idSortBySize, true );
      break;
  }

}

void KonqKfmIconView::setupSortKeys()
{

  switch ( m_eSortCriterion )
  {
    case NameCaseSensitive:
         for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
           it->setKey( it->text() );
         break;
    case NameCaseInsensitive: 
         for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
           it->setKey( it->text().lower() );
         break;
    case Size:
         for ( QIconViewItem *it = firstItem(); it; it = it->nextItem() )
           it->setKey( makeSizeKey( (KFileIVI *)it ) );
         break;
  }
}

QString KonqKfmIconView::makeSizeKey( KFileIVI *item )
{
  return QString::number( item->item()->size() ).rightJustify( 20, '0' );
}


void KonqKfmIconView::initDrag( QDropEvent *e )
{
    if ( KonqDrag::canDecode( e ) ) {	
	QValueList<KonqDragItem> lst;
	KonqDrag::decode( e, lst );
	if ( lst.count() != 0 ) {
	    setDragObjectIsKnown( TRUE );
	    QValueList<QIconDragItem> lst2;
	    for ( QValueList<KonqDragItem>::Iterator it = lst.begin();
		  it != lst.end(); ++it )
		lst2.append( *it );
	    setIconDragData( lst2 );
	} else {
	    QStringList l;
	    KonqDrag::decode( e, l );
	    setNumDragItems( l.count() );
	    setDragObjectIsKnown( FALSE );
	}
    } else if ( QUriDrag::canDecode( e ) ) {
	QStringList l;
	QUriDrag::decodeLocalFiles( e, l );
	setNumDragItems( l.count() );
	setDragObjectIsKnown( FALSE );
    } else {
	QIconView::initDrag( e );
    }
}

#include "konq_iconview.moc"
