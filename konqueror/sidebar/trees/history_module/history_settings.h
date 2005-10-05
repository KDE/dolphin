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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef HISTORY_SETTINGS_H
#define HISTORY_SETTINGS_H

#include <qfont.h>
#include <qobject.h>

#include <dcopobject.h>

class KonqSidebarHistorySettings : public QObject, public DCOPObject
{
    K_DCOP
    Q_OBJECT

public:
    enum { MINUTES, DAYS };

    KonqSidebarHistorySettings( QObject *parent, const char *name );
    virtual ~KonqSidebarHistorySettings();

    void readSettings(bool global);
    void applySettings();

    uint m_valueYoungerThan;
    uint m_valueOlderThan;

    int m_metricYoungerThan;
    int m_metricOlderThan;

    bool m_detailedTips;

    QFont m_fontYoungerThan;
    QFont m_fontOlderThan;

signals:
    void settingsChanged();

protected:
    KonqSidebarHistorySettings();
    KonqSidebarHistorySettings( const KonqSidebarHistorySettings& );

k_dcop:
    void notifySettingsChanged();

private: // to make dcopidl happy :-/
};

#endif // HISTORY_SETTINGS_H
