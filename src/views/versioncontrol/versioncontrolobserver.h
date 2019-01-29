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
class QAction;
class QTimer;
class UpdateItemStatesThread;

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
    explicit VersionControlObserver(QObject* parent = nullptr);
    ~VersionControlObserver() override;

    void setModel(KFileItemModel* model);
    KFileItemModel* model() const;

    QList<QAction*> actions(const KFileItemList& items) const;

signals:
    /**
     * Is emitted if an information message with the content \a msg
     * should be shown.
     */
    void infoMessage(const QString& msg);

    /**
     * Is emitted if an error message with the content \a msg
     * should be shown.
     */
    void errorMessage(const QString& msg);

    /**
     * Is emitted if an "operation completed" message with the content \a msg
     * should be shown.
     */
    void operationCompletedMessage(const QString& msg);

private slots:
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

    void verifyDirectory();

    /**
     * Is invoked if the thread m_updateItemStatesThread has been finished
     * and applys the item states.
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
    int createItemStatesList(QMap<QString, QVector<ItemState> >& itemStates,
                             const int firstIndex = 0);

    /**
     * Returns a matching plugin for the given directory.
     * 0 is returned, if no matching plugin has been found.
     */
    KVersionControlPlugin* searchPlugin(const QUrl& directory);

    /**
     * Returns true, if the directory contains a version control information.
     */
    bool isVersioned() const;

private:
    bool m_pendingItemStatesUpdate;
    bool m_versionedDirectory;
    bool m_silentUpdate; // if true, no messages will be send during the update
                         // of version states

    KFileItemModel* m_model;

    QTimer* m_dirVerificationTimer;

    bool m_pluginsInitialized;
    KVersionControlPlugin* m_plugin;
    QList<KVersionControlPlugin*> m_plugins;
    UpdateItemStatesThread* m_updateItemStatesThread;

    friend class UpdateItemStatesThread;
};

#endif // REVISIONCONTROLOBSERVER_H

