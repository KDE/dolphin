/* This file is part of the KDE project
   SPDX-FileCopyrightText: 2007 David Faure <faure@kde.org>

   SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "dolphinpart.h"

#include "dolphindebug.h"
#include "dolphinnewfilemenu.h"
#include "dolphinpart_ext.h"
#include "dolphinremoveaction.h"
#include "kitemviews/kfileitemmodel.h"
#include "views/dolphinnewfilemenuobserver.h"
#include "views/dolphinremoteencoding.h"
#include "views/dolphinview.h"
#include "views/dolphinviewactionhandler.h"

#include <KActionCollection>
#include <KAuthorized>
#include <KConfigGroup>
#include <KDialogJobUiDelegate>
#include <KDirLister>
#include <KFileItemListProperties>
#include <KIO/CommandLauncherJob>
#include <KIconLoader>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KMimeTypeEditor>
#include <KMoreToolsMenuFactory>
#include <KPluginFactory>
#include <KPluginMetaData>
#include <KSharedConfig>
#include <KTerminalLauncherJob>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QInputDialog>
#include <QKeyEvent>
#include <QMenu>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTextDocument>

K_PLUGIN_CLASS_WITH_JSON(DolphinPart, "dolphinpart.json")

DolphinPart::DolphinPart(QWidget* parentWidget, QObject* parent,
                         const KPluginMetaData& metaData, const QVariantList& args)
    : KParts::ReadOnlyPart(parent)
      ,m_openTerminalAction(nullptr)
      ,m_removeAction(nullptr)
{
    Q_UNUSED(args)
    setMetaData(metaData);

    m_extension = new DolphinPartBrowserExtension(this);

    // make sure that other apps using this part find Dolphin's view-file-columns icons
    KIconLoader::global()->addAppDir(QStringLiteral("dolphin"));

    m_view = new DolphinView(QUrl(), parentWidget);
    m_view->setTabsForFilesEnabled(true);
    setWidget(m_view);

    connect(&DolphinNewFileMenuObserver::instance(), &DolphinNewFileMenuObserver::errorMessage,
            this, &DolphinPart::slotErrorMessage);

    connect(m_view, &DolphinView::directoryLoadingCompleted, this, &KParts::ReadOnlyPart::completed);
    connect(m_view, &DolphinView::directoryLoadingCompleted, this, &DolphinPart::updatePasteAction);
    connect(m_view, &DolphinView::directoryLoadingProgress, this, &DolphinPart::updateProgress);
    connect(m_view, &DolphinView::errorMessage, this, &DolphinPart::slotErrorMessage);

    setXMLFile(QStringLiteral("dolphinpart.rc"));

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
    connect(m_view, &DolphinView::statusBarTextChanged, this, [this](const QString &text) {
        const QString escapedText = Qt::convertFromPlainText(text);
        Q_EMIT ReadOnlyPart::setStatusBarText(QStringLiteral("<qt>%1</qt>").arg(escapedText));
    });
    connect(m_view, &DolphinView::tabRequested,
            this, &DolphinPart::createNewWindow);
    connect(m_view, &DolphinView::requestContextMenu,
            this, &DolphinPart::slotOpenContextMenu);
    connect(m_view, &DolphinView::selectionChanged,
            m_extension, &KParts::BrowserExtension::selectionInfo);
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
    connect(m_actionHandler, &DolphinViewActionHandler::createDirectoryTriggered, this, &DolphinPart::createDirectory);

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
        qCWarning(DolphinDebug) << "NULL KDirLister object! KParts::ListingNotificationExtension will NOT be supported";
    }

    createActions();
    m_actionHandler->updateViewActions();
    slotSelectionChanged(KFileItemList()); // initially disable selection-dependent actions

    // Listen to events from the app so we can update the remove key by
    // checking for a Shift key press.
    qApp->installEventFilter(this);

    // TODO there was a "always open a new window" (when clicking on a directory) setting in konqueror
    // (sort of spacial navigation)
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

    QAction *editMimeTypeAction = actionCollection()->addAction( QStringLiteral("editMimeType") );
    editMimeTypeAction->setText( i18nc("@action:inmenu Edit", "&Edit File Type..." ) );
    connect(editMimeTypeAction, &QAction::triggered, this, &DolphinPart::slotEditMimeType);

    QAction* selectItemsMatching = actionCollection()->addAction(QStringLiteral("select_items_matching"));
    selectItemsMatching->setText(i18nc("@action:inmenu Edit", "Select Items Matching..."));
    actionCollection()->setDefaultShortcut(selectItemsMatching, Qt::CTRL | Qt::Key_S);
    connect(selectItemsMatching, &QAction::triggered, this, &DolphinPart::slotSelectItemsMatchingPattern);

    QAction* unselectItemsMatching = actionCollection()->addAction(QStringLiteral("unselect_items_matching"));
    unselectItemsMatching->setText(i18nc("@action:inmenu Edit", "Unselect Items Matching..."));
    connect(unselectItemsMatching, &QAction::triggered, this, &DolphinPart::slotUnselectItemsMatchingPattern);

    KStandardAction::selectAll(m_view, &DolphinView::selectAll, actionCollection());

    QAction* unselectAll = actionCollection()->addAction(QStringLiteral("unselect_all"));
    unselectAll->setText(i18nc("@action:inmenu Edit", "Unselect All"));
    connect(unselectAll, &QAction::triggered, m_view, &DolphinView::clearSelection);

    QAction* invertSelection = actionCollection()->addAction(QStringLiteral("invert_selection"));
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
    createGoAction("go_trash", "user-trash",
                   i18nc("@action:inmenu Go", "Trash"), QStringLiteral("trash:/"),
                   goActionGroup);
    createGoAction("go_autostart", "",
                   i18nc("@action:inmenu Go", "Autostart"), QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/autostart",
                   goActionGroup);

    // Tools menu
    m_findFileAction = KStandardAction::find(this, &DolphinPart::slotFindFile, actionCollection());
    m_findFileAction->setText(i18nc("@action:inmenu Tools", "Find File..."));

#ifndef Q_OS_WIN
    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        m_openTerminalAction = actionCollection()->addAction(QStringLiteral("open_terminal"));
        m_openTerminalAction->setIcon(QIcon::fromTheme(QStringLiteral("dialog-scripts")));
        m_openTerminalAction->setText(i18nc("@action:inmenu Tools", "Open &Terminal"));
        connect(m_openTerminalAction, &QAction::triggered, this, &DolphinPart::slotOpenTerminal);
        actionCollection()->setDefaultShortcut(m_openTerminalAction, Qt::Key_F4);
    }
#endif
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
    Q_EMIT m_extension->openUrlRequest(QUrl(url));
}

void DolphinPart::slotSelectionChanged(const KFileItemList& selection)
{
    const bool hasSelection = !selection.isEmpty();

    QAction* renameAction  = actionCollection()->action(KStandardAction::name(KStandardAction::RenameFile));
    QAction* moveToTrashAction = actionCollection()->action(KStandardAction::name(KStandardAction::MoveToTrash));
    QAction* deleteAction = actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile));
    QAction* editMimeTypeAction = actionCollection()->action(QStringLiteral("editMimeType"));
    QAction* propertiesAction = actionCollection()->action(QStringLiteral("properties"));
    QAction* deleteWithTrashShortcut = actionCollection()->action(QStringLiteral("delete_shortcut")); // see DolphinViewActionHandler

    if (!hasSelection) {
        stateChanged(QStringLiteral("has_no_selection"));

        Q_EMIT m_extension->enableAction("cut", false);
        Q_EMIT m_extension->enableAction("copy", false);
        deleteWithTrashShortcut->setEnabled(false);
        editMimeTypeAction->setEnabled(false);
    } else {
        stateChanged(QStringLiteral("has_selection"));

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
        Q_EMIT m_extension->enableAction("cut", capabilities.supportsMoving());
        Q_EMIT m_extension->enableAction("copy", true);
    }
}

void DolphinPart::updatePasteAction()
{
    QPair<bool, QString> pasteInfo = m_view->pasteInfo();
    Q_EMIT m_extension->enableAction( "paste", pasteInfo.first );
    Q_EMIT m_extension->setActionText( "paste", pasteInfo.second );
}

QString DolphinPart::urlToLocalFilePath(const QUrl &url)
{
    KIO::StatJob* statJob = KIO::mostLocalUrl(url);
    KJobWidgets::setWindow(statJob, widget());
    statJob->exec();
    QUrl localUrl = statJob->mostLocalUrl();
    if (localUrl.isLocalFile()) {
        return localUrl.toLocalFile();
    }
    return QString();
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
    setUrl(url); // remember url at the KParts level
    setLocalFilePath(urlToLocalFilePath(url)); // remember local path at the KParts level
    QUrl visibleUrl(url);
    if (!m_nameFilter.isEmpty()) {
        visibleUrl.setPath(visibleUrl.path() + '/' + m_nameFilter);
    }
    QString prettyUrl = visibleUrl.toDisplayString(QUrl::PreferLocalFile);
    Q_EMIT setWindowCaption(prettyUrl);
    Q_EMIT m_extension->setLocationBarUrl(prettyUrl);
    Q_EMIT started(nullptr); // get the wheel to spin
    m_view->setNameFilter(m_nameFilter);
    m_view->setUrl(url);
    updatePasteAction();
    Q_EMIT aboutToOpenURL();
    if (reload)
        m_view->reload();
    // Disable "Find File" and "Open Terminal" actions for non-file URLs,
    // e.g. ftp, smb, etc. #279283
    const bool isLocalUrl = !(localFilePath().isEmpty());
    m_findFileAction->setEnabled(isLocalUrl);
    if (m_openTerminalAction) {
        m_openTerminalAction->setEnabled(isLocalUrl);
    }
    return true;
}

void DolphinPart::slotMessage(const QString& msg)
{
    Q_EMIT setStatusBarText(msg);
}

void DolphinPart::slotErrorMessage(const QString& msg)
{
    qCDebug(DolphinDebug) << msg;
    Q_EMIT canceled(msg);
    //KMessageBox::error(m_view, msg);
}

void DolphinPart::slotRequestItemInfo(const KFileItem& item)
{
    Q_EMIT m_extension->mouseOverInfo(item);
    if (item.isNull()) {
        updateStatusBar();
    } else {
        const QString escapedText = Qt::convertFromPlainText(item.getStatusBarInfo());
        Q_EMIT ReadOnlyPart::setStatusBarText(QStringLiteral("<qt>%1</qt>").arg(escapedText));
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
    Q_EMIT m_extension->openUrlRequest(item.targetUrl(), args, browserArgs);
}

void DolphinPart::slotItemsActivated(const KFileItemList& items)
{
    for (const KFileItem& item : items) {
        slotItemActivated(item);
    }
}

void DolphinPart::createNewWindow(const QUrl& url)
{
    // TODO: Check issue N176832 for the missing QAIV signal; task 177399 - maybe this code
    // should be moved into DolphinPart::slotItemActivated()
    Q_EMIT m_extension->createNewWindow(url);
}

void DolphinPart::slotOpenContextMenu(const QPoint& pos,
                                      const KFileItem& _item,
                                      const KFileItemList &selectedItems,
                                      const QUrl &)
{
    KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems
                                                      | KParts::BrowserExtension::ShowProperties
                                                      | KParts::BrowserExtension::ShowUrlOperations;

    KFileItem item(_item);

    if (item.isNull()) { // viewport context menu
        item = m_view->rootItem();
        if (item.isNull())
            item = KFileItem(url());
        else
            item.setUrl(url()); // ensure we use the view url, not the canonical path (#213799)
    }

    KFileItemList items;
    if (selectedItems.isEmpty()) {
        items.append(item);
    } else {
        items = selectedItems;
    }

    KFileItemListProperties capabilities(items);

    KParts::BrowserExtension::ActionGroupMap actionGroups;
    QList<QAction *> editActions;
    editActions += m_view->versionControlActions(m_view->selectedItems());

    if (!_item.isNull()) { // only for context menu on one or more items
        const bool supportsMoving = capabilities.supportsMoving();

        if (capabilities.supportsDeleting()) {
            const bool showDeleteAction = (KSharedConfig::openConfig()->group("KDE").readEntry("ShowDeleteCommand", false) ||
                                           !item.isLocalFile());
            const bool showMoveToTrashAction = capabilities.isLocal() && supportsMoving;

            if (showDeleteAction && showMoveToTrashAction) {
                delete m_removeAction;
                m_removeAction = nullptr;
                editActions.append(actionCollection()->action(KStandardAction::name(KStandardAction::MoveToTrash)));
                editActions.append(actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile)));
            } else if (showDeleteAction && !showMoveToTrashAction) {
                editActions.append(actionCollection()->action(KStandardAction::name(KStandardAction::DeleteFile)));
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
            editActions.append(actionCollection()->action(KStandardAction::name(KStandardAction::RenameFile)));
        }

        // Normally KonqPopupMenu only shows the "Create new" submenu in the current view
        // since otherwise the created file would not be visible.
        // But in treeview mode we should allow it.
        if (m_view->itemsExpandable())
            popupFlags |= KParts::BrowserExtension::ShowCreateDirectory;

    }

    actionGroups.insert(QStringLiteral("editactions"), editActions);

    Q_EMIT m_extension->popupMenu(pos,
                                items,
                                KParts::OpenUrlArguments(),
                                KParts::BrowserArguments(),
                                popupFlags,
                                actionGroups);
}

void DolphinPart::slotDirectoryRedirection(const QUrl &oldUrl, const QUrl &newUrl)
{
    qCDebug(DolphinDebug) << oldUrl << newUrl << "currentUrl=" << url();
    if (oldUrl.matches(url(), QUrl::StripTrailingSlash /* #207572 */)) {
        KParts::ReadOnlyPart::setUrl(newUrl);
        const QString prettyUrl = newUrl.toDisplayString(QUrl::PreferLocalFile);
        Q_EMIT m_extension->setLocationBarUrl(prettyUrl);
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
    auto *dialog = new QInputDialog(m_view);
    dialog->setAttribute(Qt::WA_DeleteOnClose, true);
    dialog->setInputMode(QInputDialog::TextInput);
    dialog->setWindowTitle(title);
    dialog->setLabelText(text);

    const KConfigGroup group = KSharedConfig::openConfig("dolphinpartrc")->group("Select Dialog");
    dialog->setComboBoxEditable(true);
    dialog->setComboBoxItems(group.readEntry("History", QStringList()));

    dialog->setTextValue(QStringLiteral("*"));

    connect(dialog, &QDialog::accepted, this, [=]() {
        const QString pattern = dialog->textValue();
        if (!pattern.isEmpty()) {
            QStringList items = dialog->comboBoxItems();
            items.removeAll(pattern);
            items.prepend(pattern);

            // Need to evaluate this again here, because the captured value is const
            // (even if the const were removed from 'const KConfigGroup group =' above).
            KConfigGroup group = KSharedConfig::openConfig("dolphinpartrc")->group("Select Dialog");
            // Limit the size of the saved history.
            group.writeEntry("History", items.mid(0, 10));
            group.sync();

            const QRegularExpression patternRegExp(QRegularExpression::wildcardToRegularExpression(pattern));
            m_view->selectItems(patternRegExp, selectItems);
        }
    });

    dialog->open();
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

QString DolphinPart::localFilePathOrHome() const
{
    const QString localPath = localFilePath();
    if (!localPath.isEmpty()) {
        return localPath;
    }
    return QDir::homePath();
}

void DolphinPart::slotOpenTerminal()
{
    auto job = new KTerminalLauncherJob(QString());
    job->setWorkingDirectory(localFilePathOrHome());
    job->start();
}

void DolphinPart::slotFindFile()
{
    QMenu searchTools;
    KMoreToolsMenuFactory("dolphin/search-tools").fillMenuFromGroupingNames(
        &searchTools, { "files-find" }, QUrl::fromLocalFile(localFilePathOrHome())
    );
    QList<QAction*> actions = searchTools.actions();
    if (!(actions.isEmpty())) {
        actions.first()->trigger();
    } else {
        KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(QStringLiteral("kfind"), {url().toString()}, this);
        job->setDesktopName(QStringLiteral("org.kde.kfind"));
        job->setUiDelegate(new KDialogJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, widget()));
        job->start();
    }
}

void DolphinPart::updateNewMenu()
{
    // As requested by KNewFileMenu :
    m_newFileMenu->checkUpToDate();
    // And set the files that the menu apply on :
    m_newFileMenu->setPopupFiles(QList<QUrl>() << url());
}

void DolphinPart::updateStatusBar()
{
    m_view->requestStatusBarText();
}

void DolphinPart::updateProgress(int percent)
{
    Q_EMIT m_extension->loadingProgress(percent);
}

void DolphinPart::createDirectory()
{
    m_newFileMenu->setPopupFiles(QList<QUrl>() << url());
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
    using ShiftState = DolphinRemoveAction::ShiftState;
    const int type = event->type();

    if ((type == QEvent::KeyPress || type == QEvent::KeyRelease) && m_removeAction) {
        QMenu* menu = qobject_cast<QMenu*>(obj);
        if (menu && menu->parent() == m_view) {
            QKeyEvent* ev = static_cast<QKeyEvent*>(event);
            if (ev->key() == Qt::Key_Shift) {
                m_removeAction->update(type == QEvent::KeyPress ? ShiftState::Pressed : ShiftState::Released);
            }
        }
    }

    return KParts::ReadOnlyPart::eventFilter(obj, event);
}

#include "dolphinpart.moc"
