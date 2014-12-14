/* This file is part of the KDE project
   Copyright (c) 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dolphinpart.h"
#include "dolphinremoveaction.h"

#include <KFileItemListProperties>
#include <konq_operations.h>

#include <KAboutData>
#include <KActionCollection>
#include <KDebug>
#include <KIconLoader>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>
#include <KRun>
#include <KIO/NetAccess>
#include <KToolInvocation>
#include <kauthorized.h>
#include <QMenu>
#include <KInputDialog>
#include <kdeversion.h>
#include <KSharedConfig>
#include <KConfigGroup>
#include <KMimeTypeEditor>

#include "dolphinpart_ext.h"
#include "dolphinnewfilemenu.h"
#include "views/dolphinview.h"
#include "views/dolphinviewactionhandler.h"
#include "views/dolphinnewfilemenuobserver.h"
#include "views/dolphinremoteencoding.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/private/kfileitemmodeldirlister.h"

#include <QStandardPaths>
#include <QActionGroup>
#include <QTextDocument>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QKeyEvent>


K_PLUGIN_FACTORY(DolphinPartFactory, registerPlugin<DolphinPart>();)
K_EXPORT_PLUGIN(DolphinPartFactory("dolphinpart", "dolphin"))

DolphinPart::DolphinPart(QWidget* parentWidget, QObject* parent, const QVariantList& args)
    : KParts::ReadOnlyPart(parent)
      ,m_openTerminalAction(0)
      ,m_removeAction(0)
{
    Q_UNUSED(args)
    setComponentData(*createAboutData(), false);
    m_extension = new DolphinPartBrowserExtension(this);

    // make sure that other apps using this part find Dolphin's view-file-columns icons
    KIconLoader::global()->addAppDir("dolphin");

    m_view = new DolphinView(QUrl(), parentWidget);
    m_view->setTabsForFilesEnabled(true);
    setWidget(m_view);

    connect(&DolphinNewFileMenuObserver::instance(), &DolphinNewFileMenuObserver::errorMessage,
            this, &DolphinPart::slotErrorMessage);

    connect(m_view, &DolphinView::directoryLoadingCompleted, this, static_cast<void(DolphinPart::*)()>(&DolphinPart::completed));
    connect(m_view, &DolphinView::directoryLoadingProgress, this, &DolphinPart::updateProgress);
    connect(m_view, &DolphinView::errorMessage, this, &DolphinPart::slotErrorMessage);

    setXMLFile("dolphinpart.rc");

    connect(m_view, &DolphinView::infoMessage,
            this, &DolphinPart::slotMessage);
    connect(m_view, &DolphinView::operationCompletedMessage,
            this, &DolphinPart::slotMessage);
    connect(m_view, &DolphinView::errorMessage,
            this, &DolphinPart::slotErrorMessage);
    connect(m_view, &DolphinView::itemActivated,
            this, &DolphinPart::slotItemActivated);
    connect(m_view, &DolphinView::itemsActivated,
            this, &DolphinPart::slotItemsActivated);
    connect(m_view, &DolphinView::tabRequested,
            this, &DolphinPart::createNewWindow);
    connect(m_view, &DolphinView::requestContextMenu,
            this, &DolphinPart::slotOpenContextMenu);
    connect(m_view, &DolphinView::selectionChanged,
            m_extension, static_cast<void(DolphinPartBrowserExtension::*)(const KFileItemList&)>(&DolphinPartBrowserExtension::selectionInfo));
    connect(m_view, &DolphinView::selectionChanged,
            this, &DolphinPart::slotSelectionChanged);
    connect(m_view, &DolphinView::requestItemInfo,
            this, &DolphinPart::slotRequestItemInfo);
    connect(m_view, &DolphinView::modeChanged,
            this, &DolphinPart::viewModeChanged); // relay signal
    connect(m_view, &DolphinView::redirection,
            this, &DolphinPart::slotDirectoryRedirection);

    // Watch for changes that should result in updates to the
    // status bar text.
    connect(m_view, &DolphinView::itemCountChanged, this, &DolphinPart::updateStatusBar);
    connect(m_view,  &DolphinView::selectionChanged, this, &DolphinPart::updateStatusBar);

    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    m_actionHandler->setCurrentView(m_view);
    connect(m_actionHandler, &DolphinViewActionHandler::createDirectory, this, &DolphinPart::createDirectory);

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, &DolphinPart::aboutToOpenURL,
            m_remoteEncoding, &DolphinRemoteEncoding::slotAboutToOpenUrl);

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged,
            this, &DolphinPart::updatePasteAction);

    // Create file info and listing filter extensions.
    // NOTE: Listing filter needs to be instantiated after the creation of the view.
    new DolphinPartFileInfoExtension(this);

    new DolphinPartListingFilterExtension(this);

    KDirLister* lister = m_view->m_model->m_dirLister;
    if (lister) {
        DolphinPartListingNotificationExtension* notifyExt = new DolphinPartListingNotificationExtension(this);
        connect(lister, &KDirLister::newItems, notifyExt, &DolphinPartListingNotificationExtension::slotNewItems);
        connect(lister, &KDirLister::itemsDeleted, notifyExt, &DolphinPartListingNotificationExtension::slotItemsDeleted);
    } else {
        kWarning() << "NULL KDirLister object! KParts::ListingNotificationExtension will NOT be supported";
    }

    createActions();
    m_actionHandler->updateViewActions();
    slotSelectionChanged(KFileItemList()); // initially disable selection-dependent actions

    // Listen to events from the app so we can update the remove key by
    // checking for a Shift key press.
    qApp->installEventFilter(this);

    // TODO there was a "always open a new window" (when clicking on a directory) setting in konqueror
    // (sort of spacial navigation)

    loadPlugins(this, this, componentData());
}

DolphinPart::~DolphinPart()
{
}

void DolphinPart::createActions()
{
    // Edit menu

    m_newFileMenu = new DolphinNewFileMenu(actionCollection(), this);
    m_newFileMenu->setParentWidget(widget());
    connect(m_newFileMenu->menu(), &QMenu::aboutToShow,
            this, &DolphinPart::updateNewMenu);

    QAction *editMimeTypeAction = actionCollection()->addAction( "editMimeType" );
    editMimeTypeAction->setText( i18nc("@action:inmenu Edit", "&Edit File Type..." ) );
    connect(editMimeTypeAction, &QAction::triggered, this, &DolphinPart::slotEditMimeType);

    QAction* selectItemsMatching = actionCollection()->addAction("select_items_matching");
    selectItemsMatching->setText(i18nc("@action:inmenu Edit", "Select Items Matching..."));
    actionCollection()->setDefaultShortcut(selectItemsMatching, Qt::CTRL | Qt::Key_S);
    connect(selectItemsMatching, &QAction::triggered, this, &DolphinPart::slotSelectItemsMatchingPattern);

    QAction* unselectItemsMatching = actionCollection()->addAction("unselect_items_matching");
    unselectItemsMatching->setText(i18nc("@action:inmenu Edit", "Unselect Items Matching..."));
    connect(unselectItemsMatching, &QAction::triggered, this, &DolphinPart::slotUnselectItemsMatchingPattern);

    actionCollection()->addAction(KStandardAction::SelectAll, "select_all", m_view, SLOT(selectAll()));

    QAction* unselectAll = actionCollection()->addAction("unselect_all");
    unselectAll->setText(i18nc("@action:inmenu Edit", "Unselect All"));
    connect(unselectAll, &QAction::triggered, m_view, &DolphinView::clearSelection);

    QAction* invertSelection = actionCollection()->addAction("invert_selection");
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    actionCollection()->setDefaultShortcut(invertSelection, Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, &QAction::triggered, m_view, &DolphinView::invertSelection);

    // View menu: all done by DolphinViewActionHandler

    // Go menu

    QActionGroup* goActionGroup = new QActionGroup(this);
    connect(goActionGroup, &QActionGroup::triggered,
            this, &DolphinPart::slotGoTriggered);

    createGoAction("go_applications", "start-here-kde",
                   i18nc("@action:inmenu Go", "App&lications"), QStringLiteral("programs:/"),
                   goActionGroup);
    createGoAction("go_network_folders", "folder-remote",
                   i18nc("@action:inmenu Go", "&Network Folders"), QStringLiteral("remote:/"),
                   goActionGroup);
    createGoAction("go_settings", "preferences-system",
                   i18nc("@action:inmenu Go", "Sett&ings"), QStringLiteral("settings:/"),
                   goActionGroup);
    createGoAction("go_trash", "user-trash",
                   i18nc("@action:inmenu Go", "Trash"), QStringLiteral("trash:/"),
                   goActionGroup);
    createGoAction("go_autostart", "",
                   i18nc("@action:inmenu Go", "Autostart"), QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/autostart",
                   goActionGroup);

    // Tools menu
    m_findFileAction = actionCollection()->addAction("find_file");
    m_findFileAction->setText(i18nc("@action:inmenu Tools", "Find File..."));
    actionCollection()->setDefaultShortcut(m_findFileAction, Qt::CTRL | Qt::Key_F);
    m_findFileAction->setIcon(QIcon::fromTheme("edit-find"));
    connect(m_findFileAction, &QAction::triggered, this, &DolphinPart::slotFindFile);

    if (KAuthorized::authorizeKAction("shell_access")) {
        m_openTerminalAction = actionCollection()->addAction("open_terminal");
        m_openTerminalAction->setIcon(QIcon::fromTheme("utilities-terminal"));
        m_openTerminalAction->setText(i18nc("@action:inmenu Tools", "Open &Terminal"));
        connect(m_openTerminalAction, &QAction::triggered, this, &DolphinPart::slotOpenTerminal);
        actionCollection()->setDefaultShortcut(m_openTerminalAction, Qt::Key_F4);
    }
}

void DolphinPart::createGoAction(const char* name, const char* iconName,
                                 const QString& text, const QString& url,
                                 QActionGroup* actionGroup)
{
    QAction* action = actionCollection()->addAction(name);
    action->setIcon(QIcon::fromTheme(iconName));
    action->setText(text);
    action->setData(url);
    action->setActionGroup(actionGroup);
}

void DolphinPart::slotGoTriggered(QAction* action)
{
    const QString url = action->data().toString();
    emit m_extension->openUrlRequest(QUrl(url));
}

void DolphinPart::slotSelectionChanged(const KFileItemList& selection)
{
    const bool hasSelection = !selection.isEmpty();

    QAction* renameAction  = actionCollection()->action("rename");
    QAction* moveToTrashAction = actionCollection()->action("move_to_trash");
    QAction* deleteAction = actionCollection()->action("delete");
    QAction* editMimeTypeAction = actionCollection()->action("editMimeType");
    QAction* propertiesAction = actionCollection()->action("properties");
    QAction* deleteWithTrashShortcut = actionCollection()->action("delete_shortcut"); // see DolphinViewActionHandler

    if (!hasSelection) {
        stateChanged("has_no_selection");

        emit m_extension->enableAction("cut", false);
        emit m_extension->enableAction("copy", false);
        deleteWithTrashShortcut->setEnabled(false);
        editMimeTypeAction->setEnabled(false);
    } else {
        stateChanged("has_selection");

        // TODO share this code with DolphinMainWindow::updateEditActions (and the desktop code)
        // in libkonq
        KFileItemListProperties capabilities(selection);
        const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();

        renameAction->setEnabled(capabilities.supportsMoving());
        moveToTrashAction->setEnabled(enableMoveToTrash);
        deleteAction->setEnabled(capabilities.supportsDeleting());
        deleteWithTrashShortcut->setEnabled(capabilities.supportsDeleting() && !enableMoveToTrash);
        editMimeTypeAction->setEnabled(true);
        propertiesAction->setEnabled(true);
        emit m_extension->enableAction("cut", capabilities.supportsMoving());
        emit m_extension->enableAction("copy", true);
    }
}

void DolphinPart::updatePasteAction()
{
    QPair<bool, QString> pasteInfo = m_view->pasteInfo();
    emit m_extension->enableAction( "paste", pasteInfo.first );
    emit m_extension->setActionText( "paste", pasteInfo.second );
}

KAboutData* DolphinPart::createAboutData()
{
    return new KAboutData("dolphinpart", i18nc("@title", "Dolphin Part"), "0.1");
}

bool DolphinPart::openUrl(const QUrl &url)
{
    bool reload = arguments().reload();
    // A bit of a workaround so that changing the namefilter works: force reload.
    // Otherwise DolphinView wouldn't relist the URL, so nothing would happen.
    if (m_nameFilter != m_view->nameFilter())
        reload = true;
    if (m_view->url() == url && !reload) { // DolphinView won't do anything in that case, so don't emit started
        return true;
    }
    setUrl(url); // remember it at the KParts level
    QUrl visibleUrl(url);
    if (!m_nameFilter.isEmpty()) {
        visibleUrl.setPath(visibleUrl.path() + '/' + m_nameFilter);
    }
    QString prettyUrl = visibleUrl.toDisplayString(QUrl::PreferLocalFile);
    emit setWindowCaption(prettyUrl);
    emit m_extension->setLocationBarUrl(prettyUrl);
    emit started(0); // get the wheel to spin
    m_view->setNameFilter(m_nameFilter);
    m_view->setUrl(url);
    updatePasteAction();
    emit aboutToOpenURL();
    if (reload)
        m_view->reload();
    // Disable "Find File" and "Open Terminal" actions for non-file URLs,
    // e.g. ftp, smb, etc. #279283
    const bool isLocalUrl = url.isLocalFile();
    m_findFileAction->setEnabled(isLocalUrl);
    if (m_openTerminalAction) {
        m_openTerminalAction->setEnabled(isLocalUrl);
    }
    return true;
}

void DolphinPart::slotMessage(const QString& msg)
{
    emit setStatusBarText(msg);
}

void DolphinPart::slotErrorMessage(const QString& msg)
{
    kDebug() << msg;
    emit canceled(msg);
    //KMessageBox::error(m_view, msg);
}

void DolphinPart::slotRequestItemInfo(const KFileItem& item)
{
    emit m_extension->mouseOverInfo(item);
    if (item.isNull()) {
        updateStatusBar();
    } else {
        const QString escapedText = Qt::convertFromPlainText(item.getStatusBarInfo());
        ReadOnlyPart::setStatusBarText(QString("<qt>%1</qt>").arg(escapedText));
    }
}

void DolphinPart::slotItemActivated(const KFileItem& item)
{
    KParts::OpenUrlArguments args;
    // Forget about the known mimetype if a target URL is used.
    // Testcase: network:/ with a item (mimetype "inode/some-foo-service") pointing to a http URL (html)
    if (item.targetUrl() == item.url()) {
        args.setMimeType(item.mimetype());
    }

    // Ideally, konqueror should be changed to not require trustedSource for directory views,
    // since the idea was not to need BrowserArguments for non-browser stuff...
    KParts::BrowserArguments browserArgs;
    browserArgs.trustedSource = true;
    emit m_extension->openUrlRequest(item.targetUrl(), args, browserArgs);
}

void DolphinPart::slotItemsActivated(const KFileItemList& items)
{
    foreach (const KFileItem& item, items) {
        slotItemActivated(item);
    }
}

void DolphinPart::createNewWindow(const QUrl& url)
{
    // TODO: Check issue N176832 for the missing QAIV signal; task 177399 - maybe this code
    // should be moved into DolphinPart::slotItemActivated()
    emit m_extension->createNewWindow(url);
}

void DolphinPart::slotOpenContextMenu(const QPoint& pos,
                                      const KFileItem& _item,
                                      const QUrl &,
                                      const QList<QAction*>& customActions)
{
    KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems
                                                      | KParts::BrowserExtension::ShowProperties
                                                      | KParts::BrowserExtension::ShowUrlOperations;

    KFileItem item(_item);

    if (item.isNull()) { // viewport context menu
        popupFlags |= KParts::BrowserExtension::ShowNavigationItems | KParts::BrowserExtension::ShowUp;
        item = m_view->rootItem();
        if (item.isNull())
            item = KFileItem(url());
        else
            item.setUrl(url()); // ensure we use the view url, not the canonical path (#213799)
    }

    // TODO: We should change the signature of the slots (and signals) for being able
    //       to tell for which items we want a popup.
    KFileItemList items;
    if (m_view->selectedItems().isEmpty()) {
        items.append(item);
    } else {
        items = m_view->selectedItems();
    }

    KFileItemListProperties capabilities(items);

    KParts::BrowserExtension::ActionGroupMap actionGroups;
    QList<QAction *> editActions;
    editActions += m_view->versionControlActions(m_view->selectedItems());
    editActions += customActions;

    if (!_item.isNull()) { // only for context menu on one or more items
        const bool supportsMoving = capabilities.supportsMoving();

        if (capabilities.supportsDeleting()) {
            const bool showDeleteAction = (KSharedConfig::openConfig()->group("KDE").readEntry("ShowDeleteCommand", false) ||
                                           !item.isLocalFile());
            const bool showMoveToTrashAction = capabilities.isLocal() && supportsMoving;

            if (showDeleteAction && showMoveToTrashAction) {
                delete m_removeAction;
                m_removeAction = 0;
                editActions.append(actionCollection()->action("move_to_trash"));
                editActions.append(actionCollection()->action("delete"));
            } else if (showDeleteAction && !showMoveToTrashAction) {
                editActions.append(actionCollection()->action("delete"));
            } else {
                if (!m_removeAction)
                    m_removeAction = new DolphinRemoveAction(this, actionCollection());
                editActions.append(m_removeAction);
                m_removeAction->update();
            }
        } else {
            popupFlags |= KParts::BrowserExtension::NoDeletion;
        }

        if (supportsMoving) {
            editActions.append(actionCollection()->action("rename"));
        }

        // Normally KonqPopupMenu only shows the "Create new" submenu in the current view
        // since otherwise the created file would not be visible.
        // But in treeview mode we should allow it.
        if (m_view->itemsExpandable())
            popupFlags |= KParts::BrowserExtension::ShowCreateDirectory;

    }

    actionGroups.insert("editactions", editActions);

    emit m_extension->popupMenu(pos,
                                items,
                                KParts::OpenUrlArguments(),
                                KParts::BrowserArguments(),
                                popupFlags,
                                actionGroups);
}

void DolphinPart::slotDirectoryRedirection(const QUrl &oldUrl, const QUrl &newUrl)
{
    //kDebug() << oldUrl << newUrl << "currentUrl=" << url();
    if (oldUrl.matches(url(), QUrl::StripTrailingSlash /* #207572 */)) {
        KParts::ReadOnlyPart::setUrl(newUrl);
        const QString prettyUrl = newUrl.toDisplayString(QUrl::PreferLocalFile);
        emit m_extension->setLocationBarUrl(prettyUrl);
    }
}


void DolphinPart::slotEditMimeType()
{
    const KFileItemList items = m_view->selectedItems();
    if (!items.isEmpty()) {
        KMimeTypeEditor::editMimeType(items.first().mimetype(), m_view);
    }
}

void DolphinPart::slotSelectItemsMatchingPattern()
{
    openSelectionDialog(i18nc("@title:window", "Select"),
                        i18n("Select all items matching this pattern:"),
                        true);
}

void DolphinPart::slotUnselectItemsMatchingPattern()
{
    openSelectionDialog(i18nc("@title:window", "Unselect"),
                        i18n("Unselect all items matching this pattern:"),
                        false);
}

void DolphinPart::openSelectionDialog(const QString& title, const QString& text, bool selectItems)
{
    bool okClicked;
    QString pattern = KInputDialog::getText(title, text, "*", &okClicked, m_view);

    if (okClicked && !pattern.isEmpty()) {
        QRegExp patternRegExp(pattern, Qt::CaseSensitive, QRegExp::Wildcard);
        m_view->selectItems(patternRegExp, selectItems);
    }
}

void DolphinPart::setCurrentViewMode(const QString& viewModeName)
{
    QAction* action = actionCollection()->action(viewModeName);
    Q_ASSERT(action);
    action->trigger();
}

QString DolphinPart::currentViewMode() const
{
    return m_actionHandler->currentViewModeActionName();
}

void DolphinPart::setNameFilter(const QString& nameFilter)
{
    // This is the "/home/dfaure/*.diff" kind of name filter (KDirLister::setNameFilter)
    // which is unrelated to DolphinView::setNameFilter which is substring filtering in a proxy.
    m_nameFilter = nameFilter;
    // TODO save/restore name filter in saveState/restoreState like KonqDirPart did in kde3?
}

void DolphinPart::slotOpenTerminal()
{
    QString dir(QDir::homePath());

    QUrl u(url());

    // If the given directory is not local, it can still be the URL of an
    // ioslave using UDS_LOCAL_PATH which to be converted first.
    u = KIO::NetAccess::mostLocalUrl(u, widget());

    //If the URL is local after the above conversion, set the directory.
    if (u.isLocalFile()) {
        dir = u.toLocalFile();
    }

    KToolInvocation::invokeTerminal(QString(), dir);
}

void DolphinPart::slotFindFile()
{
    KRun::run("kfind", {url()}, widget());
}

void DolphinPart::updateNewMenu()
{
    // As requested by KNewFileMenu :
    m_newFileMenu->checkUpToDate();
    m_newFileMenu->setViewShowsHiddenFiles(m_view->hiddenFilesShown());
    // And set the files that the menu apply on :
    m_newFileMenu->setPopupFiles(url());
}

void DolphinPart::updateStatusBar()
{
    const QString escapedText = Qt::convertFromPlainText(m_view->statusBarText());
    emit ReadOnlyPart::setStatusBarText(QString("<qt>%1</qt>").arg(escapedText));
}

void DolphinPart::updateProgress(int percent)
{
    m_extension->loadingProgress(percent);
}

void DolphinPart::createDirectory()
{
    m_newFileMenu->setViewShowsHiddenFiles(m_view->hiddenFilesShown());
    m_newFileMenu->setPopupFiles(url());
    m_newFileMenu->createDirectory();
}

void DolphinPart::setFilesToSelect(const QList<QUrl>& files)
{
    if (files.isEmpty()) {
        return;
    }

    m_view->markUrlsAsSelected(files);
    m_view->markUrlAsCurrent(files.at(0));
}

bool DolphinPart::eventFilter(QObject* obj, QEvent* event)
{
    const int type = event->type();

    if ((type == QEvent::KeyPress || type == QEvent::KeyRelease) && m_removeAction) {
        QMenu* menu = qobject_cast<QMenu*>(obj);
        if (menu && menu->parent() == m_view) {
            QKeyEvent* ev = static_cast<QKeyEvent*>(event);
            if (ev->key() == Qt::Key_Shift) {
                m_removeAction->update();
            }
        }
    }

    return KParts::ReadOnlyPart::eventFilter(obj, event);
}

#include "dolphinpart.moc"
