/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Stefan Monov <logixoul@gmail.com>
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinmainwindow.h"

#include "dolphin_generalsettings.h"
#include "dolphinbookmarkhandler.h"
#include "dolphincontextmenu.h"
#include "dolphindockwidget.h"
#include "dolphinmainwindowadaptor.h"
#include "dolphinnavigatorswidgetaction.h"
#include "dolphinnewfilemenu.h"
#include "dolphinplacesmodelsingleton.h"
#include "dolphinrecenttabsmenu.h"
#include "dolphintabpage.h"
#include "dolphinurlnavigatorscontroller.h"
#include "dolphinviewcontainer.h"
#include "global.h"
#include "middleclickactioneventfilter.h"
#include "panels/folders/folderspanel.h"
#include "panels/places/placespanel.h"
#include "panels/terminal/terminalpanel.h"
#include "selectionmode/actiontexthelper.h"
#include "settings/dolphinsettingsdialog.h"
#include "statusbar/dolphinstatusbar.h"
#include "views/dolphinnewfilemenuobserver.h"
#include "views/dolphinremoteencoding.h"
#include "views/dolphinviewactionhandler.h"
#include "views/draganddrophelper.h"
#include "views/viewproperties.h"

#include <KActionCollection>
#include <KActionMenu>
#include <KAuthorized>
#include <KConfig>
#include <KConfigGui>
#include <KDualAction>
#include <KFileItemListProperties>
#include <KIO/CommandLauncherJob>
#include <KIO/JobUiDelegateFactory>
#include <KIO/OpenFileManagerWindowJob>
#include <KIO/OpenUrlJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProtocolInfo>
#include <KProtocolManager>
#include <KShell>
#include <KShortcutsDialog>
#include <KStandardAction>
#include <KSycoca>
#include <KTerminalLauncherJob>
#include <KToggleAction>
#include <KToolBar>
#include <KToolBarPopupAction>
#include <KUrlComboBox>
#include <KUrlNavigator>
#include <KWindowSystem>
#include <KXMLGUIFactory>

#include <kwidgetsaddons_version.h>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDialog>
#include <QDomDocument>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>

#include <algorithm>

#if HAVE_X11
#include <KStartupInfo>
#endif

namespace
{
// Used for GeneralSettings::version() to determine whether
// an updated version of Dolphin is running, so as to migrate
// removed/renamed ...etc config entries; increment it in such
// cases
const int CurrentDolphinVersion = 202;
// The maximum number of entries in the back/forward popup menu
const int MaxNumberOfNavigationentries = 12;
// The maximum number of "Activate Tab" shortcuts
const int MaxActivateTabShortcuts = 9;
}

DolphinMainWindow::DolphinMainWindow()
    : KXmlGuiWindow(nullptr)
    , m_newFileMenu(nullptr)
    , m_tabWidget(nullptr)
    , m_activeViewContainer(nullptr)
    , m_actionHandler(nullptr)
    , m_remoteEncoding(nullptr)
    , m_settingsDialog()
    , m_bookmarkHandler(nullptr)
    , m_lastHandleUrlOpenJob(nullptr)
    , m_terminalPanel(nullptr)
    , m_placesPanel(nullptr)
    , m_tearDownFromPlacesRequested(false)
    , m_backAction(nullptr)
    , m_forwardAction(nullptr)
{
    Q_INIT_RESOURCE(dolphin);

    new MainWindowAdaptor(this);

#ifndef Q_OS_WIN
    setWindowFlags(Qt::WindowContextHelpButtonHint);
#endif
    setComponentName(QStringLiteral("dolphin"), QGuiApplication::applicationDisplayName());
    setObjectName(QStringLiteral("Dolphin#"));

    setStateConfigGroup("State");

    connect(&DolphinNewFileMenuObserver::instance(), &DolphinNewFileMenuObserver::errorMessage, this, &DolphinMainWindow::showErrorMessage);

    KIO::FileUndoManager *undoManager = KIO::FileUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface());

    connect(undoManager, &KIO::FileUndoManager::undoAvailable, this, &DolphinMainWindow::slotUndoAvailable);
    connect(undoManager, &KIO::FileUndoManager::undoTextChanged, this, &DolphinMainWindow::slotUndoTextChanged);
    connect(undoManager, &KIO::FileUndoManager::jobRecordingStarted, this, &DolphinMainWindow::clearStatusBar);
    connect(undoManager, &KIO::FileUndoManager::jobRecordingFinished, this, &DolphinMainWindow::showCommand);

    const bool firstRun = (GeneralSettings::version() < 200);
    if (firstRun) {
        GeneralSettings::setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    auto *navigatorsWidgetAction = new DolphinNavigatorsWidgetAction(this);
    actionCollection()->addAction(QStringLiteral("url_navigators"), navigatorsWidgetAction);
    m_tabWidget = new DolphinTabWidget(navigatorsWidgetAction, this);
    m_tabWidget->setObjectName("tabWidget");
    connect(m_tabWidget, &DolphinTabWidget::activeViewChanged, this, &DolphinMainWindow::activeViewChanged);
    connect(m_tabWidget, &DolphinTabWidget::tabCountChanged, this, &DolphinMainWindow::tabCountChanged);
    connect(m_tabWidget, &DolphinTabWidget::currentUrlChanged, this, &DolphinMainWindow::updateWindowTitle);
    setCentralWidget(m_tabWidget);

    m_actionTextHelper = new SelectionMode::ActionTextHelper(this);
    setupActions();

    m_actionHandler = new DolphinViewActionHandler(actionCollection(), m_actionTextHelper, this);
    connect(m_actionHandler, &DolphinViewActionHandler::actionBeingHandled, this, &DolphinMainWindow::clearStatusBar);
    connect(m_actionHandler, &DolphinViewActionHandler::createDirectoryTriggered, this, &DolphinMainWindow::createDirectory);
    connect(m_actionHandler, &DolphinViewActionHandler::selectionModeChangeTriggered, this, &DolphinMainWindow::slotSetSelectionMode);

    Q_CHECK_PTR(actionCollection()->action(QStringLiteral("create_dir")));
    m_newFileMenu->setNewFolderShortcutAction(actionCollection()->action(QStringLiteral("create_dir")));

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, &DolphinMainWindow::urlChanged, m_remoteEncoding, &DolphinRemoteEncoding::slotAboutToOpenUrl);

    setupDockWidgets();

    setupGUI(Save | Create | ToolBar);
    stateChanged(QStringLiteral("new_file"));

    QClipboard *clipboard = QApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged, this, &DolphinMainWindow::updatePasteAction);

    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(GeneralSettings::filterBar());

    if (firstRun) {
        menuBar()->setVisible(false);
    }

    const bool showMenu = !menuBar()->isHidden();
    QAction *showMenuBarAction = actionCollection()->action(KStandardAction::name(KStandardAction::ShowMenubar));
    showMenuBarAction->setChecked(showMenu); // workaround for bug #171080

    auto hamburgerMenu = static_cast<KHamburgerMenu *>(actionCollection()->action(KStandardAction::name(KStandardAction::HamburgerMenu)));
    hamburgerMenu->setMenuBar(menuBar());
    hamburgerMenu->setShowMenuBarAction(showMenuBarAction);
    connect(hamburgerMenu, &KHamburgerMenu::aboutToShowMenu, this, &DolphinMainWindow::updateHamburgerMenu);
    hamburgerMenu->hideActionsOf(toolBar());
    if (GeneralSettings::version() < 201 && !toolBar()->actions().contains(hamburgerMenu)) {
        addHamburgerMenuToToolbar();
    }

    updateAllowedToolbarAreas();

    // enable middle-click on back/forward/up to open in a new tab
    auto *middleClickEventFilter = new MiddleClickActionEventFilter(this);
    connect(middleClickEventFilter, &MiddleClickActionEventFilter::actionMiddleClicked, this, &DolphinMainWindow::slotToolBarActionMiddleClicked);
    toolBar()->installEventFilter(middleClickEventFilter);

    setupWhatsThis();

    connect(KSycoca::self(), &KSycoca::databaseChanged, this, &DolphinMainWindow::updateOpenPreferredSearchToolAction);

    QTimer::singleShot(0, this, &DolphinMainWindow::updateOpenPreferredSearchToolAction);

    m_fileItemActions.setParentWidget(this);
    connect(&m_fileItemActions, &KFileItemActions::error, this, [this](const QString &errorMessage) {
        showErrorMessage(errorMessage);
    });

    connect(GeneralSettings::self(), &GeneralSettings::splitViewChanged, this, &DolphinMainWindow::slotSplitViewChanged);
}

DolphinMainWindow::~DolphinMainWindow()
{
    // This fixes a crash on Wayland when closing the mainwindow while another dialog is open.
    disconnect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &DolphinMainWindow::updatePasteAction);
}

QVector<DolphinViewContainer *> DolphinMainWindow::viewContainers() const
{
    QVector<DolphinViewContainer *> viewContainers;

    for (int i = 0; i < m_tabWidget->count(); ++i) {
        DolphinTabPage *tabPage = m_tabWidget->tabPageAt(i);

        viewContainers << tabPage->primaryViewContainer();
        if (tabPage->splitViewEnabled()) {
            viewContainers << tabPage->secondaryViewContainer();
        }
    }
    return viewContainers;
}

void DolphinMainWindow::openDirectories(const QList<QUrl> &dirs, bool splitView)
{
    m_tabWidget->openDirectories(dirs, splitView);
}

void DolphinMainWindow::openDirectories(const QStringList &dirs, bool splitView)
{
    openDirectories(QUrl::fromStringList(dirs), splitView);
}

void DolphinMainWindow::openFiles(const QList<QUrl> &files, bool splitView)
{
    m_tabWidget->openFiles(files, splitView);
}

bool DolphinMainWindow::isFoldersPanelEnabled() const
{
    return actionCollection()->action(QStringLiteral("show_folders_panel"))->isChecked();
}

bool DolphinMainWindow::isInformationPanelEnabled() const
{
#if HAVE_BALOO
    return actionCollection()->action(QStringLiteral("show_information_panel"))->isChecked();
#else
    return false;
#endif
}

bool DolphinMainWindow::isSplitViewEnabledInCurrentTab() const
{
    return m_tabWidget->currentTabPage()->splitViewEnabled();
}

void DolphinMainWindow::openFiles(const QStringList &files, bool splitView)
{
    openFiles(QUrl::fromStringList(files), splitView);
}

void DolphinMainWindow::activateWindow(const QString &activationToken)
{
    window()->setAttribute(Qt::WA_NativeWindow, true);

    if (KWindowSystem::isPlatformWayland()) {
        KWindowSystem::setCurrentXdgActivationToken(activationToken);
    } else if (KWindowSystem::isPlatformX11()) {
#if HAVE_X11
        KStartupInfo::setNewStartupId(window()->windowHandle(), activationToken.toUtf8());
#endif
    }

    KWindowSystem::activateWindow(window()->windowHandle());
}

bool DolphinMainWindow::isActiveWindow()
{
    return window()->isActiveWindow();
}

void DolphinMainWindow::showCommand(CommandType command)
{
    DolphinStatusBar *statusBar = m_activeViewContainer->statusBar();
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

void DolphinMainWindow::slotTerminalDirectoryChanged(const QUrl &url)
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
    KToggleAction *editableLocationAction = static_cast<KToggleAction *>(actionCollection()->action(QStringLiteral("editable_location")));
    editableLocationAction->setChecked(editable);
}

void DolphinMainWindow::slotSelectionChanged(const KFileItemList &selection)
{
    updateFileAndEditActions();

    const int selectedUrlsCount = m_tabWidget->currentTabPage()->selectedItemsCount();

    QAction *compareFilesAction = actionCollection()->action(QStringLiteral("compare_files"));
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

    QAction *backAction = actionCollection()->action(KStandardAction::name(KStandardAction::Back));
    if (backAction) {
        backAction->setToolTip(i18nc("@info", "Go back"));
        backAction->setWhatsThis(i18nc("@info:whatsthis go back", "Return to the previously viewed folder."));
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction *forwardAction = actionCollection()->action(KStandardAction::name(KStandardAction::Forward));
    if (forwardAction) {
        forwardAction->setToolTip(i18nc("@info", "Go forward"));
        forwardAction->setWhatsThis(xi18nc("@info:whatsthis go forward", "This undoes a <interface>Go|Back</interface> action."));
        forwardAction->setEnabled(index > 0);
    }
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
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
        QString icon;
        if (m_activeViewContainer->isSearchModeEnabled()) {
            icon = QStringLiteral("folder-saved-search-symbolic");
        } else {
            icon = KIO::iconNameForUrl(url);
        }
        DolphinPlacesModelSingleton::instance().placesModel()->addPlace(name, url, icon);
    }
}

void DolphinMainWindow::openNewTab(const QUrl &url)
{
    m_tabWidget->openNewTab(url, QUrl());
}

void DolphinMainWindow::openNewTabAndActivate(const QUrl &url)
{
    m_tabWidget->openNewActivatedTab(url, QUrl());
}

void DolphinMainWindow::openNewWindow(const QUrl &url)
{
    Dolphin::openNewWindow({url}, this);
}

void DolphinMainWindow::slotSplitViewChanged()
{
    m_tabWidget->currentTabPage()->setSplitViewEnabled(GeneralSettings::splitView(), WithAnimation);
    updateSplitAction();
}

void DolphinMainWindow::openInNewTab()
{
    const KFileItemList &list = m_activeViewContainer->view()->selectedItems();
    bool tabCreated = false;

    for (const KFileItem &item : list) {
        const QUrl &url = DolphinView::openItemAsFolderUrl(item);
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
        const KFileItem &item = list.first();
        newWindowUrl = DolphinView::openItemAsFolderUrl(item);
    }

    if (!newWindowUrl.isEmpty()) {
        Dolphin::openNewWindow({newWindowUrl}, this);
    }
}

void DolphinMainWindow::openInSplitView(const QUrl &url)
{
    QUrl newSplitViewUrl = url;

    if (newSplitViewUrl.isEmpty()) {
        const KFileItemList list = m_activeViewContainer->view()->selectedItems();
        if (list.count() == 1) {
            const KFileItem &item = list.first();
            newSplitViewUrl = DolphinView::openItemAsFolderUrl(item);
        }
    }

    if (newSplitViewUrl.isEmpty()) {
        return;
    }

    DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    if (tabPage->splitViewEnabled()) {
        tabPage->switchActiveView();
        tabPage->activeViewContainer()->setUrl(newSplitViewUrl);
    } else {
        tabPage->setSplitViewEnabled(true, WithAnimation, newSplitViewUrl);
        updateViewActions();
    }
}

void DolphinMainWindow::showTarget()
{
    const KFileItem link = m_activeViewContainer->view()->selectedItems().at(0);
    const QUrl destinationUrl = link.url().resolved(QUrl(link.linkDest()));

    auto job = KIO::stat(destinationUrl, KIO::StatJob::SourceSide, KIO::StatNoDetails);

    connect(job, &KJob::finished, this, [this, destinationUrl](KJob *job) {
        KIO::StatJob *statJob = static_cast<KIO::StatJob *>(job);

        if (statJob->error()) {
            m_activeViewContainer->showMessage(job->errorString(), DolphinViewContainer::Error);
        } else {
            KIO::highlightInFileManager({destinationUrl});
        }
    });
}

bool DolphinMainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::ShortcutOverride) {
        const QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Space && m_activeViewContainer->view()->handleSpaceAsNormalKey()) {
            event->accept();
            return true;
        }
    }

    return KXmlGuiWindow::event(event);
}

void DolphinMainWindow::showEvent(QShowEvent *event)
{
    KXmlGuiWindow::showEvent(event);

    if (!event->spontaneous()) {
        m_activeViewContainer->view()->setFocus();
    }
}

void DolphinMainWindow::closeEvent(QCloseEvent *event)
{
    // Find out if Dolphin is closed directly by the user or
    // by the session manager because the session is closed
    bool closedByUser = true;
    if (qApp->isSavingSession()) {
        closedByUser = false;
    }

    if (m_tabWidget->count() > 1 && GeneralSettings::confirmClosingMultipleTabs() && !GeneralSettings::rememberOpenedTabs() && closedByUser) {
        // Ask the user if he really wants to quit and close all tabs.
        // Open a confirmation dialog with 3 buttons:
        // QDialogButtonBox::Yes    -> Quit
        // QDialogButtonBox::No     -> Close only the current tab
        // QDialogButtonBox::Cancel -> do nothing
        QDialog *dialog = new QDialog(this, Qt::Dialog);
        dialog->setWindowTitle(i18nc("@title:window", "Confirmation"));
        dialog->setModal(true);
        QDialogButtonBox *buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel);
        KGuiItem::assign(buttons->button(QDialogButtonBox::Yes),
                         KGuiItem(i18nc("@action:button 'Quit Dolphin' button", "&Quit %1", QGuiApplication::applicationDisplayName()),
                                  QIcon::fromTheme(QStringLiteral("application-exit"))));
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
            KGuiItem::assign(buttons->button(QDialogButtonBox::No), KGuiItem(i18n("Show &Terminal Panel"), QIcon::fromTheme(QStringLiteral("dialog-scripts"))));
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
            KMessageBox::Notify | KMessageBox::Dangerous);

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

    if (GeneralSettings::rememberOpenedTabs()) {
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

void DolphinMainWindow::saveProperties(KConfigGroup &group)
{
    m_tabWidget->saveProperties(group);
}

void DolphinMainWindow::readProperties(const KConfigGroup &group)
{
    m_tabWidget->readProperties(group);
}

void DolphinMainWindow::updateNewMenu()
{
    m_newFileMenu->checkUpToDate();
    m_newFileMenu->setWorkingDirectory(activeViewContainer()->url());
}

void DolphinMainWindow::createDirectory()
{
    m_newFileMenu->setWorkingDirectory(activeViewContainer()->url());
    m_newFileMenu->createDirectory();
}

void DolphinMainWindow::quit()
{
    close();
}

void DolphinMainWindow::showErrorMessage(const QString &message)
{
    m_activeViewContainer->showMessage(message, DolphinViewContainer::Error);
}

void DolphinMainWindow::slotUndoAvailable(bool available)
{
    QAction *undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    if (undoAction) {
        undoAction->setEnabled(available);
    }
}

void DolphinMainWindow::slotUndoTextChanged(const QString &text)
{
    QAction *undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
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
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::CutContents);
    } else {
        m_activeViewContainer->view()->cutSelectedItemsToClipboard();
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
}

void DolphinMainWindow::copy()
{
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::CopyContents);
    } else {
        m_activeViewContainer->view()->copySelectedItemsToClipboard();
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
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
    QAction *toggleSearchAction = actionCollection()->action(QStringLiteral("toggle_search"));
    toggleSearchAction->setChecked(m_activeViewContainer->isSearchModeEnabled());
}

void DolphinMainWindow::updatePasteAction()
{
    QAction *pasteAction = actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
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

QAction *DolphinMainWindow::urlNavigatorHistoryAction(const KUrlNavigator *urlNavigator, int historyIndex, QObject *parent)
{
    const QUrl url = urlNavigator->locationUrl(historyIndex);

    QString text = url.toDisplayString(QUrl::PreferLocalFile);

    if (!urlNavigator->showFullPath()) {
        const KFilePlacesModel *placesModel = DolphinPlacesModelSingleton::instance().placesModel();

        const QModelIndex closestIdx = placesModel->closestItem(url);
        if (closestIdx.isValid()) {
            const QUrl placeUrl = placesModel->url(closestIdx);

            text = placesModel->text(closestIdx);

            QString pathInsidePlace = url.path().mid(placeUrl.path().length());

            if (!pathInsidePlace.isEmpty() && !pathInsidePlace.startsWith(QLatin1Char('/'))) {
                pathInsidePlace.prepend(QLatin1Char('/'));
            }

            if (pathInsidePlace != QLatin1Char('/')) {
                text.append(pathInsidePlace);
            }
        }
    }

    QAction *action = new QAction(QIcon::fromTheme(KIO::iconNameForUrl(url)), text, parent);
    action->setData(historyIndex);
    return action;
}

void DolphinMainWindow::slotAboutToShowBackPopupMenu()
{
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    int entries = 0;
    QMenu *menu = m_backAction->popupMenu();
    menu->clear();
    for (int i = urlNavigator->historyIndex() + 1; i < urlNavigator->historySize() && entries < MaxNumberOfNavigationentries; ++i, ++entries) {
        QAction *action = urlNavigatorHistoryAction(urlNavigator, i, menu);
        menu->addAction(action);
    }
}

void DolphinMainWindow::slotGoBack(QAction *action)
{
    int gotoIndex = action->data().value<int>();
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    for (int i = gotoIndex - urlNavigator->historyIndex(); i > 0; --i) {
        goBack();
    }
}

void DolphinMainWindow::slotBackForwardActionMiddleClicked(QAction *action)
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
    QMenu *menu = m_forwardAction->popupMenu();
    for (int i = urlNavigator->historyIndex() - 1; i >= 0 && entries < MaxNumberOfNavigationentries; --i, ++entries) {
        QAction *action = urlNavigatorHistoryAction(urlNavigator, i, menu);
        menu->addAction(action);
    }
}

void DolphinMainWindow::slotGoForward(QAction *action)
{
    int gotoIndex = action->data().value<int>();
    const KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigatorInternalWithHistory();
    for (int i = urlNavigator->historyIndex() - gotoIndex; i > 0; --i) {
        goForward();
    }
}

void DolphinMainWindow::slotSetSelectionMode(bool enabled, SelectionMode::BottomBar::Contents bottomBarContents)
{
    m_activeViewContainer->setSelectionModeEnabled(enabled, actionCollection(), bottomBarContents);
}

void DolphinMainWindow::selectAll()
{
    clearStatusBar();

    // if the URL navigator is editable and focused, select the whole
    // URL instead of all items of the view

    KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigator();
    QLineEdit *lineEdit = urlNavigator->editor()->lineEdit();
    const bool selectUrl = urlNavigator->isUrlEditable() && lineEdit->hasFocus();
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
    DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    tabPage->setSplitViewEnabled(!tabPage->splitViewEnabled(), WithAnimation);
    updateViewActions();
}

void DolphinMainWindow::toggleSplitStash()
{
    DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
    tabPage->setSplitViewEnabled(false, WithAnimation);
    tabPage->setSplitViewEnabled(true, WithAnimation, QUrl("stash:/"));
}

void DolphinMainWindow::copyToInactiveSplitView()
{
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::CopyToOtherViewContents);
    } else {
        m_tabWidget->copyToInactiveSplitView();
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
}

void DolphinMainWindow::moveToInactiveSplitView()
{
    if (m_activeViewContainer->view()->selectedItems().isEmpty()) {
        m_activeViewContainer->setSelectionModeEnabled(true, actionCollection(), SelectionMode::BottomBar::Contents::MoveToOtherViewContents);
    } else {
        m_tabWidget->moveToInactiveSplitView();
        m_activeViewContainer->setSelectionModeEnabled(false);
    }
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

void DolphinMainWindow::toggleSelectionMode()
{
    const bool checked = !m_activeViewContainer->isSelectionModeEnabled();

    m_activeViewContainer->setSelectionModeEnabled(checked, actionCollection(), SelectionMode::BottomBar::Contents::GeneralContents);
    actionCollection()->action(QStringLiteral("toggle_selection_mode"))->setChecked(checked);
}

void DolphinMainWindow::showFilterBar()
{
    m_activeViewContainer->setFilterBarVisible(true);
}

void DolphinMainWindow::toggleFilterBar()
{
    const bool checked = !m_activeViewContainer->isFilterBarVisible();
    m_activeViewContainer->setFilterBarVisible(checked);

    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(checked);
}

void DolphinMainWindow::toggleEditLocation()
{
    clearStatusBar();

    QAction *action = actionCollection()->action(QStringLiteral("editable_location"));
    KUrlNavigator *urlNavigator = m_activeViewContainer->urlNavigator();
    urlNavigator->setUrlEditable(action->isChecked());
}

void DolphinMainWindow::replaceLocation()
{
    KUrlNavigator *navigator = m_activeViewContainer->urlNavigator();
    QLineEdit *lineEdit = navigator->editor()->lineEdit();

    // If the text field currently has focus and everything is selected,
    // pressing the keyboard shortcut returns the whole thing to breadcrumb mode
    if (navigator->isUrlEditable() && lineEdit->hasFocus() && lineEdit->selectedText() == lineEdit->text()) {
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
    for (QObject *child : childrenObjects) {
        DolphinDockWidget *dock = qobject_cast<DolphinDockWidget *>(child);
        if (dock) {
            dock->setLocked(newLockState);
        }
    }

    DolphinPlacesModelSingleton::instance().placesModel()->setPanelsLocked(newLockState);

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
    const KUrlNavigator *urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
    const int index = urlNavigator->historyIndex() + 1;
    openNewTab(urlNavigator->locationUrl(index));
}

void DolphinMainWindow::goForwardInNewTab()
{
    const KUrlNavigator *urlNavigator = activeViewContainer()->urlNavigatorInternalWithHistory();
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
}

QPointer<QAction> DolphinMainWindow::preferredSearchTool()
{
    m_searchTools.clear();

    KService::Ptr kfind = KService::serviceByDesktopName(QStringLiteral("org.kde.kfind"));

    if (!kfind) {
        return nullptr;
    }

    auto *action = new QAction(QIcon::fromTheme(kfind->icon()), kfind->name(), this);

    connect(action, &QAction::triggered, this, [kfind] {
        auto *job = new KIO::ApplicationLauncherJob(kfind);
        job->start();
    });

    return action;
}

void DolphinMainWindow::updateOpenPreferredSearchToolAction()
{
    QAction *openPreferredSearchTool = actionCollection()->action(QStringLiteral("open_preferred_search_tool"));
    if (!openPreferredSearchTool) {
        return;
    }
    QPointer<QAction> tool = preferredSearchTool();
    if (tool) {
        openPreferredSearchTool->setVisible(true);
        openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open %1", tool->text()));
        // Only override with the app icon if it is the default, i.e. the user hasn't configured one manually
        // https://bugs.kde.org/show_bug.cgi?id=442815
        if (openPreferredSearchTool->icon().name() == QLatin1String("search")) {
            openPreferredSearchTool->setIcon(tool->icon());
        }
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
    openTerminalJob(m_activeViewContainer->url());
}

void DolphinMainWindow::openTerminalHere()
{
    QList<QUrl> urls = {};

    const auto selectedItems = m_activeViewContainer->view()->selectedItems();
    for (const KFileItem &item : selectedItems) {
        QUrl url = item.targetUrl();
        if (item.isFile()) {
            url.setPath(QFileInfo(url.path()).absolutePath());
        }
        if (!urls.contains(url)) {
            urls << url;
        }
    }

    // No items are selected. Open a terminal window for the current location.
    if (urls.count() == 0) {
        openTerminal();
        return;
    }

    if (urls.count() > 5) {
        QString question = i18np("Are you sure you want to open 1 terminal window?", "Are you sure you want to open %1 terminal windows?", urls.count());
        const int answer = KMessageBox::warningContinueCancel(
            this,
            question,
            {},
            KGuiItem(i18ncp("@action:button", "Open %1 Terminal", "Open %1 Terminals", urls.count()), QStringLiteral("utilities-terminal")),
            KStandardGuiItem::cancel(),
            QStringLiteral("ConfirmOpenManyTerminals"));
        if (answer != KMessageBox::PrimaryAction && answer != KMessageBox::Continue) {
            return;
        }
    }

    for (const QUrl &url : std::as_const(urls)) {
        openTerminalJob(url);
    }
}

void DolphinMainWindow::openTerminalJob(const QUrl &url)
{
    if (url.isLocalFile()) {
        auto job = new KTerminalLauncherJob(QString());
        job->setWorkingDirectory(url.toLocalFile());
        job->start();
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

            auto job = new KTerminalLauncherJob(QString());
            job->setWorkingDirectory(statUrl.isLocalFile() ? statUrl.toLocalFile() : QDir::homePath());
            job->start();
        });

        return;
    }

    // Nothing worked, just use $HOME
    auto job = new KTerminalLauncherJob(QString());
    job->setWorkingDirectory(QDir::homePath());
    job->start();
}

void DolphinMainWindow::editSettings()
{
    if (!m_settingsDialog) {
        DolphinViewContainer *container = activeViewContainer();
        container->view()->writeSettings();

        const QUrl url = container->url();
        DolphinSettingsDialog *settingsDialog = new DolphinSettingsDialog(url, this, actionCollection());
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged, this, &DolphinMainWindow::refreshViews);
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged, &DolphinUrlNavigatorsController::slotReadSettings);
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        settingsDialog->show();
        m_settingsDialog = settingsDialog;
    } else {
        m_settingsDialog.data()->raise();
    }
}

void DolphinMainWindow::handleUrl(const QUrl &url)
{
    delete m_lastHandleUrlOpenJob;
    m_lastHandleUrlOpenJob = nullptr;

    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()) {
        activeViewContainer()->setUrl(url);
    } else {
        m_lastHandleUrlOpenJob = new KIO::OpenUrlJob(url);
        m_lastHandleUrlOpenJob->setUiDelegate(KIO::createDefaultJobUiDelegate(KJobUiDelegate::AutoHandlingEnabled, this));
        m_lastHandleUrlOpenJob->setShowOpenOrExecuteDialog(true);

        connect(m_lastHandleUrlOpenJob, &KIO::OpenUrlJob::mimeTypeFound, this, [this, url](const QString &mimetype) {
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

void DolphinMainWindow::openContextMenu(const QPoint &pos, const KFileItem &item, const KFileItemList &selectedItems, const QUrl &url)
{
    QPointer<DolphinContextMenu> contextMenu = new DolphinContextMenu(this, item, selectedItems, url, &m_fileItemActions);
    contextMenu.data()->exec(pos);

    // Delete the menu, unless it has been deleted in its own nested event loop already.
    if (contextMenu) {
        contextMenu->deleteLater();
    }
}

QMenu *DolphinMainWindow::createPopupMenu()
{
    QMenu *menu = KXmlGuiWindow::createPopupMenu();

    menu->addSeparator();
    menu->addAction(actionCollection()->action(QStringLiteral("lock_panels")));

    return menu;
}

void DolphinMainWindow::updateHamburgerMenu()
{
    KActionCollection *ac = actionCollection();
    auto hamburgerMenu = static_cast<KHamburgerMenu *>(ac->action(KStandardAction::name(KStandardAction::HamburgerMenu)));
    auto menu = hamburgerMenu->menu();
    if (!menu) {
        menu = new QMenu(this);
        hamburgerMenu->setMenu(menu);
        hamburgerMenu->hideActionsOf(ac->action(QStringLiteral("basic_actions"))->menu());
        hamburgerMenu->hideActionsOf(ac->action(QStringLiteral("zoom"))->menu());
    } else {
        menu->clear();
    }
    const QList<QAction *> toolbarActions = toolBar()->actions();

    if (!toolBar()->isVisible()) {
        // If neither the menu bar nor the toolbar are visible, these actions should be available.
        menu->addAction(ac->action(KStandardAction::name(KStandardAction::ShowMenubar)));
        menu->addAction(toolBarMenuAction());
        menu->addSeparator();
    }

    // This group of actions (until the next separator) contains all the most basic actions
    // necessary to use Dolphin effectively.
    menu->addAction(ac->action(QStringLiteral("go_back")));
    menu->addAction(ac->action(QStringLiteral("go_forward")));

    menu->addMenu(m_newFileMenu->menu());
    if (!toolBar()->isVisible() || !toolbarActions.contains(ac->action(QStringLiteral("toggle_selection_mode_tool_bar")))) {
        menu->addAction(ac->action(QStringLiteral("toggle_selection_mode")));
    }
    menu->addAction(ac->action(QStringLiteral("basic_actions")));
    menu->addAction(ac->action(KStandardAction::name(KStandardAction::Undo)));
    if (!toolBar()->isVisible()
        || (!toolbarActions.contains(ac->action(QStringLiteral("toggle_search")))
            && !toolbarActions.contains(ac->action(QStringLiteral("open_preferred_search_tool"))))) {
        menu->addAction(ac->action(KStandardAction::name(KStandardAction::Find)));
        // This way a search action will only be added if none of the three available
        // search actions is present on the toolbar.
    }
    if (!toolBar()->isVisible() || !toolbarActions.contains(ac->action(QStringLiteral("toggle_filter")))) {
        menu->addAction(ac->action(QStringLiteral("show_filter_bar")));
        // This way a filter action will only be added if none of the two available
        // filter actions is present on the toolbar.
    }
    menu->addSeparator();

    // The second group of actions (up until the next separator) contains actions for opening
    // additional views to interact with the file system.
    menu->addAction(ac->action(QStringLiteral("file_new")));
    menu->addAction(ac->action(QStringLiteral("new_tab")));
    if (ac->action(QStringLiteral("undo_close_tab"))->isEnabled()) {
        menu->addAction(ac->action(QStringLiteral("closed_tabs")));
    }
    menu->addAction(ac->action(QStringLiteral("open_terminal")));
    menu->addSeparator();

    // The third group contains actions to change what one sees in the view
    // and to change the more general UI.
    if (!toolBar()->isVisible()
        || (!toolbarActions.contains(ac->action(QStringLiteral("icons"))) && !toolbarActions.contains(ac->action(QStringLiteral("compact")))
            && !toolbarActions.contains(ac->action(QStringLiteral("details"))) && !toolbarActions.contains(ac->action(QStringLiteral("view_mode"))))) {
        menu->addAction(ac->action(QStringLiteral("view_mode")));
    }
    menu->addAction(ac->action(QStringLiteral("show_hidden_files")));
    menu->addAction(ac->action(QStringLiteral("sort")));
    menu->addAction(ac->action(QStringLiteral("additional_info")));
    if (!GeneralSettings::showStatusBar() || !GeneralSettings::showZoomSlider()) {
        menu->addAction(ac->action(QStringLiteral("zoom")));
    }
    menu->addAction(ac->action(QStringLiteral("panels")));

    // The "Configure" menu is not added to the actionCollection() because there is hardly
    // a good reason for users to put it on their toolbar.
    auto configureMenu = menu->addMenu(QIcon::fromTheme(QStringLiteral("configure")), i18nc("@action:inmenu menu for configure actions", "Configure"));
    configureMenu->addAction(ac->action(KStandardAction::name(KStandardAction::SwitchApplicationLanguage)));
    configureMenu->addAction(ac->action(KStandardAction::name(KStandardAction::KeyBindings)));
    configureMenu->addAction(ac->action(KStandardAction::name(KStandardAction::ConfigureToolbars)));
    configureMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Preferences)));
    hamburgerMenu->hideActionsOf(configureMenu);
}

void DolphinMainWindow::slotPlaceActivated(const QUrl &url)
{
    DolphinViewContainer *view = activeViewContainer();

    if (view->url() == url) {
        view->clearFilterBar(); // Fixes bug 259382.

        // We can end up here if the user clicked a device in the Places Panel
        // which had been unmounted earlier, see https://bugs.kde.org/show_bug.cgi?id=161385.
        reloadView();
    } else {
        view->disableUrlNavigatorSelectionRequests();
        changeUrl(url);
        view->enableUrlNavigatorSelectionRequests();
    }
}

void DolphinMainWindow::closedTabsCountChanged(unsigned int count)
{
    actionCollection()->action(QStringLiteral("undo_close_tab"))->setEnabled(count > 0);
}

void DolphinMainWindow::activeViewChanged(DolphinViewContainer *viewContainer)
{
    DolphinViewContainer *oldViewContainer = m_activeViewContainer;
    Q_ASSERT(viewContainer);

    m_activeViewContainer = viewContainer;

    if (oldViewContainer) {
        const QAction *toggleSearchAction = actionCollection()->action(QStringLiteral("toggle_search"));
        toggleSearchAction->disconnect(oldViewContainer);

        // Disconnect all signals between the old view container (container,
        // view and url navigator) and main window.
        oldViewContainer->disconnect(this);
        oldViewContainer->view()->disconnect(this);
        oldViewContainer->urlNavigatorInternalWithHistory()->disconnect(this);
        auto navigators = static_cast<DolphinNavigatorsWidgetAction *>(actionCollection()->action(QStringLiteral("url_navigators")));
        navigators->primaryUrlNavigator()->disconnect(this);
        if (auto secondaryUrlNavigator = navigators->secondaryUrlNavigator()) {
            secondaryUrlNavigator->disconnect(this);
        }

        // except the requestItemInfo so that on hover the information panel can still be updated
        connect(oldViewContainer->view(), &DolphinView::requestItemInfo, this, &DolphinMainWindow::requestItemInfo);

        // Disconnect other slots.
        disconnect(oldViewContainer,
                   &DolphinViewContainer::selectionModeChanged,
                   actionCollection()->action(QStringLiteral("toggle_selection_mode")),
                   &QAction::setChecked);
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

void DolphinMainWindow::slotStorageTearDownFromPlacesRequested(const QString &mountPath)
{
    connect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, this, [this, mountPath]() {
        setViewsToHomeIfMountPathOpen(mountPath);
    });

    if (m_terminalPanel && m_terminalPanel->currentWorkingDirectoryIsChildOf(mountPath)) {
        m_tearDownFromPlacesRequested = true;
        m_terminalPanel->goHome();
        // m_placesPanel->proceedWithTearDown() will be called in slotTerminalDirectoryChanged
    } else {
        m_placesPanel->proceedWithTearDown();
    }
}

void DolphinMainWindow::slotStorageTearDownExternallyRequested(const QString &mountPath)
{
    connect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, this, [this, mountPath]() {
        setViewsToHomeIfMountPathOpen(mountPath);
    });

    if (m_terminalPanel && m_terminalPanel->currentWorkingDirectoryIsChildOf(mountPath)) {
        m_tearDownFromPlacesRequested = false;
        m_terminalPanel->goHome();
    }
}

void DolphinMainWindow::slotKeyBindings()
{
    KShortcutsDialog dialog(KShortcutsEditor::AllActions, KShortcutsEditor::LetterShortcutsAllowed, this);
    dialog.addCollection(actionCollection());
    if (m_terminalPanel) {
        KActionCollection *konsolePartActionCollection = m_terminalPanel->actionCollection();
        if (konsolePartActionCollection) {
            dialog.addCollection(konsolePartActionCollection, QStringLiteral("KonsolePart"));
        }
    }
    dialog.configure();
}

void DolphinMainWindow::setViewsToHomeIfMountPathOpen(const QString &mountPath)
{
    const QVector<DolphinViewContainer *> theViewContainers = viewContainers();
    for (DolphinViewContainer *viewContainer : theViewContainers) {
        if (viewContainer && viewContainer->url().toLocalFile().startsWith(mountPath)) {
            viewContainer->setUrl(QUrl::fromLocalFile(QDir::homePath()));
        }
    }
    disconnect(m_placesPanel, &PlacesPanel::storageTearDownSuccessful, nullptr, nullptr);
}

void DolphinMainWindow::setupActions()
{
    auto hamburgerMenuAction = KStandardAction::hamburgerMenu(nullptr, nullptr, actionCollection());

    // setup 'File' menu
    m_newFileMenu = new DolphinNewFileMenu(nullptr, this);
    actionCollection()->addAction(QStringLiteral("new_menu"), m_newFileMenu);
    QMenu *menu = m_newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(QIcon::fromTheme(QStringLiteral("list-add")));
    m_newFileMenu->setPopupMode(QToolButton::InstantPopup);
    connect(menu, &QMenu::aboutToShow, this, &DolphinMainWindow::updateNewMenu);

    QAction *newWindow = KStandardAction::openNew(this, &DolphinMainWindow::openNewMainWindow, actionCollection());
    newWindow->setText(i18nc("@action:inmenu File", "New &Window"));
    newWindow->setToolTip(i18nc("@info", "Open a new Dolphin window"));
    newWindow->setWhatsThis(xi18nc("@info:whatsthis",
                                   "This opens a new "
                                   "window just like this one with the current location and view."
                                   "<nl/>You can drag and drop items between windows."));
    newWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));

    QAction *newTab = actionCollection()->addAction(QStringLiteral("new_tab"));
    newTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    newTab->setText(i18nc("@action:inmenu File", "New Tab"));
    newTab->setWhatsThis(xi18nc("@info:whatsthis",
                                "This opens a new "
                                "<emphasis>Tab</emphasis> with the current location and view.<nl/>"
                                "A tab is an additional view within this window. "
                                "You can drag and drop items between tabs."));
    actionCollection()->setDefaultShortcut(newTab, Qt::CTRL | Qt::Key_T);
    connect(newTab, &QAction::triggered, this, &DolphinMainWindow::openNewActivatedTab);

    QAction *addToPlaces = actionCollection()->addAction(QStringLiteral("add_to_places"));
    addToPlaces->setIcon(QIcon::fromTheme(QStringLiteral("bookmark-new")));
    addToPlaces->setText(i18nc("@action:inmenu Add current folder to places", "Add to Places"));
    addToPlaces->setWhatsThis(xi18nc("@info:whatsthis",
                                     "This adds the selected folder "
                                     "to the Places panel."));
    connect(addToPlaces, &QAction::triggered, this, &DolphinMainWindow::addToPlaces);

    QAction *closeTab = KStandardAction::close(m_tabWidget, QOverload<>::of(&DolphinTabWidget::closeTab), actionCollection());
    closeTab->setText(i18nc("@action:inmenu File", "Close Tab"));
    closeTab->setWhatsThis(i18nc("@info:whatsthis",
                                 "This closes the "
                                 "currently viewed tab. If no more tabs are left this window "
                                 "will close instead."));

    QAction *quitAction = KStandardAction::quit(this, &DolphinMainWindow::quit, actionCollection());
    quitAction->setWhatsThis(i18nc("@info:whatsthis quit", "This closes this window."));

    // setup 'Edit' menu
    KStandardAction::undo(this, &DolphinMainWindow::undo, actionCollection());

    // i18n: This will be the last paragraph for the whatsthis for all three:
    // Cut, Copy and Paste
    const QString cutCopyPastePara = xi18nc("@info:whatsthis",
                                            "<para><emphasis>Cut, "
                                            "Copy</emphasis> and <emphasis>Paste</emphasis> work between many "
                                            "applications and are among the most used commands. That's why their "
                                            "<emphasis>keyboard shortcuts</emphasis> are prominently placed right "
                                            "next to each other on the keyboard: <shortcut>Ctrl+X</shortcut>, "
                                            "<shortcut>Ctrl+C</shortcut> and <shortcut>Ctrl+V</shortcut>.</para>");
    QAction *cutAction = KStandardAction::cut(this, &DolphinMainWindow::cut, actionCollection());
    m_actionTextHelper->registerTextWhenNothingIsSelected(cutAction, i18nc("@action", "Cut…"));
    cutAction->setWhatsThis(xi18nc("@info:whatsthis cut",
                                   "This copies the items "
                                   "in your current selection to the <emphasis>clipboard</emphasis>.<nl/>"
                                   "Use the <emphasis>Paste</emphasis> action afterwards to copy them from "
                                   "the clipboard to a new location. The items will be removed from their "
                                   "initial location.")
                            + cutCopyPastePara);
    QAction *copyAction = KStandardAction::copy(this, &DolphinMainWindow::copy, actionCollection());
    m_actionTextHelper->registerTextWhenNothingIsSelected(copyAction, i18nc("@action", "Copy…"));
    copyAction->setWhatsThis(xi18nc("@info:whatsthis copy",
                                    "This copies the "
                                    "items in your current selection to the <emphasis>clipboard</emphasis>."
                                    "<nl/>Use the <emphasis>Paste</emphasis> action afterwards to copy them "
                                    "from the clipboard to a new location.")
                             + cutCopyPastePara);
    QAction *paste = KStandardAction::paste(this, &DolphinMainWindow::paste, actionCollection());
    // The text of the paste-action is modified dynamically by Dolphin
    // (e. g. to "Paste One Folder"). To prevent that the size of the toolbar changes
    // due to the long text, the text "Paste" is used:
    paste->setIconText(i18nc("@action:inmenu Edit", "Paste"));
    paste->setWhatsThis(xi18nc("@info:whatsthis paste",
                               "This copies the items from "
                               "your <emphasis>clipboard</emphasis> to the currently viewed folder.<nl/>"
                               "If the items were added to the clipboard by the <emphasis>Cut</emphasis> "
                               "action they are removed from their old location.")
                        + cutCopyPastePara);

    QAction *copyToOtherViewAction = actionCollection()->addAction(QStringLiteral("copy_to_inactive_split_view"));
    copyToOtherViewAction->setText(i18nc("@action:inmenu", "Copy to Other View"));
    m_actionTextHelper->registerTextWhenNothingIsSelected(copyToOtherViewAction, i18nc("@action:inmenu", "Copy to Other View…"));
    copyToOtherViewAction->setWhatsThis(xi18nc("@info:whatsthis Copy",
                                               "This copies the selected items from "
                                               "the <emphasis>active</emphasis> view to the inactive split view."));
    copyToOtherViewAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-copy")));
    copyToOtherViewAction->setIconText(i18nc("@action:inmenu Edit", "Copy to Inactive Split View"));
    actionCollection()->setDefaultShortcut(copyToOtherViewAction, Qt::SHIFT | Qt::Key_F5);
    connect(copyToOtherViewAction, &QAction::triggered, this, &DolphinMainWindow::copyToInactiveSplitView);

    QAction *moveToOtherViewAction = actionCollection()->addAction(QStringLiteral("move_to_inactive_split_view"));
    moveToOtherViewAction->setText(i18nc("@action:inmenu", "Move to Other View"));
    m_actionTextHelper->registerTextWhenNothingIsSelected(moveToOtherViewAction, i18nc("@action:inmenu", "Move to Other View…"));
    moveToOtherViewAction->setWhatsThis(xi18nc("@info:whatsthis Move",
                                               "This moves the selected items from "
                                               "the <emphasis>active</emphasis> view to the inactive split view."));
    moveToOtherViewAction->setIcon(QIcon::fromTheme(QStringLiteral("edit-cut")));
    moveToOtherViewAction->setIconText(i18nc("@action:inmenu Edit", "Move to Inactive Split View"));
    actionCollection()->setDefaultShortcut(moveToOtherViewAction, Qt::SHIFT | Qt::Key_F6);
    connect(moveToOtherViewAction, &QAction::triggered, this, &DolphinMainWindow::moveToInactiveSplitView);

    QAction *showFilterBar = actionCollection()->addAction(QStringLiteral("show_filter_bar"));
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Filter…"));
    showFilterBar->setToolTip(i18nc("@info:tooltip", "Show Filter Bar"));
    showFilterBar->setWhatsThis(xi18nc("@info:whatsthis",
                                       "This opens the "
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
    searchAction->setText(i18n("Search…"));
    searchAction->setToolTip(i18nc("@info:tooltip", "Search for files and folders"));
    searchAction->setWhatsThis(xi18nc("@info:whatsthis find",
                                      "<para>This helps you "
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

    QAction *toggleSelectionModeAction = actionCollection()->addAction(QStringLiteral("toggle_selection_mode"));
    // i18n: This action toggles a selection mode.
    toggleSelectionModeAction->setText(i18nc("@action:inmenu", "Select Files and Folders"));
    // i18n: Opens a selection mode for selecting files/folders.
    // The text is kept so unspecific because it will be shown on the toolbar where space is at a premium.
    toggleSelectionModeAction->setIconText(i18nc("@action:intoolbar", "Select"));
    toggleSelectionModeAction->setWhatsThis(xi18nc(
        "@info:whatsthis",
        "<para>This application only knows which files or folders should be acted on if they are"
        " <emphasis>selected</emphasis> first. Press this to toggle a <emphasis>Selection Mode</emphasis> which makes selecting and deselecting as easy as "
        "pressing an item once.</para><para>While in this mode, a quick access bar at the bottom shows available actions for the currently selected items."
        "</para>"));
    toggleSelectionModeAction->setIcon(QIcon::fromTheme(QStringLiteral("quickwizard")));
    toggleSelectionModeAction->setCheckable(true);
    actionCollection()->setDefaultShortcut(toggleSelectionModeAction, Qt::Key_Space );
    connect(toggleSelectionModeAction, &QAction::triggered, this, &DolphinMainWindow::toggleSelectionMode);

    // A special version of the toggleSelectionModeAction for the toolbar that also contains a menu
    // with the selectAllAction and invertSelectionAction.
    auto *toggleSelectionModeToolBarAction =
        new KToolBarPopupAction(toggleSelectionModeAction->icon(), toggleSelectionModeAction->iconText(), actionCollection());
    toggleSelectionModeToolBarAction->setToolTip(toggleSelectionModeAction->text());
    toggleSelectionModeToolBarAction->setWhatsThis(toggleSelectionModeAction->whatsThis());
    actionCollection()->addAction(QStringLiteral("toggle_selection_mode_tool_bar"), toggleSelectionModeToolBarAction);
    toggleSelectionModeToolBarAction->setCheckable(true);
    toggleSelectionModeToolBarAction->setPopupMode(KToolBarPopupAction::DelayedPopup);
    connect(toggleSelectionModeToolBarAction, &QAction::triggered, toggleSelectionModeAction, &QAction::trigger);
    connect(toggleSelectionModeAction, &QAction::toggled, toggleSelectionModeToolBarAction, &QAction::setChecked);

    QAction *selectAllAction = KStandardAction::selectAll(this, &DolphinMainWindow::selectAll, actionCollection());
    selectAllAction->setWhatsThis(xi18nc("@info:whatsthis",
                                         "This selects all "
                                         "files and folders in the current location."));

    QAction *invertSelection = actionCollection()->addAction(QStringLiteral("invert_selection"));
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    invertSelection->setWhatsThis(xi18nc("@info:whatsthis invert",
                                         "This selects all "
                                         "objects that you have currently <emphasis>not</emphasis> selected instead."));
    invertSelection->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-invert")));
    actionCollection()->setDefaultShortcut(invertSelection, Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, &QAction::triggered, this, &DolphinMainWindow::invertSelection);

    QMenu *toggleSelectionModeActionMenu = new QMenu(this);
    toggleSelectionModeActionMenu->addAction(selectAllAction);
    toggleSelectionModeActionMenu->addAction(invertSelection);
    toggleSelectionModeToolBarAction->setMenu(toggleSelectionModeActionMenu);

    // setup 'View' menu
    // (note that most of it is set up in DolphinViewActionHandler)

    QAction *split = actionCollection()->addAction(QStringLiteral("split_view"));
    split->setWhatsThis(xi18nc("@info:whatsthis find",
                               "<para>This splits "
                               "the folder view below into two autonomous views.</para><para>This "
                               "way you can see two locations at once and move items between them "
                               "quickly.</para>Click this again afterwards to recombine the views."));
    actionCollection()->setDefaultShortcut(split, Qt::Key_F3);
    connect(split, &QAction::triggered, this, &DolphinMainWindow::toggleSplitView);

    QAction *stashSplit = actionCollection()->addAction(QStringLiteral("split_stash"));
    actionCollection()->setDefaultShortcut(stashSplit, Qt::CTRL | Qt::Key_S);
    stashSplit->setText(i18nc("@action:intoolbar Stash", "Stash"));
    stashSplit->setToolTip(i18nc("@info", "Opens the stash virtual directory in a split window"));
    stashSplit->setIcon(QIcon::fromTheme(QStringLiteral("folder-stash")));
    stashSplit->setCheckable(false);
    QDBusConnectionInterface *sessionInterface = QDBusConnection::sessionBus().interface();
    stashSplit->setVisible(sessionInterface && sessionInterface->isServiceRegistered(QStringLiteral("org.kde.kio.StashNotifier")));
    connect(stashSplit, &QAction::triggered, this, &DolphinMainWindow::toggleSplitStash);

    QAction *redisplay = KStandardAction::redisplay(this, &DolphinMainWindow::reloadView, actionCollection());
    redisplay->setToolTip(i18nc("@info:tooltip", "Refresh view"));
    redisplay->setWhatsThis(xi18nc("@info:whatsthis refresh",
                                   "<para>This refreshes "
                                   "the folder view.</para>"
                                   "<para>If the contents of this folder have changed, refreshing will re-scan this folder "
                                   "and show you a newly-updated view of the files and folders contained here.</para>"
                                   "<para>If the view is split, this refreshes the one that is currently in focus.</para>"));

    QAction *stop = actionCollection()->addAction(QStringLiteral("stop"));
    stop->setText(i18nc("@action:inmenu View", "Stop"));
    stop->setToolTip(i18nc("@info", "Stop loading"));
    stop->setWhatsThis(i18nc("@info", "This stops the loading of the contents of the current folder."));
    stop->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    connect(stop, &QAction::triggered, this, &DolphinMainWindow::stopLoading);

    KToggleAction *editableLocation = actionCollection()->add<KToggleAction>(QStringLiteral("editable_location"));
    editableLocation->setText(i18nc("@action:inmenu Navigation Bar", "Editable Location"));
    editableLocation->setWhatsThis(xi18nc("@info:whatsthis",
                                          "This toggles the <emphasis>Location Bar</emphasis> to be "
                                          "editable so you can directly enter a location you want to go to.<nl/>"
                                          "You can also switch to editing by clicking to the right of the "
                                          "location and switch back by confirming the edited location."));
    actionCollection()->setDefaultShortcut(editableLocation, Qt::Key_F6);
    connect(editableLocation, &KToggleAction::triggered, this, &DolphinMainWindow::toggleEditLocation);

    QAction *replaceLocation = actionCollection()->addAction(QStringLiteral("replace_location"));
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    // i18n: "enter" is used both in the meaning of "writing" and "going to" a new location here.
    // Both meanings are useful but not necessary to understand the use of "Replace Location".
    // So you might want to be more verbose in your language to convey the meaning but it's up to you.
    replaceLocation->setWhatsThis(xi18nc("@info:whatsthis",
                                         "This switches to editing the location and selects it "
                                         "so you can quickly enter a different location."));
    actionCollection()->setDefaultShortcuts(replaceLocation, {Qt::CTRL | Qt::Key_L, Qt::ALT | Qt::Key_D});
    connect(replaceLocation, &QAction::triggered, this, &DolphinMainWindow::replaceLocation);

    // setup 'Go' menu
    {
        QScopedPointer<QAction> backAction(KStandardAction::back(nullptr, nullptr, nullptr));
        m_backAction = new KToolBarPopupAction(backAction->icon(), backAction->text(), actionCollection());
        m_backAction->setObjectName(backAction->objectName());
        m_backAction->setShortcuts(backAction->shortcuts());
    }
    m_backAction->setPopupMode(KToolBarPopupAction::DelayedPopup);
    connect(m_backAction, &QAction::triggered, this, &DolphinMainWindow::goBack);
    connect(m_backAction->popupMenu(), &QMenu::aboutToShow, this, &DolphinMainWindow::slotAboutToShowBackPopupMenu);
    connect(m_backAction->popupMenu(), &QMenu::triggered, this, &DolphinMainWindow::slotGoBack);
    actionCollection()->addAction(m_backAction->objectName(), m_backAction);

    auto backShortcuts = m_backAction->shortcuts();
    // Prepend this shortcut, to avoid being hidden by the two-slot UI (#371130)
    backShortcuts.prepend(QKeySequence(Qt::Key_Backspace));
    actionCollection()->setDefaultShortcuts(m_backAction, backShortcuts);

    DolphinRecentTabsMenu *recentTabsMenu = new DolphinRecentTabsMenu(this);
    actionCollection()->addAction(QStringLiteral("closed_tabs"), recentTabsMenu);
    connect(m_tabWidget, &DolphinTabWidget::rememberClosedTab, recentTabsMenu, &DolphinRecentTabsMenu::rememberClosedTab);
    connect(recentTabsMenu, &DolphinRecentTabsMenu::restoreClosedTab, m_tabWidget, &DolphinTabWidget::restoreClosedTab);
    connect(recentTabsMenu, &DolphinRecentTabsMenu::closedTabsCountChanged, this, &DolphinMainWindow::closedTabsCountChanged);

    QAction *undoCloseTab = actionCollection()->addAction(QStringLiteral("undo_close_tab"));
    undoCloseTab->setText(i18nc("@action:inmenu File", "Undo close tab"));
    undoCloseTab->setWhatsThis(i18nc("@info:whatsthis undo close tab", "This returns you to the previously closed tab."));
    actionCollection()->setDefaultShortcut(undoCloseTab, Qt::CTRL | Qt::SHIFT | Qt::Key_T);
    undoCloseTab->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    undoCloseTab->setEnabled(false);
    connect(undoCloseTab, &QAction::triggered, recentTabsMenu, &DolphinRecentTabsMenu::undoCloseTab);

    auto undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    undoAction->setWhatsThis(xi18nc("@info:whatsthis",
                                    "This undoes "
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
    m_forwardAction->setPopupMode(KToolBarPopupAction::DelayedPopup);
    connect(m_forwardAction, &QAction::triggered, this, &DolphinMainWindow::goForward);
    connect(m_forwardAction->popupMenu(), &QMenu::aboutToShow, this, &DolphinMainWindow::slotAboutToShowForwardPopupMenu);
    connect(m_forwardAction->popupMenu(), &QMenu::triggered, this, &DolphinMainWindow::slotGoForward);
    actionCollection()->addAction(m_forwardAction->objectName(), m_forwardAction);
    actionCollection()->setDefaultShortcuts(m_forwardAction, m_forwardAction->shortcuts());

    // enable middle-click to open in a new tab
    auto *middleClickEventFilter = new MiddleClickActionEventFilter(this);
    connect(middleClickEventFilter, &MiddleClickActionEventFilter::actionMiddleClicked, this, &DolphinMainWindow::slotBackForwardActionMiddleClicked);
    m_backAction->popupMenu()->installEventFilter(middleClickEventFilter);
    m_forwardAction->popupMenu()->installEventFilter(middleClickEventFilter);
    KStandardAction::up(this, &DolphinMainWindow::goUp, actionCollection());
    QAction *homeAction = KStandardAction::home(this, &DolphinMainWindow::goHome, actionCollection());
    homeAction->setWhatsThis(xi18nc("@info:whatsthis",
                                    "Go to your "
                                    "<filename>Home</filename> folder.<nl/>Every user account "
                                    "has their own <filename>Home</filename> that contains their data "
                                    "including folders that contain personal application data."));

    // setup 'Tools' menu
    QAction *compareFiles = actionCollection()->addAction(QStringLiteral("compare_files"));
    compareFiles->setText(i18nc("@action:inmenu Tools", "Compare Files"));
    compareFiles->setIcon(QIcon::fromTheme(QStringLiteral("kompare")));
    compareFiles->setEnabled(false);
    connect(compareFiles, &QAction::triggered, this, &DolphinMainWindow::compareFiles);

    QAction *openPreferredSearchTool = actionCollection()->addAction(QStringLiteral("open_preferred_search_tool"));
    openPreferredSearchTool->setText(i18nc("@action:inmenu Tools", "Open Preferred Search Tool"));
    openPreferredSearchTool->setWhatsThis(xi18nc("@info:whatsthis",
                                                 "<para>This opens a preferred search tool for the viewed location.</para>"
                                                 "<para>Use <emphasis>More Search Tools</emphasis> menu to configure it.</para>"));
    openPreferredSearchTool->setIcon(QIcon::fromTheme(QStringLiteral("search")));
    actionCollection()->setDefaultShortcut(openPreferredSearchTool, Qt::CTRL | Qt::SHIFT | Qt::Key_F);
    connect(openPreferredSearchTool, &QAction::triggered, this, &DolphinMainWindow::openPreferredSearchTool);

    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        QAction *openTerminal = actionCollection()->addAction(QStringLiteral("open_terminal"));
        openTerminal->setText(i18nc("@action:inmenu Tools", "Open Terminal"));
        openTerminal->setWhatsThis(xi18nc("@info:whatsthis",
                                          "<para>This opens a <emphasis>terminal</emphasis> application for the viewed location.</para>"
                                          "<para>To learn more about terminals use the help in the terminal application.</para>"));
        openTerminal->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
        actionCollection()->setDefaultShortcut(openTerminal, Qt::SHIFT | Qt::Key_F4);
        connect(openTerminal, &QAction::triggered, this, &DolphinMainWindow::openTerminal);

        QAction *openTerminalHere = actionCollection()->addAction(QStringLiteral("open_terminal_here"));
        // i18n: "Here" refers to the location(s) of the currently selected item(s) or the currently viewed location if nothing is selected.
        openTerminalHere->setText(i18nc("@action:inmenu Tools", "Open Terminal Here"));
        openTerminalHere->setWhatsThis(xi18nc("@info:whatsthis",
                                              "<para>This opens <emphasis>terminal</emphasis> applications for the selected items' locations.</para>"
                                              "<para>To learn more about terminals use the help in the terminal application.</para>"));
        openTerminalHere->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
        actionCollection()->setDefaultShortcut(openTerminalHere, Qt::SHIFT | Qt::ALT | Qt::Key_F4);
        connect(openTerminalHere, &QAction::triggered, this, &DolphinMainWindow::openTerminalHere);

#if HAVE_TERMINAL
        QAction *focusTerminalPanel = actionCollection()->addAction(QStringLiteral("focus_terminal_panel"));
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
    bookmarkMenu->setPopupMode(QToolButton::InstantPopup);
    m_bookmarkHandler = new DolphinBookmarkHandler(this, actionCollection(), bookmarkMenu->menu(), this);
    actionCollection()->addAction(QStringLiteral("bookmarks"), bookmarkMenu);

    // setup 'Settings' menu
    KToggleAction *showMenuBar = KStandardAction::showMenubar(nullptr, nullptr, actionCollection());
    showMenuBar->setWhatsThis(xi18nc("@info:whatsthis",
                                     "<para>This switches between having a <emphasis>Menubar</emphasis> "
                                     "and having a <interface>%1</interface> button. Both "
                                     "contain mostly the same actions and configuration options.</para>"
                                     "<para>The Menubar takes up more space but allows for fast and organised access to all "
                                     "actions an application has to offer.</para><para>The <interface>%1</interface> button "
                                     "is simpler and small which makes triggering advanced actions more time consuming.</para>",
                                     hamburgerMenuAction->text().replace('&', "")));
    connect(showMenuBar,
            &KToggleAction::triggered, // Fixes #286822
            this,
            &DolphinMainWindow::toggleShowMenuBar,
            Qt::QueuedConnection);

    KToggleAction *showStatusBar = KStandardAction::showStatusbar(nullptr, nullptr, actionCollection());
    showStatusBar->setChecked(GeneralSettings::showStatusBar());
    connect(GeneralSettings::self(), &GeneralSettings::showStatusBarChanged, showStatusBar, &KToggleAction::setChecked);
    connect(showStatusBar, &KToggleAction::triggered, this, [this](bool checked) {
        GeneralSettings::setShowStatusBar(checked);
        refreshViews();
    });

    KStandardAction::keyBindings(this, &DolphinMainWindow::slotKeyBindings, actionCollection());
    KStandardAction::preferences(this, &DolphinMainWindow::editSettings, actionCollection());

    // not in menu actions
    QList<QKeySequence> nextTabKeys = KStandardShortcut::tabNext();
    nextTabKeys.append(QKeySequence(Qt::CTRL | Qt::Key_Tab));

    QList<QKeySequence> prevTabKeys = KStandardShortcut::tabPrev();
    prevTabKeys.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));

    for (int i = 0; i < MaxActivateTabShortcuts; ++i) {
        QAction *activateTab = actionCollection()->addAction(QStringLiteral("activate_tab_%1").arg(i));
        activateTab->setText(i18nc("@action:inmenu", "Activate Tab %1", i + 1));
        activateTab->setEnabled(false);
        connect(activateTab, &QAction::triggered, this, [this, i]() {
            m_tabWidget->activateTab(i);
        });

        // only add default shortcuts for the first 9 tabs regardless of MaxActivateTabShortcuts
        if (i < 9) {
            actionCollection()->setDefaultShortcut(activateTab, QStringLiteral("Alt+%1").arg(i + 1));
        }
    }

    QAction *activateLastTab = actionCollection()->addAction(QStringLiteral("activate_last_tab"));
    activateLastTab->setText(i18nc("@action:inmenu", "Activate Last Tab"));
    activateLastTab->setEnabled(false);
    connect(activateLastTab, &QAction::triggered, m_tabWidget, &DolphinTabWidget::activateLastTab);
    actionCollection()->setDefaultShortcut(activateLastTab, Qt::ALT | Qt::Key_0);

    QAction *activateNextTab = actionCollection()->addAction(QStringLiteral("activate_next_tab"));
    activateNextTab->setIconText(i18nc("@action:inmenu", "Next Tab"));
    activateNextTab->setText(i18nc("@action:inmenu", "Activate Next Tab"));
    activateNextTab->setEnabled(false);
    connect(activateNextTab, &QAction::triggered, m_tabWidget, &DolphinTabWidget::activateNextTab);
    actionCollection()->setDefaultShortcuts(activateNextTab, nextTabKeys);

    QAction *activatePrevTab = actionCollection()->addAction(QStringLiteral("activate_prev_tab"));
    activatePrevTab->setIconText(i18nc("@action:inmenu", "Previous Tab"));
    activatePrevTab->setText(i18nc("@action:inmenu", "Activate Previous Tab"));
    activatePrevTab->setEnabled(false);
    connect(activatePrevTab, &QAction::triggered, m_tabWidget, &DolphinTabWidget::activatePrevTab);
    actionCollection()->setDefaultShortcuts(activatePrevTab, prevTabKeys);

    // for context menu
    QAction *showTarget = actionCollection()->addAction(QStringLiteral("show_target"));
    showTarget->setText(i18nc("@action:inmenu", "Show Target"));
    showTarget->setIcon(QIcon::fromTheme(QStringLiteral("document-open-folder")));
    showTarget->setEnabled(false);
    connect(showTarget, &QAction::triggered, this, &DolphinMainWindow::showTarget);

    QAction *openInNewTab = actionCollection()->addAction(QStringLiteral("open_in_new_tab"));
    openInNewTab->setText(i18nc("@action:inmenu", "Open in New Tab"));
    openInNewTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(openInNewTab, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction *openInNewTabs = actionCollection()->addAction(QStringLiteral("open_in_new_tabs"));
    openInNewTabs->setText(i18nc("@action:inmenu", "Open in New Tabs"));
    openInNewTabs->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    connect(openInNewTabs, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction *openInNewWindow = actionCollection()->addAction(QStringLiteral("open_in_new_window"));
    openInNewWindow->setText(i18nc("@action:inmenu", "Open in New Window"));
    openInNewWindow->setIcon(QIcon::fromTheme(QStringLiteral("window-new")));
    connect(openInNewWindow, &QAction::triggered, this, &DolphinMainWindow::openInNewWindow);

    QAction *openInSplitViewAction = actionCollection()->addAction(QStringLiteral("open_in_split_view"));
    openInSplitViewAction->setText(i18nc("@action:inmenu", "Open in Split View"));
    openInSplitViewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-right-new")));
    connect(openInSplitViewAction, &QAction::triggered, this, [this]() {
        openInSplitView(QUrl());
    });
}

void DolphinMainWindow::setupDockWidgets()
{
    const bool lock = GeneralSettings::lockPanels();

    DolphinPlacesModelSingleton::instance().placesModel()->setPanelsLocked(lock);

    KDualAction *lockLayoutAction = actionCollection()->add<KDualAction>(QStringLiteral("lock_panels"));
    lockLayoutAction->setActiveText(i18nc("@action:inmenu Panels", "Unlock Panels"));
    lockLayoutAction->setActiveIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
    lockLayoutAction->setInactiveText(i18nc("@action:inmenu Panels", "Lock Panels"));
    lockLayoutAction->setInactiveIcon(QIcon::fromTheme(QStringLiteral("object-locked")));
    lockLayoutAction->setWhatsThis(xi18nc("@info:whatsthis",
                                          "This "
                                          "switches between having panels <emphasis>locked</emphasis> or "
                                          "<emphasis>unlocked</emphasis>.<nl/>Unlocked panels can be "
                                          "dragged to the other side of the window and have a close "
                                          "button.<nl/>Locked panels are embedded more cleanly."));
    lockLayoutAction->setActive(lock);
    connect(lockLayoutAction, &KDualAction::triggered, this, &DolphinMainWindow::togglePanelLockState);

    // Setup "Information"
    DolphinDockWidget *infoDock = new DolphinDockWidget(i18nc("@title:window", "Information"));
    infoDock->setLocked(lock);
    infoDock->setObjectName(QStringLiteral("infoDock"));
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

#if HAVE_BALOO
    InformationPanel *infoPanel = new InformationPanel(infoDock);
    infoPanel->setCustomContextMenuActions({lockLayoutAction});
    connect(infoPanel, &InformationPanel::urlActivated, this, &DolphinMainWindow::handleUrl);
    infoDock->setWidget(infoPanel);

    createPanelAction(QIcon::fromTheme(QStringLiteral("dialog-information")), Qt::Key_F11, infoDock, QStringLiteral("show_information_panel"));

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, &DolphinMainWindow::urlChanged, infoPanel, &InformationPanel::setUrl);
    connect(this, &DolphinMainWindow::selectionChanged, infoPanel, &InformationPanel::setSelection);
    connect(this, &DolphinMainWindow::requestItemInfo, infoPanel, &InformationPanel::requestDelayedItemInfo);
    connect(this, &DolphinMainWindow::fileItemsChanged, infoPanel, &InformationPanel::slotFilesItemChanged);
#endif

    // i18n: This is the last paragraph for the "What's This"-texts of all four panels.
    const QString panelWhatsThis = xi18nc("@info:whatsthis",
                                          "<para>To show or "
                                          "hide panels like this go to <interface>Menu|Panels</interface> "
                                          "or <interface>View|Panels</interface>.</para>");
#if HAVE_BALOO
    actionCollection()
        ->action(QStringLiteral("show_information_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis",
                              "<para> This toggles the "
                              "<emphasis>information</emphasis> panel at the right side of the "
                              "window.</para><para>The panel provides in-depth information "
                              "about the items your mouse is hovering over or about the selected "
                              "items. Otherwise it informs you about the currently viewed folder.<nl/>"
                              "For single items a preview of their contents is provided.</para>"));
#endif
    infoDock->setWhatsThis(xi18nc("@info:whatsthis",
                                  "<para>This panel "
                                  "provides in-depth information about the items your mouse is "
                                  "hovering over or about the selected items. Otherwise it informs "
                                  "you about the currently viewed folder.<nl/>For single items a "
                                  "preview of their contents is provided.</para><para>You can configure "
                                  "which and how details are given here by right-clicking.</para>")
                           + panelWhatsThis);

    // Setup "Folders"
    DolphinDockWidget *foldersDock = new DolphinDockWidget(i18nc("@title:window", "Folders"));
    foldersDock->setLocked(lock);
    foldersDock->setObjectName(QStringLiteral("foldersDock"));
    foldersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    FoldersPanel *foldersPanel = new FoldersPanel(foldersDock);
    foldersPanel->setCustomContextMenuActions({lockLayoutAction});
    foldersDock->setWidget(foldersPanel);

    createPanelAction(QIcon::fromTheme(QStringLiteral("folder")), Qt::Key_F7, foldersDock, QStringLiteral("show_folders_panel"));

    addDockWidget(Qt::LeftDockWidgetArea, foldersDock);
    connect(this, &DolphinMainWindow::urlChanged, foldersPanel, &FoldersPanel::setUrl);
    connect(foldersPanel, &FoldersPanel::folderActivated, this, &DolphinMainWindow::changeUrl);
    connect(foldersPanel, &FoldersPanel::folderInNewTab, this, &DolphinMainWindow::openNewTab);
    connect(foldersPanel, &FoldersPanel::folderInNewActiveTab, this, &DolphinMainWindow::openNewTabAndActivate);
    connect(foldersPanel, &FoldersPanel::errorMessage, this, &DolphinMainWindow::showErrorMessage);

    actionCollection()
        ->action(QStringLiteral("show_folders_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis",
                              "This toggles the "
                              "<emphasis>folders</emphasis> panel at the left side of the window."
                              "<nl/><nl/>It shows the folders of the <emphasis>file system"
                              "</emphasis> in a <emphasis>tree view</emphasis>."));
    foldersDock->setWhatsThis(xi18nc("@info:whatsthis",
                                     "<para>This panel "
                                     "shows the folders of the <emphasis>file system</emphasis> in a "
                                     "<emphasis>tree view</emphasis>.</para><para>Click a folder to go "
                                     "there. Click the arrow to the left of a folder to see its subfolders. "
                                     "This allows quick switching between any folders.</para>")
                              + panelWhatsThis);

    // Setup "Terminal"
#if HAVE_TERMINAL
    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        DolphinDockWidget *terminalDock = new DolphinDockWidget(i18nc("@title:window Shell terminal", "Terminal"));
        terminalDock->setLocked(lock);
        terminalDock->setObjectName(QStringLiteral("terminalDock"));
        m_terminalPanel = new TerminalPanel(terminalDock);
        m_terminalPanel->setCustomContextMenuActions({lockLayoutAction});
        terminalDock->setWidget(m_terminalPanel);

        connect(m_terminalPanel, &TerminalPanel::hideTerminalPanel, terminalDock, &DolphinDockWidget::hide);
        connect(m_terminalPanel, &TerminalPanel::changeUrl, this, &DolphinMainWindow::slotTerminalDirectoryChanged);
        connect(terminalDock, &DolphinDockWidget::visibilityChanged, m_terminalPanel, &TerminalPanel::dockVisibilityChanged);
        connect(terminalDock, &DolphinDockWidget::visibilityChanged, this, &DolphinMainWindow::slotTerminalPanelVisibilityChanged);

        createPanelAction(QIcon::fromTheme(QStringLiteral("dialog-scripts")), Qt::Key_F4, terminalDock, QStringLiteral("show_terminal_panel"));

        addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
        connect(this, &DolphinMainWindow::urlChanged, m_terminalPanel, &TerminalPanel::setUrl);

        if (GeneralSettings::version() < 200) {
            terminalDock->hide();
        }

        actionCollection()
            ->action(QStringLiteral("show_terminal_panel"))
            ->setWhatsThis(xi18nc("@info:whatsthis",
                                  "<para>This toggles the "
                                  "<emphasis>terminal</emphasis> panel at the bottom of the window."
                                  "<nl/>The location in the terminal will always match the folder "
                                  "view so you can navigate using either.</para><para>The terminal "
                                  "panel is not needed for basic computer usage but can be useful "
                                  "for advanced tasks. To learn more about terminals use the help "
                                  "in a standalone terminal application like Konsole.</para>"));
        terminalDock->setWhatsThis(xi18nc("@info:whatsthis",
                                          "<para>This is "
                                          "the <emphasis>terminal</emphasis> panel. It behaves like a "
                                          "normal terminal but will match the location of the folder view "
                                          "so you can navigate using either.</para><para>The terminal panel "
                                          "is not needed for basic computer usage but can be useful for "
                                          "advanced tasks. To learn more about terminals use the help in a "
                                          "standalone terminal application like Konsole.</para>")
                                   + panelWhatsThis);
    }
#endif

    if (GeneralSettings::version() < 200) {
        infoDock->hide();
        foldersDock->hide();
    }

    // Setup "Places"
    DolphinDockWidget *placesDock = new DolphinDockWidget(i18nc("@title:window", "Places"));
    placesDock->setLocked(lock);
    placesDock->setObjectName(QStringLiteral("placesDock"));
    placesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    m_placesPanel = new PlacesPanel(placesDock);
    m_placesPanel->setCustomContextMenuActions({lockLayoutAction});
    placesDock->setWidget(m_placesPanel);

    createPanelAction(QIcon::fromTheme(QStringLiteral("compass")), Qt::Key_F9, placesDock, QStringLiteral("show_places_panel"));

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(m_placesPanel, &PlacesPanel::placeActivated, this, &DolphinMainWindow::slotPlaceActivated);
    connect(m_placesPanel, &PlacesPanel::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(m_placesPanel, &PlacesPanel::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);
    connect(m_placesPanel, &PlacesPanel::newWindowRequested, this, [this](const QUrl &url) {
        Dolphin::openNewWindow({url}, this);
    });
    connect(m_placesPanel, &PlacesPanel::openInSplitViewRequested, this, &DolphinMainWindow::openInSplitView);
    connect(m_placesPanel, &PlacesPanel::errorMessage, this, &DolphinMainWindow::showErrorMessage);
    connect(this, &DolphinMainWindow::urlChanged, m_placesPanel, &PlacesPanel::setUrl);
    connect(placesDock, &DolphinDockWidget::visibilityChanged, &DolphinUrlNavigatorsController::slotPlacesPanelVisibilityChanged);
    connect(this, &DolphinMainWindow::settingsChanged, m_placesPanel, &PlacesPanel::readSettings);
    connect(m_placesPanel, &PlacesPanel::storageTearDownRequested, this, &DolphinMainWindow::slotStorageTearDownFromPlacesRequested);
    connect(m_placesPanel, &PlacesPanel::storageTearDownExternallyRequested, this, &DolphinMainWindow::slotStorageTearDownExternallyRequested);
    DolphinUrlNavigatorsController::slotPlacesPanelVisibilityChanged(m_placesPanel->isVisible());

    auto actionShowAllPlaces = new QAction(QIcon::fromTheme(QStringLiteral("view-hidden")), i18nc("@item:inmenu", "Show Hidden Places"), this);
    actionShowAllPlaces->setCheckable(true);
    actionShowAllPlaces->setDisabled(true);
    actionShowAllPlaces->setWhatsThis(i18nc("@info:whatsthis",
                                            "This displays "
                                            "all places in the places panel that have been hidden. They will "
                                            "appear semi-transparent unless you uncheck their hide property."));

    connect(actionShowAllPlaces, &QAction::triggered, this, [actionShowAllPlaces, this](bool checked) {
        m_placesPanel->setShowAll(checked);
    });
    connect(m_placesPanel, &PlacesPanel::allPlacesShownChanged, actionShowAllPlaces, &QAction::setChecked);

    actionCollection()
        ->action(QStringLiteral("show_places_panel"))
        ->setWhatsThis(xi18nc("@info:whatsthis",
                              "<para>This toggles the "
                              "<emphasis>places</emphasis> panel at the left side of the window."
                              "</para><para>It allows you to go to locations you have "
                              "bookmarked and to access disk or media attached to the computer "
                              "or to the network. It also contains sections to find recently "
                              "saved files or files of a certain type.</para>"));
    placesDock->setWhatsThis(xi18nc("@info:whatsthis",
                                    "<para>This is the "
                                    "<emphasis>Places</emphasis> panel. It allows you to go to locations "
                                    "you have bookmarked and to access disk or media attached to the "
                                    "computer or to the network. It also contains sections to find "
                                    "recently saved files or files of a certain type.</para><para>"
                                    "Click on an entry to go there. Click with the right mouse button "
                                    "instead to open any entry in a new tab or new window.</para>"
                                    "<para>New entries can be added by dragging folders onto this panel. "
                                    "Right-click any section or entry to hide it. Right-click an empty "
                                    "space on this panel and select <interface>Show Hidden Places"
                                    "</interface> to display it again.</para>")
                             + panelWhatsThis);

    // Add actions into the "Panels" menu
    KActionMenu *panelsMenu = new KActionMenu(i18nc("@action:inmenu View", "Show Panels"), this);
    actionCollection()->addAction(QStringLiteral("panels"), panelsMenu);
    panelsMenu->setIcon(QIcon::fromTheme(QStringLiteral("view-sidetree")));
    panelsMenu->setPopupMode(QToolButton::InstantPopup);
    const KActionCollection *ac = actionCollection();
    panelsMenu->addAction(ac->action(QStringLiteral("show_places_panel")));
#if HAVE_BALOO
    panelsMenu->addAction(ac->action(QStringLiteral("show_information_panel")));
#endif
    panelsMenu->addAction(ac->action(QStringLiteral("show_folders_panel")));
    panelsMenu->addAction(ac->action(QStringLiteral("show_terminal_panel")));
    panelsMenu->addSeparator();
    panelsMenu->addAction(actionShowAllPlaces);
    panelsMenu->addAction(lockLayoutAction);

    connect(panelsMenu->menu(), &QMenu::aboutToShow, this, [actionShowAllPlaces, this] {
        actionShowAllPlaces->setEnabled(DolphinPlacesModelSingleton::instance().placesModel()->hiddenCount());
    });
}

void DolphinMainWindow::updateFileAndEditActions()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    const KActionCollection *col = actionCollection();
    KFileItemListProperties capabilitiesSource(list);

    QAction *renameAction = col->action(KStandardAction::name(KStandardAction::RenameFile));
    QAction *moveToTrashAction = col->action(KStandardAction::name(KStandardAction::MoveToTrash));
    QAction *deleteAction = col->action(KStandardAction::name(KStandardAction::DeleteFile));
    QAction *cutAction = col->action(KStandardAction::name(KStandardAction::Cut));
    QAction *duplicateAction = col->action(QStringLiteral("duplicate")); // see DolphinViewActionHandler
    QAction *addToPlacesAction = col->action(QStringLiteral("add_to_places"));
    QAction *copyToOtherViewAction = col->action(QStringLiteral("copy_to_inactive_split_view"));
    QAction *moveToOtherViewAction = col->action(QStringLiteral("move_to_inactive_split_view"));
    QAction *copyLocation = col->action(QString("copy_location"));

    if (list.isEmpty()) {
        stateChanged(QStringLiteral("has_no_selection"));

        // All actions that need a selection to function can be enabled because they should trigger selection mode.
        renameAction->setEnabled(true);
        moveToTrashAction->setEnabled(true);
        deleteAction->setEnabled(true);
        cutAction->setEnabled(true);
        duplicateAction->setEnabled(true);
        addToPlacesAction->setEnabled(true);
        copyLocation->setEnabled(true);
        // Them triggering selection mode and not directly acting on selected items is signified by adding "…" to their text.
        m_actionTextHelper->textsWhenNothingIsSelectedEnabled(true);

    } else {
        m_actionTextHelper->textsWhenNothingIsSelectedEnabled(false);
        stateChanged(QStringLiteral("has_selection"));

        QAction *deleteWithTrashShortcut = col->action(QStringLiteral("delete_shortcut")); // see DolphinViewActionHandler
        QAction *showTarget = col->action(QStringLiteral("show_target"));

        if (list.length() == 1 && list.first().isDir()) {
            addToPlacesAction->setEnabled(true);
        } else {
            addToPlacesAction->setEnabled(false);
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

    if (m_tabWidget->currentTabPage()->splitViewEnabled() && !list.isEmpty()) {
        DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
        KFileItem capabilitiesDestination;

        if (tabPage->primaryViewActive()) {
            capabilitiesDestination = tabPage->secondaryViewContainer()->rootItem();
        } else {
            capabilitiesDestination = tabPage->primaryViewContainer()->rootItem();
        }

        const auto destUrl = capabilitiesDestination.url();
        const bool allNotTargetOrigin = std::all_of(list.cbegin(), list.cend(), [destUrl](const KFileItem &item) {
            return item.url().adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash) != destUrl;
        });

        copyToOtherViewAction->setEnabled(capabilitiesDestination.isWritable() && allNotTargetOrigin);
        moveToOtherViewAction->setEnabled((list.isEmpty() || capabilitiesSource.supportsMoving()) && capabilitiesDestination.isWritable()
                                          && allNotTargetOrigin);
    } else {
        copyToOtherViewAction->setEnabled(false);
        moveToOtherViewAction->setEnabled(false);
    }
}

void DolphinMainWindow::updateViewActions()
{
    m_actionHandler->updateViewActions();

    QAction *toggleFilterBarAction = actionCollection()->action(QStringLiteral("toggle_filter"));
    toggleFilterBarAction->setChecked(m_activeViewContainer->isFilterBarVisible());

    updateSplitAction();
}

void DolphinMainWindow::updateGoActions()
{
    QAction *goUpAction = actionCollection()->action(KStandardAction::name(KStandardAction::Up));
    const QUrl currentUrl = m_activeViewContainer->url();
    // I think this is one of the best places to firstly be confronted
    // with a file system and its hierarchy. Talking about the root
    // directory might seem too much here but it is the question that
    // naturally arises in this context.
    goUpAction->setWhatsThis(xi18nc("@info:whatsthis",
                                    "<para>Go to "
                                    "the folder that contains the currently viewed one.</para>"
                                    "<para>All files and folders are organized in a hierarchical "
                                    "<emphasis>file system</emphasis>. At the top of this hierarchy is "
                                    "a directory that contains all data connected to this computer"
                                    "—the <emphasis>root directory</emphasis>.</para>"));
    goUpAction->setEnabled(KIO::upUrl(currentUrl) != currentUrl);
}

void DolphinMainWindow::refreshViews()
{
    m_tabWidget->refreshViews();

    if (GeneralSettings::modifiedStartupSettings()) {
        updateWindowTitle();
    }

    updateSplitAction();

    Q_EMIT settingsChanged();
}

void DolphinMainWindow::clearStatusBar()
{
    m_activeViewContainer->statusBar()->resetToDefaultText();
}

void DolphinMainWindow::connectViewSignals(DolphinViewContainer *container)
{
    connect(container, &DolphinViewContainer::showFilterBarChanged, this, &DolphinMainWindow::updateFilterBarAction);
    connect(container, &DolphinViewContainer::writeStateChanged, this, &DolphinMainWindow::slotWriteStateChanged);
    slotWriteStateChanged(container->view()->isFolderWritable());
    connect(container, &DolphinViewContainer::searchModeEnabledChanged, this, &DolphinMainWindow::updateSearchAction);
    connect(container, &DolphinViewContainer::captionChanged, this, &DolphinMainWindow::updateWindowTitle);
    connect(container, &DolphinViewContainer::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(container, &DolphinViewContainer::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);

    const QAction *toggleSearchAction = actionCollection()->action(QStringLiteral("toggle_search"));
    connect(toggleSearchAction, &QAction::triggered, container, &DolphinViewContainer::setSearchModeEnabled);

    // Make the toggled state of the selection mode actions visually follow the selection mode state of the view.
    auto toggleSelectionModeAction = actionCollection()->action(QStringLiteral("toggle_selection_mode"));
    toggleSelectionModeAction->setChecked(m_activeViewContainer->isSelectionModeEnabled());
    connect(m_activeViewContainer, &DolphinViewContainer::selectionModeChanged, toggleSelectionModeAction, &QAction::setChecked);

    const DolphinView *view = container->view();
    connect(view, &DolphinView::selectionChanged, this, &DolphinMainWindow::slotSelectionChanged);
    connect(view, &DolphinView::requestItemInfo, this, &DolphinMainWindow::requestItemInfo);
    connect(view, &DolphinView::fileItemsChanged, this, &DolphinMainWindow::fileItemsChanged);
    connect(view, &DolphinView::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(view, &DolphinView::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);
    connect(view, &DolphinView::windowRequested, this, &DolphinMainWindow::openNewWindow);
    connect(view, &DolphinView::requestContextMenu, this, &DolphinMainWindow::openContextMenu);
    connect(view, &DolphinView::directoryLoadingStarted, this, &DolphinMainWindow::enableStopAction);
    connect(view, &DolphinView::directoryLoadingCompleted, this, &DolphinMainWindow::disableStopAction);
    connect(view, &DolphinView::directoryLoadingCompleted, this, &DolphinMainWindow::slotDirectoryLoadingCompleted);
    connect(view, &DolphinView::goBackRequested, this, &DolphinMainWindow::goBack);
    connect(view, &DolphinView::goForwardRequested, this, &DolphinMainWindow::goForward);
    connect(view, &DolphinView::urlActivated, this, &DolphinMainWindow::handleUrl);
    connect(view, &DolphinView::goUpRequested, this, &DolphinMainWindow::goUp);

    connect(container->urlNavigatorInternalWithHistory(), &KUrlNavigator::urlChanged, this, &DolphinMainWindow::changeUrl);
    connect(container->urlNavigatorInternalWithHistory(), &KUrlNavigator::historyChanged, this, &DolphinMainWindow::updateHistory);

    auto navigators = static_cast<DolphinNavigatorsWidgetAction *>(actionCollection()->action(QStringLiteral("url_navigators")));
    const KUrlNavigator *navigator =
        m_tabWidget->currentTabPage()->primaryViewActive() ? navigators->primaryUrlNavigator() : navigators->secondaryUrlNavigator();

    QAction *editableLocactionAction = actionCollection()->action(QStringLiteral("editable_location"));
    editableLocactionAction->setChecked(navigator->isUrlEditable());
    connect(navigator, &KUrlNavigator::editableStateChanged, this, &DolphinMainWindow::slotEditableStateChanged);
    connect(navigator, &KUrlNavigator::tabRequested, this, &DolphinMainWindow::openNewTab);
    connect(navigator, &KUrlNavigator::activeTabRequested, this, &DolphinMainWindow::openNewTabAndActivate);
    connect(navigator, &KUrlNavigator::newWindowRequested, this, &DolphinMainWindow::openNewWindow);
}

void DolphinMainWindow::updateSplitAction()
{
    QAction *splitAction = actionCollection()->action(QStringLiteral("split_view"));
    const DolphinTabPage *tabPage = m_tabWidget->currentTabPage();
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
    auto navigators = static_cast<DolphinNavigatorsWidgetAction *>(actionCollection()->action(QStringLiteral("url_navigators")));
    if (toolBar()->actions().contains(navigators)) {
        toolBar()->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
        if (toolBarArea(toolBar()) == Qt::LeftToolBarArea || toolBarArea(toolBar()) == Qt::RightToolBarArea) {
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

void DolphinMainWindow::createPanelAction(const QIcon &icon, const QKeySequence &shortcut, QDockWidget *dockWidget, const QString &actionName)
{
    auto dockAction = dockWidget->toggleViewAction();
    dockAction->setIcon(icon);
    dockAction->setEnabled(true);

    QAction *panelAction = actionCollection()->addAction(actionName, dockAction);
    actionCollection()->setDefaultShortcut(panelAction, shortcut);

    connect(panelAction, &QAction::toggled, dockWidget, &QWidget::setVisible);
}
// clang-format off
void DolphinMainWindow::setupWhatsThis()
{
    // main widgets
    menuBar()->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the "
        "<emphasis>Menubar</emphasis>. It provides access to commands and "
        "configuration options. Left-click on any of the menus on this "
        "bar to see its contents.</para><para>The Menubar can be hidden "
        "by unchecking <interface>Settings|Show Menubar</interface>. Then "
        "most of its contents become available through a <interface>Menu"
        "</interface> button on the <emphasis>Toolbar</emphasis>.</para>"));
    toolBar()->setWhatsThis(xi18nc("@info:whatsthis", "<para>This is the "
        "<emphasis>Toolbar</emphasis>. It allows quick access to "
        "frequently used actions.</para><para>It is highly customizable. "
        "All items you see in the <interface>Menu</interface> or "
        "in the <interface>Menubar</interface> can be placed on the "
        "Toolbar. Just right-click on it and select <interface>Configure "
        "Toolbars…</interface> or find this action within the <interface>"
        "menu</interface>."
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
        "<para>All items you see in the <interface>Menu</interface> can also be placed on the Toolbar.</para>"));
    actionCollection()->action(KStandardAction::name(KStandardAction::Preferences))
        ->setWhatsThis(xi18nc("@info:whatsthis","This opens a window where you can "
        "change a multitude of settings for this application. For an explanation "
        "of the various settings go to the chapter <emphasis>Configuring Dolphin"
        "</emphasis> in <interface>Help|Dolphin Handbook</interface>."));

    // Help menu

    auto setStandardActionWhatsThis = [this](KStandardAction::StandardAction actionId,
                                             const QString &whatsThis) {
        // Check for the existence of an action since it can be restricted through the Kiosk system
        if (auto *action = actionCollection()->action(KStandardAction::name(actionId))) {
            action->setWhatsThis(whatsThis);
        }
    };

    // i18n: If the external link isn't available in your language it might make
    // sense to state the external link's language in brackets to not
    // frustrate the user. If there are multiple languages that the user might
    // know with a reasonable chance you might want to have 2 external links.
    // The same might be true for any external link you translate.
    setStandardActionWhatsThis(KStandardAction::HelpContents, xi18nc("@info:whatsthis handbook", "<para>This opens the Handbook for this application. It provides explanations for every part of <emphasis>Dolphin</emphasis>.</para><para>If you want more elaborate introductions to the different features of <emphasis>Dolphin</emphasis> <link url='https://userbase.kde.org/Dolphin/File_Management'>click here</link>. It will open the dedicated page in the KDE UserBase Wiki.</para>"));
    // (The i18n call should be completely in the line following the i18n: comment without any line breaks within the i18n call or the comment might not be correctly extracted. See: https://commits.kde.org/kxmlgui/a31135046e1b3335b5d7bbbe6aa9a883ce3284c1 )

    setStandardActionWhatsThis(KStandardAction::WhatsThis,
        xi18nc("@info:whatsthis whatsthis button",
        "<para>This is the button that invokes the help feature you are "
        "using right now! Click it, then click any component of this "
        "application to ask \"What's this?\" about it. The mouse cursor "
        "will change appearance if no help is available for a spot.</para>"
        "<para>There are two other ways to get help: "
        "The <link url='help:/dolphin/index.html'>Dolphin Handbook</link> and "
        "the <link url='https://userbase.kde.org/Dolphin/File_Management'>KDE "
        "UserBase Wiki</link>.</para><para>The \"What's this?\" help is "
        "missing in most other windows so don't get too used to this.</para>"));

    setStandardActionWhatsThis(KStandardAction::ReportBug,
        xi18nc("@info:whatsthis","<para>This opens a "
        "window that will guide you through reporting errors or flaws "
        "in this application or in other KDE software.</para>"
        "<para>High-quality bug reports are much appreciated. To learn "
        "how to make your bug report as effective as possible "
        "<link url='https://community.kde.org/Get_Involved/Bug_Reporting'>"
        "click here</link>.</para>"));

    setStandardActionWhatsThis(KStandardAction::Donate,
        xi18nc("@info:whatsthis", "<para>This opens a "
        "<emphasis>web page</emphasis> where you can donate to "
        "support the continued work on this application and many "
        "other projects by the <emphasis>KDE</emphasis> community.</para>"
        "<para>Donating is the easiest and fastest way to efficiently "
        "support KDE and its projects. KDE projects are available for "
        "free therefore your donation is needed to cover things that "
        "require money like servers, contributor meetings, etc.</para>"
        "<para><emphasis>KDE e.V.</emphasis> is the non-profit "
        "organization behind the KDE community.</para>"));

    setStandardActionWhatsThis(KStandardAction::SwitchApplicationLanguage,
        xi18nc("@info:whatsthis",
        "With this you can change the language this application uses."
        "<nl/>You can even set secondary languages which will be used "
        "if texts are not available in your preferred language."));

    setStandardActionWhatsThis(KStandardAction::AboutApp,
        xi18nc("@info:whatsthis","This opens a "
        "window that informs you about the version, license, "
        "used libraries and maintainers of this application."));

    setStandardActionWhatsThis(KStandardAction::AboutKDE,
        xi18nc("@info:whatsthis","This opens a "
        "window with information about <emphasis>KDE</emphasis>. "
        "The KDE community are the people behind this free software."
        "<nl/>If you like using this application but don't know "
        "about KDE or want to see a cute dragon have a look!"));
}
// clang-format on

bool DolphinMainWindow::addHamburgerMenuToToolbar()
{
    QDomDocument domDocument = KXMLGUIClient::domDocument();
    if (domDocument.isNull()) {
        return false;
    }
    QDomNode toolbar = domDocument.elementsByTagName(QStringLiteral("ToolBar")).at(0);
    if (toolbar.isNull()) {
        return false;
    }

    QDomElement hamburgerMenuElement = domDocument.createElement(QStringLiteral("Action"));
    hamburgerMenuElement.setAttribute(QStringLiteral("name"), QStringLiteral("hamburger_menu"));
    toolbar.appendChild(hamburgerMenuElement);

    KXMLGUIFactory::saveConfigFile(domDocument, xmlFile());
    reloadXML();
    createGUI();
    return true;
    // Make sure to also remove the <KXMLGUIFactory> and <QDomDocument> include
    // whenever this method is removed (maybe in the year ~2026).
}

// Set a sane initial window size
QSize DolphinMainWindow::sizeHint() const
{
    return KXmlGuiWindow::sizeHint().expandedTo(QSize(760, 550));
}

void DolphinMainWindow::saveNewToolbarConfig()
{
    KXmlGuiWindow::saveNewToolbarConfig(); // Applies the new config. This has to be called first
                                           // because the rest of this method decides things
                                           // based on the new config.
    auto navigators = static_cast<DolphinNavigatorsWidgetAction *>(actionCollection()->action(QStringLiteral("url_navigators")));
    if (!toolBar()->actions().contains(navigators)) {
        m_tabWidget->currentTabPage()->insertNavigatorsWidget(navigators);
    }
    updateAllowedToolbarAreas();
    (static_cast<KHamburgerMenu *>(actionCollection()->action(KStandardAction::name(KStandardAction::HamburgerMenu))))->hideActionsOf(toolBar());
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

DolphinMainWindow::UndoUiInterface::UndoUiInterface()
    : KIO::FileUndoManager::UiInterface()
{
}

DolphinMainWindow::UndoUiInterface::~UndoUiInterface()
{
}

void DolphinMainWindow::UndoUiInterface::jobError(KIO::Job *job)
{
    DolphinMainWindow *mainWin = qobject_cast<DolphinMainWindow *>(parentWidget());
    if (mainWin) {
        DolphinViewContainer *container = mainWin->activeViewContainer();
        container->showMessage(job->errorString(), DolphinViewContainer::Error);
    } else {
        KIO::FileUndoManager::UiInterface::jobError(job);
    }
}

bool DolphinMainWindow::isUrlOpen(const QString &url)
{
    return m_tabWidget->isUrlOpen(QUrl::fromUserInput(url));
}

bool DolphinMainWindow::isItemVisibleInAnyView(const QString &urlOfItem)
{
    return m_tabWidget->isItemVisibleInAnyView(QUrl::fromUserInput(urlOfItem));
}

#include "moc_dolphinmainwindow.cpp"
