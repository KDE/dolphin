/* This file is part of the KDE project
   Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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

#ifndef __konq_actions_h__
#define __konq_actions_h__ $Id$

#include <qguardedptr.h>

#include <kaction.h>

class KBookmarkOwner;
class KBookmarkBar;
class QComboBox;

class KonqComboAction : public QAction
{
  Q_OBJECT
public:
    KonqComboAction( const QString& text, int accel, const QObject *receiver, const char *member, QObject* parent, const char* name );

    virtual int plug( QWidget *w );

    virtual void unplug( QWidget *w );

    QGuardedPtr<QComboBox> combo() { return m_combo; }

signals:
    void plugged();

private:
    QGuardedPtr<QComboBox> m_combo;
    QStringList m_items;
    const QObject *m_receiver;
    const char *m_member;
};

class KonqHistoryAction : public KAction
{
  Q_OBJECT
public:
    KonqHistoryAction( const QString& text, int accel = 0, QObject* parent = 0, const char* name = 0 );
    KonqHistoryAction( const QString& text, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqHistoryAction( const QString& text, const QIconSet& pix, int accel = 0,
	     QObject* parent = 0, const char* name = 0 );
    KonqHistoryAction( const QString& text, const QIconSet& pix, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqHistoryAction( QObject* parent = 0, const char* name = 0 );

    virtual ~KonqHistoryAction();

    virtual int plug( QWidget *widget );
    virtual void unplug( QWidget *widget );

    virtual void setEnabled( bool b );

    virtual void setIconSet( const QIconSet& iconSet );

    QPopupMenu *popupMenu();

private:
    QPopupMenu *m_popup;
};

class KonqBookmarkBar : public QAction
{
  Q_OBJECT
public:
    KonqBookmarkBar( const QString& text, int accel, KBookmarkOwner *owner, QObject* parent, const char* name );

    virtual int plug( QWidget *w );

    virtual void unplug( QWidget *w );

private:
    KBookmarkOwner *m_pOwner;
    KBookmarkBar *m_bookmarkBar;
};

#endif
