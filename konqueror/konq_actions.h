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
#include <qlist.h>

class KBookmarkOwner;
class QComboBox;
class HistoryEntry;
class QLabel;

class KonqComboAction : public KAction
{
  Q_OBJECT
public:
    KonqComboAction( const QString& text, int accel, const QObject *receiver, const char *member, QObject* parent, const char* name );

    virtual int plug( QWidget *w, int index = -1 );

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
    KonqHistoryAction( const QString& text, const QString& icon, int accel = 0,
	     QObject* parent = 0, const char* name = 0 );
    KonqHistoryAction( const QString& text, const QIconSet& pix, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqHistoryAction( QObject* parent = 0, const char* name = 0 );

    virtual ~KonqHistoryAction();

    virtual int plug( QWidget *widget, int index = -1 );
    virtual void unplug( QWidget *widget );

    // If popup is 0, uses m_popup - useful for the toolbar buttons
    //   (clear it first)
    // Otherwise uses popup - useful for existing toolbar menu
    void fillHistoryPopup( const QList<HistoryEntry> &history,
                           QPopupMenu * popup = 0L,
                           bool onlyBack = false,
                           bool onlyForward = false,
                           bool checkCurrentItem = false,
                           uint startPos = 0 );

    void fillGoMenu( const QList<HistoryEntry> &history );

    virtual void setEnabled( bool b );

    virtual void setIconSet( const QIconSet& iconSet );

    QPopupMenu *popupMenu();

protected slots:
    void slotActivated( int );

signals:
    void menuAboutToShow();
    // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
    void activated( int );

private:
    uint m_firstIndex; // first index in the Go menu
    int m_startPos;
    int m_currentPos; // == history.at()
    bool m_goMenuDone; // hack 1 : only do this once
    QPopupMenu *m_popup; // hack 2 :)
    QPopupMenu *m_goMenu; // hack 3 :)
};

class KonqLogoAction : public KAction
{
  Q_OBJECT
public:
    KonqLogoAction( const QString& text, int accel = 0, QObject* parent = 0, const char* name = 0 );
    KonqLogoAction( const QString& text, int accel,
	            QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqLogoAction( const QString& text, const QIconSet& pix, int accel = 0,
	            QObject* parent = 0, const char* name = 0 );
    KonqLogoAction( const QString& text, const QIconSet& pix, int accel,
	            QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqLogoAction( const QStringList& icons, QObject* receiver,
                    const char* slot, QObject* parent, const char* name = 0 );

    KonqLogoAction( QObject* parent = 0, const char* name = 0 );

    virtual int plug( QWidget *widget, int index = -1 );

    void start();
    void stop();

private:
    QStringList iconList;
};

class KonqLabelAction : public KAction
{
  Q_OBJECT
public:
  KonqLabelAction( const QString &text, QObject *parent = 0, const char *name = 0 );

  virtual int plug( QWidget *widget, int index = -1 );
  virtual void unplug( QWidget *widget );
};

#endif
