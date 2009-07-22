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

#include "revisioncontrolobserver.h"

#include "dolphinmodel.h"
#include "revisioncontrolplugin.h"

#include <kdirlister.h>

#include <QAbstractProxyModel>
#include <QAbstractItemView>
#include <QTimer>

/**
 * The performance of updating the revision state of items depends
 * on the used plugin. To prevent that Dolphin gets blocked by a
 * slow plugin, the updating is delegated to a thread.
 */
class UpdateItemStatesThread : public QThread
{
public:
    UpdateItemStatesThread(QObject* parent);
    void setData(RevisionControlPlugin* plugin,
                 const QList<RevisionControlObserver::ItemState>& itemStates);
    QList<RevisionControlObserver::ItemState> itemStates() const;
    
protected:
    virtual void run();
    
private:
    RevisionControlPlugin* m_plugin;
    QList<RevisionControlObserver::ItemState> m_itemStates;
};

UpdateItemStatesThread::UpdateItemStatesThread(QObject* parent) :
    QThread(parent)
{
}

void UpdateItemStatesThread::setData(RevisionControlPlugin* plugin,
                                     const QList<RevisionControlObserver::ItemState>& itemStates)
{
    m_plugin = plugin;
    m_itemStates = itemStates;
}

void UpdateItemStatesThread::run()
{
    Q_ASSERT(m_itemStates.count() > 0);
    Q_ASSERT(m_plugin != 0);
    
    // it is assumed that all items have the same parent directory
    const QString directory = m_itemStates.first().item.url().directory(KUrl::AppendTrailingSlash);
    
    if (m_plugin->beginRetrieval(directory)) {
        const int count = m_itemStates.count();
        for (int i = 0; i < count; ++i) {
            m_itemStates[i].revision = m_plugin->revisionState(m_itemStates[i].item);
        }
        m_plugin->endRetrieval();
    }
}

QList<RevisionControlObserver::ItemState> UpdateItemStatesThread::itemStates() const
{
    return m_itemStates;
}

// ---

RevisionControlObserver::RevisionControlObserver(QAbstractItemView* view) :
    QObject(view),
    m_pendingItemStatesUpdate(false),
    m_revisionedDirectory(false),
    m_view(view),
    m_dirLister(0),
    m_dolphinModel(0),
    m_dirVerificationTimer(0),
    m_plugin(0),
    m_updateItemStatesThread(0)
{
    Q_ASSERT(view != 0);

    QAbstractProxyModel* proxyModel = qobject_cast<QAbstractProxyModel*>(view->model());
    m_dolphinModel = (proxyModel == 0) ?
                 qobject_cast<DolphinModel*>(view->model()) :
                 qobject_cast<DolphinModel*>(proxyModel->sourceModel());
    if (m_dolphinModel != 0) {
        m_dirLister = m_dolphinModel->dirLister();
        connect(m_dirLister, SIGNAL(completed()),
                this, SLOT(delayedDirectoryVerification()));
 
        // The verification timer specifies the timeout until the shown directory
        // is checked whether it is versioned. Per default it is assumed that users
        // don't iterate through versioned directories and a high timeout is used
        // The timeout will be decreased as soon as a versioned directory has been
        // found (see verifyDirectory()).
        m_dirVerificationTimer = new QTimer(this);
        m_dirVerificationTimer->setSingleShot(true);
        m_dirVerificationTimer->setInterval(500);
        connect(m_dirVerificationTimer, SIGNAL(timeout()),
                this, SLOT(verifyDirectory()));
    }
}

RevisionControlObserver::~RevisionControlObserver()
{
    delete m_plugin;
    m_plugin = 0;
}

QList<QAction*> RevisionControlObserver::contextMenuActions(const KFileItemList& items) const
{
    if (m_dolphinModel->hasRevisionData() && (m_plugin != 0)) {
        return m_plugin->contextMenuActions(items);
    }

    return QList<QAction*>();
}

void RevisionControlObserver::delayedDirectoryVerification()
{
    m_dirVerificationTimer->start();
}

void RevisionControlObserver::verifyDirectory()
{
    KUrl revisionControlUrl = m_dirLister->url();
    if (!revisionControlUrl.isLocalFile()) {
        return;
    }

    if (m_plugin == 0) {
        // TODO: just for testing purposes. A plugin approach will be used later.
        m_plugin = new SubversionPlugin();
    }

    revisionControlUrl.addPath(m_plugin->fileName());
    const KFileItem item = m_dirLister->findByUrl(revisionControlUrl);
    if (item.isNull() && m_revisionedDirectory) {
        // The directory is not versioned. Reset the verification timer to a higher
        // value, so that browsing through non-versioned directories is not slown down
        // by an immediate verification.
        m_dirVerificationTimer->setInterval(500);
        m_revisionedDirectory = false;
        disconnect(m_dirLister, SIGNAL(refreshItems(const QList<QPair<KFileItem,KFileItem>>&)),
                   this, SLOT(delayedDirectoryVerification()));
        disconnect(m_dirLister, SIGNAL(newItems(const KFileItemList&)),
                   this, SLOT(delayedDirectoryVerification()));
    } else if (!item.isNull()) {
        if (!m_revisionedDirectory) {
            // The directory is versioned. Assume that the user will further browse through
            // versioned directories and decrease the verification timer.
            m_dirVerificationTimer->setInterval(100);
            m_revisionedDirectory = true;
            connect(m_dirLister, SIGNAL(refreshItems(const QList<QPair<KFileItem,KFileItem>>&)),
                    this, SLOT(delayedDirectoryVerification()));
            connect(m_dirLister, SIGNAL(newItems(const KFileItemList&)),
                    this, SLOT(delayedDirectoryVerification()));
        }
        updateItemStates();
    }
}

void RevisionControlObserver::applyUpdatedItemStates()
{
    // QAbstractItemModel::setData() triggers a bottleneck in combination with QListView
    // (a detailed description of the root cause is given in the class KFilePreviewGenerator
    // from kdelibs). To bypass this bottleneck, the signals of the model are temporary blocked.
    // This works as the update of the data does not require a relayout of the views used in Dolphin.
    const bool signalsBlocked = m_dolphinModel->signalsBlocked();
    m_dolphinModel->blockSignals(true);

    const QList<ItemState> itemStates = m_updateItemStatesThread->itemStates();
    foreach (const ItemState& itemState, itemStates) {
        m_dolphinModel->setData(itemState.index,
                                QVariant(static_cast<int>(itemState.revision)),
                                Qt::DecorationRole);
    }

    m_dolphinModel->blockSignals(signalsBlocked);
    m_view->viewport()->repaint();
    
    if (m_pendingItemStatesUpdate) {
        m_pendingItemStatesUpdate = false;
        updateItemStates();
    }
}

void RevisionControlObserver::updateItemStates()
{
    Q_ASSERT(m_plugin != 0);
    if (m_updateItemStatesThread == 0) {
        m_updateItemStatesThread = new UpdateItemStatesThread(this);
        connect(m_updateItemStatesThread, SIGNAL(finished()),
                this, SLOT(applyUpdatedItemStates()));
    }
    if (m_updateItemStatesThread->isRunning()) {
        // An update is currently ongoing. Wait until the thread has finished
        // the update (see applyUpdatedItemStates()).
        m_pendingItemStatesUpdate = true;
        return;
    }
    
    const int rowCount = m_dolphinModel->rowCount();
    if (rowCount > 0) {
        // Build a list of all items in the current directory and delegate
        // this list to the thread, which adjusts the revision states.
        QList<ItemState> itemStates;
        for (int row = 0; row < rowCount; ++row) {
            const QModelIndex index = m_dolphinModel->index(row, DolphinModel::Revision);
            
            ItemState itemState;
            itemState.index = index;
            itemState.item = m_dolphinModel->itemForIndex(index);
            itemState.revision = RevisionControlPlugin::LocalRevision;
            
            itemStates.append(itemState);
        }
        
        m_updateItemStatesThread->setData(m_plugin, itemStates);
        m_updateItemStatesThread->start(); // applyUpdatedItemStates() is called when finished
    }
}

#include "revisioncontrolobserver.moc"
