
#include "konq_txtview.h"

#include <sys/stat.h>
#include <unistd.h>

#include <kio_job.h>
#include <kio_cache.h>
#include <kio_error.h>
#include <kurl.h>

KonqTxtView::KonqTxtView()
{
  ADD_INTERFACE( "IDL:Konqueror/TxtView:1.0" );
  
  setWidget( this );
  
  QWidget::show();
  QWidget::setFocusPolicy( StrongFocus );
  
  setReadOnly( true );
  
  m_jobId = 0;
}

KonqTxtView::~KonqTxtView()
{
  stop();
}

bool KonqTxtView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL( eventURL );
  cerr << "bool KonqTxtView::mappingOpenURL( Konqueror::EventOpenURL eventURL )" << endl;
  stop();
  
  CachedKIOJob *job = new CachedKIOJob;
  
  job->setGUImode( KIOJob::NONE );
  
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigRedirection( int, const char * ) ), this, SLOT( slotRedirection( int, const char * ) ) );
  QObject::connect( job, SIGNAL( sigData( int, const char *, int ) ), this, SLOT( slotData( int, const char *, int ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int, int, const char * ) ), this, SLOT( slotError( int, int, const char * ) ) );
  
  m_jobId = job->id();
  
  clear();
  
  m_strURL = eventURL.url;
  job->get( eventURL.url, (bool)eventURL.reload );
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)eventURL.url, 0 ) );
  return true;
}

bool KonqTxtView::mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu )
{
  //TODO
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

  //TODO: save text into data
  
  return entry;
}

void KonqTxtView::restoreState( const Konqueror::View::HistoryEntry &history )
{
  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( history.url );
  eventURL.reload = (CORBA::Boolean)false;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;
  EMIT_EVENT( this, Konqueror::eventOpenURL, eventURL  );
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
  m_vMainWindow->setPartCaption( id(), url );  
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
  if ( e->button() == RightButton )
  {
    Konqueror::View::MenuPopupRequest popupRequest;
    KURL u( m_strURL );
    popupRequest.urls.length( 1 );
    popupRequest.urls[0] = m_strURL.ascii();
    
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
    
    popupRequest.x = e->globalX();
    popupRequest.y = e->globalY();
    popupRequest.mode = mode;
    popupRequest.isLocalFile = (CORBA::Boolean)u.isLocalFile();
    SIGNAL_CALL1( "popupMenu", popupRequest );
  }
}

#include "konq_txtview.moc"