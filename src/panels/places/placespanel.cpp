/*
 * SPDX-FileCopyrightText: 2008-2012 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on KFilePlacesView from kdelibs:
 * SPDX-FileCopyrightText: 2007 Kevin Ottens <ervin@kde.org>
 * SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placespanel.h"

#include "dolphin_generalsettings.h"
#include "global.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kstandarditem.h"
#include "placesitem.h"
#include "placesitemlistgroupheader.h"
#include "placesitemlistwidget.h"
#include "placesitemmodel.h"
#include "placesview.h"
#include "trash/dolphintrash.h"
#include "views/draganddrophelper.h"
#include "settings/dolphinsettingsdialog.h"

#include <KFilePlaceEditDialog>
#include <KFilePlacesModel>
#include <KIO/DropJob>
#include <KIO/EmptyTrashJob>
#include <KIO/Job>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMountPoint>
#include <KPropertiesDialog>

#include <QActionGroup>
#include <QApplication>
#include <QGraphicsSceneDragDropEvent>
#include <QIcon>
#include <QMenu>
#include <QMimeData>
#include <QVBoxLayout>
#include <QToolTip>

PlacesPanel::PlacesPanel(QWidget* parent) :
        Panel(parent),
        m_controller(nullptr),
        m_model(nullptr),
        m_view(nullptr),
        m_storageSetupFailedUrl(),
        m_triggerStorageSetupModifier(),
        m_itemDropEventIndex(-1),
        m_itemDropEventMimeData(nullptr),
        m_itemDropEvent(nullptr),
        m_tooltipTimer()
{
    m_tooltipTimer.setInterval(500);
    m_tooltipTimer.setSingleShot(true);
    connect(&m_tooltipTimer, &QTimer::timeout, this, &PlacesPanel::slotShowTooltip);
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
    if (!url().isValid() || url().scheme().contains(QLatin1String("search"))) {
        // Skip results shown by a search, as possible identical
        // directory names are useless without parent-path information.
        return false;
    }

    if (m_controller) {
        selectItem();
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
        connect(m_model, &PlacesItemModel::storageTearDownSuccessful,
                this, &PlacesPanel::storageTearDownSuccessful);

        m_view = new PlacesView();
        m_view->setWidgetCreator(new KItemListWidgetCreator<PlacesItemListWidget>());
        m_view->setGroupHeaderCreator(new KItemListGroupHeaderCreator<PlacesItemListGroupHeader>());

        installEventFilter(this);

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
        layout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(container);

        selectItem();
    }

    Panel::showEvent(event);
}

bool PlacesPanel::eventFilter(QObject * /* obj */, QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {

        QHelpEvent *hoverEvent = reinterpret_cast<QHelpEvent *>(event);

        m_hoveredIndex = m_view->itemAt(hoverEvent->pos());
        m_hoverPos = mapToGlobal(hoverEvent->pos());

        m_tooltipTimer.start();
        return true;
    }
    return false;
}

void PlacesPanel::slotItemActivated(int index)
{
    const auto modifiers = QGuiApplication::keyboardModifiers();
    // keep in sync with KUrlNavigator::slotNavigatorButtonClicked
    if (modifiers & Qt::ControlModifier && modifiers & Qt::ShiftModifier) {
        triggerItem(index, TriggerItemModifier::ToNewActiveTab);
    } else if (modifiers & Qt::ControlModifier) {
        triggerItem(index, TriggerItemModifier::ToNewTab);
    } else if (modifiers & Qt::ShiftModifier) {
        triggerItem(index, TriggerItemModifier::ToNewWindow);
    } else {
        triggerItem(index, TriggerItemModifier::None);
    }
}

void PlacesPanel::slotItemMiddleClicked(int index)
{
    const auto modifiers = QGuiApplication::keyboardModifiers();
    // keep in sync with KUrlNavigator::slotNavigatorButtonClicked
    if (modifiers & Qt::ShiftModifier) {
        triggerItem(index, TriggerItemModifier::ToNewActiveTab);
    } else {
        triggerItem(index, TriggerItemModifier::ToNewTab);
    }
}

void PlacesPanel::slotItemContextMenuRequested(int index, const QPointF& pos)
{
    PlacesItem* item = m_model->placesItem(index);
    if (!item) {
        return;
    }

    QMenu menu(this);

    QAction* emptyTrashAction = nullptr;
    QAction* configureTrashAction = nullptr;
    QAction* editAction = nullptr;
    QAction* teardownAction = nullptr;
    QAction* ejectAction = nullptr;
    QAction* mountAction = nullptr;

    const bool isDevice = !item->udi().isEmpty();
    const bool isTrash = (item->url().scheme() == QLatin1String("trash"));
    if (isTrash) {
        emptyTrashAction = menu.addAction(QIcon::fromTheme(QStringLiteral("trash-empty")), i18nc("@action:inmenu", "Empty Trash"));
        emptyTrashAction->setEnabled(item->icon() == QLatin1String("user-trash-full"));
        menu.addSeparator();
    }

    QAction* openInNewTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-new")), i18nc("@item:inmenu", "Open in New Tab"));
    QAction* openInNewWindowAction = menu.addAction(QIcon::fromTheme(QStringLiteral("window-new")), i18nc("@item:inmenu", "Open in New Window"));
    QAction* propertiesAction = nullptr;
    if (item->url().isLocalFile()) {
        propertiesAction = menu.addAction(QIcon::fromTheme(QStringLiteral("document-properties")), i18nc("@action:inmenu", "Properties"));
    }
    if (!isDevice) {
        menu.addSeparator();
    }

    if (isDevice) {
        ejectAction = m_model->ejectAction(index);
        if (ejectAction) {
            ejectAction->setParent(&menu);
            menu.addAction(ejectAction);
        }

        teardownAction = m_model->teardownAction(index);
        if (teardownAction) {
            // Disable teardown option for root and home partitions
            bool teardownEnabled = item->url() != QUrl::fromLocalFile(QDir::rootPath());
            if (teardownEnabled) {
                KMountPoint::Ptr mountPoint = KMountPoint::currentMountPoints().findByPath(QDir::homePath());
                if (mountPoint && item->url() == QUrl::fromLocalFile(mountPoint->mountPoint())) {
                    teardownEnabled = false;
                }
            }
            teardownAction->setEnabled(teardownEnabled);

            teardownAction->setParent(&menu);
            menu.addAction(teardownAction);
        }

        if (item->storageSetupNeeded()) {
            mountAction = menu.addAction(QIcon::fromTheme(QStringLiteral("media-mount")), i18nc("@action:inmenu", "Mount"));
        }

        if (teardownAction || ejectAction || mountAction) {
            menu.addSeparator();
        }
    }

    if (isTrash) {
        configureTrashAction = menu.addAction(QIcon::fromTheme(QStringLiteral("configure")), i18nc("@action:inmenu", "Configure Trash..."));
    }

    if (!isDevice) {
        editAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-entry")), i18nc("@item:inmenu", "Edit..."));
    }

    QAction* removeAction = nullptr;
    if (!isDevice && !item->isSystemItem()) {
        removeAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-delete")), i18nc("@item:inmenu", "Remove"));
    }

    QAction* hideAction = menu.addAction(QIcon::fromTheme(QStringLiteral("view-hidden")), i18nc("@item:inmenu", "Hide"));
    hideAction->setCheckable(true);
    hideAction->setChecked(item->isHidden());

    buildGroupContextMenu(&menu, index);

    QAction* action = menu.exec(pos.toPoint());
    if (action) {
        if (action == emptyTrashAction) {
            Trash::empty(this);
        } else if (action == configureTrashAction) {
            DolphinSettingsDialog* settingsDialog = new DolphinSettingsDialog(item->url(), this);
            settingsDialog->setCurrentPage(settingsDialog->trashSettings);
            settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
            settingsDialog->show();
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
                if (!m_model->hiddenCount()) {
                    showHiddenEntries(false);
                }
            } else if (action == openInNewWindowAction) {
                Dolphin::openNewWindow({KFilePlacesModel::convertedUrl(m_model->data(index).value("url").toUrl())}, this);
            } else if (action == openInNewTabAction) {
                triggerItem(index, TriggerItemModifier::ToNewTab);
            } else if (action == mountAction) {
                m_model->requestStorageSetup(index);
            } else if (action == teardownAction) {
                m_model->requestTearDown(index);
            } else if (action == ejectAction) {
                m_model->requestEject(index);
            } else if (action == propertiesAction) {
                KPropertiesDialog* dialog = new KPropertiesDialog(item->url(), this);
                dialog->setAttribute(Qt::WA_DeleteOnClose);
                dialog->show();
            }
        }
    }

    selectItem();
}

void PlacesPanel::slotViewContextMenuRequested(const QPointF& pos)
{
    QMenu menu(this);

    QAction* addAction = menu.addAction(QIcon::fromTheme(QStringLiteral("document-new")), i18nc("@item:inmenu", "Add Entry..."));

    QAction* showAllAction = menu.addAction(i18nc("@item:inmenu", "Show Hidden Places"));
    showAllAction->setCheckable(true);
    showAllAction->setChecked(m_model->hiddenItemsShown());
    showAllAction->setIcon(QIcon::fromTheme(m_model->hiddenItemsShown() ? QStringLiteral("view-visible") : QStringLiteral("view-hidden")));
    showAllAction->setEnabled(m_model->hiddenCount());

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
        {KIconLoader::SizeSmall,        I18NC_NOOP("Small icon size", "Small (%1x%2)")},
        {KIconLoader::SizeSmallMedium,  I18NC_NOOP("Medium icon size", "Medium (%1x%2)")},
        {KIconLoader::SizeMedium,       I18NC_NOOP("Large icon size", "Large (%1x%2)")},
        {KIconLoader::SizeLarge,        I18NC_NOOP("Huge icon size", "Huge (%1x%2)")}
    };

    QHash<QAction*, int> iconSizeActionMap;
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
    const auto actions = customContextMenuActions();
    for (QAction* action : actions) {
        menu.addAction(action);
    }

    QAction* action = menu.exec(pos.toPoint());
    if (action) {
        if (action == addAction) {
            addEntry();
        } else if (action == showAllAction) {
            showHiddenEntries(showAllAction->isChecked());
        } else if (iconSizeActionMap.contains(action)) {
            m_view->setIconSize(iconSizeActionMap.value(action));
        }
    }

    selectItem();
}

QAction *PlacesPanel::buildGroupContextMenu(QMenu *menu, int index)
{
    if (index == -1) {
        return nullptr;
    }

    KFilePlacesModel::GroupType groupType = m_model->groupType(index);
    QAction *hideGroupAction = menu->addAction(QIcon::fromTheme(QStringLiteral("view-hidden")), i18nc("@item:inmenu", "Hide Section '%1'", m_model->item(index)->group()));
    hideGroupAction->setCheckable(true);
    hideGroupAction->setChecked(m_model->isGroupHidden(groupType));

    connect(hideGroupAction, &QAction::triggered, this, [this, groupType, hideGroupAction]{
        m_model->setGroupHidden(groupType, hideGroupAction->isChecked());
        if (!m_model->hiddenCount()) {
            showHiddenEntries(false);
        }
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
        connect(job, &KIO::DropJob::result, this, [this](KJob *job) { if (job->error()) Q_EMIT errorMessage(job->errorString()); });
    }
}

void PlacesPanel::slotStorageSetupDone(int index, bool success)
{
    disconnect(m_model, &PlacesItemModel::storageSetupDone,
               this, &PlacesPanel::slotStorageSetupDone);

    if (m_triggerStorageSetupModifier == TriggerItemModifier::None) {
        return;
    }

    if (success) {
        Q_ASSERT(!m_model->storageSetupNeeded(index));
        triggerItem(index, m_triggerStorageSetupModifier);
        m_triggerStorageSetupModifier = TriggerItemModifier::None;
    } else {
        setUrl(m_storageSetupFailedUrl);
        m_storageSetupFailedUrl = QUrl();
    }
}

void PlacesPanel::slotShowTooltip()
{
    const QUrl url = m_model->data(m_hoveredIndex.value_or(-1)).value("url").value<QUrl>();
    const QString text = url.toDisplayString(QUrl::PreferLocalFile);
    QToolTip::showText(m_hoverPos, text);
}

void PlacesPanel::addEntry()
{
    const int index = m_controller->selectionManager()->currentItem();
    const QUrl url = m_model->data(index).value("url").toUrl();
    const QString text = url.fileName().isEmpty() ? url.toDisplayString(QUrl::PreferLocalFile) : url.fileName();

    QPointer<KFilePlaceEditDialog> dialog = new KFilePlaceEditDialog(true, url, text, QString(), true, false, KIconLoader::SizeMedium, this);
    if (dialog->exec() == QDialog::Accepted) {
        const QString appName = dialog->applicationLocal() ? QCoreApplication::applicationName() : QString();
        m_model->createPlacesItem(dialog->label(), dialog->url(), dialog->icon(), appName);
    }

    delete dialog;
}

void PlacesPanel::editEntry(int index)
{
    QHash<QByteArray, QVariant> data = m_model->data(index);
    const QUrl url = data.value("url").toUrl();
    const QString text = data.value("text").toString();
    const QString iconName = data.value("iconName").toString();
    const bool applicationLocal = !data.value("applicationName").toString().isEmpty();

    QPointer<KFilePlaceEditDialog> dialog = new KFilePlaceEditDialog(true, url, text, iconName, true, applicationLocal, KIconLoader::SizeMedium, this);
    if (dialog->exec() == QDialog::Accepted) {
        PlacesItem* oldItem = m_model->placesItem(index);
        if (oldItem) {
            const QString appName = dialog->applicationLocal() ? QCoreApplication::applicationName() : QString();
            oldItem->setApplicationName(appName);
            oldItem->setText(dialog->label());
            oldItem->setUrl(dialog->url());
            oldItem->setIcon(dialog->icon());
            m_model->refresh();
        }
    }

    delete dialog;
}

void PlacesPanel::selectItem()
{
    const int index = m_model->closestItem(url());
    KItemListSelectionManager* selectionManager = m_controller->selectionManager();
    selectionManager->setCurrentItem(index);
    selectionManager->clearSelection();

    const QUrl closestUrl = m_model->url(index);
    if (!closestUrl.path().isEmpty() && url() == closestUrl) {
        selectionManager->setSelected(index);
    }
}

void PlacesPanel::triggerItem(int index, TriggerItemModifier modifier)
{
    const PlacesItem* item = m_model->placesItem(index);
    if (!item) {
        return;
    }

    if (m_model->storageSetupNeeded(index)) {
        m_triggerStorageSetupModifier = modifier;
        m_storageSetupFailedUrl = url();

        connect(m_model, &PlacesItemModel::storageSetupDone,
                this, &PlacesPanel::slotStorageSetupDone);

        m_model->requestStorageSetup(index);
    } else {
        m_triggerStorageSetupModifier = TriggerItemModifier::None;

        const QUrl url = m_model->data(index).value("url").toUrl();
        if (!url.isEmpty()) {
            switch (modifier) {
                case TriggerItemModifier::ToNewTab:
                    Q_EMIT placeActivatedInNewTab(KFilePlacesModel::convertedUrl(url));
                    break;
                case TriggerItemModifier::ToNewActiveTab:
                    Q_EMIT placeActivatedInNewActiveTab(KFilePlacesModel::convertedUrl(url));
                    break;
                case TriggerItemModifier::ToNewWindow:
                    Dolphin::openNewWindow({KFilePlacesModel::convertedUrl(url)}, this);
                    break;
                case TriggerItemModifier::None:
                    Q_EMIT placeActivated(KFilePlacesModel::convertedUrl(url));
                    break;
            }
        }
    }
}

void PlacesPanel::showHiddenEntries(bool shown)
{
    m_model->setHiddenItemsShown(shown);
    Q_EMIT showHiddenEntriesChanged(shown);
}

int PlacesPanel::hiddenListCount()
{
    if(!m_model) {
        return 0;
    }
    return m_model->hiddenCount();
}
