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

#include <kglobal.h>
#include <klocale.h>
#include <konq_treemodule.h>

#include "history_item.h"
#include "history_dlg.h"

class KActionCollection;
class KDialogBase;
class KonqHistoryDialog;
class KonqHistorySettings;
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

    static QString groupForURL( const KURL& url ) {
	static const QString& misc = KGlobal::staticQString(i18n("Miscellaneous"));
	return url.host().isEmpty() ? misc : url.host();
    }

public slots:
    void clear();

private slots:
    void slotCreateItems();
    void slotEntryAdded( const KonqHistoryEntry * );
    void slotEntryRemoved( const KonqHistoryEntry * );

    void slotNewWindow();
    void slotRemoveEntry();
    void slotPreferences();
    void slotDialogFinished();
    void slotSettingsChanged( const KonqHistorySettings *old );

    void slotItemExpanded( QListViewItem * );

    void slotSortByName();
    void slotSortByDate();

    void slotClearHistory();

private:
    KonqHistoryGroupItem *getGroupItem( const KURL& url );

    void sortingChanged();
    typedef QDictIterator<KonqHistoryGroupItem> HistoryItemIterator;
    QDict<KonqHistoryGroupItem> m_dict;

    KonqTreeTopLevelItem * m_topLevelItem;

    KActionCollection *m_collection;

    KDialogBase *m_dlg;
    KonqHistoryDialog *m_settingsDlg;
    QPixmap m_folderClosed;
    QPixmap m_folderOpen;
    bool m_initialized;
    bool m_sortsByName;
    QDateTime m_currentTime; // used for sorting the items by date
    static KonqHistorySettings *s_settings;
};


class KonqHistoryDialog : public KonqHistoryDlg
{
    Q_OBJECT

public:
    KonqHistoryDialog( KonqHistorySettings *settings,
		       QWidget *parent, const char *name=0 );
    ~KonqHistoryDialog();

    void initFromSettings();

public slots:
    void applySettings();
    void slotGetFontNewer();
    void slotGetFontOlder();

private slots:
    void slotExpireChanged( int );
    void slotNewerChanged( int );
    void slotOlderChanged( int );

private:
    QFont m_fontNewer;
    QFont m_fontOlder;
    KonqHistorySettings *m_settings;
};

#endif // HISTORY_MODULE_H
