/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#ifndef HISTORY_SETTINGS_H
#define HISTORY_SETTINGS_H

#include <qfont.h>
#include <qobject.h>

#include <dcopobject.h>

class KonqSidebarHistorySettings : public QObject, virtual public DCOPObject
{
    K_DCOP
    Q_OBJECT

public:
    enum { MINUTES, DAYS };

    KonqSidebarHistorySettings( QObject *parent, const char *name );
    virtual ~KonqSidebarHistorySettings();

    void readSettings();
    void applySettings();
    void setActiveDialog( QWidget *dialog );

    uint m_valueYoungerThan;
    uint m_valueOlderThan;

    int m_metricYoungerThan;
    int m_metricOlderThan;

    bool m_detailedTips;

    QFont m_fontYoungerThan;
    QFont m_fontOlderThan;

    QWidget *m_activeDialog;

signals:
    void settingsChanged( const KonqSidebarHistorySettings *oldSettings );

protected:
    KonqSidebarHistorySettings();
    KonqSidebarHistorySettings( const KonqSidebarHistorySettings& );

k_dcop:
    void notifySettingsChanged( KonqSidebarHistorySettings settings, QCString id );

private: // to make dcopidl happy :-/
};

QDataStream& operator<< (QDataStream& s, const KonqSidebarHistorySettings& e);
QDataStream& operator>> (QDataStream& s, KonqSidebarHistorySettings& e);


#endif // HISTORY_SETTINGS_H
