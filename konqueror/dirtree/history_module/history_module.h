/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef HISTORY_MODULE_H
#define HISTORY_MODULE_H

#include <qdatetime.h>
#include <qobject.h>
#include <qdict.h>
#include <qpixmap.h>

#include <konq_treemodule.h>

#include "history_item.h"

class KActionCollection;
class KonqTree;
class KonqTreeItem;

class KonqHistoryModule : public QObject, public KonqTreeModule
{
    Q_OBJECT

public:
    KonqHistoryModule( KonqTree * parentTree, const char * name = 0 );
    virtual ~KonqHistoryModule();

    virtual void addTopLevelItem( KonqTreeTopLevelItem * item );

    // called by the items
    void showPopupMenu();
    void groupOpened( KonqHistoryGroupItem *item, bool open );
    const QDateTime& currentTime() const { return m_currentTime; }
    bool sortsByName() const { return m_sortsByName; }

public slots:
    void clear();

private slots:
    void slotCreateItems();
    void slotEntryAdded( const KonqHistoryEntry * );
    void slotEntryRemoved( const KonqHistoryEntry * );

    void slotRemoveEntry();
    void slotPreferences();

    void slotItemExpanded( QListViewItem * );

    void slotSortByName();
    void slotSortByDate();

private:
    void sortingChanged();

    typedef QDictIterator<KonqHistoryGroupItem> HistoryItemIterator;
    QDict<KonqHistoryGroupItem> m_dict;

    KonqTreeTopLevelItem * m_topLevelItem;

    KActionCollection *m_collection;

    QPixmap m_folderClosed;
    QPixmap m_folderOpen;
    bool m_initialized;
    bool m_sortsByName;
    QDateTime m_currentTime; // used for sorting the items by date
};


#endif // HISTORY_MODULE_H
