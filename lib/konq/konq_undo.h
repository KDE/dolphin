#ifndef __konq_undo_h__
#define __konq_undo_h__

#include <qobject.h>
#include <qstring.h>
#include <qvaluestack.h>

#include <dcopobject.h>

#include <kurl.h>

namespace KIO
{
  class Job;
};

struct KonqBasicOperation
{
  typedef QValueStack<KonqBasicOperation> Stack;

  KonqBasicOperation()
  { m_valid = false; }

  bool m_valid;
  KURL m_src;
  KURL m_dst;
};

struct KonqCommand
{
  typedef QValueStack<KonqCommand> Stack;

  enum Type { COPY, MOVE };

  KonqCommand()
  { m_valid = false; }

  bool m_valid;

  Type m_type;
  KonqBasicOperation::Stack m_opStack;
  KURL::List m_src;
  KURL m_dst;
};

class KonqCommandRecorder : public QObject
{
  Q_OBJECT
public:
  KonqCommandRecorder( KonqCommand::Type op, const KURL::List &src, const KURL &dst, KIO::Job *job );
  virtual ~KonqCommandRecorder();

private slots:
  void slotResult( KIO::Job *job );

  void slotCopyingDone( KIO::Job *, const KURL &from, const KURL &to );

private:
  class KonqCommandRecorderPrivate;
  KonqCommandRecorderPrivate *d;
};

class KonqUndoManager : public QObject, public DCOPObject
{
  Q_OBJECT
  K_DCOP
public:
  KonqUndoManager();
  virtual ~KonqUndoManager();

  static void incRef();
  static void decRef();
  static KonqUndoManager *self();

  void addCommand( const KonqCommand &cmd );

  bool undoAvailable() const;
  QString undoText() const;

public slots:
  void undo();

signals:
  void undoAvailable( bool avail );
  void undoTextChanged( const QString &text );

private:
k_dcop:
  virtual ASYNC push( const KonqCommand &cmd );
  virtual ASYNC pop();

  virtual KonqCommand::Stack get() const;

private:
  void broadcastPush( const KonqCommand &cmd );
  void broadcastPop();

  bool initializeFromKDesky();

  class KonqUndoManagerPrivate;
  KonqUndoManagerPrivate *d;
  static KonqUndoManager *s_self;
  static unsigned long s_refCnt;
};

QDataStream &operator<<( QDataStream &stream, const KonqBasicOperation &op );
QDataStream &operator>>( QDataStream &stream, KonqBasicOperation &op );

QDataStream &operator<<( QDataStream &stream, const KonqCommand &cmd );
QDataStream &operator>>( QDataStream &stream, KonqCommand &cmd );

#endif
