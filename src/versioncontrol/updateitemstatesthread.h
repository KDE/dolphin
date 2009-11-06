/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef UPDATEITEMSTATESTHREAD_H
#define UPDATEITEMSTATESTHREAD_H

#include <libdolphin_export.h>
#include <versioncontrol/versioncontrolobserver.h>

#include <QMutex>
#include <QThread>

class KVersionControlPlugin;

/**
 * The performance of updating the version state of items depends
 * on the used plugin. To prevent that Dolphin gets blocked by a
 * slow plugin, the updating is delegated to a thread.
 */
class LIBDOLPHINPRIVATE_EXPORT UpdateItemStatesThread : public QThread
{
    Q_OBJECT

public:
    UpdateItemStatesThread(QObject* parent);
    virtual ~UpdateItemStatesThread();

    void setData(KVersionControlPlugin* plugin,
                 const QList<VersionControlObserver::ItemState>& itemStates);

    bool beginReadItemStates();
    void endReadItemStates();
    QList<VersionControlObserver::ItemState> itemStates() const;

    bool retrievedItems() const;

    void deleteWhenFinished();

protected:
    virtual void run();

private slots:
    void slotFinished();

private:
    bool m_retrievedItems;
    KVersionControlPlugin* m_plugin;
    QMutex m_mutex;
    QList<VersionControlObserver::ItemState> m_itemStates;
};

#endif // UPDATEITEMSTATESTHREAD_H
