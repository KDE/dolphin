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

#include "konq_iconview.h"
#include "konq_propsview.h"
#include "konq_mainview.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kapp.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kio_error.h>
#include <kio_job.h>
#include <kio_paste.h>
#include <klineeditdlg.h>
#include <kmimetypes.h>
#include <kpixmapcache.h>
#include <krun.h>
#include <kurl.h>

#include <qmsgbox.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <qdragobject.h>
#include <klocale.h>

#include <opUIUtils.h>

KonqKfmIconView::KonqKfmIconView( KonqMainView *mainView )
{
  kdebug(0, 1202, "+KonqKfmIconView");
  ADD_INTERFACE( "IDL:Konqueror/KfmIconView:1.0" );

  m_pMainView = mainView;
  
  // Dont repaint on configuration changes during construction
  m_bInit = true;

  setWidget( this );

  QWidget::setFocusPolicy( StrongFocus );
  viewport()->setFocusPolicy( StrongFocus );

  initConfig();

  QObject::connect( this, SIGNAL( mousePressed( KIconContainerItem*, const QPoint&, int ) ),
	   this, SLOT( slotMousePressed( KIconContainerItem*, const QPoint&, int ) ) );
  QObject::connect( this, SIGNAL( doubleClicked( KIconContainerItem*, const QPoint&, int ) ),
	   this, SLOT( slotDoubleClicked( KIconContainerItem*, const QPoint&, int ) ) );
  QObject::connect( this, SIGNAL( returnPressed( KIconContainerItem*, const QPoint& ) ),
	   this, SLOT( slotReturnPressed( KIconContainerItem*, const QPoint& ) ) );
  QObject::connect( this, SIGNAL( drop( QDropEvent*, KIconContainerItem*, QStrList& ) ),
	   this, SLOT( slotDrop( QDropEvent*, KIconContainerItem*, QStrList& ) ) );
  QObject::connect( this, SIGNAL( onItem( KIconContainerItem* ) ), this, SLOT( slotOnItem( KIconContainerItem* ) ) );
  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  // Now we may react to configuration changes
  m_bInit = false;

  m_dirLister = 0L;
}

KonqKfmIconView::~KonqKfmIconView()
{
  kdebug(0, 1202, "-KonqKfmIconView");
  if ( m_dirLister ) delete m_dirLister;
}

bool KonqKfmIconView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL(eventURL);
  openURL( eventURL.url );
  return true;
}

bool KonqKfmIconView::mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu )
{
#define MVIEW_IMAGEPREVIEW_ID_2 1594 // temporary
#define MVIEW_SHOWDOT_ID_2 1595 // temporary

  if ( !CORBA::is_nil( viewMenu.menu ) )
  {
    if ( viewMenu.create )
    {
      CORBA::WString_var text;
      //    menu->insertItem4( i18n("&Large Icons"), this, "slotLargeIcons", 0, -1, -1 );
      //    menu->insertItem4( i18n("&Small Icons"), this, "slotSmallIcons", 0, -1, -1 );
      kdebug(0,1202,"adding image preview and showdotfiles");
      text = Q2C( i18n("Image &Preview") );
      viewMenu.menu->insertItem4( text, this, "slotShowSchnauzer" , 0, MVIEW_IMAGEPREVIEW_ID_2, -1 );
      text = Q2C( i18n("Show &Dot Files") );
      viewMenu.menu->insertItem4( text, this, "slotShowDot" , 0, MVIEW_SHOWDOT_ID_2, -1 );
    }
    else
    {
      kdebug(0,1202,"removing image preview and showdotfiles");
      viewMenu.menu->removeItem( MVIEW_SHOWDOT_ID_2 );
      viewMenu.menu->removeItem( MVIEW_IMAGEPREVIEW_ID_2 );
    }
  }
  
  return true;
}

bool KonqKfmIconView::mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu )
{
#define MEDIT_SELECT_ID_2 1394 // hmm...
#define MEDIT_SELECTALL_ID_2 1395

  if ( !CORBA::is_nil( editMenu.menu ) )
  {
    if ( editMenu.create )
    {
      CORBA::WString_var text;
      //    menu->insertItem4( i18n("&Large Icons"), this, "slotLargeIcons", 0, -1, -1 );
      //    menu->insertItem4( i18n("&Small Icons"), this, "slotSmallIcons", 0, -1, -1 );
      kdebug(0,1202,"adding select and selectall");
      text = Q2C( i18n("Select") );
      editMenu.menu->insertItem4( text, this, "slotSelect" , 0, MEDIT_SELECT_ID_2, -1 );
      text = Q2C( i18n("Select &All") );
      editMenu.menu->insertItem4( text, this, "slotSelectAll" , 0, MEDIT_SELECTALL_ID_2, -1 );
    }
    else
    {
      kdebug(0,1202,"removing select and selectall");
      editMenu.menu->removeItem( MEDIT_SELECT_ID_2 );
      editMenu.menu->removeItem( MEDIT_SELECTALL_ID_2 );
    }
  }
  
  return true;
}

void KonqKfmIconView::slotLargeIcons()
{
  setDisplayMode( KIconContainer::Horizontal );
}

void KonqKfmIconView::slotSmallIcons()
{
  setDisplayMode( KIconContainer::Vertical );
}

void KonqKfmIconView::slotShowDot()
{
  // TODO
}

void KonqKfmIconView::slotSelect()
{
  KLineEditDlg l( i18n("Select files:"), "", this );
  if ( l.exec() )
  {
    QString pattern = l.text();
    if ( pattern.isEmpty() )
      return;

    // QRegExp re( pattern, true, true );
    // view->getActiveView()->select( re, true );

    // TODO
    // Do we need unicode support ? (kregexp?)
  }
}

void KonqKfmIconView::slotSelectAll()
{
  kdebug(0, 1202, "KonqKfmIconView::slotSelectAll");
  selectAll();
}

void KonqKfmIconView::stop()
{
  //slotCloseURL( 0 ); // FIXME (just trying)
  // TODO in kdirlister !
}

char *KonqKfmIconView::url()
{
  assert( m_dirLister );
  return CORBA::string_dup( m_dirLister->url().ascii() );
}

void KonqKfmIconView::initConfig()
{
  QPalette p          = viewport()->palette();
//  KfmViewSettings *settings = m_pView->settings();
//  KonqPropsView *props = m_pView->props();

  KConfig *config = kapp->getConfig();
  config->setGroup("Settings");

  KfmViewSettings *settings = new KfmViewSettings( config );
  KonqPropsView *props = new KonqPropsView( config );

  m_bgColor           = settings->bgColor();
  m_textColor         = settings->textColor();
  m_linkColor         = settings->linkColor();
  m_vLinkColor        = settings->vLinkColor();
  m_stdFontName       = settings->stdFontName();
  m_fixedFontName     = settings->fixedFontName();
  m_fontSize          = settings->fontSize();

  m_bgPixmap          = props->bgPixmap(); // !!

  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );

  m_mouseMode = settings->mouseMode();

  m_underlineLink = settings->underlineLink();
  m_changeCursor = settings->changeCursor();
  m_isShowingDotFiles = props->isShowingDotFiles();

  QColorGroup c = p.normal();
  QColorGroup n( m_textColor, m_bgColor, c.light(), c.dark(), c.mid(),
		 m_textColor, m_bgColor );
  p.setNormal( n );
  c = p.active();
  QColorGroup a( m_textColor, m_bgColor, c.light(), c.dark(), c.mid(),
		 m_textColor, m_bgColor );
  p.setActive( a );
  c = p.disabled();
  QColorGroup d( m_textColor, m_bgColor, c.light(), c.dark(), c.mid(),
		 m_textColor, m_bgColor );
  p.setDisabled( d );
  viewport()->setPalette( p );

  QFont font( m_stdFontName, m_fontSize );
  setFont( font );

  if ( !m_bInit )
    refresh();

  delete settings;
  delete props;
}

void KonqKfmIconView::slotReturnPressed( KIconContainerItem *_item, const QPoint & )
{
  if ( !_item )
    return;

  KFileItem *fileItem = ((KonqKfmIconViewItem*)_item)->item();
  openURLRequest( fileItem->url().ascii() ); // gosh, ugly line ;)
}

void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )
{
  kdebug(0,1202,"void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )");

  // Background click ?
  if ( !_item )
  {
    KURL bgUrl( m_dirLister->url() );
    // Right button ?
    if ( _button == RightButton && m_pMainView )
    {
      QString cURL = bgUrl.url( 1 );
      int i = cURL.length();

      mode_t mode = 0;
      QStringList lstPopupURLs;
      
      lstPopupURLs.append( cURL );

      // A url ending with '/' is always a directory
      if ( i >= 1 && cURL[ i - 1 ] == '/' )
	mode = S_IFDIR;
      // With HTTP we can be sure that everything that does not end with '/'
      // is NOT a directory
      else if ( strcmp( bgUrl.protocol(), "http" ) == 0 ) //?iconview and http? (Simon)
	mode = 0;
      // Local filesystem without subprotocol
      if ( bgUrl.isLocalFile() )
      {
	struct stat buff;
	if ( stat( bgUrl.path(), &buff ) == -1 )
	{
	  kioErrorDialog( ERR_COULD_NOT_STAT, cURL );
	  return;
	}
	mode = buff.st_mode;
      }

      m_pMainView->popupMenu( _global, lstPopupURLs, mode );
    }
  }
  else if ( _button == LeftButton )
    slotReturnPressed( _item, _global );
  else if ( _button == RightButton && m_pMainView )
  {
    QStringList lstPopupURLs;

    QList<KIconContainerItem> icons;
    selectedItems( icons ); // KIconContainer fills the list
    mode_t mode = 0;
    bool first = true;
    QListIterator<KIconContainerItem> icit( icons );
    int i = 0;
    // For each selected icon
    for( ; *icit; ++icit )
    {
      // Cast the iconcontainer item into an icon item
      // and get the file item out of it
      KFileItem * item = ((KonqKfmIconViewItem *)*icit)->item();
      lstPopupURLs.append( item->url() );
      
      // get common mode among all urls, if there is
      if ( first )
      {    
        mode = item->mode();
        first = false;
      }
      else if ( mode != item->mode() )
        mode = 0;
    }

    m_pMainView->popupMenu( _global, lstPopupURLs, mode );
  }
}

void KonqKfmIconView::slotDoubleClicked( KIconContainerItem *_item, const QPoint &_global, int _button )
{
  if ( _button == LeftButton )
    slotReturnPressed( _item, _global );
}

void KonqKfmIconView::slotDrop( QDropEvent *_ev, KIconContainerItem* _item, QStringList &_formats )
{
  QStringList lst;

  // Try to decode to the data you understand...
  if ( QUrlDrag::decodeToUnicodeUris( _ev, lst ) )
  {
    if( lst.count() == 0 )
    {
      kdebug(KDEBUG_WARN,1202,"Oooops, no data ....");
      return;
    }
    KIOJob* job = new KIOJob;
    // Use either the root url or the item url (we stored it as the icon "name")
    QString dest = ( _item == 0 ) ? m_dirLister->url() : _item->name();

    job->copy( lst, dest );
  }
  else if ( _formats.count() >= 1 )
  {
    if ( _item == 0L )
      pasteData( m_dirLister->url(), _ev->data( _formats.first() ) );
    else
    {
      kdebug(0,1202,"Pasting to %s", (_item->name().data() /* item's url */));
      pasteData( _item->name() /* item's url */, _ev->data( _formats.first() ) );
    }
  }
}

void KonqKfmIconView::slotStarted( const QString & url )
{
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char*)url.ascii(), 0 ) );
}

void KonqKfmIconView::slotCompleted()
{
  SIGNAL_CALL1( "completed", id() );
}

void KonqKfmIconView::slotCanceled()
{
  SIGNAL_CALL1( "canceled", id() );
}

void KonqKfmIconView::slotUpdate()
{
  kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotUpdate()");
  setup();
  viewport()->update();
}
  
void KonqKfmIconView::slotClear()
{
  //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotClear()");
  clear();
}

void KonqKfmIconView::slotNewItem( KFileItem * _fileitem )
{
  //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotNewItem(...)");
  KonqKfmIconViewItem* item = new KonqKfmIconViewItem( this, _fileitem );
  insert( item, -1, -1, false );
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
  //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotDeleteItem(...)");
  // we need to find out the iconcontainer item containing the fileitem
  iterator it = KIconContainer::begin();
  for( ; *it; ++it )
    if ( ((KonqKfmIconViewItem*)*it)->item() == _fileitem ) // compare the pointers
    {
      remove( (*it), false /* don't refresh yet */ );
      break;
    }
}

void KonqKfmIconView::openURL( const char *_url )
{
  // Create the directory lister
  m_dirLister = new KDirLister();

  QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ), 
           this, SLOT( slotStarted( const QString & ) ) );
  QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
  QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
  QObject::connect( m_dirLister, SIGNAL( update() ), this, SLOT( slotUpdate() ) );
  QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
  QObject::connect( m_dirLister, SIGNAL( newItem( KFileItem * ) ), 
           this, SLOT( slotNewItem( KFileItem * ) ) );
  QObject::connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ), 
           this, SLOT( slotDeleteItem( KFileItem * ) ) );
  // Start the directory lister !
  m_dirLister->openURL( KURL( _url ) );
  // Note : we don't store the url. KDirLister does it for us.

  CORBA::WString_var caption = Q2C( _url );
  m_vMainWindow->setPartCaption( id(), caption );
}

void KonqKfmIconView::slotOnItem( KIconContainerItem *_item )
{
  QString s;
  if ( _item )
  {
    KFileItem * fileItem = ((KonqKfmIconViewItem *)_item)->item();
    s = fileItem->getStatusBarInfo();
  }
  CORBA::WString_var ws = Q2C( s );
  SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( ws.out(), 0 ) );
}

///////// KonqKfmIconViewItem ////////
// This class is STRONGLY related (almost identical) to KDesktopIcon

KonqKfmIconViewItem::KonqKfmIconViewItem( KIconContainer *_parent, KFileItem* _fileitem )
  : KIconContainerItem( _parent ), // parent constructor
    m_parent( _parent ), // members
    m_fileitem( _fileitem )
{
  // Set the item text (the one displayed) from the text computed by KFileItem
  setText( m_fileitem->getText() );
  // Set the item name from the url hold by KFileItem
  setName( m_fileitem->url() );
  // Determine the item pixmap from one determined by KFileItem
  QPixmap *p = m_fileitem->getPixmap( m_parent->displayMode() == KIconContainer::Vertical );
  if (p) setPixmap( *p );
}

void KonqKfmIconViewItem::refresh()
{
  // No idea if this is ever called currently.
  QPixmap *p = m_fileitem->getPixmap( m_parent->displayMode() == KIconContainer::Vertical ); // determine the pixmap (KFileItem)
  if (p) setPixmap( *p ); // store it in the item (KIconContainerItem)

  KIconContainerItem::refresh();
}

void KonqKfmIconViewItem::paint( QPainter* _painter, bool _drag )
{
  if ( m_fileitem->isLink() )
  {
    QFont f = _painter->font();
    f.setItalic( true );
    _painter->setFont( f );
  }

  KIconContainerItem::paint( _painter, _drag );
}

#include "konq_iconview.moc"
