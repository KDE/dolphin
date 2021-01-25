/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Stefan Monov <logixoul@gmail.com>
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinmainwindow.h"

#include "dolphinmainwindowadaptor.h"
#include "config-terminal.h"
#include "global.h"
#include "dolphinbookmarkhandler.h"
#include "dolphindockwidget.h"
#include "dolphincontextmenu.h"
#include "dolphinnavigatorswidgetaction.h"
#include "dolphinnewfilemenu.h"
#include "dolphinrecenttabsmenu.h"
#include "dolphinurlnavigatorscontroller.h"
#include "dolphinviewcontainer.h"
#include "dolphintabpage.h"
#include "middleclickactioneventfilter.h"
#include "panels/folders/folderspanel.h"
#include "panels/places/placesitemmodel.h"
#include "panels/places/placespanel.h"
#include "panels/information/informationpanel.h"
#include "panels/terminal/terminalpanel.h"
#include "settings/dolphinsettingsdialog.h"
#include "statusbar/dolphinstatusbar.h"
#include "views/dolphinviewactionhandler.h"
#include "views/dolphinremoteencoding.h"
#include "views/draganddrophelper.h"
#include "views/viewproperties.h"
#include "views/dolphinnewfilemenuobserver.h"
#include "dolphin_generalsettings.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KAuthorized>
#include <KConfig>
#include <KConfigGui>
#include <KDualAction>
#include <KFileItemListProperties>
#include <KHelpMenu>
#include <KIO/CommandLauncherJob>
#include <KIO/JobUiDelegate>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KNS3/KMoreToolsMenuFactory>
#include <KProtocolInfo>
#include <KProtocolManager>
#include <KShell>
#include <KStandardAction>
#include <KStartupInfo>
#include <KSycoca>
#include <KToggleAction>
#include <KToolBar>
#include <KToolBarPopupAction>
#include <KToolInvocation>
#include <KUrlComboBox>
#include <KUrlNavigator>
#include <KWindowSystem>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <QWhatsThisClickedEvent>

namespace {
    // Used for GeneralSettings::version() to determine whether
    // an updated version of Dolphin is running.
    const int CurrentDolphinVersion = 200;
    // The maximum number of entries in the back/forward popup menu
    const int MaxNumberOfNavigationentries = 12;
    // The maximum number of "Activate Tab" shortcuts
    const int MaxActivateTabShortcuts = 9;
}

DolphinMainWindow::DolphinMainWindow() :
    KXmlGuiWindow(nullptr),
    m_newFileMenu(nullptr),
    m_helpMenu(nullptr),
    m_tabWidget(nullptr),
    m_activeViewContainer(nullptr),
    m_actionHandler(nullptr),
    m_remoteEncoding(nullptr),
    m_settingsDialog(),
    m_bookmarkHandler(nullptr),
    m_controlButton(nullptr),
    m_updateToolBarTimer(nullptr),
    m_lastHandleUrlOpenJob(nullptr),
    m_terminalPanel(nullptr),
    m_placesPanel(nullptr),
    m_tearDownFromPlacesRequested(false),
    m_backAction(nullptr),
    m_forwardAction(nullptr)
{
    Q_INIT_RESOURCE(dolphin);

    new MainWindowAdaptor(this);

#ifndef Q_OS_WIN
	setWindowFlags(Qt::WindowContextHelpButtonHint);
#endif
    setComponentName(QStringLiteral("dolphin"), QGuiApplication::applicationDisplayName());
    setObjectName(QStringLiteral("Dolphin#"));

    connect(&DolphinNewFileMenuObserver::instance(), &DolphinNewFileMenuObserver::errorMessage,
            this, &DolphinMainWindow::showErrorMessage);

    KIO::FileUndoManager* undoManager = KIO::FileUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface());

    connect(undoManager, QOverload<bool>::of(&KIO::FileUndoManager::undoAvailable),
            this, &DolphinMainWindow::slotUndoAvailable);
    connect(undoManager, &KIO::FileUndoManager::undoTextChanged,
            this, &DolphinMainWindow::slotUndoTextChanged);
    connect(undoManager, &KIO::FileUndoManager::jobRecordingStarted,
            this, &DolphinMainWindow::clearStatusBar);
    connect(undoManager, &KIO::FileUndoManager::jobRecordingFinished,
            this, &DolphinMainWindow::showCommand);

    const bool firstRun = (GeneralSettings::version() < 200);
    if (firstRun) {
        GeneralSettings::setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    auto *navigatorsWidgetAction = new DolphinNavigatorsWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("url_navigators"), navigatorsWidgetAction);
    m_tabWidget = new DolphinTabWidget(navigatorsWidgetAction, this);
    m_tabWidget->setObjectName("tabWidget");
    connect(m_tabWidget, &DolphinTabWidget::activeViewChanged,
            this, &DolphinMainWindow::activeViewChanged);
    connect(m_tabWidget, &DolphinTabWidget::tabCountChanged,
            this, &DolphinMainWindow::tabCountChanged);
    connect(m_tabWidget, &DolphinTabWidget::currentUrlChanged,
            this, &DolphinMainWindow::updateWindowTitle);
    setCentralWidget(m_tabWidget);

    setupActions();

    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    connect(m_actionHandler, &DolphinViewActionHandler::actionBeingHandled, this, &DolphinMainWindow::clearStatusBar);
    connect(m_actionHandler, &DolphinViewActionHandler::createDirectoryTriggered, this, &DolphinMainWindow::createDirectory);

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, &DolphinMainWindow::urlChanged,
            m_remoteEncoding, &DolphinRemoteEncoding::slotAboutToOpenUrl);

    setupDockWidgets();

    setupGUI(Keys | Save | Create | ToolBar);
    stateChanged(QStringLiteral("new_file"));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged,
            this, &DolphinMainWindow::updatePasteAction);

    QAction* toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(GeneralSettings::filterBar());

    if (firstRun) {
        menuBar()->setVisible(false);
        // Assure a proper default size if Dolphin runs the first time
        resize(750, 500);
    }

    const bool showMenu = !menuBar()->isHidden();
    QAction* showMenuBarAction = actionCollection()->action(KStandardAction::name(KStandardAction::ShowMenubar));
    showMenuBarAction->setChecked(showMenu);  // workaround for bug #171080
    if (!showMenu) {
        createControlButton();
    }

    updateAllowedToolbarAreas();

    // enable middle-click on back/forward/up to open in a new tab
    auto *middleClickEventFilter = new MiddleClickActionEventFilter(this);
    connect(middleClickEventFilter, &MiddleClickActionEventFilter::actionMiddleClicked, this, &DolphinMainWindow::slotToolBarActionMiddleClicked);
    toolBar()->installEventFilter(middleClickEventFilter);

    setupWhatsThis();

    connect(KSycoca::self(), QOverload<>::of(&KSycoca::databaseChanged), this, &DolphinMainWindow::updateOpenPreferredSearchToolAction);

    QTimer::singleShot(0, this, &DolphinMainWindow::updateOpenPreferredSearchToolAction);
}

DolphinMainWindow::~DolphinMainWindow()
{
}

QVector<DolphinViewContainer*> DolphinMainWindow::viewContainers() const
{
    QVector<DolphinViewContainer*> viewContainers;

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        DolphinTabPage *tabPage = m_tabWidget->tabPageAt(i);

        viewContainers << tabPage->primaryViewContainer();
        if (tabPage->splitViewEnabled()) {
            viewContainers << tabPage->secondaryViewContainer();
        }
    }
    return viewContainers;
}

void DolphinMainWindow::setViewsWithInvalidPathsToHome()
{
    const QVector<DolphinViewContainer*> theViewContainers = viewContainers();
    for (DolphinViewContainer *viewContainer : theViewContainers) {

        // Only consider local dirs, not remote locations and abstract protocols
        if (viewContainer->url().isLocalFile()) {
            if (!QFileInfo::exists(viewContainer->url().toLocalFile())) {
                viewContainer->setUrl(QUrl::fromLocalFile(QDir::homePath()));
            }
        }
    }
}

void DolphinMainWindow::openDirectories(const QList<QUrl>& dirs, bool splitView)
{
    m_tabWidget->openDirectories(dirs, splitView);
}

void DolphinMainWindow::openDirectories(const QStringList& dirs, bool splitView)
{
    openDirectories(QUrl::fromStringList(dirs), splitView);
}

void DolphinMainWindow::openFiles(const QList<QUrl>& files, bool splitView)
{
    m_tabWidget->openFiles(files, splitView);
}

bool DolphinMainWindow::isFoldersPanelEnabled() const
{
    return actionCollection()->action(QStringLiteral("show_folders_panel"))->isChecked();
}

bool DolphinMainWindow::isInformationPanelEnabled() const
{
#ifdef HAVE_BALOO
    return actionCollection()->action(QStringLiteral("show_information_panel"))->isChecked();
#else
    return false;
#endif
}

void DolphinMainWindow::openFiles(const QStringList& files, bool splitView)
{
    openFiles(QUrl::fromStringList(files), splitView);
}

void DolphinMainWindow::activateWindow()
{
    window()->setAttribute(Qt::WA_NativeWindow, true);
    KStartupInfo::setNewStartupId(window()->windowHandle(), KStartupInfo::startupId());
    KWindowSystem::activateWindow(window()->effectiveWinId());
}

void DolphinMainWindow::showCommand(CommandType command)
{
    DolphinStatusBar* statusBar = m_activeViewContainer->statusBar();
    switch (command) {
    case KIO::FileUndoManager::Copy:
        statusBar->setText(i18nc("@info:status", "Successfully copied."));
        break;
    case KIO::FileUndoManager::Move:
        statusBar->setText(i18nc("@info:status", "Successfully moved."));
        break;
    case KIO::FileUndoManager::Link:
        statusBar->setText(i18nc("@info:status", "Successfully linked."));
        break;
    case KIO::FileUndoManager::Trash:
        statusBar->setText(i18nc("@info:status", "Successfully moved to trash."));
        break;
    case KIO::FileUndoManager::Rename:
        statusBar->setText(i18nc("@info:status", "Successfully renamed."));
        break;

    case KIO::FileUndoManager::Mkdir:
        statusBar->setText(i18nc("@info:status", "Created folder."));
        break;

    default:
        break;
    }
}

void DolphinMainWindow::pasteIntoFolder()
{
    m_activeViewContainer->view()->pasteIntoFolder();
}

void DolphinMainWindow::changeUrl(const QUrl &url)
{
    if (!KProtocolManager::supportsListing(url)) {
        // The URL navigator only checks for validity, not
        // if the URL can be listed. An error message is
        // shown due to DolphinViewContainer::restoreView().
        return;
    }

    m_activeViewContainer->setUrl(url);
    updateFileAndEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();

    Q_EMIT urlChanged(url);
}

void DolphinMainWindow::slotTerminalDirectoryChanged(const QUrl& url)
{
    if (m_tearDownFromPlacesRequested && url == QUrl::fromLocalFile(QDir::homePath())) {
        m_placesPanel->proceedWithTearDown();
        m_tearDownFromPlacesRequested = false;
    }

    m_activeViewContainer->setAutoGrabFocus(false);
    changeUrl(url);
    m_activeViewContainer->setAutoGrabFocus(true);
}

void DolphinMainWindow::slotEditableStateChanged(bool editable)
{
    KToggleAction* editableLocationAction =
        static_cast<KToggleAction*>(actionCollection()->action(QStringLiteral("editable_location")));
    editableLocationAction->setChecked(editable);
}

void DolphinMainWindow::slotSelectionChanged(const KFileItemList& selection)
{
    updateFileAndEditActions();

    const int selectedUrlsCount = m_tabWidget->currentTabPage()->selectedItemsCount();

    QAction* compareFilesAction = actionCollection()->action(QStringLiteral("compare_files"));
    if (selectedUrlsCount == 2) {
        compareFilesAction->setEnabled(isKompareInstalled());
    } else {
        compareFilesAction->setEnabled(false);
    }

    Q_EMIT selectionChanged(selection);
}

void DolphinMainWindow::updateHistory()
{
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    const int index = urlNavigator->historyIndex();

    QAction* backAction = actionCollection()->action(KStandardAction::name(KStandardAction::Back));
    if (backAction) {
        backAction->setToolTip(i18nc("@info", "Go back"));
        backAction->setWhatsThis(i18nc("@info:whatsthis go back", "Return to the previously viewed folder."));
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction* forwardAction = actionCollection()->action(KStandardAction::name(KStandardAction::Forward));
    if (forwardAction) {
        forwardAction->setToolTip(i18nc("@info", "Go forward"));
        forwardAction->setWhatsThis(xi18nc("@info:whatsthis go forward",
            "This undoes a <interface>Go|Back</interface> action."));
        forwardAction->setEnabled(index > 0);
    }
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    QAction* toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(show);
}

void DolphinMainWindow::openNewMainWindow()
{
    Dolphin::openNewWindow({m_activeViewContainer->url()}, this);
}

void DolphinMainWindow::openNewActivatedTab()
{
    // keep browsers compatibility, new tab is always after last one
    auto openNewTabAfterLastTabConfigured = GeneralSettings::openNewTabAfterLastTab();
    GeneralSettings::setOpenNewTabAfterLastTab(true);
    m_tabWidget->openNewActivatedTab();
    GeneralSettings::setOpenNewTabAfterLastTab(openNewTabAfterLastTabConfigured);
}

void DolphinMainWindow::addToPlaces()
{
    QUrl url;
    QString name;

    // If nothing is selected, act on the current dir
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        url = m_activeViewContainer->url();
        name = m_activeViewContainer->placesText();
    } else {
        const auto dirToAdd = m_activeViewContainer->view()->selectedItems().first();
        url = dirToAdd.url();
        name = dirToAdd.name();
    }
    if (url.isValid()) {
        PlacesItemModel model;
        QString icon;
        if (m_activeViewContainer->isSearchModeEnabled()) {
            icon = QStringLiteral("folder-saved-search-symbolic");
        } else {
            icon = KIO::iconNameForUrl(url);
        }
        model.createPlacesItem(name, url, icon);
    }
}

void DolphinMainWindow::openNewTab(const QUrl& url)
{
    m_tabWidget->openNewTab(url, QUrl());
}

void DolphinMainWindow::openInNewTab()
{
    const KFileItemList& list = m_activeViewContainer->view()->selectedItems();
    bool tabCreated = false;

    for (const KFileItem& item : list) {
        const QUrl& url = DolphinView::openItemAsFolderUrl(item);
        if (!url.isEmpty()) {
            openNewTab(url);
            tabCreated = true;
        }
    }

    // if no new tab has been created from the selection
    // open the current directory in a new tab
    if (!tabCreated) {
        openNewTab(m_activeViewContainer->url());
    }
}

void DolphinMainWindow::openInNewWindow()
{
    QUrl newWindowUrl;

    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        newWindowUrl = m_activeViewContainer->url();
    } else if (list.count() == 1) {
        const KFileItem& item = list.first();
        newWindowUrl = DolphinView::openItemAsFolderUrl(item);
    }

    if (!newWindowUrl.isEmpty()) {
        Dolphin::openNewWindow({newWindowUrl}, this);
    }
}

void DolphinMainWindow::showTarget()
{
    const auto link = m_activeViewContainer->view()->selectedItems().at(0);
    const auto linkLocationDir = QFileInfo(link.localPath()).absoluteDir();
    auto linkDestination = link.linkDest();
    if (QFileInfo(linkDestination).isRelative()) {
        linkDestination = linkLocationDir.filePath(linkDestination);
    }
    if (QFileInfo::exists(linkDestination)) {
        KIO::highlightInFileManager({QUrl::fromLocalFile(linkDestination).adjusted(QUrl::StripTrailingSlash)});
    } else {
        m_activeViewContainer->showMessage(xi18nc("@info", "Could not access <filename>%1</filename>.", linkDestination),
                                           DolphinViewContainer::Warning);
    }
}

void DolphinMainWindow::showEvent(QShowEvent* event)
{
    KXmlGuiWindow::showEvent(event);

    if (!event->spontaneous()) {
        m_activeViewContainer->view()->setFocus();
    }
}

void DolphinMainWindow::closeEvent(QCloseEvent* event)
{
    // Find out if Dolphin is closed directly by the user or
    // by the session manager because the session is closed
    bool closedByUser = true;
    if (qApp->isSavingSession()) {
        closedByUser = false;
    }

    if (m_tabWidget->count() > 1
        && GeneralSettings::confirmClosingMultipleTabs()
        && !GeneralSettings::rememberOpenedTabs()
        && closedByUser) {
        // Ask the user if he really wants to quit and close all tabs.
        // Open a confirmation dialog with 3 buttons:
        // QDialogButtonBox::Yes    -> Quit
        // QDialogButtonBox::No     -> Close only the current tab
        // QDialogButtonBox::Cancel -> do nothing
        QDialog *dialog = new QDialog(this, Qt::Dialog);
        dialog->setWindowTitle(i18nc("@title:window", "Confirmation"));
        dialog->setModal(true);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel);
        KGuiItem::assign(buttons->button(QDialogButtonBox::Yes), KGuiItem(i18nc("@action:button 'Quit Dolphin' button", "&Quit %1", QGuiApplication::applicationDisplayName()), QIcon::fromTheme(QStringLiteral("application-exit"))));
        KGuiItem::assign(buttons->button(QDialogButtonBox::No), KGuiItem(i18n("C&lose Current Tab"), QIcon::fromTheme(QStringLiteral("tab-close"))));
        KGuiItem::assign(buttons->button(QDialogButtonBox::Cancel), KStandardGuiItem::cancel());
        buttons->button(QDialogButtonBox::Yes)->setDefault(true);

        bool doNotAskAgainCheckboxResult = false;

        const auto result = KMessageBox::createKMessageBox(dialog,
            buttons,
            QMessageBox::Warning,
            i18n("You have multiple tabs open in this window, are you sure you want to quit?"),
            QStringList(),
            i18n("Do not ask again"),
            &doNotAskAgainCheckboxResult,
            KMessageBox::Notify);

        if (doNotAskAgainCheckboxResult) {
            GeneralSettings::setConfirmClosingMultipleTabs(false);
        }

        switch (result) {
            case QDialogButtonBox::Yes:
                // Quit
                break;
            case QDialogButtonBox::No:
                // Close only the current tab
                m_tabWidget->closeTab();
                Q_FALLTHROUGH();
            default:
                event->ignore();
                return;
        }
    }

    if (m_terminalPanel && m_terminalPanel->hasProgramRunning() && GeneralSettings::confirmClosingTerminalRunningProgram() && closedByUser) {
        // Ask if the user really wants to quit Dolphin with a program that is still running in the Terminal panel
        // Open a confirmation dialog with 3 buttons:
        // QDialogButtonBox::Yes    -> Quit
        // QDialogButtonBox::No     -> Show Terminal Panel
        // QDialogButtonBox::Cancel -> do nothing
        QDialog *dialog = new QDialog(this, Qt::Dialog);
        dialog->setWindowTitle(i18nc("@title:window", "Confirmation"));
        dialog->setModal(true);
        auto standardButtons = QDialogButtonBox::Yes | QDialogButtonBox::Cancel;
        if (!m_terminalPanel->isVisible()) {
            standardButtons |= QDialogButtonBox::No;
        }
        QDialogButtonBox *buttons = new QDialogButtonBox(standardButtons);
        KGuiItem::assign(buttons->button(QDialogButtonBox::Yes), KStandardGuiItem::quit());
        if (!m_terminalPanel->isVisible()) {
            KGuiItem::assign(
                    buttons->button(QDialogButtonBox::No),
                    KGuiItem(i18n("Show &Terminal Panel"), QIcon::fromTheme(QStringLiteral("dialog-scripts"))));
        }
        KGuiItem::assign(buttons->button(QDialogButtonBox::Cancel), KStandardGuiItem::cancel());

        bool doNotAskAgainCheckboxResult = false;

        const auto result = KMessageBox::createKMessageBox(
                dialog,
                buttons,
                QMessageBox::Warning,
                i18n("The program '%1' is still running in the Terminal panel. Are you sure you want to quit?", m_terminalPanel->runningProgramName()),
                QStringList(),
                i18n("Do not ask again"),
                &doNotAskAgainCheckboxResult,
                KMessageBox::Dangerous);

        if (doNotAskAgainCheckboxResult) {
            GeneralSettings::setConfirmClosingTerminalRunningProgram(false);
        }

        switch (result) {
            case QDialogButtonBox::Yes:
                // Quit
                break;
            case QDialogButtonBox::No:
                actionCollection()->action("show_terminal_panel")->trigger();
                // Do not quit, ignore quit event
                Q_FALLTHROUGH();
            default:
                event->ignore();
                return;
        }
    }

    if (GeneralSettings::rememberOpenedTabs())  {
        KConfigGui::setSessionConfig(QStringLiteral("dolphin"), QStringLiteral("dolphin"));
        KConfig *config = KConfigGui::sessionConfig();
        saveGlobalProperties(config);
        savePropertiesInternal(config, 1);
        config->sync();
    }

    GeneralSettings::setVersion(CurrentDolphinVersion);
    GeneralSettings::self()->save();

    KXmlGuiWindow::closeEvent(event);
}

void DolphinMainWindow::saveProperties(KConfigGroup& group)
{
    m_tabWidget->saveProperties(group);
}

void DolphinMainWindow::readProperties(const KConfigGroup& group)
{
    m_tabWidget->readProperties(group);
}

void DolphinMainWindow::updateNewMenu()
{
    m_newFileMenu->setViewShowsHiddenFiles(activeViewContainer()->view()->hiddenFilesShown());
    m_newFileMenu->checkUpToDate();
    m_newFileMenu->setPopupFiles(QList<QUrl>() << activeViewContainer()->url());
}

void DolphinMainWindow::createDirectory()
{
    m_newFileMenu->setViewShowsHiddenFiles(activeViewContainer()->view()->hiddenFilesShown());
    m_newFileMenu->setPopupFiles(QList<QUrl>() << activeViewContainer()->url());
    m_newFileMenu->createDirectory();
}

void DolphinMainWindow::quit()
{
    close();
}

void DolphinMainWindow::showErrorMessage(const QString& message)
{
    m_activeViewContainer->showMessage(message, DolphinViewContainer::Error);
}

void DolphinMainWindow::slotUndoAvailable(bool available)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    if (undoAction) {
        undoAction->setEnabled(available);
    }
}

void DolphinMainWindow::slotUndoTextChanged(const QString& text)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    if (undoAction) {
        undoAction->setText(text);
    }
}

void DolphinMainWindow::undo()
{
    clearStatusBar();
    KIO::FileUndoManager::self()->uiInterface()->setParentWidget(this);
    KIO::FileUndoManager::self()->undo();
}

void DolphinMainWindow::cut()
{
    m_activeViewContainer->view()->cutSelectedItemsToClipboard();
}

void DolphinMainWindow::copy()
{
    m_activeViewContainer->view()->copySelectedItemsToClipboard();
}

void DolphinMainWindow::paste()
{
    m_activeViewContainer->view()->paste();
}

void DolphinMainWindow::find()
{
    m_activeViewContainer->setSearchModeEnabled(true);
}

void DolphinMainWindow::updateSearchAction()
{
    QAction* toggleSearchAction = actionCollection()->action(QStringLiteral("toggle_search"));
    toggleSearchAction->setChecked(m_activeViewContainer->isSearchModeEnabled());
}

void DolphinMainWindow::updatePasteAction()
{
    QAction* pasteAction = actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
    QPair<bool, QString> pasteInfo = m_activeViewContainer->view()->pasteInfo();
    pasteAction->setEnabled(pasteInfo.first);
    pasteAction->setText(pasteInfo.second);
}

void DolphinMainWindow::slotDirectoryLoadingCompleted()
{
    updatePasteAction();
}

void DolphinMainWindow::slotToolBarActionMiddleClicked(QAction *action)
{
    if (action == actionCollection()->action(KStandardAction::name(KStandardAction::Back))) {
        goBackInNewTab();
    } else if (action == actionCollection()->action(KStandardAction::name(KStandardAction::Forward))) {
        goForwardInNewTab();
    } else if (action == actionCollection()->action(QStringLiteral("go_up"))) {
        goUpInNewTab();
    } else if (action == actionCollection()->action(QStringLiteral("go_home"))) {
        goHomeInNewTab();
    }
}

void DolphinMainWindow::slotAboutToShowBackPopupMenu()
{
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    int entries = 0;
    m_backAction->menu()->clear();
    for (int i = urlNavigator->historyIndex() + 1; i < urlNavigator->historySize() && entries < MaxNumberOfNavigationentries; ++i, ++entries) {
        QAction* action = new QAction(urlNavigator->locationUrl(i).toString(QUrl::PreferLocalFile), m_backAction->menu());
        action->setData(i);
        m_backAction->menu()->addAction(action);
    }
}

void DolphinMainWindow::slotGoBack(QAction* action)
{
    int gotoIndex = action->data().value<int>();
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    for (int i = gotoIndex - urlNavigator->historyIndex(); i > 0; --i) {
        goBack();
    }
}

void DolphinMainWindow::slotBackForwardActionMiddleClicked(QAction* action)
{
    if (action) {
        const KUrlNavigator *urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
        openNewTab(urlNavigator->locationUrl(action->data().value<int>()));
    }
}

void DolphinMainWindow::slotAboutToShowForwardPopupMenu()
{
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    int entries = 0;
    m_forwardAction->menu()->clear();
    for (int i = urlNavigator->historyIndex() - 1; i >= 0 && entries < MaxNumberOfNavigationentries; --i, ++entries) {
        QAction* action = new QAction(urlNavigator->locationUrl(i).toString(QUrl::PreferLocalFile), m_forwardAction->menu());
        action->setData(i);
        m_forwardAction->menu()->addAction(action);
    }
}

void DolphinMainWindow::slotGoForward(QAction* action)
{
    int gotoIndex = action->data().value<int>();
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    for (int i = urlNavigator->historyIndex() - gotoIndex; i > 0; --i) {
        goForward();
    }
}

void DolphinMainWindow::selectAll()
{
    clearStatusBar();

    // if the URL navigator is editable and focused, select the whole
    // URL instead of all items of the view

    KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    QLineEdit* lineEdit = urlNavigator->editor()->lineEdit();
    const bool selectUrl = urlNavigator->isUrlEditable() &&
                           lineEdit->hasFocus();
    if (selectUrl) {
        lineEdit->selectAll();
    } else {
        m_activeViewContainer->view()->selectAll();
    }
}

void DolphinMainWindow::invertSelection()
{
    clearStatusBar();
    m_activeViewContainer->view()->invertSelection();
}

void DolphinMainWindow::toggleSplitView()
{
    DolphinTabPage* tabPage = m_tabWidget->currentTabPage();
    tabPage->setSplitViewEnabled(!tabPage->splitViewEnabled(), WithAnimation);

    updateViewActions();
}

void DolphinMainWindow::toggleSplitStash()
{
    DolphinTabPage* tabPage = m_tabWidget->currentTabPage();
    tabPage->setSplitViewEnabled(false, WithAnimation);
    tabPage->setSplitViewEnabled(true, WithAnimation, QUrl("stash:/"));
}

void DolphinMainWindow::reloadView()
{
    clearStatusBar();
    m_activeViewContainer->reload();
    m_activeViewContainer->statusBar()->updateSpaceInfo();
}

void DolphinMainWindow::stopLoading()
{
    m_activeViewContainer->view()->stopLoading();
}

void DolphinMainWindow::enableStopAction()
{
    actionCollection()->action(QStringLiteral("stop"))->setEnabled(true);
}

void DolphinMainWindow::disableStopAction()
{
    actionCollection()->action(QStringLiteral("stop"))->setEnabled(false);
}

void DolphinMainWindow::showFilterBar()
{
    m_activeViewContainer->setFilterBarVisible(true);
}

void DolphinMainWindow::toggleFilterBar()
{
    const bool checked = !m_activeViewContainer->isFilterBarVisible();
    m_activeViewContainer->setFilterBarVisible(checked);

    QAction* toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(checked);
}

void DolphinMainWindow::toggleEditLocation()
{
    clearStatusBar();

    QAction* action = actionCollection()->action(QStringLiteral("editable_location"));
    KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    urlNavigator->setUrlEditable(action->isChecked());
}

void DolphinMainWindow::replaceLocation()
{
    KUrlNavigator* navigator = m_activeViewContainer->urlNavigator();
    QLineEdit* lineEdit = navigator->editor()->lineEdit();

    // If the text field currently has focus and everything is selected,
    // pressing the keyboard shortcut returns the whole thing to breadcrumb mode
    if (navigator->isUrlEditable()
        && lineEdit->hasFocus()
        && lineEdit->selectedText() == lineEdit->text() ) {
        navigator->setUrlEditable(false);
    } else {
        navigator->setUrlEditable(true);
        navigator->setFocus();
        lineEdit->selectAll();
    }
}

void DolphinMainWindow::togglePanelLockState()
{
    const bool newLockState = !GeneralSettings::lockPanels();
    const auto childrenObjects = children();
    for (QObject* child : childrenObjects) {
        DolphinDockWidget* dock = qobject_cast<DolphinDockWidget*>(child);
        if (dock) {
            dock->setLocked(newLockState);
        }
    }

    GeneralSettings::setLockPanels(newLockState);
}

void DolphinMainWindow::slotTerminalPanelVisibilityChanged()
{
    if (m_terminalPanel->isHiddenInVisibleWindow() && m_activeViewContainer) {
        m_activeViewContainer->view()->setFocus();
    }
}

void DolphinMainWindow::goBack()
{
    DolphinUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    urlNavigator->goBack();

    if (urlNavigator->locationState().isEmpty()) {
        // An empty location state indicates a redirection URL,
        // which must be skipped too
        urlNavigator->goBack();
    }
}

void DolphinMainWindow::goForward()
{
    m_activeViewContainer->urlNavigatorInternalWithHistory()->goForward();
}

void DolphinMainWindow::goUp()
{
    m_activeViewContainer->urlNavigatorInternalWithHistory()->goUp();
}

void DolphinMainWindow::goHome()
{
    m_activeViewContainer->urlNavigatorInternalWithHistory()->goHome();
}

void DolphinMainWindow::goBackInNewTab()
{
    const KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
    const int index = urlNavigator->historyIndex() + 1;
    openNewTab(urlNavigator->locationUrl(index));
}

void DolphinMainWindow::goForwardInNewTab()
{
    const KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
    const int index = urlNavigator->historyIndex() - 1;
    openNewTab(urlNavigator->locationUrl(index));
}

void DolphinMainWindow::goUpInNewTab()
{
    const QUrl currentUrl = activeViewContainer()->urlNavigator()->locationUrl();
    openNewTab(KIO::upUrl(currentUrl));
}

void DolphinMainWindow::goHomeInNewTab()
{
    openNewTab(Dolphin::homeUrl());
}

void DolphinMainWindow::compareFiles()
{
    const KFileItemList items = m_tabWidget->currentTabPage()->selectedItems();
    if (items.count() != 2) {
        // The action is disabled in this case, but it could have been triggered
        // via D-Bus, see https://bugs.kde.org/show_bug.cgi?id=325517
        return;
    }

    QUrl urlA = items.at(0).url();
    QUrl urlB = items.at(1).url();

    QString command(QStringLiteral("kompare -c \""));
    command.append(urlA.toDisplayString(QUrl::PreferLocalFile));
    command.append("\" \"");
    command.append(urlB.toDisplayString(QUrl::PreferLocalFile));
    command.append('\"');

    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob(command, this);
    job->setDesktopName(QStringLiteral("org.kde.kompare"));
    job->start();
}

void DolphinMainWindow::toggleShowMenuBar()
{
    const bool visible = menuBar()->isVisible();
    menuBar()->setVisible(!visible);
    if (visible) {
        createControlButton();
    } else {
        deleteControlButton();
    }
}

QPointer<QAction> DolphinMainWindow::preferredSearchTool()
{
    m_searchTools.clear();
    KMoreToolsMenuFactory("dolphin/search-tools").fillMenuFromGroupingNames(
        &m_searchTools, { "files-find" }, m_activeViewContainer->url()
    );
    QList<QAction*> actions = m_searchTools.actions();
    if (actions.isEmpty()) {
        return nullptr;
    }
    QAction* action = actions.first();
    if (action->isSeparator()) {
        return nullptr;
    }
    return action;
}

void DolphinMainWindow::updateOpenPreferredSearchToolAction()
{
    QAction* openPreferredSearchTool = actionCollection()->action(QStringLiteral("open_preferred_search_tool"));
    if (!openPreferredSearchTool) {
        return;
    }
    QPointer<QAction> tool = preferredSearchTool();
    if (tool) {
        openPreferredSearchTool->setVisible(true);
        openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open %1", tool->text()));
        openPreferredSearchTool->setIcon(tool->icon());
    } else {
        openPreferredSearchTool->setVisible(false);
        // still visible in Shortcuts configuration window
        openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open Preferred Search Tool"));
        openPreferredSearchTool->setIcon(QIcon::fromTheme(QStringLiteral("search")));
    }
}

void DolphinMainWindow::openPreferredSearchTool()
{
    QPointer<QAction> tool = preferredSearchTool();
    if (tool) {
        tool->trigger();
    }
}

void DolphinMainWindow::openTerminal()
{
    const QUrl url = m_activeViewContainer->url();

    if (url.isLocalFile()) {
        KToolInvocation::invokeTerminal(QString(), url.toLocalFile());
        return;
    }

     // Not a local file, with protocol Class ":local", try stat'ing
    if (KProtocolInfo::protocolClass(url.scheme()) == QLatin1String(":local")) {
        KIO::StatJob *job = KIO::mostLocalUrl(url);
        KJobWidgets::setWindow(job, this);
        connect(job, &KJob::result, this, [job]() {
            QUrl statUrl;
            if (!job->error()) {
                statUrl = job->mostLocalUrl();
            }

            KToolInvocation::invokeTerminal(QString(), statUrl.isLocalFile() ? statUrl.toLocalFile() : QDir::homePath());
        });

        return;
    }

    // Nothing worked, just use $HOME
    KToolInvocation::invokeTerminal(QString(), QDir::homePath());
}

void DolphinMainWindow::editSettings()
{
    if (!m_settingsDialog) {
        DolphinViewContainer* container = activeViewContainer();
        container->view()->writeSettings();

        const QUrl url = container->url();
        DolphinSettingsDialog* settingsDialog = new DolphinSettingsDialog(url, this, actionCollection());
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged, this, &DolphinMainWindow::refreshViews);
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged,
                &DolphinUrlNavigatorsController::slotReadSettings);
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        settingsDialog->show();
        m_settingsDialog = settingsDialog;
    } else {
        m_settingsDialog.data()->raise();
    }
}

void DolphinMainWindow::handleUrl(const QUrl& url)
{
    delete m_lastHandleUrlOpenJob;
    m_lastHandleUrlOpenJob = nullptr;

    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()) {
        activeViewContainer()->setUrl(url);
    } else {
        m_lastHandleUrlOpenJob = new KIO::OpenUrlJob(url);
        m_lastHandleUrlOpenJob->setUiDelegate(new KIO::JobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        m_lastHandleUrlOpenJob->setShowOpenOrExecuteDialog(true);

        connect(m_lastHandleUrlOpenJob, &KIO::OpenUrlJob::mimeTypeFound, this,
                [this, url](const QString &mimetype) {
                    if (mimetype == QLatin1String("inode/directory")) {
                        // If it's a dir, we'll take it from here
                        m_lastHandleUrlOpenJob->kill();
                        m_lastHandleUrlOpenJob = nullptr;
                        activeViewContainer()->setUrl(url);
                    }
        });

        connect(m_lastHandleUrlOpenJob, &KIO::OpenUrlJob::result, this, [this]() {
            m_lastHandleUrlOpenJob = nullptr;
        });

        m_lastHandleUrlOpenJob->start();
    }
}

void DolphinMainWindow::slotWriteStateChanged(bool isFolderWritable)
{
    // trash:/ is writable but we don't want to create new items in it.
    // TODO: remove the trash check once https://phabricator.kde.org/T8234 is implemented
    newFileMenu()->setEnabled(isFolderWritable && m_activeViewContainer->url().scheme() != QLatin1String("trash"));
}

void DolphinMainWindow::openContextMenu(const QPoint& pos,
                                        const KFileItem& item,
                                        const QUrl& url,
                                        const QList<QAction*>& customActions)
{
    QPointer<DolphinContextMenu> contextMenu = new DolphinContextMenu(this, pos, item, url);
    contextMenu.data()->setCustomActions(customActions);
    const DolphinContextMenu::Command command = contextMenu.data()->open();

    switch (command) {
    case DolphinContextMenu::OpenParentFolder:
        changeUrl(KIO::upUrl(item.url()));
        m_activeViewContainer->view()->markUrlsAsSelected({item.url()});
        m_activeViewContainer->view()->markUrlAsCurrent(item.url());
        break;

    case DolphinContextMenu::OpenParentFolderInNewWindow:
        Dolphin::openNewWindow({item.url()}, this, Dolphin::OpenNewWindowFlag::Select);
        break;

    case DolphinContextMenu::OpenParentFolderInNewTab:
        openNewTab(KIO::upUrl(item.url()));
        break;

    case DolphinContextMenu::None:
    default:
        break;
    }

    // Delete the menu, unless it has been deleted in its own nested event loop already.
    if (contextMenu) {
        contextMenu->deleteLater();
    }
}

void DolphinMainWindow::updateControlMenu()
{
    QMenu* menu = qobject_cast<QMenu*>(sender());
    Q_ASSERT(menu);

    // All actions get cleared by QMenu::clear(). This includes the sub-menus
    // because 'menu' is their parent.
    menu->clear();

    KActionCollection* ac = actionCollection();

    menu->addMenu(m_newFileMenu->menu());
    addActionToMenu(ac->action(QStringLiteral("file_new")), menu);
    addActionToMenu(ac->action(QStringLiteral("new_tab")), menu);
    addActionToMenu(ac->action(QStringLiteral("closed_tabs")), menu);

    menu->addSeparator();

    // Add "Edit" actions
    bool added = addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Undo)), menu) |
                 addActionToMenu(ac->action(QString("copy_location")), menu) |
                 addActionToMenu(ac->action(QStringLiteral("copy_to_inactive_split_view")), menu) |
                 addActionToMenu(ac->action(QStringLiteral("move_to_inactive_split_view")), menu) |
                 addActionToMenu(ac->action(KStandardAction::name(KStandardAction::SelectAll)), menu) |
                 addActionToMenu(ac->action(QStringLiteral("invert_selection")), menu);

    if (added) {
        menu->addSeparator();
    }

    // Add "View" actions
    if (!GeneralSettings::showZoomSlider()) {
        addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ZoomIn)), menu);
        addActionToMenu(ac->action(QStringLiteral("view_zoom_reset")), menu);
        addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ZoomOut)), menu);
        menu->addSeparator();
    }

    added = addActionToMenu(ac->action(QStringLiteral("show_preview")), menu) |
            addActionToMenu(ac->action(QStringLiteral("show_in_groups")), menu) |
            addActionToMenu(ac->action(QStringLiteral("show_hidden_files")), menu) |
            addActionToMenu(ac->action(QStringLiteral("additional_info")), menu) |
            addActionToMenu(ac->action(QStringLiteral("view_properties")), menu);

    if (added) {
        menu->addSeparator();
    }

    // Add a curated assortment of items from the "Tools" menu
    addActionToMenu(ac->action(QStringLiteral("show_filter_bar")), menu);
    addActionToMenu(ac->action(QStringLiteral("open_preferred_search_tool")), menu);
    addActionToMenu(ac->action(QStringLiteral("open_terminal")), menu);

    menu->addSeparator();

    // Add "Show Panels" menu
    addActionToMenu(ac->action(QStringLiteral("panels")), menu);

    // Add "Settings" menu entries
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::KeyBindings)), menu);
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ConfigureToolbars)), menu);
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Preferences)), menu);
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ShowMenubar)), menu);

    // Add "Help" menu
    auto helpMenu = m_helpMenu->menu();
    helpMenu->setIcon(QIcon::fromTheme(QStringLiteral("system-help")));
    menu->addMenu(helpMenu);
}

void DolphinMainWindow::updateToolBar()
{
    if (!menuBar()->isVisible()) {
        createControlButton();
    }
}

void DolphinMainWindow::slotControlButtonDeleted()
{
    m_controlButton = nullptr;
    m_updateToolBarTimer->start();
}

void DolphinMainWindow::slotPlaceActivated(const QUrl& url)
{
    DolphinViewContainer* view = activeViewContainer();

    if (view->url() == url) {
        // We can end up here if the user clicked a device in the Places Panel
        // which had been unmounted earlier, see https://bugs.kde.org/show_bug.cgi?id=161385.
        reloadView();
    } else {
        changeUrl(url);
    }
}

void DolphinMainWindow::closedTabsCountChanged(unsigned int count)
{
    actionCollection()->action(QStringLiteral("undo_close_tab"))->setEnabled(count > 0);
}

void DolphinMainWindow::activeViewChanged(DolphinViewContainer* viewContainer)
{
    DolphinViewContainer* oldViewContainer = m_activeViewContainer;
    Q_ASSERT(viewContainer);

    m_activeViewContainer = viewContainer;

    if (oldViewContainer) {
        const QAction* toggleSearchAction = actionCollection()->action(QStringLiteral("toggle_search"));
        toggleSearchAction->disconnect(oldViewContainer);

        // Disconnect all signals between the old view container (container,
        // view and url navigator) and main window.
        oldViewContainer->disconnect(this);
        oldViewContainer->view()->disconnect(this);
        auto navigators = static_cast<DolphinNavigatorsWidgetAction *>
                          (actionCollection()->action(QStringLiteral("url_navigators")));
        navigators->primaryUrlNavigator()->disconnect(this);
        if (auto secondaryUrlNavigator = navigators->secondaryUrlNavigator()) {
            secondaryUrlNavigator->disconnect(this);
        }

        // except the requestItemInfo so that on hover the information panel can still be updated
        connect(oldViewContainer->view(), &DolphinView::requestItemInfo,
                this, &DolphinMainWindow::requestItemInfo);
    }

    connectViewSignals(viewContainer);

    m_actionHandler->setCurrentView(viewContainer->view());

    updateHistory();
    updateFileAndEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();
    updateSearchAction();

    const QUrl url = viewContainer->url();
    Q_EMIT urlChanged(url);
}

void DolphinMainWindow::tabCountChanged(int count)
{
    const bool enableTabActions = (count > 1);
    for (int i = 0; i < MaxActivateTabShortcuts; ++i) {
        actionCollection()->action(QStringLiteral("activate_tab_%1").arg(i))->setEnabled(enableTabActions);
    }
    actionCollection()->action(QStringLiteral("activate_last_tab"))->setEnabled(enableTabActions);
    actionCollection()->action(QStringLiteral("activate_next_tab"))->setEnabled(enableTabActions);
    actionCollection()->action(QStringLiteral("activate_prev_tab"))->setEnabled(enableTabActions);
}

void DolphinMainWindow::updateWindowTitle()
{
    const QString newTitle = m_activeViewContainer->captionWindowTitle();
    if (windowTitle() != newTitle) {
        setWindowTitle(newTitle);
    }
}

void DolphinMainWindow::slotStorageTearDownFromPlacesRequested(const QString& mountPath)
{
    connect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, this, [this, mountPath]() {
        setViewsToHomeIfMountPathOpen(mountPath);
    });

    if (m_terminalPanel && m_terminalPanel->currentWorkingDirectory().startsWith(mountPath)) {
        m_tearDownFromPlacesRequested = true;
        m_terminalPanel->goHome();
        // m_placesPanel->proceedWithTearDown() will be called in slotTerminalDirectoryChanged
    } else {
        m_placesPanel->proceedWithTearDown();
    }
}

void DolphinMainWindow::slotStorageTearDownExternallyRequested(const QString& mountPath)
{
    connect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, this, [this, mountPath]() {
        setViewsToHomeIfMountPathOpen(mountPath);
    });

    if (m_terminalPanel && m_terminalPanel->currentWorkingDirectory().startsWith(mountPath)) {
        m_tearDownFromPlacesRequested = false;
        m_terminalPanel->goHome();
    }
}

void DolphinMainWindow::setViewsToHomeIfMountPathOpen(const QString& mountPath)
{
    const QVector<DolphinViewContainer*> theViewContainers = viewContainers();
    for (DolphinViewContainer *viewContainer : theViewContainers) {
        if (viewContainer && viewContainer->url().toLocalFile().startsWith(mountPath)) {
            viewContainer->setUrl(QUrl::fromLocalFile(QDir::homePath()));
        }
    }
    disconnect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, nullptr, nullptr);
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newFileMenu = new DolphinNewFileMenu(actionCollection(), this);
    QMenu* menu = m_newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    m_newFileMenu->setDelayed(false);
    connect(menu, &QMenu::aboutToShow,
            this, &DolphinMainWindow::updateNewMenu);

    QAction* newWindow = KStandardAction::openNew(this, &DolphinMainWindow::openNewMainWindow, actionCollection());
    newWindow->setText(i18nc("@action:inmenu File", "New &Window"));
    newWindow->setToolTip(i18nc("@info", "Open a new Dolphin window"));
    newWindow->setWhatsThis(xi18nc("@info:whatsthis", "This opens a new "
        "window just like this one with the current location and view."
        "<nl/>You can drag and drop items between windows."));
    newWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));

    QAction* newTab = actionCollection()->addAction(QStringLiteral("new_tab"));
    newTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    newTab->setText(i18nc("@action:inmenu File", "New Tab"));
    newTab->setWhatsThis(xi18nc("@info:whatsthis", "This opens a new "
        "<emphasis>Tab</emphasis> with the current location and view.<nl/>"
        "A tab is an additional view within this window. "
        "You can drag and drop items between tabs."));
    actionCollection()->setDefaultShortcuts(newTab, {Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::SHIFT | Qt::Key_N});
    connect(newTab, &QAction::triggered, this, &DolphinMainWindow::openNewActivatedTab);

    QAction* addToPlaces = actionCollection()->addAction(QStringLiteral("add_to_places"));
    addToPlaces->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-new")));
    addToPlaces->setText(i18nc("@action:inmenu Add current folder to places", "Add to Places"));
    addToPlaces->setWhatsThis(xi18nc("@info:whatsthis", "This adds the selected folder "
        "to the Places panel."));
    connect(addToPlaces, &QAction::triggered, this, &DolphinMainWindow::addToPlaces);

    QAction* closeTab = KStandardAction::close(m_tabWidget, QOverload<>::of(&DolphinTabWidget::closeTab), actionCollection());
    closeTab->setText(i18nc("@action:inmenu File", "Close Tab"));
    closeTab->setWhatsThis(i18nc("@info:whatsthis", "This closes the "
        "currently viewed tab. If no more tabs are left this window "
        "will close instead."));

    QAction* quitAction = KStandardAction::quit(this, &DolphinMainWindow::quit, actionCollection());
    quitAction->setWhatsThis(i18nc("@info:whatsthis quit", "This closes this window."));

    // setup 'Edit' menu
    KStandardAction::undo(this,
                          &DolphinMainWindow::undo,
                          actionCollection());

    // i18n: This will be the last paragraph for the whatsthis for all three:
    // Cut, Copy and Paste
    const QString cutCopyPastePara = xi18nc("@info:whatsthis", "<para><emphasis>Cut, "
        "Copy</emphasis> and <emphasis>Paste</emphasis> work between many "
        "applications and are among the most used commands. That's why their "
        "<emphasis>keyboard shortcuts</emphasis> are prominently placed right "
        "next to each other on the keyboard: <shortcut>Ctrl+X</shortcut>, "
        "<shortcut>Ctrl+C</shortcut> and <shortcut>Ctrl+V</shortcut>.</para>");
    QAction* cutAction = KStandardAction::cut(this, &DolphinMainWindow::cut, actionCollection());
    cutAction->setWhatsThis(xi18nc("@info:whatsthis cut", "This copies the items "
        "in your current selection to the <emphasis>clipboard</emphasis>.<nl/>"
        "Use the <emphasis>Paste</emphasis> action afterwards to copy them from "
        "the clipboard to a new location. The items will be removed from their "
        "initial location.") + cutCopyPastePara);
    QAction* copyAction = KStandardAction::copy(this, &DolphinMainWindow::copy, actionCollection());
    copyAction->setWhatsThis(xi18nc("@info:whatsthis copy", "This copies the "
        "items in your current selection to the <emphasis>clipboard</emphasis>."
        "<nl/>Use the <emphasis>Paste</emphasis> action afterwards to copy them "
        "from the clipboard to a new location.") +  cutCopyPastePara);
    QAction* paste = KStandardAction::paste(this, &DolphinMainWindow::paste, actionCollection());
    // The text of the paste-action is modified dynamically by Dolphin
    // (e. g. to "Paste One Folder"). To prevent that the size of the toolbar changes
    // due to the long text, the text "Paste" is used:
    paste->setIconText(i18nc("@action:inmenu Edit", "Paste"));
    paste->setWhatsThis(xi18nc("@info:whatsthis paste", "This copies the items from "
        "your <emphasis>clipboard</emphasis> to the currently viewed folder.<nl/>"
        "If the items were added to the clipboard by the <emphasis>Cut</emphasis> "
        "action they are removed from their old location.") +  cutCopyPastePara);

    QAction* copyToOtherViewAction = actionCollection()->addAction(QStringLiteral("copy_to_inactive_split_view"));
    copyToOtherViewAction->setText(i18nc("@action:inmenu", "Copy to Inactive Split View"));
    copyToOtherViewAction->setWhatsThis(xi18nc("@info:whatsthis Copy", "This copies the selected items from "
        "the <emphasis>active</emphasis> view to the inactive split view."));
    copyToOtherViewAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    copyToOtherViewAction->setIconText(i18nc("@action:inmenu Edit", "Copy to Inactive Split View"));
    actionCollection()->setDefaultShortcut(copyToOtherViewAction, Qt::SHIFT | Qt::Key_F5 );
    connect(copyToOtherViewAction, &QAction::triggered, m_tabWidget, &DolphinTabWidget::copyToInactiveSplitView);

    QAction* moveToOtherViewAction = actionCollection()->addAction(QStringLiteral("move_to_inactive_split_view"));
    moveToOtherViewAction->setText(i18nc("@action:inmenu", "Move to Inactive Split View"));
    moveToOtherViewAction->setWhatsThis(xi18nc("@info:whatsthis Move", "This moves the selected items from "
        "the <emphasis>active</emphasis> view to the inactive split view."));
    moveToOtherViewAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-cut")));
    moveToOtherViewAction->setIconText(i18nc("@action:inmenu Edit", "Move to Inactive Split View"));
    actionCollection()->setDefaultShortcut(moveToOtherViewAction, Qt::SHIFT | Qt::Key_F6 );
    connect(moveToOtherViewAction, &QAction::triggered, m_tabWidget, &DolphinTabWidget::moveToInactiveSplitView);

    QAction* showFilterBar = actionCollection()->addAction(QStringLiteral("show_filter_bar"));
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Filter..."));
    showFilterBar->setToolTip(i18nc("@info:tooltip", "Toggle Filter Bar"));
    showFilterBar->setWhatsThis(xi18nc("@info:whatsthis", "This opens the "
        "<emphasis>Filter Bar</emphasis> at the bottom of the window.<nl/> "
        "There you can enter a text to filter the files and folders currently displayed. "
        "Only those that contain the text in their name will be kept in view."));
    showFilterBar->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    actionCollection()->setDefaultShortcuts(showFilterBar, {Qt::CTRL | Qt::Key_I, Qt::Key_Slash});
    connect(showFilterBar, &QAction::triggered, this, &DolphinMainWindow::showFilterBar);

    // toggle_filter acts as a copy of the main showFilterBar to be used mainly
    // in the toolbar, with no default shortcut attached, to avoid messing with
    // existing workflows (filter bar always open and Ctrl-I to focus)
    QAction *toggleFilter = actionCollection()->addAction(QStringLiteral("toggle_filter"));
    toggleFilter->setText(i18nc("@action:inmenu", "Toggle Filter Bar"));
    toggleFilter->setIconText(i18nc("@action:intoolbar", "Filter"));
    toggleFilter->setIcon(showFilterBar->icon());
    toggleFilter->setToolTip(showFilterBar->toolTip());
    toggleFilter->setWhatsThis(showFilterBar->whatsThis());
    toggleFilter->setCheckable(true);
    connect(toggleFilter, &QAction::triggered, this, &DolphinMainWindow::toggleFilterBar);

    QAction *searchAction = KStandardAction::find(this, &DolphinMainWindow::find, actionCollection());
    searchAction->setText(i18n("Search..."));
    searchAction->setToolTip(i18nc("@info:tooltip", "Search for files and folders"));
    searchAction->setWhatsThis(xi18nc("@info:whatsthis find", "<para>This helps you "
        "find files and folders by opening a <emphasis>find bar</emphasis>. "
        "There you can enter search terms and specify settings to find the "
        "objects you are looking for.</para><para>Use this help again on "
        "the find bar so we can have a look at it while the settings are "
        "explained.</para>"));

    // toggle_search acts as a copy of the main searchAction to be used mainly
    // in the toolbar, with no default shortcut attached, to avoid messing with
    // existing workflows (search bar always open and Ctrl-F to focus)
    QAction *toggleSearchAction = actionCollection()->addAction(QStringLiteral("toggle_search"));
    toggleSearchAction->setText(i18nc("@action:inmenu", "Toggle Search Bar"));
    toggleSearchAction->setIconText(i18nc("@action:intoolbar", "Search"));
    toggleSearchAction->setIcon(searchAction->icon());
    toggleSearchAction->setToolTip(searchAction->toolTip());
    toggleSearchAction->setWhatsThis(searchAction->whatsThis());
    toggleSearchAction->setCheckable(true);

    QAction* selectAllAction = KStandardAction::selectAll(this, &DolphinMainWindow::selectAll, actionCollection());
    selectAllAction->setWhatsThis(xi18nc("@info:whatsthis", "This selects all "
        "files and folders in the current location."));

    QAction* invertSelection = actionCollection()->addAction(QStringLiteral("invert_selection"));
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    invertSelection->setWhatsThis(xi18nc("@info:whatsthis invert", "This selects all "
        "objects that you have currently <emphasis>not</emphasis> selected instead."));
    invertSelection->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-invert")));
    actionCollection()->setDefaultShortcut(invertSelection, Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, &QAction::triggered, this, &DolphinMainWindow::invertSelection);

    // setup 'View' menu
    // (note that most of it is set up in DolphinViewActionHandler)

    QAction* split = actionCollection()->addAction(QStringLiteral("split_view"));
    split->setWhatsThis(xi18nc("@info:whatsthis find", "<para>This splits "
        "the folder view below into two autonomous views.</para><para>This "
        "way you can see two locations at once and move items between them "
        "quickly.</para>Click this again afterwards to recombine the views."));
    actionCollection()->setDefaultShortcut(split, Qt::Key_F3);
    connect(split, &QAction::triggered, this, &DolphinMainWindow::toggleSplitView);

    QAction* stashSplit = actionCollection()->addAction(QStringLiteral("split_stash"));
    actionCollection()->setDefaultShortcut(stashSplit, Qt::CTRL | Qt::Key_S);
    stashSplit->setText(i18nc("@action:intoolbar Stash", "Stash"));
    stashSplit->setToolTip(i18nc("@info", "Opens the stash virtual directory in a split window"));
    stashSplit->setIcon(QIcon::fromTheme(QStringLiteral("folder-stash")));
    stashSplit->setCheckable(false);
    stashSplit->setVisible(QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.kio.StashNotifier")));
    connect(stashSplit, &QAction::triggered, this, &DolphinMainWindow::toggleSplitStash);

    KStandardAction::redisplay(this, &DolphinMainWindow::reloadView, actionCollection());

    QAction* stop = actionCollection()->addAction(QStringLiteral("stop"));
    stop->setText(i18nc("@action:inmenu View", "Stop"));
    stop->setToolTip(i18nc("@info", "Stop loading"));
    stop->setWhatsThis(i18nc("@info", "This stops the loading of the contents of the current folder."));
    stop->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    connect(stop, &QAction::triggered, this, &DolphinMainWindow::stopLoading);

    KToggleAction* editableLocation = actionCollection()->add<KToggleAction>(QStringLiteral("editable_location"));
    editableLocation->setText(i18nc("@action:inmenu Navigation Bar", "Editable Location"));
    editableLocation->setWhatsThis(xi18nc("@info:whatsthis",
        "This toggles the <emphasis>Location Bar</emphasis> to be "
        "editable so you can directly enter a location you want to go to.<nl/>"
        "You can also switch to editing by clicking to the right of the "
        "location and switch back by confirming the edited location."));
    actionCollection()->setDefaultShortcut(editableLocation, Qt::Key_F6);
    connect(editableLocation, &KToggleAction::triggered, this, &DolphinMainWindow::toggleEditLocation);

    QAction* replaceLocation = actionCollection()->addAction(QStringLiteral("replace_location"));
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    // i18n: "enter" is used both in the meaning of "writing" and "going to" a new location here.
    // Both meanings are useful but not necessary to understand the use of "Replace Location".
    // So you might want to be more verbose in your language to convey the meaning but it's up to you.
    replaceLocation->setWhatsThis(xi18nc("@info:whatsthis",
        "This switches to editing the location and selects it "
        "so you can quickly enter a different location."));
    actionCollection()->setDefaultShortcut(replaceLocation, Qt::CTRL | Qt::Key_L);
    connect(replaceLocation, &QAction::triggered, this, &DolphinMainWindow::replaceLocation);

    // setup 'Go' menu
    {
        QScopedPointer<QAction> backAction(KStandardAction::back(nullptr, nullptr, nullptr));
        m_backAction = new KToolBarPopupAction(backAction->icon(), backAction->text(), actionCollection());
        m_backAction->setObjectName(backAction->objectName());
        m_backAction->setShortcuts(backAction->shortcuts());
    }
    m_backAction->setDelayed(true);
    m_backAction->setStickyMenu(false);
    connect(m_backAction, &QAction::triggered, this, &DolphinMainWindow::goBack);
    connect(m_backAction->menu(), &QMenu::aboutToShow, this, &DolphinMainWindow::slotAboutToShowBackPopupMenu);
    connect(m_backAction->menu(), &QMenu::triggered, this, &DolphinMainWindow::slotGoBack);
    actionCollection()->addAction(m_backAction->objectName(), m_backAction);

    auto backShortcuts = m_backAction->shortcuts();
    backShortcuts.append(QKeySequence(Qt::Key_Backspace));
    actionCollection()->setDefaultShortcuts(m_backAction, backShortcuts);

    DolphinRecentTabsMenu* recentTabsMenu = new DolphinRecentTabsMenu(this);
    actionCollection()->addAction(QStringLiteral("closed_tabs"), recentTabsMenu);
    connect(m_tabWidget, &DolphinTabWidget::rememberClosedTab,
            recentTabsMenu, &DolphinRecentTabsMenu::rememberClosedTab);
    connect(recentTabsMenu, &DolphinRecentTabsMenu::restoreClosedTab,
            m_tabWidget, &DolphinTabWidget::restoreClosedTab);
    connect(recentTabsMenu, &DolphinRecentTabsMenu::closedTabsCountChanged,
            this, &DolphinMainWindow::closedTabsCountChanged);

    QAction* undoCloseTab = actionCollection()->addAction(QStringLiteral("undo_close_tab"));
    undoCloseTab->setText(i18nc("@action:inmenu File", "Undo close tab"));
    undoCloseTab->setWhatsThis(i18nc("@info:whatsthis undo close tab",
        "This returns you to the previously closed tab."));
    actionCollection()->setDefaultShortcut(undoCloseTab, Qt::CTRL | Qt::SHIFT | Qt::Key_T);
    undoCloseTab->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    undoCloseTab->setEnabled(false);
    connect(undoCloseTab, &QAction::triggered, recentTabsMenu, &DolphinRecentTabsMenu::undoCloseTab);

    auto undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    undoAction->setWhatsThis(xi18nc("@info:whatsthis", "This undoes "
        "the last change you made to files or folders.<nl/>"
        "Such changes include <interface>creating, renaming</interface> "
        "and <interface>moving</interface> them to a different location "
        "or to the <filename>Trash</filename>. <nl/>Changes that can't "
        "be undone will ask for your confirmation."));
    undoAction->setEnabled(false); // undo should be disabled by default

    {
        QScopedPointer<QAction> forwardAction(KStandardAction::forward(nullptr, nullptr, nullptr));
        m_forwardAction = new KToolBarPopupAction(forwardAction->icon(), forwardAction->text(), actionCollection());
        m_forwardAction->setObjectName(forwardAction->objectName());
        m_forwardAction->setShortcuts(forwardAction->shortcuts());
    }
    m_forwardAction->setDelayed(true);
    m_forwardAction->setStickyMenu(false);
    connect(m_forwardAction, &QAction::triggered, this, &DolphinMainWindow::goForward);
    connect(m_forwardAction->menu(), &QMenu::aboutToShow, this, &DolphinMainWindow::slotAboutToShowForwardPopupMenu);
    connect(m_forwardAction->menu(), &QMenu::triggered, this, &DolphinMainWindow::slotGoForward);
    actionCollection()->addAction(m_forwardAction->objectName(), m_forwardAction);
    actionCollection()->setDefaultShortcuts(m_forwardAction, m_forwardAction->shortcuts());

    // enable middle-click to open in a new tab
    auto *middleClickEventFilter = new MiddleClickActionEventFilter(this);
    connect(middleClickEventFilter, &MiddleClickActionEventFilter::actionMiddleClicked, this, &DolphinMainWindow::slotBackForwardActionMiddleClicked);
    m_backAction->menu()->installEventFilter(middleClickEventFilter);
    m_forwardAction->menu()->installEventFilter(middleClickEventFilter);
    KStandardAction::up(this, &DolphinMainWindow::goUp, actionCollection());
    QAction* homeAction = KStandardAction::home(this, &DolphinMainWindow::goHome, actionCollection());
    homeAction->setWhatsThis(xi18nc("@info:whatsthis", "Go to your "
        "<filename>Home</filename> folder.<nl/>Every user account "
        "has their own <filename>Home</filename> that contains their data "
        "including folders that contain personal application data."));

    // setup 'Tools' menu
    QAction* compareFiles = actionCollection()->addAction(QStringLiteral("compare_files"));
    compareFiles->setText(i18nc("@action:inmenu Tools", "Compare Files"));
    compareFiles->setIcon(QIcon::fromTheme(QStringLiteral("kompare")));
    compareFiles->setEnabled(false);
    connect(compareFiles, &QAction::triggered, this, &DolphinMainWindow::compareFiles);

    QAction* openPreferredSearchTool = actionCollection()->addAction(QStringLiteral("open_preferred_search_tool"));
    openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open Preferred Search Tool"));
    openPreferredSearchTool->setWhatsThis(xi18nc("@info:whatsthis",
        "<para>This opens a preferred search tool for the viewed location.</para>"
        "<para>Use <emphasis>More Search Tools</emphasis> menu to configure it.</para>"));
    openPreferredSearchTool->setIcon(QIcon::fromTheme(QStringLiteral("search")));
    actionCollection()->setDefaultShortcut(openPreferredSearchTool, Qt::CTRL | Qt::SHIFT | Qt::Key_F);
    connect(openPreferredSearchTool, &QAction::triggered, this, &DolphinMainWindow::openPreferredSearchTool);

    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        QAction* openTerminal = actionCollection()->addAction(QStringLiteral("open_terminal"));
        openTerminal->setText(i18nc("@action:inmenu Tools", "Open Terminal"));
        openTerminal->setWhatsThis(xi18nc("@info:whatsthis",
            "<para>This opens a <emphasis>terminal</emphasis> application for the viewed location.</para>"
            "<para>To learn more about terminals use the help in the terminal application.</para>"));
        openTerminal->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
        actionCollection()->setDefaultShortcut(openTerminal, Qt::SHIFT | Qt::Key_F4);
        connect(openTerminal, &QAction::triggered, this, &DolphinMainWindow::openTerminal);

#ifdef HAVE_TERMINAL
        QAction* focusTerminalPanel = actionCollection()->addAction(QStringLiteral("focus_terminal_panel"));
        focusTerminalPanel->setText(i18nc("@action:inmenu Tools", "Focus Terminal Panel"));
        focusTerminalPanel->setIcon(QIcon::fromTheme(QStringLiteral("swap-panels")));
        actionCollection()->setDefaultShortcut(focusTerminalPanel, Qt::CTRL | Qt::SHIFT | Qt::Key_F4);
        connect(focusTerminalPanel, &QAction::triggered, this, &DolphinMainWindow::focusTerminalPanel);
#endif
    }

    // setup 'Bookmarks' menu
    KActionMenu *bookmarkMenu = new KActionMenu(i18nc("@title:menu", "&Bookmarks"), this);
    bookmarkMenu->setIcon(QIcon::fromTheme(QStringLiteral("bookmarks")));
    // Make the toolbar button version work properly on click
    bookmarkMenu->setDelayed(false);
    m_bookmarkHandler = new DolphinBookmarkHandler(this, actionCollection(), bookmarkMenu->menu(), this);
    actionCollection()->addAction(QStringLiteral("bookmarks"), bookmarkMenu);

    // setup 'Settings' menu
    KToggleAction* showMenuBar = KStandardAction::showMenubar(nullptr, nullptr, actionCollection());
    showMenuBar->setWhatsThis(xi18nc("@info:whatsthis",
            "This switches between having a <emphasis>Menubar</emphasis> "
            "and having a <interface>Control</interface> button. Both "
            "contain mostly the same commands and configuration options."));
    connect(showMenuBar, &KToggleAction::triggered,                   // Fixes #286822
            this, &DolphinMainWindow::toggleShowMenuBar, Qt::QueuedConnection);
    KStandardAction::preferences(this, &DolphinMainWindow::editSettings, actionCollection());

    // setup 'Help' menu for the m_controlButton. The other one is set up in the base class.
    m_helpMenu = new KHelpMenu(nullptr);
    m_helpMenu->menu()->installEventFilter(this);
    // remove duplicate shortcuts
    m_helpMenu->action(KHelpMenu::menuHelpContents)->setShortcut(QKeySequence());
    m_helpMenu->action(KHelpMenu::menuWhatsThis)->setShortcut(QKeySequence());

    // not in menu actions
    QList<QKeySequence> nextTabKeys = KStandardShortcut::tabNext();
    nextTabKeys.append(QKeySequence(Qt::CTRL | Qt::Key_Tab));

    QList<QKeySequence> prevTabKeys = KStandardShortcut::tabPrev();
    prevTabKeys.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));

    for (int i = 0; i < MaxActivateTabShortcuts; ++i) {
        QAction* activateTab = actionCollection()->addAction(QStringLiteral("activate_tab_%1").arg(i));
        activateTab->setText(i18nc("@action:inmenu", "Activate Tab %1", i + 1));
        activateTab->setEnabled(false);
        connect(activateTab, &QAction::triggered, this, [this, i]() { m_tabWidget->activateTab(i); });

        // only add default shortcuts for the first 9 tabs regardless of MaxActivateTabShortcuts
        if (i < 9) {
            actionCollection()->setDefaultShortcut(activateTab, QStringLiteral("Alt+%1").arg(i + 1));
        }
    }

    QAction* activateLastTab = actionCollection()->addAction(QStringLiteral("activate_last_tab"));
    activateLastTab->setText(i18nc("@action:inmenu", "Activate Last Tab"));
    activateLastTab->setEnabled(false);
    connect(activateLastTab, &QAction::triggered, m_tabWidget, &DolphinTabWidget::activateLastTab);
    actionCollection()->setDefaultShortcut(activateLastTab, Qt::ALT | Qt::Key_0);

    QAction* activateNextTab = actionCollection()->addAction(QStringLiteral("activate_next_tab"));
    activateNextTab->setIconText(i18nc("@action:inmenu", "Next Tab"));
    activateNextTab->setText(i18nc("@action:inmenu", "Activate Next Tab"));
    activateNextTab->setEnabled(false);
    connect(activateNextTab, &QAction::triggered, m_tabWidget, &DolphinTabWidget::activateNextTab);
    actionCollection()->setDefaultShortcuts(activateNextTab, nextTabKeys);

    QAction* activatePrevTab = actionCollection()->addAction(QStringLiteral("activate_prev_tab"));
    activatePrevTab->setIconText(i18nc("@action:inmenu", "Previous Tab"));
    activatePrevTab->setText(i18nc("@action:inmenu", "Activate Previous Tab"));
    activatePrevTab->setEnabled(false);
    connect(activatePrevTab, &QAction::triggered, m_tabWidget, &DolphinTabWidget::activatePrevTab);
    actionCollection()->setDefaultShortcuts(activatePrevTab, prevTabKeys);

    // for context menu
    QAction* showTarget = actionCollection()->addAction(QStringLiteral("show_target"));
    showTarget->setText(i18nc("@action:inmenu", "Show Target"));
    showTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    showTarget->setEnabled(false);
    connect(showTarget, &QAction::triggered, this, &DolphinMainWindow::showTarget);

    QAction* openInNewTab = actionCollection()->addAction(QStringLiteral("open_in_new_tab"));
    openInNewTab->setText(i18nc("@action:inmenu", "Open in New Tab"));
    openInNewTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(openInNewTab, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction* openInNewTabs = actionCollection()->addAction(QStringLiteral("open_in_new_tabs"));
    openInNewTabs->setText(i18nc("@action:inmenu", "Open in New Tabs"));
    openInNewTabs->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(openInNewTabs, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction* openInNewWindow = actionCollection()->addAction(QStringLiteral("open_in_new_window"));
    openInNewWindow->setText(i18nc("@action:inmenu", "Open in New Window"));
    openInNewWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    connect(openInNewWindow, &QAction::triggered, this, &DolphinMainWindow::openInNewWindow);
}

void DolphinMainWindow::setupDockWidgets()
{
    const bool lock = GeneralSettings::lockPanels();

    KDualAction* lockLayoutAction = actionCollection()->add<KDualAction>(QStringLiteral("lock_panels"));
    lockLayoutAction->setActiveText(i18nc("@action:inmenu Panels", "Unlock Panels"));
    lockLayoutAction->setActiveIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
    lockLayoutAction->setInactiveText(i18nc("@action:inmenu Panels", "Lock Panels"));
    lockLayoutAction->setInactiveIcon(QIcon::fromTheme(QStringLiteral("object-locked")));
    lockLayoutAction->setWhatsThis(xi18nc("@info:whatsthis", "This "
        "switches between having panels <emphasis>locked</emphasis> or "
        "<emphasis>unlocked</emphasis>.<nl/>Unlocked panels can be "
        "dragged to the other side of the window and have a close "
        "button.<nl/>Locked panels are embedded more cleanly."));
    lockLayoutAction->setActive(lock);
    connect(lockLayoutAction, &KDualAction::triggered, this, &DolphinMainWindow::togglePanelLockState);

    // Setup "Information"
    DolphinDockWidget* infoDock = new DolphinDockWidget(i18nc("@title:window", "Information"));
    infoDock->setLocked(lock);
    infoDock->setObjectName(QStringLiteral("infoDock"));
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

#ifdef HAVE_BALOO
    InformationPanel* infoPanel = new InformationPanel(infoDock);
    infoPanel->setCustomContextMenuActions({lockLayoutAction});
    connect(infoPanel, &InformationPanel::urlActivated, this, &DolphinMainWindow::handleUrl);
    infoDock->setWidget(infoPanel);

    QAction* infoAction = infoDock->toggleViewAction();
    createPanelAction(QIcon::fromTheme(QStringLiteral("dialog-information")), Qt::Key_F11, infoAction, QStringLiteral("show_information_panel"));

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, &DolphinMainWindow::urlChanged,
            infoPanel, &InformationPanel::setUrl);
    connect(this, &DolphinMainWindow::selectionChanged,
            infoPanel, &InformationPanel::setSelection);
    connect(this, &DolphinMainWindow::requestItemInfo,
            infoPanel, &InformationPanel::requestDelayedItemInfo);
#endif

    // i18n: This is the last paragraph for the "What's This"-texts of all four panels.
    const QString panelWhatsThis = xi18nc("@info:whatsthis", "<para>To show or "
        "hide panels like this go to <interface>Control|Panels</interface> "
        "or <interface>View|Panels</interface>.</para>");
#ifdef HAVE_BALOO
    actionCollection()->action(QStringLiteral("show_information_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis", "<para> This toggles the "
        "<emphasis>information</emphasis> panel at the right side of the "
        "window.</para><para>The panel provides in-depth information "
        "about the items your mouse is hovering over or about the selected "
        "items. Otherwise it informs you about the currently viewed folder.<nl/>"
        "For single items a preview of their contents is provided.</para>"));
#endif
    infoDock->setWhatsThis(xi18nc("@info:whatsthis", "<para>This panel "
        "provides in-depth information about the items your mouse is "
        "hovering over or about the selected items. Otherwise it informs "
        "you about the currently viewed folder.<nl/>For single items a "
        "preview of their contents is provided.</para><para>You can configure "
        "which and how details are given here by right-clicking.</para>") + panelWhatsThis);

    // Setup "Folders"
    DolphinDockWidget* foldersDock = new DolphinDockWidget(i18nc("@title:window", "Folders"));
    foldersDock->setLocked(lock);
    foldersDock->setObjectName(QStringLiteral("foldersDock"));
    foldersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    FoldersPanel* foldersPanel = new FoldersPanel(foldersDock);
    foldersPanel->setCustomContextMenuActions({lockLayoutAction});
    foldersDock->setWidget(foldersPanel);

    QAction* foldersAction = foldersDock->toggleViewAction();
    createPanelAction(QIcon::fromTheme(QStringLiteral("folder")), Qt::Key_F7, foldersAction, QStringLiteral("show_folders_panel"));

    addDockWidget(Qt::LeftDockWidgetArea, foldersDock);
    connect(this, &DolphinMainWindow::urlChanged,
            foldersPanel, &FoldersPanel::setUrl);
    connect(foldersPanel, &FoldersPanel::folderActivated,
            this, &DolphinMainWindow::changeUrl);
    connect(foldersPanel, &FoldersPanel::folderMiddleClicked,
            this, &DolphinMainWindow::openNewTab);
    connect(foldersPanel, &FoldersPanel::errorMessage,
            this, &DolphinMainWindow::showErrorMessage);

    actionCollection()->action(QStringLiteral("show_folders_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis", "This toggles the "
        "<emphasis>folders</emphasis> panel at the left side of the window."
        "<nl/><nl/>It shows the folders of the <emphasis>file system"
        "</emphasis> in a <emphasis>tree view</emphasis>."));
    foldersDock->setWhatsThis(xi18nc("@info:whatsthis", "<para>This panel "
        "shows the folders of the <emphasis>file system</emphasis> in a "
        "<emphasis>tree view</emphasis>.</para><para>Click a folder to go "
        "there. Click the arrow to the left of a folder to see its subfolders. "
        "This allows quick switching between any folders.</para>") + panelWhatsThis);

    // Setup "Terminal"
#ifdef HAVE_TERMINAL
    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        DolphinDockWidget* terminalDock = new DolphinDockWidget(i18nc("@title:window Shell terminal", "Terminal"));
        terminalDock->setLocked(lock);
        terminalDock->setObjectName(QStringLiteral("terminalDock"));
        m_terminalPanel = new TerminalPanel(terminalDock);
        m_terminalPanel->setCustomContextMenuActions({lockLayoutAction});
        terminalDock->setWidget(m_terminalPanel);

        connect(m_terminalPanel, &TerminalPanel::hideTerminalPanel, terminalDock, &DolphinDockWidget::hide);
        connect(m_terminalPanel, &TerminalPanel::changeUrl, this, &DolphinMainWindow::slotTerminalDirectoryChanged);
        connect(terminalDock, &DolphinDockWidget::visibilityChanged,
                m_terminalPanel, &TerminalPanel::dockVisibilityChanged);
        connect(terminalDock, &DolphinDockWidget::visibilityChanged,
                this, &DolphinMainWindow::slotTerminalPanelVisibilityChanged);

        QAction* terminalAction = terminalDock->toggleViewAction();
        createPanelAction(QIcon::fromTheme(QStringLiteral("dialog-scripts")), Qt::Key_F4, terminalAction, QStringLiteral("show_terminal_panel"));

        addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
        connect(this, &DolphinMainWindow::urlChanged,
                m_terminalPanel, &TerminalPanel::setUrl);

        if (GeneralSettings::version() < 200) {
            terminalDock->hide();
        }

        actionCollection()->action(QStringLiteral("show_terminal_panel"))
            ->setWhatsThis(xi18nc("@info:whatsthis", "<para>This toggles the "
            "<emphasis>terminal</emphasis> panel at the bottom of the window."
            "<nl/>The location in the terminal will always match the folder "
            "view so you can navigate using either.</para><para>The terminal "
            "panel is not needed for basic computer usage but can be useful "
            "for advanced tasks. To learn more about terminals use the help "
            "in a standalone terminal application like Konsole.</para>"));
        terminalDock->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is "
            "the <emphasis>terminal</emphasis> panel. It behaves like a "
            "normal terminal but will match the location of the folder view "
            "so you can navigate using either.</para><para>The terminal panel "
            "is not needed for basic computer usage but can be useful for "
            "advanced tasks. To learn more about terminals use the help in a "
            "standalone terminal application like Konsole.</para>") + panelWhatsThis);
    }
#endif

    if (GeneralSettings::version() < 200) {
        infoDock->hide();
        foldersDock->hide();
    }

    // Setup "Places"
    DolphinDockWidget* placesDock = new DolphinDockWidget(i18nc("@title:window", "Places"));
    placesDock->setLocked(lock);
    placesDock->setObjectName(QStringLiteral("placesDock"));
    placesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_placesPanel = new PlacesPanel(placesDock);
    m_placesPanel->setCustomContextMenuActions({lockLayoutAction});
    placesDock->setWidget(m_placesPanel);

    QAction *placesAction = placesDock->toggleViewAction();
    createPanelAction(QIcon::fromTheme(QStringLiteral("compass")), Qt::Key_F9, placesAction, QStringLiteral("show_places_panel"));

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(m_placesPanel, &PlacesPanel::placeActivated,
            this, &DolphinMainWindow::slotPlaceActivated);
    connect(m_placesPanel, &PlacesPanel::placeMiddleClicked,
            this, &DolphinMainWindow::openNewTab);
    connect(m_placesPanel, &PlacesPanel::errorMessage,
            this, &DolphinMainWindow::showErrorMessage);
    connect(this, &DolphinMainWindow::urlChanged,
            m_placesPanel, &PlacesPanel::setUrl);
    connect(placesDock, &DolphinDockWidget::visibilityChanged,
            &DolphinUrlNavigatorsController::slotPlacesPanelVisibilityChanged);
    connect(this, &DolphinMainWindow::settingsChanged,
        m_placesPanel, &PlacesPanel::readSettings);
    connect(m_placesPanel, &PlacesPanel::storageTearDownRequested,
            this, &DolphinMainWindow::slotStorageTearDownFromPlacesRequested);
    connect(m_placesPanel, &PlacesPanel::storageTearDownExternallyRequested,
            this, &DolphinMainWindow::slotStorageTearDownExternallyRequested);
    DolphinUrlNavigatorsController::slotPlacesPanelVisibilityChanged(m_placesPanel->isVisible());

    auto actionShowAllPlaces = new QAction(QIcon::fromTheme(QStringLiteral("view-hidden")), i18nc("@item:inmenu", "Show Hidden Places"), this);
    actionShowAllPlaces->setCheckable(true);
    actionShowAllPlaces->setDisabled(true);
    actionShowAllPlaces->setWhatsThis(i18nc("@info:whatsthis", "This displays "
        "all places in the places panel that have been hidden. They will "
        "appear semi-transparent unless you uncheck their hide property."));

    connect(actionShowAllPlaces, &QAction::triggered, this, [actionShowAllPlaces, this](bool checked){
        actionShowAllPlaces->setIcon(QIcon::fromTheme(checked ? QStringLiteral("view-visible") : QStringLiteral("view-hidden")));
        m_placesPanel->showHiddenEntries(checked);
    });

    connect(m_placesPanel, &PlacesPanel::showHiddenEntriesChanged, this, [actionShowAllPlaces] (bool checked){
        actionShowAllPlaces->setChecked(checked);
        actionShowAllPlaces->setIcon(QIcon::fromTheme(checked ? QStringLiteral("view-visible") : QStringLiteral("view-hidden")));
   });

    actionCollection()->action(QStringLiteral("show_places_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis", "<para>This toggles the "
        "<emphasis>places</emphasis> panel at the left side of the window."
        "</para><para>It allows you to go to locations you have "
        "bookmarked and to access disk or media attached to the computer "
        "or to the network. It also contains sections to find recently "
        "saved files or files of a certain type.</para>"));
    placesDock->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the "
        "<emphasis>Places</emphasis> panel. It allows you to go to locations "
        "you have bookmarked and to access disk or media attached to the "
        "computer or to the network. It also contains sections to find "
        "recently saved files or files of a certain type.</para><para>"
        "Click on an entry to go there. Click with the right mouse button "
        "instead to open any entry in a new tab or new window.</para>"
        "<para>New entries can be added by dragging folders onto this panel. "
        "Right-click any section or entry to hide it. Right-click an empty "
        "space on this panel and select <interface>Show Hidden Places"
        "</interface> to display it again.</para>") + panelWhatsThis);

    // Add actions into the "Panels" menu
    KActionMenu* panelsMenu = new KActionMenu(i18nc("@action:inmenu View", "Show Panels"), this);
    actionCollection()->addAction(QStringLiteral("panels"), panelsMenu);
    panelsMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-sidetree")));
    panelsMenu->setDelayed(false);
    const KActionCollection* ac = actionCollection();
    panelsMenu->addAction(ac->action(QStringLiteral("show_places_panel")));
#ifdef HAVE_BALOO
    panelsMenu->addAction(ac->action(QStringLiteral("show_information_panel")));
#endif
    panelsMenu->addAction(ac->action(QStringLiteral("show_folders_panel")));
    panelsMenu->addAction(ac->action(QStringLiteral("show_terminal_panel")));
    panelsMenu->addSeparator();
    panelsMenu->addAction(actionShowAllPlaces);
    panelsMenu->addAction(lockLayoutAction);

    connect(panelsMenu->menu(), &QMenu::aboutToShow, this, [actionShowAllPlaces, this]{
        actionShowAllPlaces->setEnabled(m_placesPanel->hiddenListCount());
    });
}


void DolphinMainWindow::updateFileAndEditActions()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    const KActionCollection* col = actionCollection();
    KFileItemListProperties capabilitiesSource(list);

    QAction* addToPlacesAction = col->action(QStringLiteral("add_to_places"));
    QAction* copyToOtherViewAction   = col->action(QStringLiteral("copy_to_inactive_split_view"));
    QAction* moveToOtherViewAction   = col->action(QStringLiteral("move_to_inactive_split_view"));
    QAction* copyLocation            = col->action(QString("copy_location"));

    if (list.isEmpty()) {
        stateChanged(QStringLiteral("has_no_selection"));

        addToPlacesAction->setEnabled(true);
        copyToOtherViewAction->setEnabled(false);
        moveToOtherViewAction->setEnabled(false);
        copyLocation->setEnabled(false);
    } else {
        stateChanged(QStringLiteral("has_selection"));

        QAction* renameAction            = col->action(KStandardAction::name(KStandardAction::RenameFile));
        QAction* moveToTrashAction       = col->action(KStandardAction::name(KStandardAction::MoveToTrash));
        QAction* deleteAction            = col->action(KStandardAction::name(KStandardAction::DeleteFile));
        QAction* cutAction               = col->action(KStandardAction::name(KStandardAction::Cut));
        QAction* deleteWithTrashShortcut = col->action(QStringLiteral("delete_shortcut")); // see DolphinViewActionHandler
        QAction* showTarget              = col->action(QStringLiteral("show_target"));
        QAction* duplicateAction         = col->action(QStringLiteral("duplicate")); // see DolphinViewActionHandler

        if (list.length() == 1 && list.first().isDir()) {
            addToPlacesAction->setEnabled(true);
        } else {
            addToPlacesAction->setEnabled(false);
        }

        if (m_tabWidget->currentTabPage()->splitViewEnabled()) {
            DolphinTabPage* tabPage = m_tabWidget->currentTabPage();
            KFileItem capabilitiesDestination;

            if (tabPage->primaryViewActive()) {
                capabilitiesDestination = tabPage->secondaryViewContainer()->url();
            } else {
                capabilitiesDestination = tabPage->primaryViewContainer()->url();
            }

            copyToOtherViewAction->setEnabled(capabilitiesDestination.isWritable());
            moveToOtherViewAction->setEnabled(capabilitiesSource.supportsMoving() && capabilitiesDestination.isWritable());
        } else {
            copyToOtherViewAction->setEnabled(false);
            moveToOtherViewAction->setEnabled(false);
        }

        const bool enableMoveToTrash = capabilitiesSource.isLocal() && capabilitiesSource.supportsMoving();

        renameAction->setEnabled(capabilitiesSource.supportsMoving());
        moveToTrashAction->setEnabled(enableMoveToTrash);
        deleteAction->setEnabled(capabilitiesSource.supportsDeleting());
        deleteWithTrashShortcut->setEnabled(capabilitiesSource.supportsDeleting() && !enableMoveToTrash);
        cutAction->setEnabled(capabilitiesSource.supportsMoving());
        copyLocation->setEnabled(list.length() == 1);
        showTarget->setEnabled(list.length() == 1 && list.at(0).isLink());
        duplicateAction->setEnabled(capabilitiesSource.supportsWriting());
    }
}

void DolphinMainWindow::updateViewActions()
{
    m_actionHandler->updateViewActions();

    QAction* toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(m_activeViewContainer->isFilterBarVisible());

    updateSplitAction();
}

void DolphinMainWindow::updateGoActions()
{
    QAction* goUpAction = actionCollection()->action(KStandardAction::name(KStandardAction::Up));
    const QUrl currentUrl = m_activeViewContainer->url();
    // I think this is one of the best places to firstly be confronted
    // with a file system and its hierarchy. Talking about the root
    // directory might seem too much here but it is the question that
    // naturally arises in this context.
    goUpAction->setWhatsThis(xi18nc("@info:whatsthis", "<para>Go to "
        "the folder that contains the currently viewed one.</para>"
        "<para>All files and folders are organized in a hierarchical "
        "<emphasis>file system</emphasis>. At the top of this hierarchy is "
        "a directory that contains all data connected to this computer"
        "—the <emphasis>root directory</emphasis>.</para>"));
    goUpAction->setEnabled(KIO::upUrl(currentUrl) != currentUrl);
}

void DolphinMainWindow::createControlButton()
{
    if (m_controlButton) {
        return;
    }
    Q_ASSERT(!m_controlButton);

    m_controlButton = new QToolButton(this);
    m_controlButton->setAccessibleName(i18nc("@action:intoolbar", "Control"));
    m_controlButton->setIcon(QIcon::fromTheme(QStringLiteral("application-menu")));
    m_controlButton->setToolTip(i18nc("@action", "Show menu"));
    m_controlButton->setAttribute(Qt::WidgetAttribute::WA_CustomWhatsThis);
    m_controlButton->setPopupMode(QToolButton::InstantPopup);

    QMenu* controlMenu = new QMenu(m_controlButton);
    connect(controlMenu, &QMenu::aboutToShow, this, &DolphinMainWindow::updateControlMenu);
    controlMenu->installEventFilter(this);

    m_controlButton->setMenu(controlMenu);

    toolBar()->addWidget(m_controlButton);
    connect(toolBar(), &KToolBar::iconSizeChanged,
            m_controlButton, &QToolButton::setIconSize);

    // The added widgets are owned by the toolbar and may get deleted when e.g. the toolbar
    // gets edited. In this case we must add them again. The adding is done asynchronously by
    // m_updateToolBarTimer.
    connect(m_controlButton, &QToolButton::destroyed, this, &DolphinMainWindow::slotControlButtonDeleted);
    m_updateToolBarTimer = new QTimer(this);
    m_updateToolBarTimer->setInterval(500);
    connect(m_updateToolBarTimer, &QTimer::timeout, this, &DolphinMainWindow::updateToolBar);
}

void DolphinMainWindow::deleteControlButton()
{
    delete m_controlButton;
    m_controlButton = nullptr;

    delete m_updateToolBarTimer;
    m_updateToolBarTimer = nullptr;
}

bool DolphinMainWindow::addActionToMenu(QAction* action, QMenu* menu)
{
    Q_ASSERT(action);
    Q_ASSERT(menu);

    const KToolBar* toolBarWidget = toolBar();
    const auto associatedWidgets = action->associatedWidgets();
    for (const QWidget* widget : associatedWidgets) {
        if (widget == toolBarWidget) {
            return false;
        }
    }

    menu->addAction(action);
    return true;
}

void DolphinMainWindow::refreshViews()
{
    m_tabWidget->refreshViews();

    if (GeneralSettings::modifiedStartupSettings()) {
        // The startup settings have been changed by the user (see bug #254947).
        // Synchronize the split-view setting with the active view:
        const bool splitView = GeneralSettings::splitView();
        m_tabWidget->currentTabPage()->setSplitViewEnabled(splitView, WithAnimation);
        updateSplitAction();
        updateWindowTitle();
    }

    Q_EMIT settingsChanged();
}

void DolphinMainWindow::clearStatusBar()
{
    m_activeViewContainer->statusBar()->resetToDefaultText();
}

void DolphinMainWindow::connectViewSignals(DolphinViewContainer* container)
{
    connect(container, &DolphinViewContainer::showFilterBarChanged,
            this, &DolphinMainWindow::updateFilterBarAction);
    connect(container, &DolphinViewContainer::writeStateChanged,
            this, &DolphinMainWindow::slotWriteStateChanged);
    connect(container, &DolphinViewContainer::searchModeEnabledChanged,
            this, &DolphinMainWindow::updateSearchAction);

    const QAction* toggleSearchAction = actionCollection()->action(QStringLiteral("toggle_search"));
    connect(toggleSearchAction, &QAction::triggered, container, &DolphinViewContainer::setSearchModeEnabled);

    const DolphinView* view = container->view();
    connect(view, &DolphinView::selectionChanged,
            this, &DolphinMainWindow::slotSelectionChanged);
    connect(view, &DolphinView::requestItemInfo,
            this, &DolphinMainWindow::requestItemInfo);
    connect(view, &DolphinView::tabRequested,
            this, &DolphinMainWindow::openNewTab);
    connect(view, &DolphinView::requestContextMenu,
            this, &DolphinMainWindow::openContextMenu);
    connect(view, &DolphinView::directoryLoadingStarted,
            this, &DolphinMainWindow::enableStopAction);
    connect(view, &DolphinView::directoryLoadingCompleted,
            this, &DolphinMainWindow::disableStopAction);
    connect(view, &DolphinView::directoryLoadingCompleted,
            this, &DolphinMainWindow::slotDirectoryLoadingCompleted);
    connect(view, &DolphinView::goBackRequested,
            this, &DolphinMainWindow::goBack);
    connect(view, &DolphinView::goForwardRequested,
            this, &DolphinMainWindow::goForward);
    connect(view, &DolphinView::urlActivated,
            this, &DolphinMainWindow::handleUrl);
    connect(view, &DolphinView::goUpRequested,
            this, &DolphinMainWindow::goUp);

    auto navigators = static_cast<DolphinNavigatorsWidgetAction *>
        (actionCollection()->action(QStringLiteral("url_navigators")));

    const KUrlNavigator *navigator = m_tabWidget->currentTabPage()->primaryViewActive() ?
                                     navigators->primaryUrlNavigator() :
                                     navigators->secondaryUrlNavigator();

    connect(navigator, &KUrlNavigator::urlChanged,
            this, &DolphinMainWindow::changeUrl);
    QAction *editableLocactionAction = actionCollection()->action(QStringLiteral("editable_location"));
    editableLocactionAction->setChecked(navigator->isUrlEditable());
    connect(navigator, &KUrlNavigator::editableStateChanged,
            this, &DolphinMainWindow::slotEditableStateChanged);
    connect(navigator, &KUrlNavigator::tabRequested,
            this, &DolphinMainWindow::openNewTab);

    disconnect(m_updateHistoryConnection);
    m_updateHistoryConnection = connect(
            container->urlNavigatorInternalWithHistory(), &KUrlNavigator::historyChanged,
            this, &DolphinMainWindow::updateHistory);
}

void DolphinMainWindow::updateSplitAction()
{
    QAction* splitAction = actionCollection()->action(QStringLiteral("split_view"));
    const DolphinTabPage* tabPage = m_tabWidget->currentTabPage();
    if (tabPage->splitViewEnabled()) {
        if (GeneralSettings::closeActiveSplitView() ? tabPage->primaryViewActive() : !tabPage->primaryViewActive()) {
            splitAction->setText(i18nc("@action:intoolbar Close left view", "Close"));
            splitAction->setToolTip(i18nc("@info", "Close left view"));
            splitAction->setIcon(QIcon::fromTheme(QStringLiteral("view-left-close")));
        } else {
            splitAction->setText(i18nc("@action:intoolbar Close right view", "Close"));
            splitAction->setToolTip(i18nc("@info", "Close right view"));
            splitAction->setIcon(QIcon::fromTheme(QStringLiteral("view-right-close")));
        }
    } else {
        splitAction->setText(i18nc("@action:intoolbar Split view", "Split"));
        splitAction->setToolTip(i18nc("@info", "Split view"));
        splitAction->setIcon(QIcon::fromTheme(QStringLiteral("view-right-new")));
    }
}

void DolphinMainWindow::updateAllowedToolbarAreas()
{
    auto navigators = static_cast<DolphinNavigatorsWidgetAction *>
                        (actionCollection()->action(QStringLiteral("url_navigators")));
    if (toolBar()->actions().contains(navigators)) {
        toolBar()->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
        if (toolBarArea(toolBar()) == Qt::LeftToolBarArea ||
            toolBarArea(toolBar()) == Qt::RightToolBarArea) {
            addToolBar(Qt::TopToolBarArea, toolBar());
        }
    } else {
        toolBar()->setAllowedAreas(Qt::AllToolBarAreas);
    }
}

bool DolphinMainWindow::isKompareInstalled() const
{
    static bool initialized = false;
    static bool installed = false;
    if (!initialized) {
        // TODO: maybe replace this approach later by using a menu
        // plugin like kdiff3plugin.cpp
        installed = !QStandardPaths::findExecutable(QStringLiteral("kompare")).isEmpty();
        initialized = true;
    }
    return installed;
}

void DolphinMainWindow::createPanelAction(const QIcon& icon,
                                          const QKeySequence& shortcut,
                                          QAction* dockAction,
                                          const QString& actionName)
{
    QAction* panelAction = actionCollection()->addAction(actionName);
    panelAction->setCheckable(true);
    panelAction->setChecked(dockAction->isChecked());
    panelAction->setText(dockAction->text());
    panelAction->setIcon(icon);
    actionCollection()->setDefaultShortcut(panelAction, shortcut);

    connect(panelAction, &QAction::triggered, dockAction, &QAction::trigger);
    connect(dockAction, &QAction::toggled, panelAction, &QAction::setChecked);
}

void DolphinMainWindow::setupWhatsThis()
{
    // main widgets
    menuBar()->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the "
        "<emphasis>Menubar</emphasis>. It provides access to commands and "
        "configuration options. Left-click on any of the menus on this "
        "bar to see its contents.</para><para>The Menubar can be hidden "
        "by unchecking <interface>Settings|Show Menubar</interface>. Then "
        "most of its contents become available through a <interface>Control"
        "</interface> button on the <emphasis>Toolbar</emphasis>.</para>"));
    toolBar()->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the "
        "<emphasis>Toolbar</emphasis>. It allows quick access to "
        "frequently used actions.</para><para>It is highly customizable. "
        "All items you see in the <interface>Control</interface> menu or "
        "in the <interface>Menubar</interface> can be placed on the "
        "Toolbar. Just right-click on it and select <interface>Configure "
        "Toolbars…</interface> or find this action in the <interface>"
        "Control</interface> or <interface>Settings</interface> menu."
        "</para><para>The location of the bar and the style of its "
        "buttons can also be changed in the right-click menu. Right-click "
        "a button if you want to show or hide its text.</para>"));
    m_tabWidget->setWhatsThis(xi18nc("@info:whatsthis main view",
        "<para>Here you can see the <emphasis>folders</emphasis> and "
        "<emphasis>files</emphasis> that are at the location described in "
        "the <interface>Location Bar</interface> above. This area is the "
        "central part of this application where you navigate to the files "
        "you want to use.</para><para>For an elaborate and general "
        "introduction to this application <link "
        "url='https://userbase.kde.org/Dolphin/File_Management#Introduction_to_Dolphin'>"
        "click here</link>. This will open an introductory article from "
        "the <emphasis>KDE UserBase Wiki</emphasis>.</para><para>For brief "
        "explanations of all the features of this <emphasis>view</emphasis> "
        "<link url='help:/dolphin/dolphin-view.html'>click here</link> "
        "instead. This will open a page from the <emphasis>Handbook"
        "</emphasis> that covers the basics.</para>"));

    // Settings menu
    actionCollection()->action(KStandardAction::name(KStandardAction::KeyBindings))
        ->setWhatsThis(xi18nc("@info:whatsthis","<para>This opens a window "
        "that lists the <emphasis>keyboard shortcuts</emphasis>.<nl/>"
        "There you can set up key combinations to trigger an action when "
        "they are pressed simultaneously. All commands in this application can "
        "be triggered this way.</para>"));
    actionCollection()->action(KStandardAction::name(KStandardAction::ConfigureToolbars))
        ->setWhatsThis(xi18nc("@info:whatsthis","<para>This opens a window in which "
        "you can change which buttons appear on the <emphasis>Toolbar</emphasis>.</para>"
        "<para>All items you see in the <interface>Control</interface> menu "
        "or in the <interface>Menubar</interface> can also be placed on the Toolbar.</para>"));
    actionCollection()->action(KStandardAction::name(KStandardAction::Preferences))
        ->setWhatsThis(xi18nc("@info:whatsthis","This opens a window where you can "
        "change a multitude of settings for this application. For an explanation "
        "of the various settings go to the chapter <emphasis>Configuring Dolphin"
        "</emphasis> in <interface>Help|Dolphin Handbook</interface>."));

    // Help menu
    // The whatsthis has to be set for the m_helpMenu and for the
    // StandardAction separately because both are used in different locations.
    // m_helpMenu is only used for createControlButton() button.

    // Links do not work within the Menubar so texts without links are provided there.

    // i18n: If the external link isn't available in your language you should
    // probably state the external link language at least in brackets to not
    // frustrate the user. If there are multiple languages that the user might
    // know with a reasonable chance you might want to have 2 external links.
    // The same is in my opinion true for every external link you translate.
    const QString whatsThisHelpContents = xi18nc("@info:whatsthis handbook",
        "<para>This opens the Handbook for this application. It provides "
        "explanations for every part of <emphasis>Dolphin</emphasis>.</para>");
    actionCollection()->action(KStandardAction::name(KStandardAction::HelpContents))
        ->setWhatsThis(whatsThisHelpContents
        + xi18nc("@info:whatsthis second half of handbook hb text without link",
        "<para>If you want more elaborate introductions to the "
        "different features of <emphasis>Dolphin</emphasis> "
        "go to the KDE UserBase Wiki.</para>"));
    m_helpMenu->action(KHelpMenu::menuHelpContents)->setWhatsThis(whatsThisHelpContents
        + xi18nc("@info:whatsthis second half of handbook text with link",
        "<para>If you want more elaborate introductions to the "
        "different features of <emphasis>Dolphin</emphasis> "
        "<link url='https://userbase.kde.org/Dolphin/File_Management'>click here</link>. "
        "It will open the dedicated page in the KDE UserBase Wiki.</para>"));

    const QString whatsThisWhatsThis = xi18nc("@info:whatsthis whatsthis button",
        "<para>This is the button that invokes the help feature you are "
        "using right now! Click it, then click any component of this "
        "application to ask \"What's this?\" about it. The mouse cursor "
        "will change appearance if no help is available for a spot.</para>");
    actionCollection()->action(KStandardAction::name(KStandardAction::WhatsThis))
       ->setWhatsThis(whatsThisWhatsThis
        + xi18nc("@info:whatsthis second half of whatsthis button text without link",
        "<para>There are two other ways to get help for this application: The "
        "<interface>Dolphin Handbook</interface> in the <interface>Help"
        "</interface> menu and the <emphasis>KDE UserBase Wiki</emphasis> "
        "article about <emphasis>File Management</emphasis> online."
        "</para><para>The \"What's this?\" help is "
        "missing in most other windows so don't get too used to this.</para>"));
    m_helpMenu->action(KHelpMenu::menuWhatsThis)->setWhatsThis(whatsThisWhatsThis
        + xi18nc("@info:whatsthis second half of whatsthis button text with link",
        "<para>There are two other ways to get help: "
        "The <link url='help:/dolphin/index.html'>Dolphin Handbook</link> and "
        "the <link url='https://userbase.kde.org/Dolphin/File_Management'>KDE "
        "UserBase Wiki</link>.</para><para>The \"What's this?\" help is "
        "missing in most other windows so don't get too used to this.</para>"));

    const QString whatsThisReportBug = xi18nc("@info:whatsthis","<para>This opens a "
        "window that will guide you through reporting errors or flaws "
        "in this application or in other KDE software.</para>");
    actionCollection()->action(KStandardAction::name(KStandardAction::ReportBug))
       ->setWhatsThis(whatsThisReportBug);
    m_helpMenu->action(KHelpMenu::menuReportBug)->setWhatsThis(whatsThisReportBug
        + xi18nc("@info:whatsthis second half of reportbug text with link",
        "<para>High-quality bug reports are much appreciated. To learn "
        "how to make your bug report as effective as possible "
        "<link url='https://community.kde.org/Get_Involved/Bug_Reporting'>"
        "click here</link>.</para>"));

    const QString whatsThisDonate = xi18nc("@info:whatsthis","<para>This opens a "
        "<emphasis>web page</emphasis> where you can donate to "
        "support the continued work on this application and many "
        "other projects by the <emphasis>KDE</emphasis> community.</para>"
        "<para>Donating is the easiest and fastest way to efficiently "
        "support KDE and its projects. KDE projects are available for "
        "free therefore your donation is needed to cover things that "
        "require money like servers, contributor meetings, etc.</para>"
        "<para><emphasis>KDE e.V.</emphasis> is the non-profit "
        "organization behind the KDE community.</para>");
    actionCollection()->action(KStandardAction::name(KStandardAction::Donate))
       ->setWhatsThis(whatsThisDonate);
    m_helpMenu->action(KHelpMenu::menuDonate)->setWhatsThis(whatsThisDonate);

    const QString whatsThisSwitchLanguage = xi18nc("@info:whatsthis",
        "With this you can change the language this application uses."
        "<nl/>You can even set secondary languages which will be used "
        "if texts are not available in your preferred language.");
    actionCollection()->action(KStandardAction::name(KStandardAction::SwitchApplicationLanguage))
       ->setWhatsThis(whatsThisSwitchLanguage);
    m_helpMenu->action(KHelpMenu::menuSwitchLanguage)->setWhatsThis(whatsThisSwitchLanguage);

    const QString whatsThisAboutApp = xi18nc("@info:whatsthis","This opens a "
        "window that informs you about the version, license, "
        "used libraries and maintainers of this application.");
    actionCollection()->action(KStandardAction::name(KStandardAction::AboutApp))
       ->setWhatsThis(whatsThisAboutApp);
    m_helpMenu->action(KHelpMenu::menuAboutApp)->setWhatsThis(whatsThisAboutApp);

    const QString whatsThisAboutKDE = xi18nc("@info:whatsthis","This opens a "
        "window with information about <emphasis>KDE</emphasis>. "
        "The KDE community are the people behind this free software."
        "<nl/>If you like using this application but don't know "
        "about KDE or want to see a cute dragon have a look!");
    actionCollection()->action(KStandardAction::name(KStandardAction::AboutKDE))
       ->setWhatsThis(whatsThisAboutKDE);
    m_helpMenu->action(KHelpMenu::menuAboutKDE)->setWhatsThis(whatsThisAboutKDE);
}

bool DolphinMainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WhatsThisClicked) {
        event->accept();
        QWhatsThisClickedEvent* whatsThisEvent = dynamic_cast<QWhatsThisClickedEvent*>(event);
        QDesktopServices::openUrl(QUrl(whatsThisEvent->href()));
        return true;
    }
    return KXmlGuiWindow::event(event);
}

bool DolphinMainWindow::eventFilter(QObject* obj, QEvent* event)
{
    Q_UNUSED(obj)
    if (event->type() == QEvent::WhatsThisClicked) {
        event->accept();
        QWhatsThisClickedEvent* whatsThisEvent = dynamic_cast<QWhatsThisClickedEvent*>(event);
        QDesktopServices::openUrl(QUrl(whatsThisEvent->href()));
        return true;
    }
    return false;
}

void DolphinMainWindow::saveNewToolbarConfig()
{
    KXmlGuiWindow::saveNewToolbarConfig(); // Applies the new config. This has to be called first
                                           // because the rest of this method decides things
                                           // based on the new config.
    auto navigators = static_cast<DolphinNavigatorsWidgetAction *>
                        (actionCollection()->action(QStringLiteral("url_navigators")));
    if (!toolBar()->actions().contains(navigators)) {
        m_tabWidget->currentTabPage()->insertNavigatorsWidget(navigators);
    }
    updateAllowedToolbarAreas();
}

void DolphinMainWindow::focusTerminalPanel()
{
    if (m_terminalPanel->isVisible()) {
        if (m_terminalPanel->terminalHasFocus()) {
            m_activeViewContainer->view()->setFocus(Qt::FocusReason::ShortcutFocusReason);
            actionCollection()->action(QStringLiteral("focus_terminal_panel"))->setText(i18nc("@action:inmenu Tools", "Focus Terminal Panel"));
        } else {
            m_terminalPanel->setFocus(Qt::FocusReason::ShortcutFocusReason);
            actionCollection()->action(QStringLiteral("focus_terminal_panel"))->setText(i18nc("@action:inmenu Tools", "Defocus Terminal Panel"));
        }
    } else {
        actionCollection()->action(QStringLiteral("show_terminal_panel"))->trigger();
        actionCollection()->action(QStringLiteral("focus_terminal_panel"))->setText(i18nc("@action:inmenu Tools", "Defocus Terminal Panel"));
    }
}

DolphinMainWindow::UndoUiInterface::UndoUiInterface() :
    KIO::FileUndoManager::UiInterface()
{
}

DolphinMainWindow::UndoUiInterface::~UndoUiInterface()
{
}

void DolphinMainWindow::UndoUiInterface::jobError(KIO::Job* job)
{
    DolphinMainWindow* mainWin= qobject_cast<DolphinMainWindow *>(parentWidget());
    if (mainWin) {
        DolphinViewContainer* container = mainWin->activeViewContainer();
        container->showMessage(job->errorString(), DolphinViewContainer::Error);
    } else {
        KIO::FileUndoManager::UiInterface::jobError(job);
    }
}

bool DolphinMainWindow::isUrlOpen(const QString& url)
{
    return m_tabWidget->isUrlOpen(QUrl::fromUserInput((url)));
}

