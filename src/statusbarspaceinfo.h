/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/
#ifndef STATUSBARSPACEINFO_H
#define STATUSBARSPACEINFO_H

#include <kurl.h>

#include <QColor>
#include <QKeyEvent>
#include <QString>

#include <kcapacitybar.h>

class KDiskFreeSp;

/**
 * @short Shows the available space for the volume represented
 *        by the given URL as part of the status bar.
 */
class StatusBarSpaceInfo : public KCapacityBar
{
    Q_OBJECT

public:
    explicit StatusBarSpaceInfo(QWidget* parent);
    virtual ~StatusBarSpaceInfo();

    void setUrl(const KUrl& url);
    const KUrl& url() const;

private slots:
    void slotFoundMountPoint(const QString& mountPoint,
                             quint64 kBSize,
                             quint64 kBUsed,
                             quint64 kBAvailable);

    void slotDiskFreeSpaceDone();

    /** Refreshes the space information for the current set URL. */
    void refresh();

    void showGettingSizeInfo();

private:
    bool m_gettingSize;
    bool m_foundMountPoint;
    KUrl m_url;
};

inline const KUrl& StatusBarSpaceInfo::url() const
{
    return m_url;
}

#endif
