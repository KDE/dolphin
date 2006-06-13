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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef HISTORY_MODULE_H
#define HISTORY_MODULE_H

#include <QDateTime>
#include <QObject>
#include <q3dict.h>
#include <QPixmap>

#include <kglobal.h>
#include <klocale.h>
#include <konq_sidebartreemodule.h>

#include "history_item.h"

class KActionCollection;
class KonqSidebarHistorySettings;
class KonqSidebarTree;
class KonqSidebarTreeItem;

class KonqSidebarHistoryModule : public QObject, public KonqSidebarTreeModule
{
    Q_OBJECT

public:
    enum {
        ModuleContextMenu = 1,
        EntryContextMenu  = 2
    };

    KonqSidebarHistoryModule( KonqSidebarTree * parentTree, const char * name = 0 );
    virtual ~KonqSidebarHistoryModule();

    virtual void addTopLevelItem( KonqSidebarTreeTopLevelItem * item );
    virtual bool handleTopLevelContextMenu( KonqSidebarTreeTopLevelItem *item, const QPoint& pos );

    void showPopupMenu( int which, const QPoint& pos );

    // called by the items
    void showPopupMenu();
    void groupOpened( KonqSidebarHistoryGroupItem *item, bool open );
    const QDateTime& currentTime() const { return m_currentTime; }
    bool sortsByName() const { return m_sortsByName; }

    static QString groupForURL( const KUrl& url ) {
	static const QString& misc = KGlobal::staticQString(i18n("Miscellaneous"));
	return url.host().isEmpty() ? misc : url.host();
    }

public Q_SLOTS:
    void clear();

private Q_SLOTS:
    void slotCreateItems();
    void slotEntryAdded( const KonqHistoryEntry & );
    void slotEntryRemoved( const KonqHistoryEntry & );

    void slotNewWindow();
    void slotRemoveEntry();
    void slotPreferences();
    void slotSettingsChanged();

    void slotItemExpanded( Q3ListViewItem * );

    void slotSortByName();
    void slotSortByDate();

    void slotClearHistory();

private:
    KonqSidebarHistoryGroupItem *getGroupItem( const KUrl& url );

    void sortingChanged();
    typedef Q3DictIterator<KonqSidebarHistoryGroupItem> HistoryItemIterator;
    Q3Dict<KonqSidebarHistoryGroupItem> m_dict;

    KonqSidebarTreeTopLevelItem * m_topLevelItem;

    KActionCollection *m_collection;

    QPixmap m_folderClosed;
    QPixmap m_folderOpen;
    bool m_initialized;
    bool m_sortsByName;
    QDateTime m_currentTime; // used for sorting the items by date
    static KonqSidebarHistorySettings *s_settings;
};

#endif // HISTORY_MODULE_H
