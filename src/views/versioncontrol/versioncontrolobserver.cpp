/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "versioncontrolobserver.h"

#include "dolphin_versioncontrolsettings.h"
#include "dolphindebug.h"
#include "views/dolphinview.h"
#include "kitemviews/kfileitemmodel.h"
#include "updateitemstatesthread.h"

#include <KLocalizedString>
#include <KService>
#include <KServiceTypeTrader>

#include <QTimer>

VersionControlObserver::VersionControlObserver(QObject* parent) :
    QObject(parent),
    m_pendingItemStatesUpdate(false),
    m_versionedDirectory(false),
    m_silentUpdate(false),
    m_view(nullptr),
    m_model(nullptr),
    m_dirVerificationTimer(nullptr),
    m_pluginsInitialized(false),
    m_plugin(nullptr),
    m_updateItemStatesThread(nullptr)
{
    // The verification timer specifies the timeout until the shown directory
    // is checked whether it is versioned. Per default it is assumed that users
    // don't iterate through versioned directories and a high timeout is used
    // The timeout will be decreased as soon as a versioned directory has been
    // found (see verifyDirectory()).
    m_dirVerificationTimer = new QTimer(this);
    m_dirVerificationTimer->setSingleShot(true);
    m_dirVerificationTimer->setInterval(500);
    connect(m_dirVerificationTimer, &QTimer::timeout,
            this, &VersionControlObserver::verifyDirectory);
}

VersionControlObserver::~VersionControlObserver()
{
    if (m_plugin) {
        m_plugin->disconnect(this);
        m_plugin = nullptr;
    }
}

void VersionControlObserver::setModel(KFileItemModel* model)
{
    if (m_model) {
        disconnect(m_model, &KFileItemModel::itemsInserted,
                   this, &VersionControlObserver::delayedDirectoryVerification);
        disconnect(m_model, &KFileItemModel::itemsChanged,
                   this, &VersionControlObserver::slotItemsChanged);
    }

    m_model = model;

    if (model) {
        connect(m_model, &KFileItemModel::itemsInserted,
                this, &VersionControlObserver::delayedDirectoryVerification);
        connect(m_model, &KFileItemModel::itemsChanged,
                this, &VersionControlObserver::slotItemsChanged);
    }
}

KFileItemModel* VersionControlObserver::model() const
{
    return m_model;
}

void VersionControlObserver::setView(DolphinView* view)
{
    if (m_view) {
        disconnect(m_view, &DolphinView::activated,
                   this, &VersionControlObserver::delayedDirectoryVerification);
    }

    m_view = view;

    if (m_view) {
        connect(m_view, &DolphinView::activated,
                this, &VersionControlObserver::delayedDirectoryVerification);
    }
}

DolphinView* VersionControlObserver::view() const
{
    return m_view;
}

QList<QAction*> VersionControlObserver::actions(const KFileItemList& items) const
{
    bool hasNullItems = false;
    foreach (const KFileItem& item, items) {
        if (item.isNull()) {
            qCWarning(DolphinDebug) << "Requesting version-control-actions for empty items";
            hasNullItems = true;
            break;
        }
    }

    if (!m_model || hasNullItems) {
        return {};
    }

    if (isVersionControlled()) {
        return m_plugin->versionControlActions(items);
    } else {
        QList<QAction*> actions;
        for (const auto &plugin : qAsConst(m_plugins)) {
            actions << plugin.first->outOfVersionControlActions(items);
        }
        return actions;
    }
}

void VersionControlObserver::delayedDirectoryVerification()
{
    m_silentUpdate = false;
    m_dirVerificationTimer->start();
}

void VersionControlObserver::silentDirectoryVerification()
{
    m_silentUpdate = true;
    m_dirVerificationTimer->start();
}

void VersionControlObserver::slotItemsChanged(const KItemRangeList& itemRanges, const QSet<QByteArray>& roles)
{
    Q_UNUSED(itemRanges)

    // Because "version" role is emitted by VCS plugin (ourselfs) we don't need to
    // analyze it and update directory item states information. So lets check if
    // there is only "version".
    if ( !(roles.count() == 1 && roles.contains("version")) ) {
        delayedDirectoryVerification();
    }
}

void VersionControlObserver::verifyDirectory()
{
    if (!m_model) {
        return;
    }

    const KFileItem rootItem = m_model->rootItem();
    if (rootItem.isNull() || !rootItem.url().isLocalFile()) {
        return;
    }

    if (m_plugin) {
        m_plugin->disconnect(this);
    }

    m_plugin = searchPlugin(rootItem.url());
    if (m_plugin) {
        connect(m_plugin, &KVersionControlPlugin::itemVersionsChanged,
                this, &VersionControlObserver::silentDirectoryVerification);
        connect(m_plugin, &KVersionControlPlugin::infoMessage,
                this, &VersionControlObserver::infoMessage);
        connect(m_plugin, &KVersionControlPlugin::errorMessage,
                this, &VersionControlObserver::errorMessage);
        connect(m_plugin, &KVersionControlPlugin::operationCompletedMessage,
                this, &VersionControlObserver::operationCompletedMessage);

        if (!m_versionedDirectory) {
            m_versionedDirectory = true;

            // The directory is versioned. Assume that the user will further browse through
            // versioned directories and decrease the verification timer.
            m_dirVerificationTimer->setInterval(100);
        }
        updateItemStates();
    } else if (m_versionedDirectory) {
        m_versionedDirectory = false;

        // The directory is not versioned. Reset the verification timer to a higher
        // value, so that browsing through non-versioned directories is not slown down
        // by an immediate verification.
        m_dirVerificationTimer->setInterval(500);
    }
}

void VersionControlObserver::slotThreadFinished()
{
    UpdateItemStatesThread* thread = m_updateItemStatesThread;
    m_updateItemStatesThread = nullptr; // The thread deletes itself automatically (see updateItemStates())

    if (!m_plugin || !thread) {
        return;
    }

    const QMap<QString, QVector<ItemState> >& itemStates = thread->itemStates();
    QMap<QString, QVector<ItemState> >::const_iterator it = itemStates.constBegin();
    for (; it != itemStates.constEnd(); ++it) {
        const QVector<ItemState>& items = it.value();

        foreach (const ItemState& item, items) {
            const KFileItem& fileItem = item.first;
            const KVersionControlPlugin::ItemVersion version = item.second;
            QHash<QByteArray, QVariant> values;
            values.insert("version", QVariant(version));
            m_model->setData(m_model->index(fileItem), values);
        }
    }

    if (!m_silentUpdate) {
        // Using an empty message results in clearing the previously shown information message and showing
        // the default status bar information. This is useful as the user already gets feedback that the
        // operation has been completed because of the icon emblems.
        emit operationCompletedMessage(QString());
    }

    if (m_pendingItemStatesUpdate) {
        m_pendingItemStatesUpdate = false;
        updateItemStates();
    }
}

void VersionControlObserver::updateItemStates()
{
    Q_ASSERT(m_plugin);
    if (m_updateItemStatesThread) {
        // An update is currently ongoing. Wait until the thread has finished
        // the update (see slotThreadFinished()).
        m_pendingItemStatesUpdate = true;
        return;
    }

    QMap<QString, QVector<ItemState> > itemStates;
    createItemStatesList(itemStates);

    if (!itemStates.isEmpty()) {
        if (!m_silentUpdate) {
            emit infoMessage(i18nc("@info:status", "Updating version information..."));
        }
        m_updateItemStatesThread = new UpdateItemStatesThread(m_plugin, itemStates);
        connect(m_updateItemStatesThread, &UpdateItemStatesThread::finished,
                this, &VersionControlObserver::slotThreadFinished);
        connect(m_updateItemStatesThread, &UpdateItemStatesThread::finished,
                m_updateItemStatesThread, &UpdateItemStatesThread::deleteLater);

        m_updateItemStatesThread->start(); // slotThreadFinished() is called when finished
    }
}

int VersionControlObserver::createItemStatesList(QMap<QString, QVector<ItemState> >& itemStates,
                                                 const int firstIndex)
{
    const int itemCount = m_model->count();
    const int currentExpansionLevel = m_model->expandedParentsCount(firstIndex);

    QVector<ItemState> items;
    items.reserve(itemCount - firstIndex);

    int index;
    for (index = firstIndex; index < itemCount; ++index) {
        const int expansionLevel = m_model->expandedParentsCount(index);

        if (expansionLevel == currentExpansionLevel) {
            ItemState itemState;
            itemState.first = m_model->fileItem(index);
            itemState.second = KVersionControlPlugin::UnversionedVersion;

            items.append(itemState);
        } else if (expansionLevel > currentExpansionLevel) {
            // Sub folder
            index += createItemStatesList(itemStates, index) - 1;
        } else {
            break;
        }
    }

    if (!items.isEmpty()) {
        const QUrl& url = items.first().first.url();
        itemStates.insert(url.adjusted(QUrl::RemoveFilename).path(), items);
    }

    return index - firstIndex; // number of processed items
}

KVersionControlPlugin* VersionControlObserver::searchPlugin(const QUrl& directory)
{
    if (!m_pluginsInitialized) {
        // No searching for plugins has been done yet. Query the KServiceTypeTrader for
        // all fileview version control plugins and remember them in 'plugins'.
        const QStringList enabledPlugins = VersionControlSettings::enabledPlugins();

        const KService::List pluginServices = KServiceTypeTrader::self()->query(QStringLiteral("FileViewVersionControlPlugin"));
        for (KService::List::ConstIterator it = pluginServices.constBegin(); it != pluginServices.constEnd(); ++it) {
            if (enabledPlugins.contains((*it)->name())) {
                KVersionControlPlugin* plugin = (*it)->createInstance<KVersionControlPlugin>(this);
                if (plugin) {
                    m_plugins.append( qMakePair(plugin, plugin->fileName()) );
                }
            }
        }
        m_pluginsInitialized = true;
    }

    if (m_plugins.empty()) {
        // A searching for plugins has already been done, but no
        // plugins are installed
        return nullptr;
    }

    // We use the number of upUrl() calls to find the best matching plugin
    // for the given directory. The smaller value, the better it is (0 is best).
    KVersionControlPlugin* bestPlugin = nullptr;
    int bestScore = INT_MAX;

    // Verify whether the current directory contains revision information
    // like .svn, .git, ...
    for (const auto &it : qAsConst(m_plugins)) {
        const QString fileName = directory.path() + '/' + it.second;
        if (QFile::exists(fileName)) {
            // The score of this plugin is 0 (best), so we can just return this plugin,
            // instead of going through the plugin scoring procedure, we can't find a better one ;)
            return it.first;
        }

        // Version control systems like Git provide the version information
        // file only in the root directory. Check whether the version information file can
        // be found in one of the parent directories. For performance reasons this
        // step is only done, if the previous directory was marked as versioned by
        // m_versionedDirectory. Drawback: Until e. g. Git is recognized, the root directory
        // must be shown at least once.
        if (m_versionedDirectory) {
            QUrl dirUrl(directory);
            QUrl upUrl = KIO::upUrl(dirUrl);
            int upUrlCounter = 1;
            while ((upUrlCounter < bestScore) && (upUrl != dirUrl)) {
                const QString fileName = dirUrl.path() + '/' + it.second;
                if (QFile::exists(fileName)) {
                    if (upUrlCounter < bestScore) {
                        bestPlugin = it.first;
                        bestScore = upUrlCounter;
                    }
                    break;
                }
                dirUrl = upUrl;
                upUrl = KIO::upUrl(dirUrl);
                ++upUrlCounter;
            }
        }
    }

    return bestPlugin;
}

bool VersionControlObserver::isVersionControlled() const
{
    return m_versionedDirectory && m_plugin;
}

