
#include "konq_undo.h"

#include <assert.h>

#include <qdatastream.h>

#include <dcopclient.h>

#include <kapp.h>
#include <kdatastream.h>
#include <kdebug.h>
#include <klocale.h>

#include <kio/job.h>

class KonqCommandRecorder::KonqCommandRecorderPrivate
{
public:
  KonqCommandRecorderPrivate()
  {
  }
  ~KonqCommandRecorderPrivate()
  {
  }

  KonqCommand m_cmd;
  KIO::Job *m_job;
};

KonqCommandRecorder::KonqCommandRecorder( KonqCommand::Type op, const KURL::List &src, const KURL &dst, KIO::Job *job )
  : QObject( job, "konqcmdrecorder" )
{
  d = new KonqCommandRecorderPrivate;
  d->m_job = job;
  d->m_cmd.m_type = op;
  d->m_cmd.m_valid = true;
  d->m_cmd.m_src = src;
  d->m_cmd.m_dst = dst;
  connect( job, SIGNAL( result( KIO::Job * ) ),
	   this, SLOT( slotResult( KIO::Job * ) ) );

  connect( job, SIGNAL( copyingDone( KIO::Job *, const KURL &, const KURL & ) ),
	   this, SLOT( slotCopyingDone( KIO::Job *, const KURL &, const KURL & ) ) );
};

KonqCommandRecorder::~KonqCommandRecorder()
{
  delete d;
}

void KonqCommandRecorder::slotResult( KIO::Job *job )
{
  assert( job == d->m_job );
  if ( job->error() )
    return;

  KonqUndoManager::self()->addCommand( d->m_cmd );
}

void KonqCommandRecorder::slotCopyingDone( KIO::Job *, const KURL &from, const KURL &to )
{
  KonqBasicOperation op;
  op.m_valid = true;
  op.m_src = from;
  op.m_dst = to;
  d->m_cmd.m_opStack.prepend( op );
}

KonqUndoManager *KonqUndoManager::s_self = 0;
unsigned long KonqUndoManager::s_refCnt = 0;

class KonqUndoManager::KonqUndoManagerPrivate
{
public:
  KonqUndoManagerPrivate()
  {
  }
  ~KonqUndoManagerPrivate()
  {
  }

  bool m_syncronized;

  KonqCommand::Stack m_commands;
};

KonqUndoManager::KonqUndoManager()
: DCOPObject( "KonqUndoManager" )
{
  assert( kapp->dcopClient()->isRegistered() );
  d = new KonqUndoManagerPrivate;
  d->m_syncronized = initializeFromKDesky();
}

KonqUndoManager::~KonqUndoManager()
{
  delete d;
}

void KonqUndoManager::incRef()
{
  s_refCnt++;
}

void KonqUndoManager::decRef()
{
  s_refCnt--;
  if ( s_refCnt == 0 && s_self )
  {
    delete s_self;
    s_self = 0;
  }
}

KonqUndoManager *KonqUndoManager::self()
{
  if ( !s_self )
  {
    if ( s_refCnt == 0 )
      s_refCnt++; // someone forgot to call incRef
    s_self = new KonqUndoManager;
  }
  return s_self;
}

void KonqUndoManager::addCommand( const KonqCommand &cmd )
{
  broadcastPush( cmd );
}

bool KonqUndoManager::undoAvailable() const
{
  return ( d->m_commands.count() > 0 );
}

QString KonqUndoManager::undoText() const
{
  if ( d->m_commands.count() == 0 ) 
    return i18n( "Und&o" );
  
  if ( d->m_commands.top().m_type == KonqCommand::COPY )
    return i18n( "Und&o : Copy" );
  else
    return i18n( "Und&o : Move" );
}

void KonqUndoManager::undo()
{
  KonqCommand cmd = d->m_commands.top();

  broadcastPop();

  assert( cmd.m_valid );

  while ( cmd.m_opStack.count() > 0 )
  {
    KonqBasicOperation op = cmd.m_opStack.pop();
    assert( op.m_valid );
    if ( cmd.m_type == KonqCommand::MOVE )
      kdDebug() << "moving " << op.m_dst.prettyURL() << " back to " << op.m_src.prettyURL() << endl;
    else
      kdDebug() << "copying " << op.m_dst.prettyURL() << " back to " << op.m_src.prettyURL() << endl;
  }
}

void KonqUndoManager::push( const KonqCommand &cmd )
{
 kdDebug() << kapp->dcopClient()->appId()
	   << " : KonqUndoManager::push! " << d->m_commands.count() + 1
	   << " -- sync " << d->m_syncronized << endl;
 
  d->m_commands.push( cmd );
  emit undoAvailable( true );
  emit undoTextChanged( undoText() );
}

void KonqUndoManager::pop()
{
  kdDebug() << kapp->dcopClient()->appId()
	    << " : KonqUndoManager::pop!"
	    << " -- sync " << d->m_syncronized << endl;
  d->m_commands.pop();
  emit undoAvailable( undoAvailable() );
  emit undoTextChanged( undoText() );
}

KonqCommand::Stack KonqUndoManager::get() const
{
  return d->m_commands;
}

void KonqUndoManager::broadcastPush( const KonqCommand &cmd )
{
  if ( !d->m_syncronized )
  {
    push( cmd );
    return;
  }
  QByteArray data;
  QDataStream stream( data, IO_WriteOnly );
  stream << cmd;
  kapp->dcopClient()->send( "kdesktop", "KonqUndoManager", "push(KonqCommand)", data );
  kapp->dcopClient()->send( "konqueror*", "KonqUndoManager", "push(KonqCommand)", data );
}

void KonqUndoManager::broadcastPop()
{
  if ( !d->m_syncronized )
  {
    pop();
    return;
  }
  QByteArray data;
  kapp->dcopClient()->send( "kdesktop", "KonqUndoManager", "pop()", data );
  kapp->dcopClient()->send( "konqueror*", "KonqUndoManager", "pop()", data );
}

bool KonqUndoManager::initializeFromKDesky()
{
  DCOPClient *client = kapp->dcopClient();

  if ( client->appId() == "kdesktop" ) // we are master :)
    return true;

  if ( !client->isApplicationRegistered( "kdesktop" ) )
    return false;

  QByteArray data;
  QCString replyType;
  QByteArray replyData;

  bool res = client->call( "kdesktop", "KonqUndoManager", "get()", data, replyType, replyData );
  if ( !res )
    return false;

  QDataStream stream( replyData, IO_ReadOnly );
  stream >> d->m_commands;
  return true;
}

QDataStream &operator<<( QDataStream &stream, const KonqBasicOperation &op )
{
  stream << op.m_valid << op.m_src << op.m_dst;
  return stream;
}
QDataStream &operator>>( QDataStream &stream, KonqBasicOperation &op )
{
  stream >> op.m_valid >> op.m_src >> op.m_dst;
  return stream;
}

QDataStream &operator<<( QDataStream &stream, const KonqCommand &cmd )
{
  stream << cmd.m_valid << (Q_INT8)cmd.m_type << cmd.m_opStack << cmd.m_src << cmd.m_dst;
  return stream;
}

QDataStream &operator>>( QDataStream &stream, KonqCommand &cmd )
{
  Q_INT8 type;
  stream >> cmd.m_valid >> type >> cmd.m_opStack >> cmd.m_src >> cmd.m_dst;
  cmd.m_type = static_cast<KonqCommand::Type>( type );
  return stream;
}

#include "konq_undo.moc"
