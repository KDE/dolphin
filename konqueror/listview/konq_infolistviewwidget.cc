/* This file is part of the KDE project
   Copyright (C) 2002 Rolf Magnus <ramagnus@kde.org>
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation version 2.0

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/
#include "konq_infolistviewwidget.h"
#include "konq_infolistviewwidget.moc"
#include "konq_infolistviewitem.h"
#include "konq_listview.h"

#include <klocale.h>
#include <kfilemetainfo.h>
#include <kdebug.h>
#include <kaction.h>
#include <kservicetype.h>
#include <kselectaction.h>
#include <kio/metainfojob.h>

#include <QStringList>

KonqInfoListViewWidget::KonqInfoListViewWidget( KonqListView* parent,
                                                QWidget* parentWidget)
  : KonqBaseListViewWidget(parent, parentWidget)
{
    m_metaInfoJob = 0;

    m_mtSelector = new KSelectAction(i18n("View &As"), parent->actionCollection(), "view_as" );
    connect(m_mtSelector, SIGNAL(triggered(bool)), SLOT(slotSelectMimeType()));

    kDebug(1203) << "created info list view\n";
}

KonqInfoListViewWidget::~KonqInfoListViewWidget()
{
    delete m_mtSelector;
    delete m_metaInfoJob;
}

void KonqInfoListViewWidget::slotSelectMimeType()
{
    QString comment = m_mtSelector->currentText();

    // find the mime type by comment
    QMap<QString, KonqILVMimeType>::iterator it;
    for (it = m_counts.begin(); it!=m_counts.end(); ++it)
    {
        if ((*it).mimetype->comment() == comment)
        {
            m_favorite = *it;
            createFavoriteColumns();
            rebuildView();
            break;
        }
    }

}

void KonqInfoListViewWidget::createColumns()
{
   if (columns()<1)
   {
        // we can only create the minimum columns here, because we don't get the
        // items, from which we determine the columns yet.
        addColumn(i18n("Filename"), m_filenameColumnWidth);
   }
}

void KonqInfoListViewWidget::createFavoriteColumns()
{
    while (columns()>1)
    {
        kDebug(1203) << "removing column " << columnText(columns()-1) << endl;
        removeColumn(columns()-1);
    }

    // we need to get the preferred keys of the favorite
    const KFileMimeTypeInfo* mimeTypeInfo;

    if (m_favorite.mimetype &&
          (mimeTypeInfo = KFileMetaInfoProvider::self()
                            ->mimeTypeInfo(m_favorite.mimetype->name())))
    {
        QStringList preferredCols = mimeTypeInfo->preferredKeys();
        m_columnKeys.clear(); //We create the columnKeys as we're creating
        //the actual columns, to make sure they're synced

        // get the translations
        QStringList groups = mimeTypeInfo->preferredGroups();
        if (groups.isEmpty()) groups = mimeTypeInfo->supportedGroups();

        QStringList::Iterator prefKey = preferredCols.begin();
        for (; prefKey != preferredCols.end(); ++prefKey)
        {
            QStringList::Iterator group = groups.begin();
            for (; group != groups.end(); ++group)
            {
                const KFileMimeTypeInfo::GroupInfo* groupInfo =
                                    mimeTypeInfo->groupInfo(*group);
                if(groupInfo)
                {
                    QStringList keys = groupInfo->supportedKeys();
                    QStringList::Iterator key = keys.begin();
                    for (; key != keys.end(); ++key)
                    {
                        if ( *key == *prefKey )
                        {
                            const KFileMimeTypeInfo::ItemInfo* itemInfo =
                                groupInfo->itemInfo(*key);
                            addColumn(itemInfo->translatedKey());
                            m_columnKeys.append(*key);
                        }
                    }
                }
            }
        }
    }
    else
    {
        KonqBaseListViewWidget::createColumns();
    }

}

bool KonqInfoListViewWidget::openURL( const KUrl &url )
{
    bool ret = KonqBaseListViewWidget::openURL(url);
    m_bTopLevelComplete = true;
    return ret;
}

void KonqInfoListViewWidget::rebuildView()
{
    // make a KFileItemList from all our Items
    KFileItemList list;

    Q3ListViewItemIterator it(this);
    for (; it.current(); ++it)
    {
        list.append(static_cast<KonqInfoListViewItem*>(it.current())->item());
    }

    // now we can remove all the listview items
    clear();

    // and rebuild them
    KFileItemList::const_iterator kit = list.begin();
    const KFileItemList::const_iterator kend = list.end();
    for (; kit != kend; ++kit )
    {
        KonqInfoListViewItem* tmp = new KonqInfoListViewItem( this, *kit );
//        if (m_goToFirstItem==false)
//            if (m_itemFound==false)
//                if (tmp->text(0)==m_itemToGoTo)
//                {
//kDebug() << "Line " << __LINE__ << endl;
//                    setCurrentItem(tmp);
//kDebug() << "Line " << __LINE__ << endl;
//                    ensureItemVisible(tmp);
//kDebug() << "Line " << __LINE__ << endl;
//                    emit selectionChanged();
                    //selectCurrentItemAndEnableSelectedBySimpleMoveMode();
//                    m_itemFound=true;
//kDebug() << "Line " << __LINE__ << endl;
//                };

        tmp->gotMetaInfo();
    }


    if ( !viewport()->updatesEnabled() )
    {
        viewport()->setUpdatesEnabled( true );
        setUpdatesEnabled( true );
        triggerUpdate();
    }
}

void KonqInfoListViewWidget::slotNewItems( const KFileItemList& entries )
{
    slotStarted(); // ############# WHY?
    KFileItemList::const_iterator kit = entries.begin();
    const KFileItemList::const_iterator kend = entries.end();
    for (; kit != kend; ++kit )
    {
        KonqInfoListViewItem * tmp = new KonqInfoListViewItem( this, *kit );

        if (!m_itemFound && tmp->text(0)==m_itemToGoTo)
        {
            setCurrentItem(tmp);
            m_itemFound=true;
        }

        if ( !m_itemsToSelect.isEmpty() ) {
           int ind = m_itemsToSelect.indexOf( (*kit)->name() );
           if ( ind >= 0 ) {
              m_itemsToSelect.removeAt(ind);
              setSelected( tmp, true );
           }
        }

        if ( !(*kit)->isMimeTypeKnown() )
            m_pBrowserView->lstPendingMimeIconItems().append( tmp );
    }
    m_pBrowserView->newItems( entries );

    if ( !viewport()->updatesEnabled() )
    {
        viewport()->setUpdatesEnabled( true );
        setUpdatesEnabled( true );
        triggerUpdate();
    }

    slotUpdateBackground();

    if ( !m_favorite.mimetype )
        determineCounts( entries );

    kDebug(1203) << " ------------------------ startin metainfo job ------\n";

    // start getting metainfo from the files
    if (m_metaInfoJob)
    {
        KFileItemList::const_iterator kit = entries.begin();
        const KFileItemList::const_iterator kend = entries.end();
        for (; kit != kend; ++kit )
            m_metaInfoTodo.append(*kit);
    }
    else
    {
        m_metaInfoJob = KIO::fileMetaInfo( entries );
        connect( m_metaInfoJob, SIGNAL( gotMetaInfo( const KFileItem*)),
             this, SLOT( slotMetaInfo( const KFileItem*)));
        connect( m_metaInfoJob, SIGNAL( result( KJob*)),
             this, SLOT( slotMetaInfoResult()));
    }
}

void KonqInfoListViewWidget::slotRefreshItems( const KFileItemList& entries )
{
    kDebug(1203) << " ------------------------ starting metainfo job ------\n";

    // start getting metainfo from the files
    if (m_metaInfoJob)
    {
        KFileItemList::const_iterator kit = entries.begin();
        const KFileItemList::const_iterator kend = entries.end();
        for (; kit != kend; ++kit )
          m_metaInfoTodo.append(*kit);
    }
    else
    {
        m_metaInfoJob = KIO::fileMetaInfo( entries );
        connect( m_metaInfoJob, SIGNAL( gotMetaInfo( const KFileItem*)),
             this, SLOT( slotMetaInfo( const KFileItem*)));
        connect( m_metaInfoJob, SIGNAL( result( KJob*)),
             this, SLOT( slotMetaInfoResult()));
    }
    KonqBaseListViewWidget::slotRefreshItems( entries );
}

void KonqInfoListViewWidget::slotDeleteItem( KFileItem * item )
{
    m_metaInfoTodo.removeAll(item);
    if (m_metaInfoJob)
       m_metaInfoJob->removeItem(item);

    KonqBaseListViewWidget::slotDeleteItem(item);
}

void KonqInfoListViewWidget::slotClear()
{
    m_metaInfoTodo.clear();
    delete m_metaInfoJob;
    m_metaInfoJob = 0;
    m_favorite = KonqILVMimeType();

    KonqBaseListViewWidget::slotClear();
}

void KonqInfoListViewWidget::slotMetaInfo(const KFileItem* item)
{
    // need to search for the item (any better way?)
    Q3ListViewItemIterator it(this);
    for (; it.current(); ++it)
    {
        if (static_cast<KonqInfoListViewItem*>(it.current())->item()==item)
        {
            static_cast<KonqInfoListViewItem*>(it.current())->gotMetaInfo();
            return;
        }
    }

    // we should never reach this place
    Q_ASSERT(false);
}

void KonqInfoListViewWidget::slotMetaInfoResult()
{
    m_metaInfoJob = 0;
    if (m_metaInfoTodo.isEmpty())
    {
       m_bTopLevelComplete = false;
       kDebug(1203) << "emitting completed!!!!!!!!!!!!!!!!\n";
       slotCompleted();
    }
    else
    {
        m_metaInfoJob = KIO::fileMetaInfo(m_metaInfoTodo);
        connect( m_metaInfoJob, SIGNAL( gotMetaInfo( const KFileItem*)),
             this, SLOT( slotMetaInfo( const KFileItem*)));
        connect( m_metaInfoJob, SIGNAL( result( KJob*)),
             this, SLOT( slotMetaInfoResult()));
        m_metaInfoTodo.clear();
    }
}

void KonqInfoListViewWidget::determineCounts(const KFileItemList& list)
{
    m_counts.clear();
    m_favorite = KonqILVMimeType();
    // get the counts
    KFileItemList::const_iterator kit = list.begin();
    const KFileItemList::const_iterator kend = list.end();
    for (; kit != kend; ++kit )
    {
        const QString mt = (*kit)->mimetype();
        m_counts[mt].count++;
        if (!m_counts[mt].mimetype)
            m_counts[mt].mimetype = (*kit)->determineMimeType();
    }

    // and look for the plugins
    kDebug(1203) << "counts are:\n";

    KFileMetaInfoProvider* prov = KFileMetaInfoProvider::self();

    QStringList mtlist;

    QMap<QString, KonqILVMimeType>::iterator it;
    for (it = m_counts.begin(); it!=m_counts.end(); ++it)
    {
        // look if there is a plugin for this mimetype
        // and look for the "favorite" mimetype
#ifdef _GNUC
#warning ### change this
#endif
        // this will load the plugin which we don't need because we delegate
        // the job to the kioslave
        (*it).hasPlugin = prov->plugin(it.key());

        if ( (*it).hasPlugin)
        {
            mtlist << (*it).mimetype->comment();
            if (m_favorite.count <= (*it).count)
                m_favorite = *it;
        }

        kDebug(1203) << it.key() << " -> " << (*it).count
                      << " item(s) / plugin: "
                      << ((*it).hasPlugin ? "yes" : "no ") << endl;
    }

    m_mtSelector->setItems(mtlist);
//    QPopupMenu* menu = m_mtSelector->popupMenu();

    // insert the icons
//    for (int i=0; i<menu->count(); ++i)
//    {
//        menu->changeItem(i, QIcon(blah));
//    }

    if (m_favorite.mimetype)
    {
          m_mtSelector->setCurrentItem(mtlist.indexOf(m_favorite.mimetype->comment()));
          kDebug(1203) << "favorite mimetype is " << m_favorite.mimetype->name() << endl;
    }
    createFavoriteColumns();
}

