/* This file is part of the KDE project
   Copyright (C) 2003 Simon Hausmann <hausmann@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __delayedinitializer_h__
#define __delayedinitializer_h__

#include <QObject>
//Added by qt3to4:
#include <QEvent>

class DelayedInitializer : public QObject
{
    Q_OBJECT
public:
    DelayedInitializer( int eventType, QObject *parent, const char *name = 0 );

    virtual bool eventFilter( QObject *receiver, QEvent *event );

Q_SIGNALS:
    void initialize();

private Q_SLOTS:
    void slotInitialize();
private:
    int m_eventType;
    bool m_signalEmitted;
};

#endif
/* vim: et sw=4
 */
