/*
 * SPDX-FileCopyrightText: 2008-2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2010 Christian Muehlhaeuser <muesli@gmail.com>
 * SPDX-FileCopyrightText: 2021 Kai Uwe Broulik <kde@broulik.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLACESPANEL_H
#define PLACESPANEL_H

#include "panels/panel.h"

#include <QUrl>
#include <KFilePlacesView>

#include <Solid/SolidNamespace> // Solid::ErrorType

class QTimer;
namespace Solid
{
class StorageAccess;
}

/**
 * @brief Combines bookmarks and mounted devices as list.
 */
class PlacesPanel : public KFilePlacesView
{
    Q_OBJECT

public:
    explicit PlacesPanel(QWidget* parent);
    ~PlacesPanel() override;

    void setUrl(const QUrl &url); // override

    // for compatibility with Panel, actions that are shown
    // on the view's context menu
    QList<QAction*> customContextMenuActions() const;
    void setCustomContextMenuActions(const QList<QAction*>& actions);

    void requestTearDown();
    void proceedWithTearDown();

public Q_SLOTS:
    void readSettings();

Q_SIGNALS:
    void errorMessage(const QString& error);
    void storageTearDownRequested(const QString& mountPath);
    void storageTearDownExternallyRequested(const QString& mountPath);
    void storageTearDownSuccessful();

protected:
    void showEvent(QShowEvent* event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dragLeaveEvent(QDragLeaveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private Q_SLOTS:
    void slotConfigureTrash();
    void slotDragActivationTimeout();
    void slotUrlsDropped(const QUrl& dest, QDropEvent* event, QWidget* parent);
    void slotContextMenuAboutToShow(const QModelIndex &index, QMenu *menu);
    void slotSetupDone(const QModelIndex &index, bool success);
    void slotTearDownRequested(const QModelIndex &index);
    void slotTearDownRequestedExternally(const QString &udi);
    void slotTearDownDone(Solid::ErrorType error, const QVariant& errorData);
    void slotRowsInserted(const QModelIndex &parent, int first, int last);
    void slotRowsAboutToBeRemoved(const QModelIndex &parent, int first, int last);

private:
    void connectDeviceSignals(const QModelIndex &idx);

    QUrl m_url; // only used for initial setUrl
    QList<QAction*> m_customContextMenuActions;

    QTimer *m_dragActivationTimer = nullptr;
    QPersistentModelIndex m_pendingDragActivation;

    Solid::StorageAccess *m_deviceToTearDown = nullptr;

    QAction *m_configureTrashAction;
    QAction *m_lockPanelsAction;
};

#endif // PLACESPANEL_H
