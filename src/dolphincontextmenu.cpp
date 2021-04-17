/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Cvetoslav Ludmiloff
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphincontextmenu.h"

#include "dolphin_generalsettings.h"
#include "dolphin_contextmenusettings.h"
#include "dolphinmainwindow.h"
#include "dolphinnewfilemenu.h"
#include "dolphinplacesmodelsingleton.h"
#include "dolphinremoveaction.h"
#include "dolphinviewcontainer.h"
#include "panels/places/placesitem.h"
#include "panels/places/placesitemmodel.h"
#include "trash/dolphintrash.h"
#include "views/dolphinview.h"
#include "views/viewmodecontroller.h"

#include <KActionCollection>
#include <KFileItemActions>
#include <KFileItemListProperties>
#include <KHamburgerMenu>
#include <KIO/EmptyTrashJob>
#include <KIO/JobUiDelegate>
#include <KIO/Paste>
#include <KIO/RestoreJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KNewFileMenu>
#include <KPluginMetaData>
#include <KStandardAction>
#include <KToolBar>
#include <kio_version.h>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QMenuBar>
#include <QMimeDatabase>

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow* parent,
                                       const QPoint& pos,
                                       const KFileItem& fileInfo,
                                       const QUrl& baseUrl) :
    QMenu(parent),
    m_pos(pos),
    m_mainWindow(parent),
    m_fileInfo(fileInfo),
    m_baseUrl(baseUrl),
    m_baseFileItem(nullptr),
    m_selectedItems(),
    m_selectedItemsProperties(nullptr),
    m_context(NoContext),
    m_copyToMenu(parent),
    m_customActions(),
    m_command(None),
    m_removeAction(nullptr)
{
    // The context menu either accesses the URLs of the selected items
    // or the items itself. To increase the performance both lists are cached.
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    m_selectedItems = view->selectedItems();

    QApplication::instance()->installEventFilter(this);

    static_cast<KHamburgerMenu *>(m_mainWindow->actionCollection()->
                action(QStringLiteral("hamburger_menu")))->addToMenu(this);
}

DolphinContextMenu::~DolphinContextMenu()
{
    delete m_baseFileItem;
    m_baseFileItem = nullptr;
    delete m_selectedItemsProperties;
    m_selectedItemsProperties = nullptr;
}

void DolphinContextMenu::setCustomActions(const QList<QAction*>& actions)
{
    m_customActions = actions;
}

DolphinContextMenu::Command DolphinContextMenu::open()
{
    // get the context information
    const auto scheme = m_baseUrl.scheme();
    if (scheme == QLatin1String("trash")) {
        m_context |= TrashContext;
    } else if (scheme.contains(QLatin1String("search"))) {
        m_context |= SearchContext;
    } else if (scheme.contains(QLatin1String("timeline"))) {
        m_context |= TimelineContext;
    }

    if (!m_fileInfo.isNull() && !m_selectedItems.isEmpty()) {
        m_context |= ItemContext;
        // TODO: handle other use cases like devices + desktop files
    }

    // open the corresponding popup for the context
    if (m_context & TrashContext) {
        if (m_context & ItemContext) {
            openTrashItemContextMenu();
        } else {
            openTrashContextMenu();
        }
    } else if (m_context & ItemContext) {
        openItemContextMenu();
    } else {
        openViewportContextMenu();
    }

    return m_command;
}

bool DolphinContextMenu::eventFilter(QObject* object, QEvent* event)
{
    Q_UNUSED(object)

    if(event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        if (m_removeAction && keyEvent->key() == Qt::Key_Shift) {
            if (event->type() == QEvent::KeyPress) {
                m_removeAction->update(DolphinRemoveAction::ShiftState::Pressed);
            } else {
                m_removeAction->update(DolphinRemoveAction::ShiftState::Released);
            }
        }
    }

    return false;
}

void DolphinContextMenu::openTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    QAction* emptyTrashAction = new QAction(QIcon::fromTheme(QStringLiteral("trash-empty")), i18nc("@action:inmenu", "Empty Trash"), this);
    emptyTrashAction->setEnabled(!Trash::isEmpty());
    addAction(emptyTrashAction);

    addCustomActions();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    if (exec(m_pos) == emptyTrashAction) {
        Trash::empty(m_mainWindow);
    }
}

void DolphinContextMenu::openTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    QAction* restoreAction = new QAction(QIcon::fromTheme("restoration"), i18nc("@action:inmenu", "Restore"), m_mainWindow);
    addAction(restoreAction);

    QAction* deleteAction = m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile));
    addAction(deleteAction);

    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    if (exec(m_pos) == restoreAction) {
        QList<QUrl> selectedUrls;
        selectedUrls.reserve(m_selectedItems.count());
        for (const KFileItem &item : qAsConst(m_selectedItems)) {
            selectedUrls.append(item.url());
        }

        KIO::RestoreJob *job = KIO::restoreFromTrash(selectedUrls);
        KJobWidgets::setWindow(job, m_mainWindow);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    }
}

void DolphinContextMenu::addDirectoryItemContextMenu(KFileItemActions &fileItemActions)
{
    // insert 'Open in new window' and 'Open in new tab' entries
    const KFileItemListProperties& selectedItemsProps = selectedItemsProperties();
    if (ContextMenuSettings::showOpenInNewTab()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tab")));
    }
    if (ContextMenuSettings::showOpenInNewWindow()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_window")));
    }

    // Insert 'Open With' entries
    addOpenWithActions(fileItemActions);

    // set up 'Create New' menu
    DolphinNewFileMenu* newFileMenu = new DolphinNewFileMenu(m_mainWindow->actionCollection(), m_mainWindow);
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    newFileMenu->setViewShowsHiddenFiles(view->hiddenFilesShown());
    newFileMenu->checkUpToDate();
    newFileMenu->setPopupFiles(QList<QUrl>() << m_fileInfo.url());
    newFileMenu->setEnabled(selectedItemsProps.supportsWriting());
    connect(newFileMenu, &DolphinNewFileMenu::fileCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);
    connect(newFileMenu, &DolphinNewFileMenu::directoryCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);

    QMenu* menu = newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    addMenu(menu);

    addSeparator();
}

void DolphinContextMenu::openItemContextMenu()
{
    Q_ASSERT(!m_fileInfo.isNull());

    QAction* openParentAction = nullptr;
    QAction* openParentInNewWindowAction = nullptr;
    QAction* openParentInNewTabAction = nullptr;
    const KFileItemListProperties& selectedItemsProps = selectedItemsProperties();

    KFileItemActions fileItemActions;
    fileItemActions.setParentWidget(m_mainWindow);
#if KIO_VERSION >= QT_VERSION_CHECK(5, 82, 0)
    connect(&fileItemActions, &KFileItemActions::error, this, [this](const QString &errorMessage) {
        m_mainWindow->activeViewContainer()->showMessage(errorMessage, DolphinViewContainer::Error);
    });
#endif
    fileItemActions.setItemListProperties(selectedItemsProps);

    if (m_selectedItems.count() == 1) {
        // single files
        if (m_fileInfo.isDir()) {
            addDirectoryItemContextMenu(fileItemActions);
        } else if (m_context & TimelineContext || m_context & SearchContext) {
            addOpenWithActions(fileItemActions);

            openParentAction = new QAction(QIcon::fromTheme(QStringLiteral("document-open-folder")),
                                           i18nc("@action:inmenu",
                                                 "Open Path"),
                                           this);
            addAction(openParentAction);

            openParentInNewWindowAction = new QAction(QIcon::fromTheme(QStringLiteral("window-new")),
                                                    i18nc("@action:inmenu",
                                                          "Open Path in New Window"),
                                                    this);
            addAction(openParentInNewWindowAction);

            openParentInNewTabAction = new QAction(QIcon::fromTheme(QStringLiteral("tab-new")),
                                                   i18nc("@action:inmenu",
                                                         "Open Path in New Tab"),
                                                   this);
            addAction(openParentInNewTabAction);

            addSeparator();
        } else {
            // Insert 'Open With" entries
            addOpenWithActions(fileItemActions);
        }
        if (m_fileInfo.isLink()) {
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("show_target")));
            addSeparator();
        }
    } else {
        // multiple files
        bool selectionHasOnlyDirs = true;
        for (const auto &item : qAsConst(m_selectedItems)) {
            const QUrl& url = DolphinView::openItemAsFolderUrl(item);
            if (url.isEmpty()) {
                selectionHasOnlyDirs = false;
                break;
            }
        }

        if (selectionHasOnlyDirs && ContextMenuSettings::showOpenInNewTab()) {
            // insert 'Open in new tab' entry
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tabs")));
        }
        // Insert 'Open With" entries
        addOpenWithActions(fileItemActions);
    }

    insertDefaultItemActions(selectedItemsProps);

    addAdditionalActions(fileItemActions, selectedItemsProps);

    // insert 'Copy To' and 'Move To' sub menus
    if (ContextMenuSettings::showCopyMoveMenu()) {
        m_copyToMenu.setUrls(m_selectedItems.urlList());
        m_copyToMenu.setReadOnly(!selectedItemsProps.supportsWriting());
        m_copyToMenu.setAutoErrorHandlingEnabled(true);
        m_copyToMenu.addActionsTo(this);
    }

    // insert 'Properties...' entry
    addSeparator();
    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    QAction* activatedAction = exec(m_pos);
    if (activatedAction) {
        if (activatedAction == openParentAction) {
            m_command = OpenParentFolder;
        } else if (activatedAction == openParentInNewWindowAction) {
            m_command = OpenParentFolderInNewWindow;
        } else if (activatedAction == openParentInNewTabAction) {
            m_command = OpenParentFolderInNewTab;
        }
    }
}

void DolphinContextMenu::openViewportContextMenu()
{
    const DolphinView* view = m_mainWindow->activeViewContainer()->view();

    const KFileItemListProperties baseUrlProperties(KFileItemList() << baseFileItem());
    KFileItemActions fileItemActions;
    fileItemActions.setParentWidget(m_mainWindow);
#if KIO_VERSION >= QT_VERSION_CHECK(5, 82, 0)
    connect(&fileItemActions, &KFileItemActions::error, this, [this](const QString &errorMessage) {
        m_mainWindow->activeViewContainer()->showMessage(errorMessage, DolphinViewContainer::Error);
    });
#endif
    fileItemActions.setItemListProperties(baseUrlProperties);

    // Set up and insert 'Create New' menu
    KNewFileMenu* newFileMenu = m_mainWindow->newFileMenu();
    newFileMenu->setViewShowsHiddenFiles(view->hiddenFilesShown());
    newFileMenu->checkUpToDate();
    newFileMenu->setPopupFiles(QList<QUrl>() << m_baseUrl);
    addMenu(newFileMenu->menu());

    // Show "open with" menu items even if the dir is empty, because there are legitimate
    // use cases for this, such as opening an empty dir in Kate or VSCode or something
    addOpenWithActions(fileItemActions);

    QAction* pasteAction = createPasteAction();
    if (pasteAction) {
        addAction(pasteAction);
    }

    // Insert 'Add to Places' entry if it's not already in the places panel
    if (ContextMenuSettings::showAddToPlaces() &&
            !placeExists(m_mainWindow->activeViewContainer()->url())) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("add_to_places")));
    }
    addSeparator();

    // Insert 'Sort By' and 'View Mode'
    if (ContextMenuSettings::showSortBy()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("sort")));
    }
    if (ContextMenuSettings::showViewMode()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("view_mode")));
    }
    if (ContextMenuSettings::showSortBy() || ContextMenuSettings::showViewMode()) {
        addSeparator();
    }

    addAdditionalActions(fileItemActions, baseUrlProperties);
    addCustomActions();

    addSeparator();

    QAction* propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);

    exec(m_pos);
}

void DolphinContextMenu::insertDefaultItemActions(const KFileItemListProperties& properties)
{
    const KActionCollection* collection = m_mainWindow->actionCollection();

    // Insert 'Cut', 'Copy', 'Copy Location' and 'Paste'
    addAction(collection->action(KStandardAction::name(KStandardAction::Cut)));
    addAction(collection->action(KStandardAction::name(KStandardAction::Copy)));
    if (ContextMenuSettings::showCopyLocation()) {
        QAction* copyPathAction = collection->action(QString("copy_location"));
        copyPathAction->setEnabled(m_selectedItems.size() == 1);
        addAction(copyPathAction);
    }
    QAction* pasteAction = createPasteAction();
    if (pasteAction) {
        addAction(pasteAction);
    }

    // Insert 'Duplicate Here'
    if (ContextMenuSettings::showDuplicateHere()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("duplicate")));
    }

    // Insert 'Rename'
    addAction(collection->action(KStandardAction::name(KStandardAction::RenameFile)));

    // Insert 'Add to Places' entry if appropriate
    if (ContextMenuSettings::showAddToPlaces() &&
            m_selectedItems.count() == 1 &&
            m_fileInfo.isDir() &&
            !placeExists(m_fileInfo.url())) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("add_to_places")));
    }

    addSeparator();

    // Insert 'Move to Trash' and/or 'Delete'
    const bool showDeleteAction = (KSharedConfig::openConfig()->group("KDE").readEntry("ShowDeleteCommand", false) ||
                                    !properties.isLocal());
    const bool showMoveToTrashAction = (properties.isLocal() &&
                                        properties.supportsMoving());

    if (showDeleteAction && showMoveToTrashAction) {
        delete m_removeAction;
        m_removeAction = nullptr;
        addAction(m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::MoveToTrash)));
        addAction(m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile)));
    } else if (showDeleteAction && !showMoveToTrashAction) {
        addAction(m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile)));
    } else {
        if (!m_removeAction) {
            m_removeAction = new DolphinRemoveAction(this, m_mainWindow->actionCollection());
        }
        addAction(m_removeAction);
        m_removeAction->update();
    }
}

bool DolphinContextMenu::placeExists(const QUrl& url) const
{
    const KFilePlacesModel* placesModel = DolphinPlacesModelSingleton::instance().placesModel();

    const auto& matchedPlaces = placesModel->match(placesModel->index(0,0), KFilePlacesModel::UrlRole, url, 1, Qt::MatchExactly);

    return !matchedPlaces.isEmpty();
}

QAction* DolphinContextMenu::createPasteAction()
{
    QAction* action = nullptr;
    KFileItem destItem;
    if (!m_fileInfo.isNull() && m_selectedItems.count() <= 1) {
        destItem = m_fileInfo;
    } else {
        destItem = baseFileItem();
    }

    if (!destItem.isNull() && destItem.isDir()) {
        const QMimeData *mimeData = QApplication::clipboard()->mimeData();
        bool canPaste;
        const QString text = KIO::pasteActionText(mimeData, &canPaste, destItem);
        if (canPaste) {
            if (destItem == m_fileInfo) {
                // if paste destination is a selected folder
                action = new QAction(QIcon::fromTheme(QStringLiteral("edit-paste")), text, this);
                connect(action, &QAction::triggered, m_mainWindow, &DolphinMainWindow::pasteIntoFolder);
            } else {
                action = m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
            }
        }
    }

    return action;
}

KFileItemListProperties& DolphinContextMenu::selectedItemsProperties() const
{
    if (!m_selectedItemsProperties) {
        m_selectedItemsProperties = new KFileItemListProperties(m_selectedItems);
    }
    return *m_selectedItemsProperties;
}

KFileItem DolphinContextMenu::baseFileItem()
{
    if (!m_baseFileItem) {
        const DolphinView* view = m_mainWindow->activeViewContainer()->view();
        KFileItem baseItem = view->rootItem();
        if (baseItem.isNull() || baseItem.url() != m_baseUrl) {
            m_baseFileItem = new KFileItem(m_baseUrl);
        } else {
            m_baseFileItem = new KFileItem(baseItem);
        }
    }
    return *m_baseFileItem;
}

void DolphinContextMenu::addOpenWithActions(KFileItemActions& fileItemActions)
{
    // insert 'Open With...' action or sub menu
    fileItemActions.addOpenWithActionsTo(this, QStringLiteral("DesktopEntryName != '%1'").arg(qApp->desktopFileName()));
}

void DolphinContextMenu::addCustomActions()
{
    addActions(m_customActions);
}

void DolphinContextMenu::addAdditionalActions(KFileItemActions &fileItemActions, const KFileItemListProperties &props)
{
    addSeparator();

    QList<QAction *> additionalActions;
    if (props.isDirectory() && props.isLocal() && ContextMenuSettings::showOpenTerminal()) {
        additionalActions << m_mainWindow->actionCollection()->action(QStringLiteral("open_terminal"));
    }
    fileItemActions.addActionsTo(this, KFileItemActions::MenuActionSource::All, additionalActions);

    const DolphinView* view = m_mainWindow->activeViewContainer()->view();
    const QList<QAction*> versionControlActions = view->versionControlActions(m_selectedItems);
    if (!versionControlActions.isEmpty()) {
        addActions(versionControlActions);
        addSeparator();
    }
}

