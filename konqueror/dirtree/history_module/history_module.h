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

#include <qobject.h>
#include <qptrdict.h>

#include <konq_treemodule.h>

#include "history_item.h"

class KonqTree;
class KonqTreeItem;

class KonqHistoryModule : public QObject, public KonqTreeModule
{
    Q_OBJECT
    
public:
    KonqHistoryModule( KonqTree * parentTree, const char * name = 0 );
    virtual ~KonqHistoryModule() {}

    virtual void addTopLevelItem( KonqTreeTopLevelItem * item );

    // Used by copy() and cut()
    virtual QDragObject * dragObject( QWidget * parent, bool move = false );

public slots:
    virtual void clearAll();

private slots:
    void slotCreateItems();
    void slotEntryAdded( const KonqHistoryEntry * );
    void slotEntryRemoved( const KonqHistoryEntry * );

private:
    typedef QPtrDictIterator<KonqHistoryItem> HistoryItemIterator;
    QPtrDict<KonqHistoryItem> m_dict;

    KonqTreeTopLevelItem * m_topLevelItem;
};


#endif // HISTORY_MODULE_H
