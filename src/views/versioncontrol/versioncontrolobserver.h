/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef VERSIONCONTROLOBSERVER_H
#define VERSIONCONTROLOBSERVER_H

#include "dolphin_export.h"

#include "kversioncontrolplugin.h"

#include <KFileItem>

#include <QList>
#include <QObject>
#include <QString>
#include <QUrl>

class KFileItemList;
class KFileItemModel;
class KItemRangeList;
class QAction;
class QTimer;
class UpdateItemStatesThread;

class DolphinView;

/**
 * @brief Observes all version control plugins.
 *
 * The items of the directory-model get updated automatically if the currently
 * shown directory is under version control.
 *
 * @see VersionControlPlugin
 */
class DOLPHIN_EXPORT VersionControlObserver : public QObject
{
    Q_OBJECT

public:
    explicit VersionControlObserver(QObject *parent = nullptr);
    ~VersionControlObserver() override;

    void setModel(KFileItemModel *model);
    KFileItemModel *model() const;
    void setView(DolphinView *view);
    DolphinView *view() const;

    QList<QAction *> actions(const KFileItemList &items) const;

Q_SIGNALS:
    /**
     * Is emitted if an information message with the content \a msg
     * should be shown.
     */
    void infoMessage(const QString &msg);

    /**
     * Is emitted if an error message with the content \a msg
     * should be shown.
     */
    void errorMessage(const QString &msg);

    /**
     * Is emitted if an "operation completed" message with the content \a msg
     * should be shown.
     */
    void operationCompletedMessage(const QString &msg);

private Q_SLOTS:
    /**
     * Invokes verifyDirectory() with a small delay. If delayedDirectoryVerification()
     * is invoked before the delay has been exceeded, the delay will be reset. This
     * assures that a lot of short requests for directory verification only result
     * in one (expensive) call.
     */
    void delayedDirectoryVerification();

    /**
     * Invokes verifyDirectory() with a small delay. In opposite to
     * delayedDirectoryVerification() it and assures that the verification of
     * the directory is done silently without information messages.
     */
    void silentDirectoryVerification();

    /**
     * Invokes delayedDirectoryVerification() only if the itemsChanged() signal has not
     * been triggered by the VCS plugin itself.
     */
    void slotItemsChanged(const KItemRangeList &itemRanges, const QSet<QByteArray> &roles);

    void verifyDirectory();

    /**
     * Is invoked if the thread m_updateItemStatesThread has been finished
     * and applies the item states.
     */
    void slotThreadFinished();

private:
    typedef QPair<KFileItem, KVersionControlPlugin::ItemVersion> ItemState;

    void updateItemStates();

    /**
     * It creates a item state list for every expanded directory and stores
     * this list together with the directory url in the \a itemStates map.
     *
     * @itemStates      A map of item state lists for every expanded directory
     *                  and its items, where the "key" is the directory url and
     *                  the "value" is a list of ItemStates for every item
     *                  within this directory.
     * @firstIndex      The index to start the processing from, this is needed
     *                  because this function is recursively called.
     *
     * @return          The number of (recursive) processed items.
     */
    int createItemStatesList(QMap<QString, QVector<ItemState>> &itemStates, const int firstIndex = 0);

    /**
     * Returns a matching plugin for the given directory.
     * 0 is returned, if no matching plugin has been found.
     */
    KVersionControlPlugin *searchPlugin(const QUrl &directory);

    /**
     * Returns true, if the directory contains a version control information.
     */
    bool isVersionControlled() const;

private:
    void initPlugins();

    bool m_pendingItemStatesUpdate;
    bool m_silentUpdate; // if true, no messages will be send during the update
                         // of version states
    QString m_localRepoRoot;

    DolphinView *m_view;
    KFileItemModel *m_model;

    QTimer *m_dirVerificationTimer;

    bool m_pluginsInitialized;
    KVersionControlPlugin *m_plugin;
    QList<QPointer<KVersionControlPlugin>> m_plugins;
    UpdateItemStatesThread *m_updateItemStatesThread;

    friend class UpdateItemStatesThread;
};

#endif // REVISIONCONTROLOBSERVER_H
