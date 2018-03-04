/***************************************************************************
 *   Copyright (C) 2014 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#ifndef SPACEINFOOBSERVER_H
#define SPACEINFOOBSERVER_H

#include <KIO/Job>

#include <QObject>

class QUrl;
class MountPointObserver;

class SpaceInfoObserver : public QObject
{
    Q_OBJECT

public:
    explicit SpaceInfoObserver(const QUrl& url, QObject* parent = nullptr);
    ~SpaceInfoObserver() override;

    quint64 size() const;
    quint64 available() const;

    void setUrl(const QUrl& url);

signals:
    /**
     * This signal is emitted when the size or available space changes.
     */
    void valuesChanged();

private slots:
    void spaceInfoChanged(quint64 size, quint64 available);

private:
    MountPointObserver* m_mountPointObserver;

    quint64 m_dataSize;
    quint64 m_dataAvailable;
};

#endif
