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
#include <kdirlister.h>
#include <kfileici.h>
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
#include <qclipboard.h>
#include <qregexp.h>

#include <opQCProxy.h>
#include <opUIUtils.h>

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
  QObject::connect( this, SIGNAL( drop( QDropEvent*, KIconContainerItem*, QStringList& ) ),
	   this, SLOT( slotDrop( QDropEvent*, KIconContainerItem*, QStringList& ) ) );
  QObject::connect( this, SIGNAL( onItem( KIconContainerItem* ) ), this, SLOT( slotOnItem( KIconContainerItem* ) ) );
  QObject::connect( this, SIGNAL( selectionChanged() ), this, SLOT( slotSelectionChanged() ) );
  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  // Now we may react to configuration changes
  m_bInit = false;

  m_dirLister = 0L;
  m_bLoading = false;
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
  openURL( eventURL.url, (int)eventURL.xOffset, (int)eventURL.yOffset );
  return true;
}

bool KonqKfmIconView::mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu )
{
  m_vViewMenu = OpenPartsUI::Menu::_duplicate( viewMenu );
  
  if ( !CORBA::is_nil( viewMenu ) )
  {
    CORBA::WString_var text;
//    text = Q2C( i18n("Image &Preview") );
//    viewMenu->insertItem4( text, this, "slotShowSchnauzer" , 0, -1, -1 );
    text = Q2C( i18n("Show &Dot Files") );
    m_idShowDotFiles = viewMenu->insertItem4( text, this, "slotShowDot" , 0, -1, -1 );
    viewMenu->setItemChecked( m_idShowDotFiles, m_pProps->m_bShowDot );
    
    text = Q2C( i18n( "Sort..." ) );
    viewMenu->insertItem8( text, m_vSortMenu, -1, -1 );
    
    m_vSortMenu->setCheckable( true );

    text = Q2C( i18n( "by Name ( Case Sensitive )" ) );
    m_idSortByNameCaseSensitive = m_vSortMenu->insertItem4( text, this, "slotSortByNameCaseSensitive", 0, -1, -1 );

    text = Q2C( i18n( "by Name ( Case Insensitive )" ) );
    m_idSortByNameCaseInsensitive = m_vSortMenu->insertItem4( text, this, "slotSortByNameCaseInsensitive", 0, -1, -1 );
    
    text = Q2C( i18n( "by Size" ) );
    m_idSortBySize = m_vSortMenu->insertItem4( text, this, "slotSortBySize", 0, -1, -1 );

    m_vSortMenu->insertSeparator( -1 );
    
    text = Q2C( i18n( "Descending" ) );
    m_idSortDescending = m_vSortMenu->insertItem4( text, this, "slotSetSortDirectionDescending", 0, -1, -1 );
    m_vSortMenu->setItemChecked( m_idSortDescending, sortDirection() == KIconContainerItem::Descending );
    
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
    CORBA::WString_var text;
    text = Q2C( i18n("&Select") );
    editMenu->insertItem4( text, this, "slotSelect" , 0, -1, -1 );
    text = Q2C( i18n("&Unselect") );
    editMenu->insertItem4( text, this, "slotUnselect" , 0, -1, -1 );
    text = Q2C( i18n("Select &All") );
    editMenu->insertItem4( text, this, "slotSelectAll" , 0, -1, -1 );
    text = Q2C( i18n("U&nselect All") );
    editMenu->insertItem4( text, this, "slotUnselectAll" , 0, -1, -1 );
    
    m_proxySelectAll = new Qt2CORBAProxy( this, "slotSelectAll" );
    m_pMainView->accel()->connectItem( "SelectAll", m_proxySelectAll, SLOT( callback() ) );
  }
  else // cleanup
  {
    if ( m_proxySelectAll ) {
      m_pMainView->accel()->disconnectItem( "SelectAll", m_proxySelectAll, SLOT( callback() ) );
      delete m_proxySelectAll;
      m_proxySelectAll = 0L;
    }
  }
  
  return true;
}

void KonqKfmIconView::slotShowDot()
{
  kdebug(0, 1202, "KonqKfmIconView::slotShowDot()");
  m_pProps->m_bShowDot = !m_pProps->m_bShowDot;
  m_dirLister->setShowingDotFiles( m_pProps->m_bShowDot );
  bSetupNeeded = true; // we don't want the non-dot files to remain where they are !

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
    
    iterator it = KIconContainer::begin();
    for( ; *it; ++it )
      if ( re.match( (*it)->text() ) != -1 )
        setSelected( *it, true );
    
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
    
    iterator it = KIconContainer::begin();
    for( ; *it; ++it )
      if ( re.match( (*it)->text() ) != -1 )
        setSelected( *it, false );
    
    emit selectionChanged();
  }
}

void KonqKfmIconView::slotSelectAll()
{
  selectAll();
}

void KonqKfmIconView::slotUnselectAll()
{
  unselectAll();
}

void KonqKfmIconView::slotSortByNameCaseSensitive()
{
  setSortCriterion( KIconContainerItem::NameCaseSensitive );

  refresh();
  
  setupSortMenu();
}

void KonqKfmIconView::slotSortByNameCaseInsensitive()
{
  setSortCriterion( KIconContainerItem::NameCaseInsensitive );

  refresh();
  
  setupSortMenu();
}

void KonqKfmIconView::slotSortBySize()
{
  setSortCriterion( KIconContainerItem::Size );
  
  refresh();
  
  setupSortMenu();
}

void KonqKfmIconView::slotSetSortDirectionDescending()
{
  if ( sortDirection() == KIconContainerItem::Ascending )
    setSortDirection( KIconContainerItem::Descending );
  else
    setSortDirection( KIconContainerItem::Ascending );

  m_vSortMenu->setItemChecked( m_idSortDescending, sortDirection() == KIconContainerItem::Descending );
  
  refresh();
}

void KonqKfmIconView::setViewMode( Konqueror::DirectoryDisplayMode mode )
{
  switch ( mode )
  {
    case Konqueror::LargeIcons:
      setDisplayMode( KIconContainer::Horizontal );
      break;
    case Konqueror::SmallIcons:
      setDisplayMode( KIconContainer::Vertical );
      break;
    case Konqueror::SmallVerticalIcons:
      setDisplayMode( KIconContainer::SmallVertical );
      break;
    default: assert( 0 );
  }
}

Konqueror::DirectoryDisplayMode KonqKfmIconView::viewMode()
{
  switch ( m_displayMode )
  {
    case Horizontal:
      return Konqueror::LargeIcons;
    case Vertical:
      return Konqueror::SmallIcons;
    case SmallVertical:
      return Konqueror::SmallVerticalIcons;
  }
}

void KonqKfmIconView::stop()
{
  m_dirLister->stop();
}

char *KonqKfmIconView::url()
{
  assert( m_dirLister );
  return CORBA::string_dup( m_dirLister->url().ascii() );
}

CORBA::Long KonqKfmIconView::xOffset()
{
  return (CORBA::Long)contentsX();
}

CORBA::Long KonqKfmIconView::yOffset()
{
  return (CORBA::Long)contentsY();
}

void KonqKfmIconView::initConfig()
{
  QPalette p          = viewport()->palette();
//  KfmViewSettings *settings = m_pView->settings();

  KConfig *config = kapp->getConfig();
  config->setGroup("Settings");

  KfmViewSettings *settings = new KfmViewSettings( config );

  m_bgColor           = settings->bgColor();
  m_textColor         = settings->textColor();
  m_linkColor         = settings->linkColor();
  m_vLinkColor        = settings->vLinkColor();
  m_stdFontName       = settings->stdFontName();
  m_fixedFontName     = settings->fixedFontName();
  m_fontSize          = settings->fontSize();

  //  m_bgPixmap          = props->bgPixmap(); // !!

  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );

  m_mouseMode = (KIconContainer::MouseMode) settings->mouseMode();

  m_underlineLink = settings->underlineLink();
  m_changeCursor = settings->changeCursor();

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
  //  delete props;
}

void KonqKfmIconView::slotReturnPressed( KIconContainerItem *_item, const QPoint & )
{
  if ( !_item )
    return;

  KFileItem *fileItem = ((KFileICI*)_item)->item();
  openURLRequest( fileItem->url().url().ascii() );
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
      // This is a directory. Always.
      mode_t mode = S_IFDIR;

      KFileItem item( "viewURL" /*whatever*/, mode, bgUrl );
      KFileItemList items;
      items.append( &item );
      m_pMainView->popupMenu( _global, items );
    }
  }
  else if ( _button == LeftButton )
    slotReturnPressed( _item, _global );
  else if ( _button == RightButton && m_pMainView )
  {
    KFileItemList lstItems;

    QList<KIconContainerItem> icons;
    selectedItems( icons ); // KIconContainer fills the list
    QListIterator<KIconContainerItem> icit( icons );
    // For each selected icon
    for( ; *icit; ++icit )
    {
      // Cast the iconcontainer item into an icon item
      // and get the file item out of it
      KFileItem * item = ((KFileICI *)*icit)->item();
      lstItems.append( item );
    }

    m_pMainView->popupMenu( _global, lstItems );
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

void KonqKfmIconView::slotSelectionChanged()
{
  SIGNAL_CALL0( "selectionChanged" );
}

void KonqKfmIconView::slotStarted( const QString & url )
{
  unselectAll();
  if ( m_bLoading )
    SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char*)url.ascii(), 0 ) );
  bSetupNeeded = false;
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

void KonqKfmIconView::slotUpdate()
{
  kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotUpdate()");
  if ( bSetupNeeded )
  {
    bSetupNeeded = false;
    setup();
  }
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
  KFileICI* item = new KFileICI( this, _fileitem );
  insert( item, -1, -1, false );
  bSetupNeeded = true;
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
  //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotDeleteItem(...)");
  // we need to find out the iconcontainer item containing the fileitem
  iterator it = KIconContainer::begin();
  for( ; *it; ++it )
    if ( ((KFileICI*)*it)->item() == _fileitem ) // compare the pointers
    {
      remove( (*it), false /* don't refresh yet */ );
      // bSetupNeeded not set to true, so that simply deleting a file leaves
      // a blank space. Well that's just my preference (David)
      break;
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
    QObject::connect( m_dirLister, SIGNAL( update() ), this, SLOT( slotUpdate() ) );
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

  QString caption = _url;
  
  if ( m_pMainView ) //builtin view?
    caption.prepend( "Konqueror: " );
    
  CORBA::WString_var wcaption = Q2C( caption );
  m_vMainWindow->setPartCaption( id(), wcaption );
}

void KonqKfmIconView::can( CORBA::Boolean &copy, CORBA::Boolean &paste, CORBA::Boolean &move )
{
  QList<KIconContainerItem> selection;
  selectedItems( selection );
  move = copy = (CORBA::Boolean) ( selection.count() != 0 );

  bool bKIOClipboard = !isClipboardEmpty();
  
  QMimeSource *data = QApplication::clipboard()->data();
  
  paste = (CORBA::Boolean) ( bKIOClipboard || data->encodedData( data->format() ).size() != 0 );
}

void KonqKfmIconView::copySelection()
{
  QList<KIconContainerItem> selection;
  selectedItems( selection );
  
  QStringList lstURLs;
 
  QListIterator<KIconContainerItem> it( selection );
  for (; it.current(); ++it )
    lstURLs.append( ( (KFileICI *)it.current() )->item()->url().url() );
  
  QUriDrag *urlData = new QUriDrag;
  urlData->setUnicodeUris( lstURLs );
  QApplication::clipboard()->setData( urlData );
}

void KonqKfmIconView::pasteSelection()
{
  pasteClipboard( m_dirLister->url() );
}

void KonqKfmIconView::moveSelection( const char *destinationURL )
{
  QList<KIconContainerItem> selection;
  selectedItems( selection );
  
  QStringList lstURLs;
 
  QListIterator<KIconContainerItem> it( selection );
  for (; it.current(); ++it )
    lstURLs.append( ( (KFileICI *)it.current() )->item()->url().url() );
  
  KIOJob *job = new KIOJob;
  
  if ( destinationURL )
    job->move( lstURLs, destinationURL );
  else
    job->del( lstURLs );
}

void KonqKfmIconView::slotOnItem( KIconContainerItem *_item )
{
  QString s;
  if ( _item )
  {
    KFileItem * fileItem = ((KFileICI *)_item)->item();
    s = fileItem->getStatusBarInfo();
  }
  CORBA::WString_var ws = Q2C( s );
  SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( ws.out(), 0 ) );
}

void KonqKfmIconView::setupSortMenu()
{
  switch ( sortCriterion() )
  {
    case KIconContainerItem::NameCaseSensitive:
      m_vSortMenu->setItemChecked( m_idSortByNameCaseSensitive, true );
      m_vSortMenu->setItemChecked( m_idSortByNameCaseInsensitive, false );
      m_vSortMenu->setItemChecked( m_idSortBySize, false );
      break;
    case KIconContainerItem::NameCaseInsensitive:
      m_vSortMenu->setItemChecked( m_idSortByNameCaseSensitive, false );
      m_vSortMenu->setItemChecked( m_idSortByNameCaseInsensitive, true );
      m_vSortMenu->setItemChecked( m_idSortBySize, false );
      break;
    case KIconContainerItem::Size:
      m_vSortMenu->setItemChecked( m_idSortByNameCaseSensitive, false );
      m_vSortMenu->setItemChecked( m_idSortByNameCaseInsensitive, false );
      m_vSortMenu->setItemChecked( m_idSortBySize, true );
      break;
  }
}

#include "konq_iconview.moc"
