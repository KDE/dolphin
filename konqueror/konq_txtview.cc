/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#include "konq_txtview.h"
#include "konq_mainview.h"
#include "konq_searchdia.h"
#include "konq_progressproxy.h"

#include <sys/stat.h>
#include <unistd.h>

#include <qdragobject.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>

#include <kio_job.h>
#include <kio_cache.h>
#include <kio_error.h>
#include <kurl.h>
#include <klocale.h>
#include <opUIUtils.h>
#include <kapp.h>
#include <kglobal.h>
#include <kmessagebox.h>
#include <kpixmapcache.h>
#include <konqdefaults.h>

#define TOOLBAR_SEARCH_ID Browser::View::TOOLBAR_ITEM_ID_BEGIN
#define TOOLBAR_EDITOR_ID Browser::View::TOOLBAR_ITEM_ID_BEGIN + 1

KonqTxtView::KonqTxtView( KonqMainView *mainView )
{
  kdebug(KDEBUG_INFO, 1202, "+KonqTxtView");
  ADD_INTERFACE( "IDL:Konqueror/TxtView:1.0" );
  ADD_INTERFACE( "IDL:Browser/PrintingExtension:1.0" );
  ADD_INTERFACE( "IDL:Browser/EditExtension:1.0" );

  SIGNAL_IMPL( "loadingProgress" );
  SIGNAL_IMPL( "speedProgress" );
  
  setWidget( this );
  
  QWidget::setFocusPolicy( StrongFocus );
  
  setReadOnly( true );
  
  m_pMainView = mainView;
  m_jobId = 0;
  setAcceptDrops( true );
  m_bFixedFont = false;
  setFont( KGlobal::generalFont() );
}

KonqTxtView::~KonqTxtView()
{
  kdebug(KDEBUG_INFO, 1202, "-KonqTxtView");
  stop();
}

bool KonqTxtView::mappingOpenURL( Browser::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL( eventURL );
  kdebug(0,1202,"bool KonqTxtView::mappingOpenURL( Konqueror::EventOpenURL eventURL )");
  stop();
  
  KIOCachedJob *job = new KIOCachedJob;
  
  job->setGUImode( KIOJob::NONE );
  
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigRedirection( int, const char * ) ), this, SLOT( slotRedirection( int, const char * ) ) );
  QObject::connect( job, SIGNAL( sigData( int, const char *, int ) ), this, SLOT( slotData( int, const char *, int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char * ) ), this, SLOT( slotError( int, int, const char * ) ) );
  
  (void)new KonqProgressProxy( this, job );
  
  m_jobId = job->id();
  
  clear();
  
  m_iXOffset = eventURL.xOffset;
  m_iYOffset = eventURL.yOffset;
  
  m_strURL = eventURL.url;
  job->get( eventURL.url, eventURL.reload );
  SIGNAL_CALL2( "started", id(), eventURL.url );
  
  setCaptionFromURL( m_strURL );

  return true;
}

bool KonqTxtView::mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu )
{
  m_vMenuView = OpenPartsUI::Menu::_duplicate( viewMenu );

  if ( !CORBA::is_nil( viewMenu ) )
  {
    m_idFixedFont = m_vMenuView->insertItem4( i18n( "Use Fixed Font" ), 
					      this, "slotFixedFont", 0, -1, -1 );
    m_vMenuView->setItemChecked( m_idFixedFont, m_bFixedFont );
  }

  return true;
}

bool KonqTxtView::mappingFillMenuEdit( Browser::View::EventFillMenu_ptr editMenu )
{
  if ( !CORBA::is_nil( editMenu ) )
  {
    editMenu->insertItem4( i18n( "Select &All" ), this, "slotSelectAll", 0, -1, -1 );
    editMenu->insertItem4( i18n( "Launch &Editor" ), this, "slotEdit", 0, -1, -1 );
    editMenu->insertItem4( i18n( "Search..." ), this, "slotSearch", 0, -1, -1 );
  }

  return true;
}

bool KonqTxtView::mappingFillToolBar( Browser::View::EventFillToolBar toolBar )
{
  if ( CORBA::is_nil( toolBar.toolBar ) )
    return false;
    
  if ( toolBar.create )
  {
    QString toolTip = i18n( "Search" );
    OpenPartsUI::Pixmap_var pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "search.png" ) );
    toolBar.toolBar->insertButton2( pix, TOOLBAR_SEARCH_ID, SIGNAL(clicked()),
                                    this, "slotSearch", true, toolTip, toolBar.startIndex++ );
    toolTip = i18n( "Launch Editor" );
    pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "pencil.png" ) );
    toolBar.toolBar->insertButton2( pix, TOOLBAR_EDITOR_ID, SIGNAL(clicked()),
                                    this, "slotEdit", true, toolTip, toolBar.startIndex++ );
  }
  else
  {
    toolBar.toolBar->removeItem( TOOLBAR_SEARCH_ID );
    toolBar.toolBar->removeItem( TOOLBAR_EDITOR_ID );
  }
    
  return true;
}

long int KonqTxtView::xOffset()
{
  return QTableView::xOffset();
}

long int KonqTxtView::yOffset()
{
  return QTableView::yOffset();
}

void KonqTxtView::stop()
{
  if ( m_jobId )
  {
    KIOJob *job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
}

void KonqTxtView::slotSelectAll()
{
  QMultiLineEdit::selectAll();
}

void KonqTxtView::slotEdit()
{
  KConfig *config = kapp->getConfig();
  config->setGroup( "Misc Defaults" );
  QString editor = config->readEntry( "Editor", DEFAULT_EDITOR );
    
  QCString cmd;
  cmd.sprintf( "%s %s &", editor.ascii(), m_strURL.ascii() );
  system( cmd.data() );
}

void KonqTxtView::slotFixedFont()
{
  m_bFixedFont = !m_bFixedFont;
  if ( !CORBA::is_nil( m_vMenuView ) )
    m_vMenuView->setItemChecked( m_idFixedFont, m_bFixedFont );
    
  if ( m_bFixedFont )
    setFont( KGlobal::fixedFont() );
  else
    setFont( KGlobal::generalFont() );
}

void KonqTxtView::slotSearch()
{
  m_pSearchDialog = new KonqSearchDialog( this );
  
  QObject::connect( m_pSearchDialog, SIGNAL( findFirst( const QString &, bool, bool ) ),
                    this, SLOT( slotFindFirst( const QString &, bool, bool ) ) );
  QObject::connect( m_pSearchDialog, SIGNAL( findNext( bool, bool ) ),
                    this, SLOT( slotFindNext( bool, bool ) ) );
  
  m_pSearchDialog->exec();
  
  delete m_pSearchDialog;
  m_pSearchDialog = 0L;
}

void KonqTxtView::print()
{
  QPrinter printer;

  SIGNAL_CALL1( "setStatusBarText", i18n( "Printing..." ) );
  
  if ( printer.setup( this ) )
  {
    QPainter painter;
    painter.begin( &printer );
    
    painter.setFont( font() );
    int lineSpacing = painter.fontMetrics().lineSpacing();
    QPaintDeviceMetrics paintDevMetrics( &printer );
    
    int y = 0, i = 0, page = 1;
    const int Margin = 10;
    
    SIGNAL_CALL1( "setStatusBarText" , i18n( "Printing page %1 ..." ).arg( page ) );
    
    for (; i < numLines(); i++ )
    {
      if ( Margin + y > paintDevMetrics.height() - Margin )
      {
	SIGNAL_CALL1( "setStatusBarText" , i18n( "Printing page %1 ..." ).arg( ++page ) );
	printer.newPage();
	y = 0;
      }
    
      painter.drawText( Margin, Margin + y, paintDevMetrics.width(),
                        lineSpacing, ExpandTabs | DontClip,
			textLine( i ) );
			
      y += lineSpacing;
    }
    
    painter.end();
    
  }

  SIGNAL_CALL1( "setStatusBarText", QString() );
}

void KonqTxtView::can( bool &copy, bool &paste, bool &move )
{
  copy = (bool)hasMarkedText();
  paste = false;
  move = false;
}

void KonqTxtView::copySelection()
{
  copy();
}

void KonqTxtView::pasteSelection()
{
  assert( 0 );
}

void KonqTxtView::moveSelection( const QCString & )
{
  assert( 0 );
}

void KonqTxtView::slotFinished( int )
{
  m_jobId = 0;
  SIGNAL_CALL1( "completed", id() );
  setXOffset( m_iXOffset );
  setYOffset( m_iYOffset );
}

void KonqTxtView::slotRedirection( int, const char *url )
{
//  m_strURL = url;
  SIGNAL_CALL2( "setLocationBarURL", id(), QCString(url) );
  m_vMainWindow->setPartCaption( id(), QString(url) );  
}

void KonqTxtView::slotData( int, const char *data, int len )
{
  QByteArray a( len );
  memcpy( a.data(), data, len );
  QString s( a );
  append( s );
}

void KonqTxtView::slotError( int, int err, const char *text )
{
  kioErrorDialog( err, text );
}

void KonqTxtView::slotFindFirst( const QString &_text, bool backwards, bool caseSensitive )
{
  m_strSearchText = _text;
  
  if ( backwards )
  {
    m_iSearchPos = lineLength( numLines() - 1 );
    m_iSearchLine = numLines() - 1;
  }
  else
  {
    m_iSearchPos = 0;
    m_iSearchLine = 0;
  }    
  
  m_bFound = false;
  slotFindNext( backwards, caseSensitive );
}

void KonqTxtView::slotFindNext( bool backwards, bool caseSensitive )
{
  int index;
  
  if ( backwards )
    index = textLine( m_iSearchLine ).findRev( m_strSearchText, m_iSearchPos, caseSensitive );
  else
    index = textLine( m_iSearchLine ).find( m_strSearchText, m_iSearchPos, caseSensitive );
    
  while ( index == -1 )
  {
    if ( backwards )
    {
      m_iSearchLine--;
      if ( m_iSearchLine >= 0 )
        m_iSearchPos = lineLength( m_iSearchLine );
    }
    else
    {
      m_iSearchLine++;
      m_iSearchPos = 0;
    }      
    
    if ( ( backwards && m_iSearchLine < 0 ) ||
         ( !backwards && m_iSearchLine >= numLines() ) )
    {
      if ( !m_bFound )
      {
        KMessageBox::information( m_pSearchDialog , i18n( "Search string not found." ));
        return;      
      }

      if ( backwards )
      {
        m_iSearchPos = lineLength( numLines() - 1 );
        m_iSearchLine = numLines() - 1;
      }
      else
      {
        m_iSearchPos = 0;
        m_iSearchLine = 0;
      }    
      
      m_bFound = false;	
    }

    if ( backwards )
      index = textLine( m_iSearchLine ).findRev( m_strSearchText, m_iSearchPos, caseSensitive );
    else
      index = textLine( m_iSearchLine ).find( m_strSearchText, m_iSearchPos, caseSensitive );
  }

  m_bFound = true;
  setCursorPosition( m_iSearchLine, index, false );
  setCursorPosition( m_iSearchLine, index + m_strSearchText.length(), true );
  update();
  
  if ( backwards )
    m_iSearchPos = index-1;
  else
    m_iSearchPos = index+1;
}

void KonqTxtView::mousePressEvent( QMouseEvent *e )
{
  if ( e->button() == RightButton && m_pMainView )
  {
    KURL u( m_strURL );
    
    mode_t mode = 0;
    if ( u.isLocalFile() )
    {
      struct stat buff;
      if ( stat( u.path(), &buff ) == -1 )
      {
        kioErrorDialog( KIO::ERR_COULD_NOT_STAT, m_strURL );
	return;
      }
      mode = buff.st_mode;
    }
    
    KFileItem item( "textURL" /*whatever*/, mode, u );
    KFileItemList items;
    items.append( &item );
    m_pMainView->popupMenu( QPoint( e->globalX(), e->globalY() ), items );
  }
  else
    QMultiLineEdit::mousePressEvent( e );
}

#include "konq_txtview.moc"
