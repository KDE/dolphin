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
#include "konq_childview.h"
#include "konq_frame.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <stdlib.h>

#include <kaccel.h>
#include <kcursor.h>
#include <kdirlister.h>
#include <kfileivi.h>
#include <kfileitem.h>
#include <kio_error.h>
#include <kio_job.h>
#include <kio_paste.h>
#include <klineeditdlg.h>
#include <kmimetype.h>
#include <kpixmapcache.h>
#include <kurl.h>
#include <kdebug.h>
#include <konqsettings.h>
#include <klibloader.h>

#include <qmsgbox.h>
#include <qfile.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <qdragobject.h>
#include <klocale.h>
#include <qapplication.h>
#include <qclipboard.h>
#include <qregexp.h>

class KonqIconViewFactory : public KLibFactory
{
public:
  KonqIconViewFactory() {}

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
	
	QIconView *iconView = obj->iconView();
	
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

IconEditExtension::IconEditExtension( KonqKfmIconView *iconView )
 : EditExtension( iconView, "IconEditExtension" )
{
  m_iconView = iconView;
}

void IconEditExtension::can( bool &copy, bool &paste, bool &move )
{
  bool bItemSelected = false;

  for ( QIconViewItem *it = m_iconView->iconView()->firstItem(); it; it = it->nextItem() )
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

void IconEditExtension::copySelection()
{
  QDragObject * obj = m_iconView->iconView()->dragObject();
  QApplication::clipboard()->setData( obj );
}

void IconEditExtension::pasteSelection()
{
  pasteClipboard( m_iconView->url() );
}

void IconEditExtension::moveSelection( const QString &destinationURL )
{
  QStringList lstURLs;

  for ( QIconViewItem *it = m_iconView->iconView()->firstItem(); it; it = it->nextItem() )
    if ( it->isSelected() )
      lstURLs.append( ( (KFileIVI *)it )->item()->url().url() );

  KIOJob *job = new KIOJob;

  if ( !destinationURL.isEmpty() )
    job->move( lstURLs, destinationURL );
  else
    job->del( lstURLs );
}

KonqKfmIconView::KonqKfmIconView()
{
  kdebug(0, 1202, "+KonqKfmIconView");

  // Create a properties instance for this view
  // (copying the default values)
  m_pProps = new KonqPropsView( * KonqPropsView::defaultProps() );

  m_pIconView = new KonqIconViewWidget( m_pProps, this, "qiconview" );

  m_extension = new IconEditExtension( this );

  m_ulTotalFiles = 0;

  // Dont repaint on configuration changes during construction
  m_bInit = true;

  m_paDotFiles = new KToggleAction( i18n( "Show &Dot Files" ), 0, this, SLOT( slotShowDot() ), this );
  m_pamSort = new KActionMenu( i18n( "Sort..." ), this );

  KToggleAction *aSortByNameCS = new KToggleAction( i18n( "by Name (Case Sensitive" ), 0, this );
  KToggleAction *aSortByNameCI = new KToggleAction( i18n( "by Name (Case Insensitive" ), 0, this );
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
    
  KToggleAction *aSortDescending = new KToggleAction( i18n( "Descending" ), 0, this, SLOT( foo ), this );

  connect( aSortDescending, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDescending( bool ) ) );

  m_pamSort->insert( aSortByNameCS );
  m_pamSort->insert( aSortByNameCI );
  m_pamSort->insert( aSortBySize );
  
  m_pamSort->popupMenu()->insertSeparator();
  
  m_pamSort->insert( aSortDescending );

  m_paSelect = new KAction( i18n( "&Select" ), 0, this, SLOT( slotSelect() ), this );
  m_paUnselect = new KAction( i18n( "&Unselect" ), 0, this, SLOT( slotUnselect() ), this );
  m_paSelectAll = new KAction( i18n( "Select &All" ), 0, this, SLOT( slotSelectAll() ), this );
  m_paUnselectAll = new KAction( i18n( "U&nselect All" ), 0, this, SLOT( slotUnselectAll() ), this );

  KToggleAction *aLargeIcons = new KToggleAction( i18n( "&Large View" ), 0, this );
  KToggleAction *aNormalIcons = new KToggleAction( i18n( "&Normal View" ), 0, this );
  KToggleAction *aSmallIcons = new KToggleAction( i18n( "&Small View" ), 0, this );
  m_paKOfficeMode = new KToggleAction( i18n( "&KOffice mode" ), 0, this );

  aLargeIcons->setExclusiveGroup( "ViewMode" );
  aNormalIcons->setExclusiveGroup( "ViewMode" );
  aSmallIcons->setExclusiveGroup( "ViewMode" );
  m_paKOfficeMode->setExclusiveGroup( "ViewMode" );

  aLargeIcons->setChecked( true );
  aNormalIcons->setChecked( false );
  aSmallIcons->setChecked( false );
  m_paKOfficeMode->setChecked( false );
  m_paKOfficeMode->setEnabled( false );

  KToggleAction *aBottomText = new KToggleAction( i18n( "Text at the &bottom" ), 0, this );
  KToggleAction *aRightText = new KToggleAction( i18n( "Text at the &right" ), 0, this );

  aBottomText->setExclusiveGroup( "TextPos" );
  aRightText->setExclusiveGroup( "TextPos" );

  aBottomText->setChecked( true );
  aRightText->setChecked( false );

  connect( aLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewLarge( bool ) ) );
  connect( aNormalIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewNormal( bool ) ) );
  connect( aSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewSmall( bool ) ) );
  connect( m_paKOfficeMode, SIGNAL( toggled( bool ) ), this, SLOT( slotKofficeMode( bool ) ) );

  connect( aBottomText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextBottom( bool ) ) );
  connect( aRightText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextRight( bool ) ) );

  actions()->append( BrowserView::ViewAction( m_paDotFiles, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_pamSort, BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( aBottomText, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( aRightText, BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( aLargeIcons, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( aNormalIcons, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( aSmallIcons, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paKOfficeMode, BrowserView::MenuView ) );

  actions()->append( BrowserView::ViewAction( m_paSelect, BrowserView::MenuEdit ) );
  actions()->append( BrowserView::ViewAction( m_paUnselect, BrowserView::MenuEdit ) );
  actions()->append( BrowserView::ViewAction( m_paSelectAll, BrowserView::MenuEdit ) );
  actions()->append( BrowserView::ViewAction( m_paUnselectAll, BrowserView::MenuEdit ) );


  m_pIconView->initConfig();

  QObject::connect( m_pIconView, SIGNAL( doubleClicked( QIconViewItem * ) ),
                    this, SLOT( slotMousePressed( QIconViewItem * ) ) );
		
  QObject::connect( m_pIconView, SIGNAL( dropped( QDropEvent * ) ),
	            this, SLOT( slotDrop( QDropEvent* ) ) );
	
  QObject::connect( m_pIconView, SIGNAL( onItem( QIconViewItem * ) ),
                    this, SLOT( slotOnItem( QIconViewItem * ) ) );
		
  QObject::connect( m_pIconView, SIGNAL( onViewport() ),
                    this, SLOT( slotOnViewport() ) );
		
  QObject::connect( m_pIconView, SIGNAL( selectionChanged() ),
                    m_extension, SIGNAL( selectionChanged() ) );

  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  QObject::connect( m_pIconView, SIGNAL( itemRightPressed( QIconViewItem * ) ),
                    this, SLOT( slotItemRightClicked( QIconViewItem * ) ) );
  QObject::connect( m_pIconView, SIGNAL( viewportRightPressed() ),
                    this, SLOT( slotViewportRightClicked() ) );

  // Now we may react to configuration changes
  m_bInit = false;

  m_dirLister = 0L;
  m_bLoading = false;

  m_pIconView->setSelectionMode( QIconView::Multi );
  m_pIconView->setViewMode( QIconSet::Large );
  m_pIconView->setItemTextPos( QIconView::Bottom );
  m_pIconView->setResizeMode( QIconView::Adjust );
  m_pIconView->setGridX( 70 );
#warning "Uncomment the line below if we switch to a more recent Qt 2.1 snaphsot"
  //m_pIconView->setWordWrapIconText( FALSE );
  m_pIconView->setReorderItemsWhenInsert( true );
  m_pIconView->setResortItemsWhenInsert( true, m_pIconView->sortOrder() );

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
  //bSetupNeeded = true; // we don't want the non-dot files to remain where they are !

//  m_vViewMenu->setItemChecked( m_idShowDotFiles, m_pProps->m_bShowDot );
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

    emit m_extension->selectionChanged();
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

    emit m_extension->selectionChanged();
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

  m_pIconView->sortItems( m_pIconView->sortOrder() );
}

void KonqKfmIconView::resizeEvent( QResizeEvent * )
{
  m_pIconView->setGeometry( 0, 0, width(), height() );
}

void KonqKfmIconView::slotSortDescending( bool toggle )
{
  if ( !toggle )
    return;

  if ( m_pIconView->sortOrder() )
    m_pIconView->setResortItemsWhenInsert( true, false );
  else
    m_pIconView->setResortItemsWhenInsert( true, true );

  m_pIconView->sortItems( m_pIconView->sortOrder() );
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
    if ( b ) {
	m_pIconView->setViewMode( QIconSet::Large );
	m_pIconView->orderItemsInGrid();
	m_pIconView->repaintContents( m_pIconView->contentsX(), m_pIconView->contentsY(), 
				      m_pIconView->viewport()->width(), m_pIconView->viewport()->height(), 
				      FALSE );
    }
}

void KonqKfmIconView::slotViewNormal( bool b )
{
    if ( b ) {
	m_pIconView->setViewMode( QIconSet::Automatic );
	m_pIconView->orderItemsInGrid();
	m_pIconView->repaintContents( m_pIconView->contentsX(), m_pIconView->contentsY(), 
				      m_pIconView->viewport()->width(), m_pIconView->viewport()->height(), 
				      FALSE );
    }
}

void KonqKfmIconView::slotViewSmall( bool b )
{
    if ( b ) {
	m_pIconView->setViewMode( QIconSet::Small );
	m_pIconView->orderItemsInGrid();
	m_pIconView->repaintContents( m_pIconView->contentsX(), m_pIconView->contentsY(), 
				      m_pIconView->viewport()->width(), m_pIconView->viewport()->height(), 
				      FALSE );
	m_pIconView->repaint( FALSE );
    }
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

void KonqKfmIconView::stop()
{
  debug("KonqKfmIconView::stop()");
  if ( m_dirLister ) m_dirLister->stop();
}

void KonqKfmIconView::saveState( QDataStream &stream )
{
  BrowserView::saveState( stream );

  stream << (Q_INT32)m_pIconView->viewMode() << (Q_INT32)m_pIconView->itemTextPos();
}

void KonqKfmIconView::restoreState( QDataStream &stream )
{
  BrowserView::restoreState( stream );

  Q_INT32 iIconSize, iTextPos;

  stream >> iIconSize >> iTextPos;

  QIconSet::Size iconSize = (QIconSet::Size)iIconSize;
  QIconView::ItemTextPos textPos = (QIconView::ItemTextPos)iTextPos;

  m_pIconView->setViewMode( iconSize );
  m_pIconView->setItemTextPos( textPos );
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


void KonqKfmIconView::slotMousePressed( QIconViewItem *item )
{
  KFileItem *fileItem = ((KFileIVI*)item)->item();
  emit openURLRequest( fileItem->url().url(), false, 0, 0 );
}

void KonqKfmIconView::slotDrop( QDropEvent *e )
{
  dropStuff( e );
}

void KonqKfmIconView::slotDropItem( KFileIVI *item, QDropEvent *e )
{
  dropStuff( e, item );
}

void KonqKfmIconView::slotItemRightClicked( QIconViewItem *item )
{
  item->setSelected( true );

  KFileItemList lstItems;

  QIconViewItem *it = m_pIconView->firstItem();
  for (; it; it = it->nextItem() )
    if ( it->isSelected() )
    {
      KFileItem *fItem = ((KFileIVI *)it)->item();
      lstItems.append( fItem );
    }

  emit popupMenu( QCursor::pos(), lstItems );
}

void KonqKfmIconView::slotViewportRightClicked()
{
  KURL bgUrl( m_dirLister->url() );

  // This is a directory. Always.
  mode_t mode = S_IFDIR;

  KFileItem item( "viewURL" /*whatever*/, mode, bgUrl );
  KFileItemList items;
  items.append( &item );
  emit popupMenu( QCursor::pos(), items );
}

void KonqKfmIconView::slotStarted( const QString & /*url*/ )
{
  m_pIconView->selectAll( false );
  if ( m_bLoading )
    emit started();
  //bSetupNeeded = false;
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
}

void KonqKfmIconView::slotNewItem( KFileItem * _fileitem )
{
//  kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotNewItem(...)");
  KFileIVI* item = new KFileIVI( m_pIconView, _fileitem );
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

  if ( m_ulTotalFiles > 0 )
    emit loadingProgress( ( m_pIconView->count() * 100 ) / m_ulTotalFiles );

  //bSetupNeeded = true;

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
  //TODO : find a way to get the view to update itself
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
  if ( m_pProps->enterDir( u ) )
  {
    // nothing to do yet
  }
  
  // Start the directory lister !
  m_dirLister->openURL( u, m_pProps->m_bShowDot );
  // Note : we don't store the url. KDirLister does it for us.

  KIOJob *job = KIOJob::find( m_dirLister->jobId() );
  if ( job )
  {
    connect( job, SIGNAL( sigTotalFiles( int, unsigned long ) ),
             this, SLOT( slotTotalFiles( int, unsigned long ) ) );
  }

  m_ulTotalFiles = 0;

#warning FIXME (Simon)
//  setCaptionFromURL( _url );
  m_pIconView->show();
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

///////

void KonqIconViewWidget::initConfig()
{
  m_pSettings = KonqSettings::defaultFMSettings();
  // Color settings
  QColor textColor         = m_pSettings->textColor();
  QColor linkColor         = m_pSettings->linkColor();

  /*
    // Does this make sense ? (David)
  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );
  */

  // Font settings
  QFont font( m_pSettings->stdFontName(), m_pSettings->fontSize() );
  font.setUnderline( m_pSettings->underlineLink() );
  setItemFont( font );

  // Color settings
  setItemColor( textColor );

  // Behaviour (single click/double click, autoselect, ...)
  bool bChangeCursor = m_pSettings->changeCursor();
  setSingleClickConfiguration( new QFont(font), new QColor(textColor), new QFont(font), new QColor(linkColor),
                    new QCursor(bChangeCursor ? KCursor().handCursor() : KCursor().arrowCursor()),
                    m_pSettings->autoSelect() );
  setUseSingleClickMode( m_pSettings->singleClick() );
}
void KonqIconViewWidget::drawBackground( QPainter *p, const QRect &r )
{
  if ( m_pProps->bgPixmap().isNull() )
    p->fillRect( r, QBrush( m_pProps->bgColor() ) );
  else
    p->drawTiledPixmap( r, m_pProps->bgPixmap() );
}

QDragObject * KonqIconViewWidget::dragObject()
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
}


void KonqIconViewWidget::initDrag( QDropEvent *e )
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
