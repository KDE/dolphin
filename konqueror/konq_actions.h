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

#ifndef __konq_actions_h__
#define __konq_actions_h__ $Id$

#include <qguardedptr.h>

#include <kaction.h>
#include <qlist.h>

class KBookmarkOwner;
class KComboBox;
class HistoryEntry;
class QLabel;
class QPopupMenu;

class KonqComboAction : public KAction
{
  Q_OBJECT
public:
    KonqComboAction( const QString& text, int accel, const QObject *receiver, const char *member, QObject* parent, const char* name );

    virtual int plug( QWidget *w, int index = -1 );

    virtual void unplug( QWidget *w );

    QGuardedPtr<KComboBox> combo() { return m_combo; }

signals:
    void plugged();

private:
    QGuardedPtr<KComboBox> m_combo;  // Is this a BCI ?? Although we inherit from QComboBox?
    QStringList m_items;
    const QObject *m_receiver;
    const char *m_member;
};

/**
 * This is a one-direction history action (such as : the list up, back or forward)
 */
class KonqHistoryAction : public KAction
{
  Q_OBJECT
public:
    /**
     * Only constructor - because we need an icon, since this action only makes
     * sense in a toolbar (as well as menubar)
     */
    KonqHistoryAction( const QString& text, const QString& icon, int accel = 0,
	     QObject* parent = 0, const char* name = 0 );

    virtual ~KonqHistoryAction();

    virtual int plug( QWidget *widget, int index = -1 );
    virtual void unplug( QWidget *widget );

    // Used by KonqHistoryAction and KonqBidiHistoryAction
    static void fillHistoryPopup( const QList<HistoryEntry> &history,
                           QPopupMenu * popup,
                           bool onlyBack = false,
                           bool onlyForward = false,
                           bool checkCurrentItem = false,
                           uint startPos = 0 );

    virtual void setEnabled( bool b );

    virtual void setIconSet( const QIconSet& iconSet );

    QPopupMenu *popupMenu();

signals:
    // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
    void activated( int );

private:
    QPopupMenu *m_popup; // hack
};

/**
 * Plug this action into a menu to get a bidirectional history
 * (both back and forward, including current location)
 */
class KonqBidiHistoryAction : public KAction
{
  Q_OBJECT
public:
    /**
     * Only constructor - because we need an icon, since this action only makes
     * sense in a toolbar (as well as menubar)
     */
    KonqBidiHistoryAction( QObject* parent = 0, const char* name = 0 );

    virtual ~KonqBidiHistoryAction() {};

    virtual int plug( QWidget *widget, int index = -1 );
    //virtual void unplug( QWidget *widget );

    void fillGoMenu( const QList<HistoryEntry> &history );

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
    QPopupMenu *m_goMenu; // hack
};

/////

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
