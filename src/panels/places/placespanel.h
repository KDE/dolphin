/*
 * SPDX-FileCopyrightText: 2008-2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2010 Christian Muehlhaeuser <muesli@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLACESPANEL_H
#define PLACESPANEL_H

#include <optional>

#include "panels/panel.h"

#include <QUrl>
#include <QTimer>

class KItemListController;
class PlacesItemModel;
class PlacesView;
class QGraphicsSceneDragDropEvent;
class QMenu;
class QMimeData;
/**
 * @brief Combines bookmarks and mounted devices as list.
 */
class PlacesPanel : public Panel
{
    Q_OBJECT

public:
    explicit PlacesPanel(QWidget* parent);
    ~PlacesPanel() override;
    void proceedWithTearDown();

    bool eventFilter(QObject *obj, QEvent *event) override;

Q_SIGNALS:
    void placeActivated(const QUrl& url);
    void placeActivatedInNewTab(const QUrl &url);
    void placeActivatedInNewActiveTab(const QUrl &url);
    void errorMessage(const QString& error);
    void storageTearDownRequested(const QString& mountPath);
    void storageTearDownExternallyRequested(const QString& mountPath);
    void showHiddenEntriesChanged(bool shown);
    void storageTearDownSuccessful();

protected:
    bool urlChanged() override;
    void showEvent(QShowEvent* event) override;

public Q_SLOTS:
    void readSettings() override;
    void showHiddenEntries(bool shown);
    int hiddenListCount();

private Q_SLOTS:
    void slotItemActivated(int index);
    void slotItemMiddleClicked(int index);
    void slotItemContextMenuRequested(int index, const QPointF& pos);
    void slotViewContextMenuRequested(const QPointF& pos);
    void slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event);
    void slotItemDropEventStorageSetupDone(int index, bool success);
    void slotAboveItemDropEvent(int index, QGraphicsSceneDragDropEvent* event);
    void slotUrlsDropped(const QUrl& dest, QDropEvent* event, QWidget* parent);
    void slotStorageSetupDone(int index, bool success);
    void slotShowTooltip();

private:
    enum class TriggerItemModifier { None, ToNewTab, ToNewActiveTab, ToNewWindow };

private:
    void addEntry();
    void editEntry(int index);

    /**
     * Selects the item that matches the URL set
     * for the panel (see Panel::setUrl()).
     */
    void selectItem();

    void triggerItem(int index, TriggerItemModifier modifier);

    QAction* buildGroupContextMenu(QMenu* menu, int index);

private:
    KItemListController* m_controller;
    PlacesItemModel* m_model;
    PlacesView* m_view;

    QUrl m_storageSetupFailedUrl;
    TriggerItemModifier m_triggerStorageSetupModifier;

    int m_itemDropEventIndex;
    QMimeData* m_itemDropEventMimeData;
    QDropEvent* m_itemDropEvent;
    QTimer m_tooltipTimer;
    std::optional<int> m_hoveredIndex;
    QPoint m_hoverPos;
};

#endif // PLACESPANEL_H
