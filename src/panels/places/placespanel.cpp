/***************************************************************************
 *   Copyright (C) 2008-2012 by Peter Penz <peter.penz19@gmail.com>        *
 *                                                                         *
 *   Based on KFilePlacesView from kdelibs:                                *
 *   Copyright (C) 2007 Kevin Ottens <ervin@kde.org>                       *
 *   Copyright (C) 2007 David Faure <faure@kde.org>                        *
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

#include "placespanel.h"

#include "dolphin_generalsettings.h"
#include "global.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kstandarditem.h"
#include "placesitem.h"
#include "placesitemeditdialog.h"
#include "placesitemlistgroupheader.h"
#include "placesitemlistwidget.h"
#include "placesitemmodel.h"
#include "placesview.h"
#include "trash/dolphintrash.h"
#include "views/draganddrophelper.h"

#include <KDirNotify>
#include <KFileItem>
#include <KFilePlacesModel>
#include <KIO/DropJob>
#include <KIO/EmptyTrashJob>
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KIconLoader>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNotification>

#include <QGraphicsSceneDragDropEvent>
#include <QIcon>
#include <QMenu>
#include <QVBoxLayout>

PlacesPanel::PlacesPanel(QWidget* parent) :
    Panel(parent),
    m_controller(nullptr),
    m_model(nullptr),
    m_view(nullptr),
    m_storageSetupFailedUrl(),
    m_triggerStorageSetupButton(),
    m_itemDropEventIndex(-1),
    m_itemDropEventMimeData(nullptr),
    m_itemDropEvent(nullptr)
{
}

PlacesPanel::~PlacesPanel()
{
}

void PlacesPanel::proceedWithTearDown()
{
    m_model->proceedWithTearDown();
}

bool PlacesPanel::urlChanged()
{
    if (!url().isValid() || url().scheme().contains(QStringLiteral("search"))) {
        // Skip results shown by a search, as possible identical
        // directory names are useless without parent-path information.
        return false;
    }

    if (m_controller) {
        selectClosestItem();
    }

    return true;
}

void PlacesPanel::readSettings()
{
    if (m_controller) {
	const int delay = GeneralSettings::autoExpandFolders() ? 750 : -1;
	m_controller->setAutoActivationDelay(delay);
    }
}

void PlacesPanel::showEvent(QShowEvent* event)
{
    if (event->spontaneous()) {
        Panel::showEvent(event);
        return;
    }

    if (!m_controller) {
        // Postpone the creating of the controller to the first show event.
        // This assures that no performance and memory overhead is given when the folders panel is not
        // used at all and stays invisible.
        m_model = new PlacesItemModel(this);
        m_model->setGroupedSorting(true);
        connect(m_model, &PlacesItemModel::errorMessage,
                this, &PlacesPanel::errorMessage);
        connect(m_model, &PlacesItemModel::storageTearDownRequested,
                this, &PlacesPanel::storageTearDownRequested);
        connect(m_model, &PlacesItemModel::storageTearDownExternallyRequested,
                this, &PlacesPanel::storageTearDownExternallyRequested);

        m_view = new PlacesView();
        m_view->setWidgetCreator(new KItemListWidgetCreator<PlacesItemListWidget>());
        m_view->setGroupHeaderCreator(new KItemListGroupHeaderCreator<PlacesItemListGroupHeader>());

        m_controller = new KItemListController(m_model, m_view, this);
        m_controller->setSelectionBehavior(KItemListController::SingleSelection);
        m_controller->setSingleClickActivationEnforced(true);

        readSettings();

        connect(m_controller, &KItemListController::itemActivated, this, &PlacesPanel::slotItemActivated);
        connect(m_controller, &KItemListController::itemMiddleClicked, this, &PlacesPanel::slotItemMiddleClicked);
        connect(m_controller, &KItemListController::itemContextMenuRequested, this, &PlacesPanel::slotItemContextMenuRequested);
        connect(m_controller, &KItemListController::viewContextMenuRequested, this, &PlacesPanel::slotViewContextMenuRequested);
        connect(m_controller, &KItemListController::itemDropEvent, this, &PlacesPanel::slotItemDropEvent);
        connect(m_controller, &KItemListController::aboveItemDropEvent, this, &PlacesPanel::slotAboveItemDropEvent);

        KItemListContainer* container = new KItemListContainer(m_controller, this);
        container->setEnabledFrame(false);

        QVBoxLayout* layout = new QVBoxLayout(this);
        layout->setMargin(0);
        layout->addWidget(container);

        selectClosestItem();
    }

    Panel::showEvent(event);
}

void PlacesPanel::slotItemActivated(int index)
{
    triggerItem(index, Qt::LeftButton);
}

void PlacesPanel::slotItemMiddleClicked(int index)
{
    triggerItem(index, Qt::MiddleButton);
}

void PlacesPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    PlacesItem* item = m_model->placesItem(index);
    if (!item) {
        return;
    }

    QMenu menu(this);

    QAction* emptyTrashAction = nullptr;
    QAction* editAction = nullptr;
    QAction* teardownAction = nullptr;
    QAction* ejectAction = nullptr;

    const QString label = item->text();

    const bool isDevice = !item->udi().isEmpty();
    const bool isTrash = (item->url().scheme() == QLatin1String("trash"));
    if (isDevice) {
        ejectAction = m_model->ejectAction(index);
        if (ejectAction) {
            ejectAction->setParent(&menu);
            menu.addAction(ejectAction);
        }

        teardownAction = m_model->teardownAction(index);
        if (teardownAction) {
            teardownAction->setParent(&menu);
            menu.addAction(teardownAction);
        }

        if (teardownAction || ejectAction) {
            menu.addSeparator();
        }
    } else {
        if (isTrash) {
            emptyTrashAction = menu.addAction(QIcon::fromTheme(QStringLiteral("trash-empty")), i18nc("@action:inmenu", "Empty Trash"));
            emptyTrashAction->setEnabled(item->icon() == QLatin1String("user-trash-full"));
            menu.addSeparator();
        }
    }

    QAction* openInNewWindowAction = menu.addAction(QIcon::fromTheme("window-new"), i18nc("@item:inmenu", "Open in New Window"));
    QAction* openInNewTabAction = menu.addAction(QIcon::fromTheme("tab-new"), i18nc("@item:inmenu", "Open in New Tab"));
    if (!isDevice && !isTrash) {
        menu.addSeparator();
    }

    if (!isDevice) {
        editAction = menu.addAction(QIcon::fromTheme("document-properties"), i18nc("@item:inmenu", "Edit..."));
    }

    QAction* removeAction = nullptr;
    if (!isDevice && !item->isSystemItem()) {
        removeAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@item:inmenu", "Remove"));
    }

    QAction* hideAction = menu.addAction(i18nc("@item:inmenu", "Hide"));
    hideAction->setCheckable(true);
    hideAction->setChecked(item->isHidden());

    buildGroupContextMenu(&menu, index);

    QAction* action = menu.exec(pos.toPoint());
    if (action) {
        if (action == emptyTrashAction) {
            Trash::empty(this);
        } else {
            // The index might have changed if devices were added/removed while
            // the context menu was open.
            index = m_model->index(item);
            if (index < 0) {
                // The item is not in the model any more, probably because it was an
                // external device that has been removed while the context menu was open.
                return;
            }

            if (action == editAction) {
                editEntry(index);
            } else if (action == removeAction) {
                m_model->deleteItem(index);
            } else if (action == hideAction) {
                item->setHidden(hideAction->isChecked());
            } else if (action == openInNewWindowAction) {
                Dolphin::openNewWindow({KFilePlacesModel::convertedUrl(m_model->data(index).value("url").toUrl())}, this);
            } else if (action == openInNewTabAction) {
                // TriggerItem does set up the storage first and then it will
                // emit the slotItemMiddleClicked signal, because of Qt::MiddleButton.
                triggerItem(index, Qt::MiddleButton);
            } else if (action == teardownAction) {
                m_model->requestTearDown(index);
            } else if (action == ejectAction) {
                m_model->requestEject(index);
            }
        }
    }

    selectClosestItem();
}

void PlacesPanel::slotViewContextMenuRequested(const QPointF& pos)
{
    QMenu menu(this);

    QAction* addAction = menu.addAction(QIcon::fromTheme(QStringLiteral("document-new")), i18nc("@item:inmenu", "Add Entry..."));

    QAction* showAllAction = nullptr;
    if (m_model->hiddenCount() > 0) {
        showAllAction = menu.addAction(i18nc("@item:inmenu", "Show All Entries"));
        showAllAction->setCheckable(true);
        showAllAction->setChecked(m_model->hiddenItemsShown());
    }

    buildGroupContextMenu(&menu, m_controller->indexCloseToMousePressedPosition());

    QMenu* iconSizeSubMenu = new QMenu(i18nc("@item:inmenu", "Icon Size"), &menu);

    struct IconSizeInfo
    {
        int size;
        const char* context;
        const char* text;
    };

    const int iconSizeCount = 4;
    static const IconSizeInfo iconSizes[iconSizeCount] = {
        {KIconLoader::SizeSmall,        I18N_NOOP2_NOSTRIP("Small icon size", "Small (%1x%2)")},
        {KIconLoader::SizeSmallMedium,  I18N_NOOP2_NOSTRIP("Medium icon size", "Medium (%1x%2)")},
        {KIconLoader::SizeMedium,       I18N_NOOP2_NOSTRIP("Large icon size", "Large (%1x%2)")},
        {KIconLoader::SizeLarge,        I18N_NOOP2_NOSTRIP("Huge icon size", "Huge (%1x%2)")}
    };

    QMap<QAction*, int> iconSizeActionMap;
    QActionGroup* iconSizeGroup = new QActionGroup(iconSizeSubMenu);

    for (int i = 0; i < iconSizeCount; ++i) {
        const int size = iconSizes[i].size;
        const QString text = i18nc(iconSizes[i].context, iconSizes[i].text,
                                   size, size);

        QAction* action = iconSizeSubMenu->addAction(text);
        iconSizeActionMap.insert(action, size);
        action->setActionGroup(iconSizeGroup);
        action->setCheckable(true);
        action->setChecked(m_view->iconSize() == size);
    }

    menu.addMenu(iconSizeSubMenu);

    menu.addSeparator();
    foreach (QAction* action, customContextMenuActions()) {
        menu.addAction(action);
    }

    QAction* action = menu.exec(pos.toPoint());
    if (action) {
        if (action == addAction) {
            addEntry();
        } else if (action == showAllAction) {
            m_model->setHiddenItemsShown(showAllAction->isChecked());
        } else if (iconSizeActionMap.contains(action)) {
            m_view->setIconSize(iconSizeActionMap.value(action));
        }
    }

    selectClosestItem();
}

QAction *PlacesPanel::buildGroupContextMenu(QMenu *menu, int index)
{
    if (index == -1) {
        return nullptr;
    }

    KFilePlacesModel::GroupType groupType = m_model->groupType(index);
    QAction *hideGroupAction = menu->addAction(i18nc("@item:inmenu", "Hide Section '%1'", m_model->item(index)->group()));
    hideGroupAction->setCheckable(true);
    hideGroupAction->setChecked(m_model->isGroupHidden(groupType));

    connect(hideGroupAction, &QAction::triggered, this, [this, groupType, hideGroupAction]{
        m_model->setGroupHidden(groupType, hideGroupAction->isChecked());
    });

    return hideGroupAction;
}

void PlacesPanel::slotItemDropEvent(int index, QGraphicsSceneDragDropEvent* event)
{
    if (index < 0) {
        return;
    }

    const PlacesItem* destItem = m_model->placesItem(index);

    if (destItem->isSearchOrTimelineUrl()) {
        return;
    }

    if (m_model->storageSetupNeeded(index)) {
        connect(m_model, &PlacesItemModel::storageSetupDone,
                this, &PlacesPanel::slotItemDropEventStorageSetupDone);

        m_itemDropEventIndex = index;

        // Make a full copy of the Mime-Data
        m_itemDropEventMimeData = new QMimeData;
        m_itemDropEventMimeData->setText(event->mimeData()->text());
        m_itemDropEventMimeData->setHtml(event->mimeData()->html());
        m_itemDropEventMimeData->setUrls(event->mimeData()->urls());
        m_itemDropEventMimeData->setImageData(event->mimeData()->imageData());
        m_itemDropEventMimeData->setColorData(event->mimeData()->colorData());

        m_itemDropEvent = new QDropEvent(event->pos().toPoint(),
                                         event->possibleActions(),
                                         m_itemDropEventMimeData,
                                         event->buttons(),
                                         event->modifiers());

        m_model->requestStorageSetup(index);
        return;
    }

    QUrl destUrl = destItem->url();
    QDropEvent dropEvent(event->pos().toPoint(),
                         event->possibleActions(),
                         event->mimeData(),
                         event->buttons(),
                         event->modifiers());

    slotUrlsDropped(destUrl, &dropEvent, this);
}

void PlacesPanel::slotItemDropEventStorageSetupDone(int index, bool success)
{
    disconnect(m_model, &PlacesItemModel::storageSetupDone,
               this, &PlacesPanel::slotItemDropEventStorageSetupDone);

    if ((index == m_itemDropEventIndex) && m_itemDropEvent && m_itemDropEventMimeData) {
        if (success) {
            QUrl destUrl = m_model->placesItem(index)->url();
            slotUrlsDropped(destUrl, m_itemDropEvent, this);
        }

        delete m_itemDropEventMimeData;
        delete m_itemDropEvent;

        m_itemDropEventIndex = -1;
        m_itemDropEventMimeData = nullptr;
        m_itemDropEvent = nullptr;
    }
}

void PlacesPanel::slotAboveItemDropEvent(int index, QGraphicsSceneDragDropEvent* event)
{
    m_model->dropMimeDataBefore(index, event->mimeData());
}

void PlacesPanel::slotUrlsDropped(const QUrl& dest, QDropEvent* event, QWidget* parent)
{
    KIO::DropJob *job = DragAndDropHelper::dropUrls(dest, event, parent);
    if (job) {
        connect(job, &KIO::DropJob::result, this, [this](KJob *job) { if (job->error()) emit errorMessage(job->errorString()); });
    }
}

void PlacesPanel::slotStorageSetupDone(int index, bool success)
{
    disconnect(m_model, &PlacesItemModel::storageSetupDone,
               this, &PlacesPanel::slotStorageSetupDone);

    if (m_triggerStorageSetupButton == Qt::NoButton) {
        return;
    }

    if (success) {
        Q_ASSERT(!m_model->storageSetupNeeded(index));
        triggerItem(index, m_triggerStorageSetupButton);
        m_triggerStorageSetupButton = Qt::NoButton;
    } else {
        setUrl(m_storageSetupFailedUrl);
        m_storageSetupFailedUrl = QUrl();
    }
}

void PlacesPanel::addEntry()
{
    const int index = m_controller->selectionManager()->currentItem();
    const QUrl url = m_model->data(index).value("url").toUrl();

    QPointer<PlacesItemEditDialog> dialog = new PlacesItemEditDialog(this);
    dialog->setWindowTitle(i18nc("@title:window", "Add Places Entry"));
    dialog->setAllowGlobal(true);
    dialog->setUrl(url);
    if (dialog->exec() == QDialog::Accepted) {
        m_model->createPlacesItem(dialog->text(), dialog->url(), dialog->icon());
    }

    delete dialog;
}

void PlacesPanel::editEntry(int index)
{
    QHash<QByteArray, QVariant> data = m_model->data(index);

    QPointer<PlacesItemEditDialog> dialog = new PlacesItemEditDialog(this);
    dialog->setWindowTitle(i18nc("@title:window", "Edit Places Entry"));
    dialog->setIcon(data.value("iconName").toString());
    dialog->setText(data.value("text").toString());
    dialog->setUrl(data.value("url").toUrl());
    dialog->setAllowGlobal(true);
    if (dialog->exec() == QDialog::Accepted) {
        PlacesItem* oldItem = m_model->placesItem(index);
        if (oldItem) {
            oldItem->setText(dialog->text());
            oldItem->setUrl(dialog->url());
            oldItem->setIcon(dialog->icon());
            m_model->refresh();
        }
    }

    delete dialog;
}

void PlacesPanel::selectClosestItem()
{
    const int index = m_model->closestItem(url());
    KItemListSelectionManager* selectionManager = m_controller->selectionManager();
    selectionManager->setCurrentItem(index);
    selectionManager->clearSelection();
    selectionManager->setSelected(index);
}

void PlacesPanel::triggerItem(int index, Qt::MouseButton button)
{
    const PlacesItem* item = m_model->placesItem(index);
    if (!item) {
        return;
    }

    if (m_model->storageSetupNeeded(index)) {
        m_triggerStorageSetupButton = button;
        m_storageSetupFailedUrl = url();

        connect(m_model, &PlacesItemModel::storageSetupDone,
                this, &PlacesPanel::slotStorageSetupDone);

        m_model->requestStorageSetup(index);
    } else {
        m_triggerStorageSetupButton = Qt::NoButton;

        const QUrl url = m_model->data(index).value("url").toUrl();
        if (!url.isEmpty()) {
            if (button == Qt::MiddleButton) {
                emit placeMiddleClicked(KFilePlacesModel::convertedUrl(url));
            } else {
                emit placeActivated(KFilePlacesModel::convertedUrl(url));
            }
        }
    }
}
