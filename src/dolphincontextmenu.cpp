/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Cvetoslav Ludmiloff
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphincontextmenu.h"

#include "dolphin_contextmenusettings.h"
#include "dolphinmainwindow.h"
#include "dolphinnewfilemenu.h"
#include "dolphinplacesmodelsingleton.h"
#include "dolphinremoveaction.h"
#include "dolphinviewcontainer.h"
#include "global.h"
#include "trash/dolphintrash.h"
#include "views/dolphinview.h"

#include <KActionCollection>
#include <KFileItemListProperties>
#include <KHamburgerMenu>
#include <KIO/EmptyTrashJob>
#include <KIO/JobUiDelegate>
#include <KIO/Paste>
#include <KIO/RestoreJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KNewFileMenu>
#include <KStandardAction>

#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>

DolphinContextMenu::DolphinContextMenu(DolphinMainWindow *parent,
                                       const KFileItem &fileInfo,
                                       const KFileItemList &selectedItems,
                                       const QUrl &baseUrl,
                                       KFileItemActions *fileItemActions)
    : QMenu(parent)
    , m_mainWindow(parent)
    , m_fileInfo(fileInfo)
    , m_baseUrl(baseUrl)
    , m_baseFileItem(nullptr)
    , m_selectedItems(selectedItems)
    , m_selectedItemsProperties(nullptr)
    , m_context(NoContext)
    , m_copyToMenu(parent)
    , m_removeAction(nullptr)
    , m_fileItemActions(fileItemActions)
{
    QApplication::instance()->installEventFilter(this);

    addAllActions();
}

DolphinContextMenu::~DolphinContextMenu()
{
    delete m_baseFileItem;
    m_baseFileItem = nullptr;
    delete m_selectedItemsProperties;
    m_selectedItemsProperties = nullptr;
}

void DolphinContextMenu::addAllActions()
{
    static_cast<KHamburgerMenu *>(m_mainWindow->actionCollection()->action(QStringLiteral("hamburger_menu")))->addToMenu(this);

    // get the context information
    const auto scheme = m_baseUrl.scheme();
    if (scheme == QLatin1String("trash")) {
        m_context |= TrashContext;
    } else if (scheme.contains(QLatin1String("search"))) {
        m_context |= SearchContext;
    } else if (scheme.contains(QLatin1String("timeline"))) {
        m_context |= TimelineContext;
    } else if (scheme == QStringLiteral("recentlyused")) {
        m_context |= RecentlyUsedContext;
    }

    if (!m_fileInfo.isNull() && !m_selectedItems.isEmpty()) {
        m_context |= ItemContext;
        // TODO: handle other use cases like devices + desktop files
    }

    // open the corresponding popup for the context
    if (m_context & TrashContext) {
        if (m_context & ItemContext) {
            addTrashItemContextMenu();
        } else {
            addTrashContextMenu();
        }
    } else if (m_context & ItemContext) {
        addItemContextMenu();
    } else {
        addViewportContextMenu();
    }
}

bool DolphinContextMenu::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object)

    if (event->type() == QEvent::KeyPress || event->type() == QEvent::KeyRelease) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

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

void DolphinContextMenu::addTrashContextMenu()
{
    Q_ASSERT(m_context & TrashContext);

    QAction *emptyTrashAction = addAction(QIcon::fromTheme(QStringLiteral("trash-empty")), i18nc("@action:inmenu", "Empty Trash"), [this]() {
        Trash::empty(m_mainWindow);
    });
    emptyTrashAction->setEnabled(!Trash::isEmpty());

    QAction *propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);
}

void DolphinContextMenu::addTrashItemContextMenu()
{
    Q_ASSERT(m_context & TrashContext);
    Q_ASSERT(m_context & ItemContext);

    addAction(QIcon::fromTheme("restoration"), i18nc("@action:inmenu", "Restore"), [this]() {
        QList<QUrl> selectedUrls;
        selectedUrls.reserve(m_selectedItems.count());
        for (const KFileItem &item : std::as_const(m_selectedItems)) {
            selectedUrls.append(item.url());
        }

        KIO::RestoreJob *job = KIO::restoreFromTrash(selectedUrls);
        KJobWidgets::setWindow(job, m_mainWindow);
        job->uiDelegate()->setAutoErrorHandlingEnabled(true);
    });

    QAction *deleteAction = m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile));
    addAction(deleteAction);

    QAction *propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);
}

void DolphinContextMenu::addDirectoryItemContextMenu()
{
    // insert 'Open in new window' and 'Open in new tab' entries
    const KFileItemListProperties &selectedItemsProps = selectedItemsProperties();
    if (ContextMenuSettings::showOpenInNewTab()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_tab")));
    }
    if (ContextMenuSettings::showOpenInNewWindow()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_new_window")));
    }

    if (ContextMenuSettings::showOpenInSplitView()) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("open_in_split_view")));
    }

    // Insert 'Open With' entries
    addOpenWithActions();

    // set up 'Create New' menu
    DolphinNewFileMenu *newFileMenu = new DolphinNewFileMenu(m_mainWindow->actionCollection()->action(QStringLiteral("create_dir")), m_mainWindow);
    newFileMenu->checkUpToDate();
    newFileMenu->setWorkingDirectory(m_fileInfo.url());
    newFileMenu->setEnabled(selectedItemsProps.supportsWriting());
    connect(newFileMenu, &DolphinNewFileMenu::fileCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);
    connect(newFileMenu, &DolphinNewFileMenu::directoryCreated, newFileMenu, &DolphinNewFileMenu::deleteLater);

    QMenu *menu = newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    addMenu(menu);

    addSeparator();
}

void DolphinContextMenu::addOpenParentFolderActions()
{
    addAction(QIcon::fromTheme(QStringLiteral("document-open-folder")), i18nc("@action:inmenu", "Open Path"), [this]() {
        const QUrl url = m_fileInfo.targetUrl();
        const QUrl parentUrl = KIO::upUrl(url);
        m_mainWindow->changeUrl(parentUrl);
        m_mainWindow->activeViewContainer()->view()->markUrlsAsSelected({url});
        m_mainWindow->activeViewContainer()->view()->markUrlAsCurrent(url);
    });

    addAction(QIcon::fromTheme(QStringLiteral("tab-new")), i18nc("@action:inmenu", "Open Path in New Tab"), [this]() {
        m_mainWindow->openNewTab(KIO::upUrl(m_fileInfo.targetUrl()));
    });

    addAction(QIcon::fromTheme(QStringLiteral("window-new")), i18nc("@action:inmenu", "Open Path in New Window"), [this]() {
        Dolphin::openNewWindow({m_fileInfo.targetUrl()}, m_mainWindow, Dolphin::OpenNewWindowFlag::Select);
    });
}

void DolphinContextMenu::addItemContextMenu()
{
    Q_ASSERT(!m_fileInfo.isNull());

    const KFileItemListProperties &selectedItemsProps = selectedItemsProperties();

    m_fileItemActions->setItemListProperties(selectedItemsProps);

    if (m_selectedItems.count() == 1) {
        // single files
        if (m_fileInfo.isDir()) {
            addDirectoryItemContextMenu();
        } else if (m_context & TimelineContext || m_context & SearchContext || m_context & RecentlyUsedContext) {
            addOpenWithActions();

            addOpenParentFolderActions();

            addSeparator();
        } else {
            // Insert 'Open With" entries
            addOpenWithActions();
        }
        if (m_fileInfo.isLink()) {
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("show_target")));
            addSeparator();
        }
    } else {
        // multiple files
        bool selectionHasOnlyDirs = true;
        for (const auto &item : std::as_const(m_selectedItems)) {
            const QUrl &url = DolphinView::openItemAsFolderUrl(item);
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
        addOpenWithActions();
    }

    insertDefaultItemActions(selectedItemsProps);

    addAdditionalActions(selectedItemsProps);

    // insert 'Copy To' and 'Move To' sub menus
    if (ContextMenuSettings::showCopyMoveMenu()) {
        m_copyToMenu.setUrls(m_selectedItems.urlList());
        m_copyToMenu.setReadOnly(!selectedItemsProps.supportsWriting());
        m_copyToMenu.setAutoErrorHandlingEnabled(true);
        m_copyToMenu.addActionsTo(this);
    }

    if (m_mainWindow->isSplitViewEnabledInCurrentTab()) {
        if (ContextMenuSettings::showCopyToOtherSplitView()) {
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("copy_to_inactive_split_view")));
        }

        if (ContextMenuSettings::showMoveToOtherSplitView()) {
            addAction(m_mainWindow->actionCollection()->action(QStringLiteral("move_to_inactive_split_view")));
        }
    }

    // insert 'Properties...' entry
    addSeparator();
    QAction *propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);
}

void DolphinContextMenu::addViewportContextMenu()
{
    const KFileItemListProperties baseUrlProperties(KFileItemList() << baseFileItem());
    m_fileItemActions->setItemListProperties(baseUrlProperties);

    // Set up and insert 'Create New' menu
    KNewFileMenu *newFileMenu = m_mainWindow->newFileMenu();
    newFileMenu->checkUpToDate();
    newFileMenu->setWorkingDirectory(m_baseUrl);
    addMenu(newFileMenu->menu());

    // Show "open with" menu items even if the dir is empty, because there are legitimate
    // use cases for this, such as opening an empty dir in Kate or VSCode or something
    addOpenWithActions();

    QAction *pasteAction = createPasteAction();
    if (pasteAction) {
        addAction(pasteAction);
    }

    // Insert 'Add to Places' entry if it's not already in the places panel
    if (ContextMenuSettings::showAddToPlaces() && !placeExists(m_mainWindow->activeViewContainer()->url())) {
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

    addAdditionalActions(baseUrlProperties);

    addSeparator();

    QAction *propertiesAction = m_mainWindow->actionCollection()->action(QStringLiteral("properties"));
    addAction(propertiesAction);
}

void DolphinContextMenu::insertDefaultItemActions(const KFileItemListProperties &properties)
{
    const KActionCollection *collection = m_mainWindow->actionCollection();

    // Insert 'Cut', 'Copy', 'Copy Location' and 'Paste'
    addAction(collection->action(KStandardAction::name(KStandardAction::Cut)));
    addAction(collection->action(KStandardAction::name(KStandardAction::Copy)));
    if (ContextMenuSettings::showCopyLocation()) {
        QAction *copyPathAction = collection->action(QString("copy_location"));
        copyPathAction->setEnabled(m_selectedItems.size() == 1);
        addAction(copyPathAction);
    }
    QAction *pasteAction = createPasteAction();
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
    if (ContextMenuSettings::showAddToPlaces() && m_selectedItems.count() == 1 && m_fileInfo.isDir() && !placeExists(m_fileInfo.url())) {
        addAction(m_mainWindow->actionCollection()->action(QStringLiteral("add_to_places")));
    }

    addSeparator();

    // Insert 'Move to Trash' and/or 'Delete'
    const bool showDeleteAction = (KSharedConfig::openConfig()->group(QStringLiteral("KDE")).readEntry("ShowDeleteCommand", false) || !properties.isLocal());
    const bool showMoveToTrashAction = (properties.isLocal() && properties.supportsMoving());

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

bool DolphinContextMenu::placeExists(const QUrl &url) const
{
    const KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();

    const auto &matchedPlaces = placesModel->match(placesModel->index(0, 0), KFilePlacesModel::UrlRole, url, 1, Qt::MatchExactly);

    return !matchedPlaces.isEmpty();
}

QAction *DolphinContextMenu::createPasteAction()
{
    QAction *action = nullptr;
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

KFileItemListProperties &DolphinContextMenu::selectedItemsProperties() const
{
    if (!m_selectedItemsProperties) {
        m_selectedItemsProperties = new KFileItemListProperties(m_selectedItems);
    }
    return *m_selectedItemsProperties;
}

KFileItem DolphinContextMenu::baseFileItem()
{
    if (!m_baseFileItem) {
        const DolphinView *view = m_mainWindow->activeViewContainer()->view();
        KFileItem baseItem = view->rootItem();
        if (baseItem.isNull() || baseItem.url() != m_baseUrl) {
            m_baseFileItem = new KFileItem(m_baseUrl);
        } else {
            m_baseFileItem = new KFileItem(baseItem);
        }
    }
    return *m_baseFileItem;
}

void DolphinContextMenu::addOpenWithActions()
{
    // insert 'Open With...' action or sub menu
    m_fileItemActions->insertOpenWithActionsTo(nullptr, this, QStringList{qApp->desktopFileName()});

    // For a single file, hint in "Open with" menu that middle-clicking would open it in the secondary app.
    if (m_selectedItems.count() == 1 && !m_fileInfo.isDir()) {
        if (QAction *openWithSubMenu = findChild<QAction *>(QStringLiteral("openWith_submenu"))) {
            Q_ASSERT(openWithSubMenu->menu());
            Q_ASSERT(!openWithSubMenu->menu()->isEmpty());

            auto *secondaryApp = openWithSubMenu->menu()->actions().first();
            // Add it like a keyboard shortcut, Qt uses \t as a separator.
            if (!secondaryApp->text().contains(QLatin1Char('\t'))) {
                secondaryApp->setText(secondaryApp->text() + QLatin1Char('\t')
                                      + i18nc("@action:inmenu Shortcut, middle click to trigger menu item, keep short", "Middle Click"));
            }
        }
    }
}

void DolphinContextMenu::addAdditionalActions(const KFileItemListProperties &props)
{
    addSeparator();

    QList<QAction *> additionalActions;
    if (props.isLocal() && ContextMenuSettings::showOpenTerminal()) {
        additionalActions << m_mainWindow->actionCollection()->action(QStringLiteral("open_terminal_here"));
    }
    m_fileItemActions->addActionsTo(this, KFileItemActions::MenuActionSource::All, additionalActions);

    const DolphinView *view = m_mainWindow->activeViewContainer()->view();
    const QList<QAction *> versionControlActions = view->versionControlActions(m_selectedItems);
    if (!versionControlActions.isEmpty()) {
        addSeparator();
        addActions(versionControlActions);
        addSeparator();
    }
}

#include "moc_dolphincontextmenu.cpp"
