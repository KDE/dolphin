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

#include "konq_textview.h"
#include "konq_factory.h"
#include "konq_searchdia.h"
#include "konq_progressproxy.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <qdragobject.h>
#include <qprinter.h>
#include <qpainter.h>
#include <qpaintdevicemetrics.h>
#include <qmultilineedit.h>

#include <kaction.h>
#include <kio_job.h>
#include <kio_cache.h>
#include <kio_error.h>
#include <kurl.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kfiledialog.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <konqdefaults.h>
#include <kfileitem.h>
#include <klibloader.h>
#include <kglobal.h>

class KonqTextViewFactory : public KLibFactory
{
public:
  KonqTextViewFactory() 
  {
    KonqFactory::instanceRef();
  }
  virtual ~KonqTextViewFactory()
  {
    KonqFactory::instanceUnref();
  }

  virtual QObject* create( QObject*, const char*, const char*, const QStringList & )
  {
    QObject *obj = new KonqTextView;
    emit objectCreated( obj );
    return obj;
  }
  
};

extern "C"
{
  void *init_libkonqtextview()
  {
    return new KonqTextViewFactory;
  }
};

KonqTextPrintingExtension::KonqTextPrintingExtension( KonqTextView *textView )
: PrintingExtension( textView, "KonqTextPrintingExtension" )
{
  m_textView = textView;
}

void KonqTextPrintingExtension::print()
{
  QPrinter printer;

  emit m_textView->setStatusBarText( i18n( "Printing..." ) );
  
  KMultiLineEdit *edit = m_textView->multiLineEdit();
  
  if ( printer.setup( edit ) )
  {
    QPainter painter;
    painter.begin( &printer );
    
    painter.setFont( edit->font() );
    int lineSpacing = painter.fontMetrics().lineSpacing();
    QPaintDeviceMetrics paintDevMetrics( &printer );
    
    int y = 0, i = 0, page = 1;
    const int Margin = 10;
    
    emit m_textView->setStatusBarText( i18n( "Printing page %1 ..." ).arg( page ) );
    
    for (; i < edit->numLines(); i++ )
    {
      if ( Margin + y > paintDevMetrics.height() - Margin )
      {
	emit m_textView->setStatusBarText( i18n( "Printing page %1 ..." ).arg( ++page ) );
	printer.newPage();
	y = 0;
      }
    
      painter.drawText( Margin, Margin + y, paintDevMetrics.width(),
                        lineSpacing, ExpandTabs | DontClip,
			edit->textLine( i ) );
			
      y += lineSpacing;
    }
    
    painter.end();
    
  }

  emit m_textView->setStatusBarText( QString::null );
}

KonqTextEditExtension::KonqTextEditExtension( KonqTextView *textView )
 : EditExtension( textView, "KonqTextEditExtension" )
{
  m_textView = textView;
}

void KonqTextEditExtension::can( bool &cut, bool &copy, bool &paste, bool &move )
{
  cut = false;
  copy = m_textView->multiLineEdit()->textMarked();
  paste = false;
  move = false;
}

void KonqTextEditExtension::copySelection()
{
  m_textView->multiLineEdit()->copy();
}

KonqTextView::KonqTextView()
{
  kdebug(KDEBUG_INFO, 1202, "+KonqTextView");

  (void)new KonqTextPrintingExtension( this );
  (void)new KonqTextEditExtension( this );

  m_pEdit = new KMultiLineEdit( this );
  
  m_pEdit->setReadOnly( true );

  m_pEdit->installEventFilter( this );
  
  m_jobId = 0;
  m_bFixedFont = false;
  m_pEdit->setFont( KGlobal::generalFont() );
  
  m_paSelectAll = new KAction( i18n( "Select &All" ), 0, this, SLOT( slotSelectAll() ), this );
  m_paEdit = new KAction( i18n( "Launch &Editor" ), QIconSet( BarIcon( "pencil", KonqFactory::instance() ) ), 0, this, SLOT( slotEdit() ), this );
  m_paSearch = new KAction( i18n( "Search..." ), QIconSet( BarIcon( "search", KonqFactory::instance() ) ), 0, this, SLOT( slotSearch() ), this );
  m_ptaFixedFont = new KToggleAction( i18n( "Use Fixed Font" ), 0, this, SLOT( slotFixedFont() ), this );
  m_paSaveDocument = new KAction( i18n( "Save Document As..." ), 0, this, SLOT( slotSaveDocumentAs() ), this ); 

  actions()->append( BrowserView::ViewAction( m_paSelectAll, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paEdit, BrowserView::MenuEdit | BrowserView::ToolBar ) );
  actions()->append( BrowserView::ViewAction( m_paSearch, BrowserView::MenuEdit | BrowserView::ToolBar ) );
  actions()->append( BrowserView::ViewAction( m_ptaFixedFont, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paSaveDocument, BrowserView::MenuView ) );
}

KonqTextView::~KonqTextView()
{
  kdebug(KDEBUG_INFO, 1202, "-KonqTextView");
  stop();
}

void KonqTextView::openURL( const QString &url, bool reload,
                           int xOffset, int yOffset )
{
  kdebug(0,1202,"bool KonqTextView::mappingOpenURL( Konqueror::EventOpenURL eventURL )");
  stop();
  
  KIOCachedJob *job = new KIOCachedJob;
  
  job->setGUImode( KIOJob::NONE );
  
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigRedirection( int, const char * ) ), this, SLOT( slotRedirection( int, const char * ) ) );
  QObject::connect( job, SIGNAL( sigData( int, const char *, int ) ), this, SLOT( slotData( int, const char *, int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char * ) ), this, SLOT( slotError( int, int, const char * ) ) );
  
  (void)new KonqProgressProxy( this, job );
  
  m_jobId = job->id();
  
  m_pEdit->clear();
  
  m_iXOffset = xOffset;
  m_iYOffset = yOffset;
  
  m_strURL = url;
  job->get( url, reload );
  emit started();
  
//  setCaptionFromURL( m_strURL );
}

QString KonqTextView::url()
{
  return m_strURL;
}

/*
bool KonqTextView::mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu )
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

bool KonqTextView::mappingFillMenuEdit( Browser::View::EventFillMenu_ptr editMenu )
{
  if ( !CORBA::is_nil( editMenu ) )
  {
    editMenu->insertItem4( i18n( "Select &All" ), this, "slotSelectAll", 0, -1, -1 );
    editMenu->insertItem4( i18n( "Launch &Editor" ), this, "slotEdit", 0, -1, -1 );
    editMenu->insertItem4( i18n( "Search..." ), this, "slotSearch", 0, -1, -1 );
  }

  return true;
}

bool KonqTextView::mappingFillToolBar( Browser::View::EventFillToolBar toolBar )
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
*/

int KonqTextView::xOffset()
{
  return m_pEdit->xScrollOffset();
}

int KonqTextView::yOffset()
{
  return m_pEdit->yScrollOffset();
}

void KonqTextView::stop()
{
  if ( m_jobId )
  {
    KIOJob *job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
}

bool KonqTextView::eventFilter( QObject *o, QEvent *e )
{
  if ( o == m_pEdit && e->type() == QEvent::MouseButtonPress &&
       ((QMouseEvent *)e)->button() == RightButton )
  {
    KURL u( m_strURL );
    
    mode_t mode = 0;
    
    if ( u.isLocalFile() )
    {
      struct stat buff;
      if ( stat( u.path(), &buff ) == -1 )
      {
        kioErrorDialog( KIO::ERR_COULD_NOT_STAT, m_strURL );
	return true;
      }
      mode = buff.st_mode;
    }
    
    KFileItem item( mode, u );
    KFileItemList items;
    items.append( &item );
    emit popupMenu( QCursor::pos(), items );
    return true;
  }
  return false;
}

void KonqTextView::slotSelectAll()
{
  m_pEdit->selectAll();
}

void KonqTextView::slotEdit()
{
  KConfig *config = KonqFactory::instance()->config();
  config->setGroup( "Misc Defaults" );
  QString editor = config->readEntry( "Editor", DEFAULT_EDITOR );
    
  QCString cmd;
  cmd.sprintf( "%s %s &", editor.ascii(), m_strURL.ascii() );
  system( cmd.data() );
}

void KonqTextView::slotFixedFont()
{
  m_bFixedFont = !m_bFixedFont;
//  if ( !CORBA::is_nil( m_vMenuView ) )
//    m_vMenuView->setItemChecked( m_idFixedFont, m_bFixedFont );
    
  if ( m_bFixedFont )
    m_pEdit->setFont( KGlobal::fixedFont() );
  else
    m_pEdit->setFont( KGlobal::generalFont() );
}

void KonqTextView::slotSearch()
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

void KonqTextView::slotSaveDocumentAs()
{
  KURL u( m_strURL );

  KFileDialog *dlg = new KFileDialog( QString::null, "*\n*.txt",
				      this, "filedialog", true, false );

  dlg->setCaption( i18n( "Save as" ) );
  dlg->setSelection( dlg->dirPath() + u.filename() );
  if ( dlg->exec() )
  {
    KURL dest = dlg->selectedFileURL();
    KIOJob *job = new KIOJob;
    job->copy( m_strURL, dest.url() );
  }
  delete dlg;
}

/*
void KonqTextView::can( bool &copy, bool &paste, bool &move )
{
  copy = (bool)hasMarkedText();
  paste = false;
  move = false;
}

void KonqTextView::copySelection()
{
  copy();
}

void KonqTextView::pasteSelection()
{
  assert( 0 );
}

void KonqTextView::moveSelection( const QCString & )
{
  assert( 0 );
}
*/
void KonqTextView::slotFinished( int )
{
  m_jobId = 0;
  emit completed();
  m_pEdit->setScrollOffset( m_iXOffset, m_iYOffset );
}

void KonqTextView::slotRedirection( int, const char *url )
{
//  m_strURL = url;
  
  QString decodedURL = url;
  KURL::decode( decodedURL );
  emit setLocationBarURL( decodedURL );
  
//  m_vMainWindow->setPartCaption( id(), QString(url) );  
}

void KonqTextView::slotData( int, const char *data, int len )
{
  m_pEdit->append( QString::fromUtf8( data, len ) );
}

void KonqTextView::slotError( int, int err, const char *text )
{
  kioErrorDialog( err, text );
}

void KonqTextView::slotFindFirst( const QString &_text, bool backwards, bool caseSensitive )
{
  m_strSearchText = _text;
  
  if ( backwards )
  {
    m_iSearchPos = m_pEdit->textLine( m_pEdit->numLines() - 1 ).length();
    m_iSearchLine = m_pEdit->numLines() - 1;
  }
  else
  {
    m_iSearchPos = 0;
    m_iSearchLine = 0;
  }    
  
  m_bFound = false;
  slotFindNext( backwards, caseSensitive );
}

void KonqTextView::slotFindNext( bool backwards, bool caseSensitive )
{
  int index;
  
  if ( backwards )
    index = m_pEdit->textLine( m_iSearchLine ).findRev( m_strSearchText, m_iSearchPos, caseSensitive );
  else
    index = m_pEdit->textLine( m_iSearchLine ).find( m_strSearchText, m_iSearchPos, caseSensitive );
    
  while ( index == -1 )
  {
    if ( backwards )
    {
      m_iSearchLine--;
      if ( m_iSearchLine >= 0 )
        m_iSearchPos = m_pEdit->textLine( m_iSearchLine ).length();
    }
    else
    {
      m_iSearchLine++;
      m_iSearchPos = 0;
    }      
    
    if ( ( backwards && m_iSearchLine < 0 ) ||
         ( !backwards && m_iSearchLine >= m_pEdit->numLines() ) )
    {
      if ( !m_bFound )
      {
        KMessageBox::information( m_pSearchDialog , i18n( "Search string not found." ));
        return;      
      }

      if ( backwards )
      {
        m_iSearchPos = m_pEdit->textLine( m_pEdit->numLines() - 1 ).length();
        m_iSearchLine = m_pEdit->numLines() - 1;
      }
      else
      {
        m_iSearchPos = 0;
        m_iSearchLine = 0;
      }    
      
      m_bFound = false;	
    }

    if ( backwards )
      index = m_pEdit->textLine( m_iSearchLine ).findRev( m_strSearchText, m_iSearchPos, caseSensitive );
    else
      index = m_pEdit->textLine( m_iSearchLine ).find( m_strSearchText, m_iSearchPos, caseSensitive );
  }

  m_bFound = true;
  m_pEdit->setCursorPosition( m_iSearchLine, index, false );
  m_pEdit->setCursorPosition( m_iSearchLine, index + m_strSearchText.length(), true );
  m_pEdit->update();
  
  if ( backwards )
    m_iSearchPos = index-1;
  else
    m_iSearchPos = index+1;
}

void KonqTextView::resizeEvent( QResizeEvent * )
{
  m_pEdit->setGeometry( 0, 0, width(), height() );
}

/*
void KonqTextView::mousePressEvent( QMouseEvent *e )
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
        kioErrorDialog( ERR_COULD_NOT_STAT, m_strURL );
	return;
      }
      mode = buff.st_mode;
    }
    
    KFileItem item( mode, u );
    KFileItemList items;
    items.append( &item );
    m_pMainView->popupMenu( QPoint( e->globalX(), e->globalY() ), items );
  }
  else
    QMultiLineEdit::mousePressEvent( e );
}
*/

#include "konq_textview.moc"
