/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
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

class KonqUndoJob;

struct KonqBasicOperation
{
  typedef QValueStack<KonqBasicOperation> Stack;

  KonqBasicOperation()
  { m_valid = false; }

  bool m_valid;
  bool m_directory;
  bool m_renamed;
  bool m_link;
  KURL m_src;
  KURL m_dst;
  QString m_target;
};

struct KonqCommand
{
  typedef QValueStack<KonqCommand> Stack;

  enum Type { COPY, MOVE, LINK };

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

  void slotCopyingDone( KIO::Job *, const KURL &from, const KURL &to, bool directory, bool renamed );
  void slotCopyingLinkDone( KIO::Job *, const KURL &from, const QString &target, const KURL &to );

private:
  class KonqCommandRecorderPrivate;
  KonqCommandRecorderPrivate *d;
};

class KonqUndoManager : public QObject, public DCOPObject
{
  Q_OBJECT
  K_DCOP
  friend class KonqUndoJob;
public:
  enum UndoState { MAKINGDIRS, MOVINGFILES, REMOVINGDIRS, REMOVINGFILES };

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

protected:
  /** 
   * @internal
   */
  void stopUndo( bool step );

private:
k_dcop:
  virtual ASYNC push( const KonqCommand &cmd );
  virtual ASYNC pop();
  virtual ASYNC lock();
  virtual ASYNC unlock();

  virtual KonqCommand::Stack get() const;

private slots:
  void slotResult( KIO::Job *job );

private:
  void undoStep();

  void broadcastPush( const KonqCommand &cmd );
  void broadcastPop();
  void broadcastLock();
  void broadcastUnlock();

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
