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

KonqTxtView::KonqTxtView( KonqMainView *mainView )
{
  kdebug(KDEBUG_INFO, 1202, "+KonqTxtView");
  ADD_INTERFACE( "IDL:Konqueror/TxtView:1.0" );
  ADD_INTERFACE( "IDL:Konqueror/PrintingExtension:1.0" );
  
  setWidget( this );
  
  QWidget::setFocusPolicy( StrongFocus );
  
  setReadOnly( true );
  
  m_pMainView = mainView;
  m_jobId = 0;
  setAcceptDrops( true );
}

KonqTxtView::~KonqTxtView()
{
  kdebug(KDEBUG_INFO, 1202, "-KonqTxtView");
  stop();
}

bool KonqTxtView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL( eventURL );
 kdebug(0,1202,"bool KonqTxtView::mappingOpenURL( Konqueror::EventOpenURL eventURL )");
  stop();
  
  CachedKIOJob *job = new CachedKIOJob;
  
  job->setGUImode( KIOJob::NONE );
  
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigRedirection( int, const char * ) ), this, SLOT( slotRedirection( int, const char * ) ) );
  QObject::connect( job, SIGNAL( sigData( int, const char *, int ) ), this, SLOT( slotData( int, const char *, int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char * ) ), this, SLOT( slotError( int, int, const char * ) ) );
  
  m_jobId = job->id();
  
  clear();
  
  m_strURL = eventURL.url;
  job->get( eventURL.url, (bool)eventURL.reload );
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)eventURL.url, 0 ) );
  return true;
}

bool KonqTxtView::mappingFillMenuView( Konqueror::View::EventFillMenu /*viewMenu*/ )
{
  //TODO
  return true;
}

bool KonqTxtView::mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu )
{
//HACK
#define MEDIT_BASE_ID 1523
#define MEDIT_SELECTALL_ID MEDIT_BASE_ID+1

  if ( !CORBA::is_nil( editMenu.menu ) )
  {
    if ( editMenu.create )
    {
      CORBA::WString_var txt = Q2C( i18n( "Select &All" ) );
      editMenu.menu->insertItem4( txt, this, "slotSelectAll", 0, MEDIT_SELECTALL_ID, -1 );
    }
    else
    {
      editMenu.menu->removeItem( MEDIT_SELECTALL_ID );
    }
  }

  return true;
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

Konqueror::View::HistoryEntry *KonqTxtView::saveState()
{
  Konqueror::View::HistoryEntry *entry = new Konqueror::View::HistoryEntry;
  
  entry->url = CORBA::string_dup( m_strURL.ascii() );

  CORBA::WString_var txt = Q2C( text() );
  
  entry->data <<= CORBA::Any::from_wstring( txt, 0 );
  
  return entry;
}

void KonqTxtView::restoreState( const Konqueror::View::HistoryEntry &history )
{
  m_strURL = history.url.in();
  
  CORBA::WChar *txt;
  history.data >>= CORBA::Any::to_wstring( txt, 0 );
  
  QString s = C2Q( txt );
  setText( s );
  
  SIGNAL_CALL2( "started", id(), history.url );
  SIGNAL_CALL2( "setLocationBarURL", id(), CORBA::Any::from_string( (char*)history.url.in(), 0 ) );
  SIGNAL_CALL1( "completed", id() );
}

void KonqTxtView::slotSelectAll()
{
  QMultiLineEdit::selectAll();
}

void KonqTxtView::print()
{
  QPrinter printer;
  CORBA::WString_var text;

  text = Q2C( i18n( "Printing..." ) );
  SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( text.out(), 0 ) );
  
  if ( printer.setup( this ) )
  {
    QPainter painter;
    painter.begin( &printer );
    
    painter.setFont( font() );
    int lineSpacing = painter.fontMetrics().lineSpacing();
    QPaintDeviceMetrics paintDevMetrics( &printer );
    
    int y = 0, i = 0, page = 1;
    const int Margin = 10;
    
    text = Q2C( i18n( "Printing page %1 ..." ).arg( page ) );
    SIGNAL_CALL1( "setStatusBarText" , CORBA::Any::from_wstring( text.out(), 0 ) );
    
    for (; i < numLines(); i++ )
    {
      if ( Margin + y > paintDevMetrics.height() - Margin )
      {
        text = Q2C( i18n( "Printing page %1 ..." ).arg( ++page ) );
	SIGNAL_CALL1( "setStatusBarText" , CORBA::Any::from_wstring( text.out(), 0 ) );
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

  SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( (CORBA::WChar*)0L, 0 ) );
}

void KonqTxtView::slotFinished( int )
{
  m_jobId = 0;
  SIGNAL_CALL1( "completed", id() );
}

void KonqTxtView::slotRedirection( int, const char *url )
{
//  m_strURL = url;
  SIGNAL_CALL2( "setLocationBarURL", id(), CORBA::Any::from_string( (char *)url, 0 ) );
  CORBA::WString_var caption = Q2C( QString( url ) );  
  m_vMainWindow->setPartCaption( id(), caption );  
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

void KonqTxtView::mousePressEvent( QMouseEvent *e )
{
  if ( e->button() == RightButton && m_pMainView )
  {
    QStringList lstPopupURLs;
    lstPopupURLs.append( m_strURL );
    
    KURL u( m_strURL );
    
    mode_t mode = 0;
    if ( u.isLocalFile() )
    {
      struct stat buff;
      if ( stat( u.path(), &buff ) == -1 )
      {
        kioErrorDialog( ERR_COULD_NOT_STAT, m_strURL );
	return;
      }
      mode = buff.st_mode;
    }
    
    m_pMainView->popupMenu( QPoint( e->globalX(), e->globalY() ),
                            lstPopupURLs, mode );
  }
  else
    QMultiLineEdit::mousePressEvent( e );
}

#include "konq_txtview.moc"
