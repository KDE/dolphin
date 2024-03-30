/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "versioncontrolobserver.h"

#include "dolphin_versioncontrolsettings.h"
#include "dolphindebug.h"
#include "kitemviews/kfileitemmodel.h"
#include "updateitemstatesthread.h"
#include "views/dolphinview.h"

#include <KLocalizedString>
#include <KPluginFactory>
#include <KPluginMetaData>

#include <QTimer>

VersionControlObserver::VersionControlObserver(QObject *parent)
    : QObject(parent)
    , m_pendingItemStatesUpdate(false)
    , m_silentUpdate(false)
    , m_view(nullptr)
    , m_model(nullptr)
    , m_dirVerificationTimer(nullptr)
    , m_pluginsInitialized(false)
    , m_currentPlugin(nullptr)
    , m_updateItemStatesThread(nullptr)
{
    // The verification timer specifies the timeout until the shown directory
    // is checked whether it is versioned. Per default it is assumed that users
    // don't iterate through versioned directories and a high timeout is used
    // The timeout will be decreased as soon as a versioned directory has been
    // found (see verifyDirectory()).
    m_dirVerificationTimer = new QTimer(this);
    m_dirVerificationTimer->setSingleShot(true);
    m_dirVerificationTimer->setInterval(500);
    connect(m_dirVerificationTimer, &QTimer::timeout, this, &VersionControlObserver::verifyDirectory);
}

VersionControlObserver::~VersionControlObserver()
{
    if (m_currentPlugin) {
        m_currentPlugin->disconnect(this);
    }
    if (m_updateItemStatesThread) {
        m_updateItemStatesThread->requestInterruption();
        m_updateItemStatesThread->wait();
        m_updateItemStatesThread->deleteLater();
    }

    if (m_currentPlugin) {
        delete m_currentPlugin;
        m_currentPlugin = nullptr;
    }
    m_plugins.clear();
}

void VersionControlObserver::setModel(KFileItemModel *model)
{
    if (m_model) {
        disconnect(m_model, &KFileItemModel::itemsInserted, this, &VersionControlObserver::delayedDirectoryVerification);
        disconnect(m_model, &KFileItemModel::itemsChanged, this, &VersionControlObserver::slotItemsChanged);
    }

    m_model = model;

    if (model) {
        connect(m_model, &KFileItemModel::itemsInserted, this, &VersionControlObserver::delayedDirectoryVerification);
        connect(m_model, &KFileItemModel::itemsChanged, this, &VersionControlObserver::slotItemsChanged);
    }
}

KFileItemModel *VersionControlObserver::model() const
{
    return m_model;
}

void VersionControlObserver::setView(DolphinView *view)
{
    if (m_view) {
        disconnect(m_view, &DolphinView::activated, this, &VersionControlObserver::delayedDirectoryVerification);
    }

    m_view = view;

    if (m_view) {
        connect(m_view, &DolphinView::activated, this, &VersionControlObserver::delayedDirectoryVerification);
    }
}

DolphinView *VersionControlObserver::view() const
{
    return m_view;
}

QList<QAction *> VersionControlObserver::actions(const KFileItemList &items) const
{
    bool hasNullItems = false;
    for (const KFileItem &item : items) {
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
        return m_currentPlugin->versionControlActions(items);
    } else {
        QList<QAction *> actions;
        for (const KVersionControlPlugin *plugin : std::as_const(m_plugins)) {
            actions << plugin->outOfVersionControlActions(items);
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

void VersionControlObserver::slotItemsChanged(const KItemRangeList &itemRanges, const QSet<QByteArray> &roles)
{
    Q_UNUSED(itemRanges)

    // Because "version" role is emitted by VCS plugin (ourselves) we don't need to
    // analyze it and update directory item states information. So lets check if
    // there is only "version".
    if (!(roles.count() == 1 && roles.contains("version"))) {
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

    if (m_currentPlugin && rootItem.url().path().startsWith(m_localRepoRoot) && QFile::exists(m_localRepoRoot + '/' + m_currentPlugin->fileName())) {
        // current directory is still versionned
        updateItemStates();
        return;
    }

    if ((m_currentPlugin = searchPlugin(rootItem.url()))) {
        // The directory is versioned. Assume that the user will further browse through
        // versioned directories and decrease the verification timer.
        m_dirVerificationTimer->setInterval(100);
        updateItemStates();
        return;
    }

    // The directory is not versioned. Reset the verification timer to a higher
    // value, so that browsing through non-versioned directories is not slown down
    // by an immediate verification.
    m_dirVerificationTimer->setInterval(500);
}

void VersionControlObserver::slotThreadFinished()
{
    UpdateItemStatesThread *thread = m_updateItemStatesThread;
    m_updateItemStatesThread = nullptr; // The thread deletes itself automatically (see updateItemStates())

    if (!m_currentPlugin || !thread) {
        return;
    }

    const QMap<QString, QVector<ItemState>> &itemStates = thread->itemStates();
    QMap<QString, QVector<ItemState>>::const_iterator it = itemStates.constBegin();
    for (; it != itemStates.constEnd(); ++it) {
        const QVector<ItemState> &items = it.value();

        for (const ItemState &item : items) {
            const KFileItem &fileItem = item.first;
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
        Q_EMIT operationCompletedMessage(QString());
    }

    if (m_pendingItemStatesUpdate) {
        m_pendingItemStatesUpdate = false;
        updateItemStates();
    }
}

void VersionControlObserver::updateItemStates()
{
    Q_ASSERT(m_currentPlugin);
    if (m_updateItemStatesThread) {
        // An update is currently ongoing. Wait until the thread has finished
        // the update (see slotThreadFinished()).
        m_pendingItemStatesUpdate = true;
        return;
    }

    QMap<QString, QVector<ItemState>> itemStates;
    createItemStatesList(itemStates);

    if (!itemStates.isEmpty()) {
        if (!m_silentUpdate) {
            Q_EMIT infoMessage(i18nc("@info:status", "Updating version information…"));
        }
        m_updateItemStatesThread = new UpdateItemStatesThread(m_currentPlugin, itemStates);
        connect(m_updateItemStatesThread, &UpdateItemStatesThread::finished, this, &VersionControlObserver::slotThreadFinished);
        connect(m_updateItemStatesThread, &UpdateItemStatesThread::finished, m_updateItemStatesThread, &UpdateItemStatesThread::deleteLater);

        m_updateItemStatesThread->start(); // slotThreadFinished() is called when finished
    }
}

int VersionControlObserver::createItemStatesList(QMap<QString, QVector<ItemState>> &itemStates, const int firstIndex)
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
        const QUrl &url = items.first().first.url();
        itemStates.insert(url.adjusted(QUrl::RemoveFilename).path(), items);
    }

    return index - firstIndex; // number of processed items
}

void VersionControlObserver::initPlugins()
{
    if (!m_pluginsInitialized) {
        // No searching for plugins has been done yet. Query all fileview version control
        // plugins and remember them in 'plugins'.
        const QStringList enabledPlugins = VersionControlSettings::enabledPlugins();

        const QVector<KPluginMetaData> plugins = KPluginMetaData::findPlugins(QStringLiteral("dolphin/vcs"));

        for (const auto &p : plugins) {
            if (enabledPlugins.contains(p.name())) {
                auto plugin = KPluginFactory::instantiatePlugin<KVersionControlPlugin>(p, parent()).plugin;
                if (plugin) {
                    m_plugins.append(plugin);
                }
            }
        }

        for (const auto *plugin : std::as_const(m_plugins)) {
            connect(plugin, &KVersionControlPlugin::itemVersionsChanged, this, &VersionControlObserver::silentDirectoryVerification);
            connect(plugin, &KVersionControlPlugin::infoMessage, this, &VersionControlObserver::infoMessage);
            connect(plugin, &KVersionControlPlugin::errorMessage, this, &VersionControlObserver::errorMessage);
            connect(plugin, &KVersionControlPlugin::operationCompletedMessage, this, &VersionControlObserver::operationCompletedMessage);
        }

        m_pluginsInitialized = true;
    }
}

KVersionControlPlugin *VersionControlObserver::searchPlugin(const QUrl &directory)
{
    initPlugins();

    // Verify whether the current directory is under a version system
    for (KVersionControlPlugin *plugin : std::as_const(m_plugins)) {
        // first naively check if we are at working copy root
        const QString fileName = directory.path() + '/' + plugin->fileName();
        if (QFile::exists(fileName)) {
            m_localRepoRoot = directory.path();
            return plugin;
        }

        const QString root = plugin->localRepositoryRoot(directory.path());
        if (!root.isEmpty()) {
            m_localRepoRoot = root;
            return plugin;
        }
    }
    return nullptr;
}

bool VersionControlObserver::isVersionControlled() const
{
    return m_currentPlugin != nullptr;
}

#include "moc_versioncontrolobserver.cpp"
