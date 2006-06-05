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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#ifndef __konq_undo_h__
#define __konq_undo_h__

#include <QObject>
#include <QString>
#include <qstack.h>

#include <kurl.h>
#include <libkonq_export.h>

namespace KIO
{
  class Job;
}

class KonqUndoJob;
class KJob;

struct KonqBasicOperation
{
  typedef QStack<KonqBasicOperation> Stack;

  KonqBasicOperation()
  { m_valid = false; }

  bool m_valid;
  bool m_directory;
  bool m_renamed;
  bool m_link;
  KUrl m_src;
  KUrl m_dst;
  QString m_target;
};

struct KonqCommand
{
  typedef QStack<KonqCommand> Stack;

  enum Type { COPY, MOVE, LINK, MKDIR, TRASH };

  KonqCommand()
  { m_valid = false; }

  bool m_valid;

  Type m_type;
  KonqBasicOperation::Stack m_opStack;
  KUrl::List m_src;
  KUrl m_dst;
};

class KonqCommandRecorder : public QObject
{
  Q_OBJECT
public:
  KonqCommandRecorder( KonqCommand::Type op, const KUrl::List &src, const KUrl &dst, KIO::Job *job );
  virtual ~KonqCommandRecorder();

private Q_SLOTS:
  void slotResult( KJob *job );

  void slotCopyingDone( KIO::Job *, const KUrl &from, const KUrl &to, bool directory, bool renamed );
  void slotCopyingLinkDone( KIO::Job *, const KUrl &from, const QString &target, const KUrl &to );

private:
  class KonqCommandRecorderPrivate;
  KonqCommandRecorderPrivate *d;
};

class LIBKONQ_EXPORT KonqUndoManager : public QObject
{
  Q_OBJECT
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

public Q_SLOTS:
  void undo();

Q_SIGNALS:
  void undoAvailable( bool avail );
  void undoTextChanged( const QString &text );

protected:
  /**
   * @internal
   */
  void stopUndo( bool step );

private Q_SLOTS:
  void push( const KonqCommand &cmd );
  void pop();
  void lock();
  void unlock();

  virtual KonqCommand::Stack get() const;

  void slotResult( KJob *job );

private:
  void undoStep();

  void undoMakingDirectories();
  void undoMovingFiles();
  void undoRemovingFiles();
  void undoRemovingDirectories();

  void broadcastPush( const KonqCommand &cmd );
  void broadcastPop();
  void broadcastLock();
  void broadcastUnlock();

  void addDirToUpdate( const KUrl& url );
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
