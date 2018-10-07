/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2006 by Stefan Monov <logixoul@gmail.com>               *
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
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

#include "dolphinmainwindow.h"

#include "global.h"
#include "dolphindockwidget.h"
#include "dolphincontextmenu.h"
#include "dolphinnewfilemenu.h"
#include "dolphinrecenttabsmenu.h"
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"
#include "dolphintabpage.h"
#include "middleclickactioneventfilter.h"
#include "panels/folders/folderspanel.h"
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
#include <KFileItemListProperties>
#include <KHelpMenu>
#include <KIO/JobUiDelegate>
#include <KIO/OpenFileManagerWindowJob>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KMessageBox>
#include <KProtocolInfo>
#include <KProtocolManager>
#include <KRun>
#include <KShell>
#include <KStandardAction>
#include <KToggleAction>
#include <KToolBar>
#include <KToolInvocation>
#include <KUrlComboBox>
#include <KUrlNavigator>

#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDialog>
#include <QFileInfo>
#include <QLineEdit>
#include <QMenu>
#include <QMenuBar>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardPaths>
#include <QTimer>
#include <QToolButton>
#include <kdualaction.h>

namespace {
    // Used for GeneralSettings::version() to determine whether
    // an updated version of Dolphin is running.
    const int CurrentDolphinVersion = 200;
}

DolphinMainWindow::DolphinMainWindow() :
    KXmlGuiWindow(nullptr),
    m_newFileMenu(nullptr),
    m_tabWidget(nullptr),
    m_activeViewContainer(nullptr),
    m_actionHandler(nullptr),
    m_remoteEncoding(nullptr),
    m_settingsDialog(),
    m_controlButton(nullptr),
    m_updateToolBarTimer(nullptr),
    m_lastHandleUrlStatJob(nullptr),
    m_terminalPanel(nullptr),
    m_placesPanel(nullptr),
    m_tearDownFromPlacesRequested(false)
{
    Q_INIT_RESOURCE(dolphin);
    setComponentName(QStringLiteral("dolphin"), QGuiApplication::applicationDisplayName());
    setObjectName(QStringLiteral("Dolphin#"));

    connect(&DolphinNewFileMenuObserver::instance(), &DolphinNewFileMenuObserver::errorMessage,
            this, &DolphinMainWindow::showErrorMessage);

    KIO::FileUndoManager* undoManager = KIO::FileUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface());

    connect(undoManager, static_cast<void(KIO::FileUndoManager::*)(bool)>(&KIO::FileUndoManager::undoAvailable),
            this, &DolphinMainWindow::slotUndoAvailable);
    connect(undoManager, &KIO::FileUndoManager::undoTextChanged,
            this, &DolphinMainWindow::slotUndoTextChanged);
    connect(undoManager, &KIO::FileUndoManager::jobRecordingStarted,
            this, &DolphinMainWindow::clearStatusBar);
    connect(undoManager, &KIO::FileUndoManager::jobRecordingFinished,
            this, &DolphinMainWindow::showCommand);

    GeneralSettings* generalSettings = GeneralSettings::self();
    const bool firstRun = (generalSettings->version() < 200);
    if (firstRun) {
        generalSettings->setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    m_tabWidget = new DolphinTabWidget(this);
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
    connect(m_actionHandler, &DolphinViewActionHandler::createDirectory, this, &DolphinMainWindow::createDirectory);

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, &DolphinMainWindow::urlChanged,
            m_remoteEncoding, &DolphinRemoteEncoding::slotAboutToOpenUrl);

    setupDockWidgets();

    setupGUI(Keys | Save | Create | ToolBar);
    stateChanged(QStringLiteral("new_file"));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged,
            this, &DolphinMainWindow::updatePasteAction);

    QAction* showFilterBarAction = actionCollection()->action(QStringLiteral("show_filter_bar"));
    showFilterBarAction->setChecked(generalSettings->filterBar());

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

    // enable middle-click on back/forward/up to open in a new tab
    auto *middleClickEventFilter = new MiddleClickActionEventFilter(this);
    connect(middleClickEventFilter, &MiddleClickActionEventFilter::actionMiddleClicked, this, &DolphinMainWindow::slotToolBarActionMiddleClicked);
    toolBar()->installEventFilter(middleClickEventFilter);
}

DolphinMainWindow::~DolphinMainWindow()
{
}

void DolphinMainWindow::openDirectories(const QList<QUrl>& dirs, bool splitView)
{
    m_tabWidget->openDirectories(dirs, splitView);
}

void DolphinMainWindow::openFiles(const QList<QUrl>& files, bool splitView)
{
    m_tabWidget->openFiles(files, splitView);
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
    updateEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();

    emit urlChanged(url);
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
    updateEditActions();

    const int selectedUrlsCount = m_tabWidget->currentTabPage()->selectedItemsCount();

    QAction* compareFilesAction = actionCollection()->action(QStringLiteral("compare_files"));
    if (selectedUrlsCount == 2) {
        compareFilesAction->setEnabled(isKompareInstalled());
    } else {
        compareFilesAction->setEnabled(false);
    }

    emit selectionChanged(selection);
}

void DolphinMainWindow::updateHistory()
{
    const KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    const int index = urlNavigator->historyIndex();

    QAction* backAction = actionCollection()->action(KStandardAction::name(KStandardAction::Back));
    if (backAction) {
        backAction->setToolTip(i18nc("@info", "Go back"));
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction* forwardAction = actionCollection()->action(KStandardAction::name(KStandardAction::Forward));
    if (forwardAction) {
        forwardAction->setToolTip(i18nc("@info", "Go forward"));
        forwardAction->setEnabled(index > 0);
    }
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    QAction* showFilterBarAction = actionCollection()->action(QStringLiteral("show_filter_bar"));
    showFilterBarAction->setChecked(show);
}

void DolphinMainWindow::openNewMainWindow()
{
    Dolphin::openNewWindow({m_activeViewContainer->url()}, this);
}

void DolphinMainWindow::openNewActivatedTab()
{
    m_tabWidget->openNewActivatedTab();
}

void DolphinMainWindow::openNewTab(const QUrl& url)
{
    m_tabWidget->openNewTab(url);
}

void DolphinMainWindow::openInNewTab()
{
    const KFileItemList& list = m_activeViewContainer->view()->selectedItems();
    bool tabCreated = false;

    foreach (const KFileItem& item, list) {
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

    if (m_tabWidget->count() > 1 && GeneralSettings::confirmClosingMultipleTabs() && closedByUser) {
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

        const int result = KMessageBox::createKMessageBox(dialog,
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
    m_newFileMenu->setPopupFiles(activeViewContainer()->url());
}

void DolphinMainWindow::createDirectory()
{
    m_newFileMenu->setViewShowsHiddenFiles(activeViewContainer()->view()->hiddenFilesShown());
    m_newFileMenu->setPopupFiles(activeViewContainer()->url());
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
    m_activeViewContainer->view()->cutSelectedItems();
}

void DolphinMainWindow::copy()
{
    m_activeViewContainer->view()->copySelectedItems();
}

void DolphinMainWindow::paste()
{
    m_activeViewContainer->view()->paste();
}

void DolphinMainWindow::find()
{
    m_activeViewContainer->setSearchModeEnabled(true);
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

void DolphinMainWindow::selectAll()
{
    clearStatusBar();

    // if the URL navigator is editable and focused, select the whole
    // URL instead of all items of the view

    KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    QLineEdit* lineEdit = urlNavigator->editor()->lineEdit(); // krazy:exclude=qclasses
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
    tabPage->setSplitViewEnabled(!tabPage->splitViewEnabled());

    updateViewActions();
}

void DolphinMainWindow::toggleSplitStash()
{
    DolphinTabPage* tabPage = m_tabWidget->currentTabPage();
    tabPage->setSplitViewEnabled(false);
    tabPage->setSplitViewEnabled(true, QUrl("stash:/"));
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
    navigator->setUrlEditable(true);
    navigator->setFocus();

    // select the whole text of the combo box editor
    QLineEdit* lineEdit = navigator->editor()->lineEdit();  // krazy:exclude=qclasses
    lineEdit->selectAll();
}

void DolphinMainWindow::togglePanelLockState()
{
    const bool newLockState = !GeneralSettings::lockPanels();
    foreach (QObject* child, children()) {
        DolphinDockWidget* dock = qobject_cast<DolphinDockWidget*>(child);
        if (dock) {
            dock->setLocked(newLockState);
        }
    }

    GeneralSettings::setLockPanels(newLockState);
}

void DolphinMainWindow::slotTerminalPanelVisibilityChanged()
{
    if (m_terminalPanel->isHiddenInVisibleWindow()) {
        m_activeViewContainer->view()->setFocus();
    }
}

void DolphinMainWindow::goBack()
{
    KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    urlNavigator->goBack();

    if (urlNavigator->locationState().isEmpty()) {
        // An empty location state indicates a redirection URL,
        // which must be skipped too
        urlNavigator->goBack();
    }
}

void DolphinMainWindow::goForward()
{
    m_activeViewContainer->urlNavigator()->goForward();
}

void DolphinMainWindow::goUp()
{
    m_activeViewContainer->urlNavigator()->goUp();
}

void DolphinMainWindow::goHome()
{
    m_activeViewContainer->urlNavigator()->goHome();
}

void DolphinMainWindow::goBackInNewTab()
{
    KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
    const int index = urlNavigator->historyIndex() + 1;
    openNewTab(urlNavigator->locationUrl(index));
}

void DolphinMainWindow::goForwardInNewTab()
{
    KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
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
    KRun::runCommand(command, QStringLiteral("Kompare"), QStringLiteral("kompare"), this);
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

void DolphinMainWindow::openTerminal()
{
    QString dir(QDir::homePath());

    // If the given directory is not local, it can still be the URL of an
    // ioslave using UDS_LOCAL_PATH which to be converted first.
    KIO::StatJob* statJob = KIO::mostLocalUrl(m_activeViewContainer->url());
    KJobWidgets::setWindow(statJob, this);
    statJob->exec();
    QUrl url = statJob->mostLocalUrl();

    //If the URL is local after the above conversion, set the directory.
    if (url.isLocalFile()) {
        dir = url.toLocalFile();
    }

    KToolInvocation::invokeTerminal(QString(), dir);
}

void DolphinMainWindow::editSettings()
{
    if (!m_settingsDialog) {
        DolphinViewContainer* container = activeViewContainer();
        container->view()->writeSettings();

        const QUrl url = container->url();
        DolphinSettingsDialog* settingsDialog = new DolphinSettingsDialog(url, this);
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged, this, &DolphinMainWindow::refreshViews);
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        settingsDialog->show();
        m_settingsDialog = settingsDialog;
    } else {
        m_settingsDialog.data()->raise();
    }
}

void DolphinMainWindow::handleUrl(const QUrl& url)
{
    delete m_lastHandleUrlStatJob;
    m_lastHandleUrlStatJob = nullptr;

    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()) {
        activeViewContainer()->setUrl(url);
    } else if (KProtocolManager::supportsListing(url)) {
        // stat the URL to see if it is a dir or not
        m_lastHandleUrlStatJob = KIO::stat(url, KIO::HideProgressInfo);
        if (m_lastHandleUrlStatJob->uiDelegate()) {
            KJobWidgets::setWindow(m_lastHandleUrlStatJob, this);
        }
        connect(m_lastHandleUrlStatJob, &KIO::Job::result,
                this, &DolphinMainWindow::slotHandleUrlStatFinished);

    } else {
        new KRun(url, this); // Automatically deletes itself after being finished
    }
}

void DolphinMainWindow::slotHandleUrlStatFinished(KJob* job)
{
    m_lastHandleUrlStatJob = nullptr;
    const KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
    const QUrl url = static_cast<KIO::StatJob*>(job)->url();
    if (entry.isDir()) {
        activeViewContainer()->setUrl(url);
    } else {
        new KRun(url, this);  // Automatically deletes itself after being finished
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

    // Add "Edit" actions
    bool added = addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Undo)), menu) |
                 addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Find)), menu) |
                 addActionToMenu(ac->action(KStandardAction::name(KStandardAction::SelectAll)), menu) |
                 addActionToMenu(ac->action(QStringLiteral("invert_selection")), menu);

    if (added) {
        menu->addSeparator();
    }

    // Add "View" actions
    if (!GeneralSettings::showZoomSlider()) {
        addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ZoomIn)), menu);
        addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ZoomOut)), menu);
        menu->addSeparator();
    }

    added = addActionToMenu(ac->action(QStringLiteral("sort")), menu) |
            addActionToMenu(ac->action(QStringLiteral("view_mode")), menu) |
            addActionToMenu(ac->action(QStringLiteral("additional_info")), menu) |
            addActionToMenu(ac->action(QStringLiteral("show_preview")), menu) |
            addActionToMenu(ac->action(QStringLiteral("show_in_groups")), menu) |
            addActionToMenu(ac->action(QStringLiteral("show_hidden_files")), menu);

    if (added) {
        menu->addSeparator();
    }

    added = addActionToMenu(ac->action(QStringLiteral("split_view")), menu) |
            addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Redisplay)), menu) |
            addActionToMenu(ac->action(QStringLiteral("view_properties")), menu);
    if (added) {
        menu->addSeparator();
    }

    addActionToMenu(ac->action(QStringLiteral("panels")), menu);
    QMenu* locationBarMenu = new QMenu(i18nc("@action:inmenu", "Location Bar"), menu);
    locationBarMenu->addAction(ac->action(QStringLiteral("editable_location")));
    locationBarMenu->addAction(ac->action(QStringLiteral("replace_location")));
    menu->addMenu(locationBarMenu);

    menu->addSeparator();

    // Add "Go" menu
    QMenu* goMenu = new QMenu(i18nc("@action:inmenu", "Go"), menu);
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Back)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Forward)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Up)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Home)));
    goMenu->addAction(ac->action(QStringLiteral("closed_tabs")));
    menu->addMenu(goMenu);

    // Add "Tool" menu
    QMenu* toolsMenu = new QMenu(i18nc("@action:inmenu", "Tools"), menu);
    toolsMenu->addAction(ac->action(QStringLiteral("show_filter_bar")));
    toolsMenu->addAction(ac->action(QStringLiteral("compare_files")));
    toolsMenu->addAction(ac->action(QStringLiteral("open_terminal")));
    toolsMenu->addAction(ac->action(QStringLiteral("change_remote_encoding")));
    menu->addMenu(toolsMenu);

    // Add "Settings" menu entries
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::KeyBindings)), menu);
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ConfigureToolbars)), menu);
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Preferences)), menu);

    // Add "Help" menu
    auto helpMenu = new KHelpMenu(menu);
    menu->addMenu(helpMenu->menu());

    menu->addSeparator();
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ShowMenubar)), menu);
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
        // Disconnect all signals between the old view container (container,
        // view and url navigator) and main window.
        oldViewContainer->disconnect(this);
        oldViewContainer->view()->disconnect(this);
        oldViewContainer->urlNavigator()->disconnect(this);
    }

    connectViewSignals(viewContainer);

    m_actionHandler->setCurrentView(viewContainer->view());

    updateHistory();
    updateEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();

    const QUrl url = viewContainer->url();
    emit urlChanged(url);
}

void DolphinMainWindow::tabCountChanged(int count)
{
    const bool enableTabActions = (count > 1);
    actionCollection()->action(KStandardAction::name(KStandardAction::Close))->setEnabled(enableTabActions);
    actionCollection()->action(QStringLiteral("activate_next_tab"))->setEnabled(enableTabActions);
    actionCollection()->action(QStringLiteral("activate_prev_tab"))->setEnabled(enableTabActions);
}

void DolphinMainWindow::updateWindowTitle()
{
    setWindowTitle(m_activeViewContainer->caption());
}

void DolphinMainWindow::slotStorageTearDownFromPlacesRequested(const QString& mountPath)
{
    if (m_terminalPanel->currentWorkingDirectory().startsWith(mountPath)) {
        m_tearDownFromPlacesRequested = true;
        m_terminalPanel->goHome();
        // m_placesPanel->proceedWithTearDown() will be called in slotTerminalDirectoryChanged
    } else {
        m_placesPanel->proceedWithTearDown();
    }
}

void DolphinMainWindow::slotStorageTearDownExternallyRequested(const QString& mountPath)
{
    if (m_terminalPanel->currentWorkingDirectory().startsWith(mountPath)) {
        m_tearDownFromPlacesRequested = false;
        m_terminalPanel->goHome();
    }
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newFileMenu = new DolphinNewFileMenu(actionCollection(), this);
    m_newFileMenu->setObjectName("newFileMenu");
    QMenu* menu = m_newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(QIcon::fromTheme(QStringLiteral("document-new")));
    m_newFileMenu->setDelayed(false);
    connect(menu, &QMenu::aboutToShow,
            this, &DolphinMainWindow::updateNewMenu);

    QAction* newWindow = KStandardAction::openNew(this, &DolphinMainWindow::openNewMainWindow, actionCollection());
    newWindow->setText(i18nc("@action:inmenu File", "New &Window"));

    QAction* newTab = actionCollection()->addAction(QStringLiteral("new_tab"));
    newTab->setIcon(QIcon::fromTheme(QStringLiteral("tab-new")));
    newTab->setText(i18nc("@action:inmenu File", "New Tab"));
    actionCollection()->setDefaultShortcuts(newTab, {Qt::CTRL + Qt::Key_T, Qt::CTRL + Qt::SHIFT + Qt::Key_N});
    connect(newTab, &QAction::triggered, this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::openNewActivatedTab));

    QAction* closeTab = KStandardAction::close(
            m_tabWidget, static_cast<void(DolphinTabWidget::*)()>(&DolphinTabWidget::closeTab), actionCollection());
    closeTab->setText(i18nc("@action:inmenu File", "Close Tab"));
    closeTab->setEnabled(false);

    KStandardAction::quit(this, &DolphinMainWindow::quit, actionCollection());

    // setup 'Edit' menu
    KStandardAction::undo(this,
                          &DolphinMainWindow::undo,
                          actionCollection());


    KStandardAction::cut(this, &DolphinMainWindow::cut, actionCollection());
    KStandardAction::copy(this, &DolphinMainWindow::copy, actionCollection());
    QAction* paste = KStandardAction::paste(this, &DolphinMainWindow::paste, actionCollection());
    // The text of the paste-action is modified dynamically by Dolphin
    // (e. g. to "Paste One Folder"). To prevent that the size of the toolbar changes
    // due to the long text, the text "Paste" is used:
    paste->setIconText(i18nc("@action:inmenu Edit", "Paste"));

    KStandardAction::find(this, &DolphinMainWindow::find, actionCollection());

    KStandardAction::selectAll(this, &DolphinMainWindow::selectAll, actionCollection());

    QAction* invertSelection = actionCollection()->addAction(QStringLiteral("invert_selection"));
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    invertSelection->setIcon(QIcon::fromTheme(QStringLiteral("edit-select-invert")));
    actionCollection()->setDefaultShortcut(invertSelection, Qt::CTRL + Qt::SHIFT + Qt::Key_A);
    connect(invertSelection, &QAction::triggered, this, &DolphinMainWindow::invertSelection);

    // setup 'View' menu
    // (note that most of it is set up in DolphinViewActionHandler)

    QAction* split = actionCollection()->addAction(QStringLiteral("split_view"));
    actionCollection()->setDefaultShortcut(split, Qt::Key_F3);
    connect(split, &QAction::triggered, this, &DolphinMainWindow::toggleSplitView);

    QAction* stashSplit = actionCollection()->addAction(QStringLiteral("split_stash"));
    actionCollection()->setDefaultShortcut(stashSplit, Qt::CTRL + Qt::Key_S);
    stashSplit->setText(i18nc("@action:intoolbar Stash", "Stash"));
    stashSplit->setToolTip(i18nc("@info", "Opens the stash virtual directory in a split window"));
    stashSplit->setIcon(QIcon::fromTheme(QStringLiteral("folder-stash")));
    stashSplit->setCheckable(false);
    stashSplit->setVisible(KProtocolInfo::isKnownProtocol("stash"));
    connect(stashSplit, &QAction::triggered, this, &DolphinMainWindow::toggleSplitStash);

    KStandardAction::redisplay(this, &DolphinMainWindow::reloadView, actionCollection());

    QAction* stop = actionCollection()->addAction(QStringLiteral("stop"));
    stop->setText(i18nc("@action:inmenu View", "Stop"));
    stop->setToolTip(i18nc("@info", "Stop loading"));
    stop->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    connect(stop, &QAction::triggered, this, &DolphinMainWindow::stopLoading);

    KToggleAction* editableLocation = actionCollection()->add<KToggleAction>(QStringLiteral("editable_location"));
    editableLocation->setText(i18nc("@action:inmenu Navigation Bar", "Editable Location"));
    actionCollection()->setDefaultShortcut(editableLocation, Qt::Key_F6);
    connect(editableLocation, &KToggleAction::triggered, this, &DolphinMainWindow::toggleEditLocation);

    QAction* replaceLocation = actionCollection()->addAction(QStringLiteral("replace_location"));
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    actionCollection()->setDefaultShortcut(replaceLocation, Qt::CTRL + Qt::Key_L);
    connect(replaceLocation, &QAction::triggered, this, &DolphinMainWindow::replaceLocation);

    // setup 'Go' menu
    QAction* backAction = KStandardAction::back(this, &DolphinMainWindow::goBack, actionCollection());
    auto backShortcuts = backAction->shortcuts();
    backShortcuts.append(QKeySequence(Qt::Key_Backspace));
    actionCollection()->setDefaultShortcuts(backAction, backShortcuts);

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
    actionCollection()->setDefaultShortcut(undoCloseTab, Qt::CTRL + Qt::SHIFT + Qt::Key_T);
    undoCloseTab->setIcon(QIcon::fromTheme(QStringLiteral("edit-undo")));
    undoCloseTab->setEnabled(false);
    connect(undoCloseTab, &QAction::triggered, recentTabsMenu, &DolphinRecentTabsMenu::undoCloseTab);

    auto undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    undoAction->setEnabled(false); // undo should be disabled by default

    KStandardAction::forward(this, &DolphinMainWindow::goForward, actionCollection());
    KStandardAction::up(this, &DolphinMainWindow::goUp, actionCollection());
    KStandardAction::home(this, &DolphinMainWindow::goHome, actionCollection());

    // setup 'Tools' menu
    QAction* showFilterBar = actionCollection()->addAction(QStringLiteral("show_filter_bar"));
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Show Filter Bar"));
    showFilterBar->setIcon(QIcon::fromTheme(QStringLiteral("view-filter")));
    actionCollection()->setDefaultShortcuts(showFilterBar, {Qt::CTRL + Qt::Key_I, Qt::Key_Slash});
    connect(showFilterBar, &QAction::triggered, this, &DolphinMainWindow::showFilterBar);

    QAction* compareFiles = actionCollection()->addAction(QStringLiteral("compare_files"));
    compareFiles->setText(i18nc("@action:inmenu Tools", "Compare Files"));
    compareFiles->setIcon(QIcon::fromTheme(QStringLiteral("kompare")));
    compareFiles->setEnabled(false);
    connect(compareFiles, &QAction::triggered, this, &DolphinMainWindow::compareFiles);

#ifndef Q_OS_WIN
    if (KAuthorized::authorize(QStringLiteral("shell_access"))) {
        QAction* openTerminal = actionCollection()->addAction(QStringLiteral("open_terminal"));
        openTerminal->setText(i18nc("@action:inmenu Tools", "Open Terminal"));
        openTerminal->setIcon(QIcon::fromTheme(QStringLiteral("utilities-terminal")));
        actionCollection()->setDefaultShortcut(openTerminal, Qt::SHIFT + Qt::Key_F4);
        connect(openTerminal, &QAction::triggered, this, &DolphinMainWindow::openTerminal);
    }
#endif

    // setup 'Settings' menu
    KToggleAction* showMenuBar = KStandardAction::showMenubar(nullptr, nullptr, actionCollection());
    connect(showMenuBar, &KToggleAction::triggered,                   // Fixes #286822
            this, &DolphinMainWindow::toggleShowMenuBar, Qt::QueuedConnection);
    KStandardAction::preferences(this, &DolphinMainWindow::editSettings, actionCollection());

    // not in menu actions
    QList<QKeySequence> nextTabKeys = KStandardShortcut::tabNext();
    nextTabKeys.append(QKeySequence(Qt::CTRL + Qt::Key_Tab));

    QList<QKeySequence> prevTabKeys = KStandardShortcut::tabPrev();
    prevTabKeys.append(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_Tab));

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

    // Setup "Terminal"
#ifndef Q_OS_WIN
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
        createPanelAction(QIcon::fromTheme(QStringLiteral("utilities-terminal")), Qt::Key_F4, terminalAction, QStringLiteral("show_terminal_panel"));

        addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
        connect(this, &DolphinMainWindow::urlChanged,
                m_terminalPanel, &TerminalPanel::setUrl);

        if (GeneralSettings::version() < 200) {
            terminalDock->hide();
        }
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
    createPanelAction(QIcon::fromTheme(QStringLiteral("bookmarks")), Qt::Key_F9, placesAction, QStringLiteral("show_places_panel"));

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
            m_tabWidget, &DolphinTabWidget::slotPlacesPanelVisibilityChanged);
    connect(this, &DolphinMainWindow::settingsChanged,
        m_placesPanel, &PlacesPanel::readSettings);
    connect(m_placesPanel, &PlacesPanel::storageTearDownRequested,
            this, &DolphinMainWindow::slotStorageTearDownFromPlacesRequested);
    connect(m_placesPanel, &PlacesPanel::storageTearDownExternallyRequested,
            this, &DolphinMainWindow::slotStorageTearDownExternallyRequested);
    m_tabWidget->slotPlacesPanelVisibilityChanged(m_placesPanel->isVisible());

    // Add actions into the "Panels" menu
    KActionMenu* panelsMenu = new KActionMenu(i18nc("@action:inmenu View", "Panels"), this);
    actionCollection()->addAction(QStringLiteral("panels"), panelsMenu);
    panelsMenu->setDelayed(false);
    const KActionCollection* ac = actionCollection();
    panelsMenu->addAction(ac->action(QStringLiteral("show_places_panel")));
#ifdef HAVE_BALOO
    panelsMenu->addAction(ac->action(QStringLiteral("show_information_panel")));
#endif
    panelsMenu->addAction(ac->action(QStringLiteral("show_folders_panel")));
    panelsMenu->addAction(ac->action(QStringLiteral("show_terminal_panel")));
    panelsMenu->addSeparator();
    panelsMenu->addAction(lockLayoutAction);
}

void DolphinMainWindow::updateEditActions()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        stateChanged(QStringLiteral("has_no_selection"));
    } else {
        stateChanged(QStringLiteral("has_selection"));

        KActionCollection* col = actionCollection();
        QAction* renameAction            = col->action(KStandardAction::name(KStandardAction::RenameFile));
        QAction* moveToTrashAction       = col->action(KStandardAction::name(KStandardAction::MoveToTrash));
        QAction* deleteAction            = col->action(KStandardAction::name(KStandardAction::DeleteFile));
        QAction* cutAction               = col->action(KStandardAction::name(KStandardAction::Cut));
        QAction* deleteWithTrashShortcut = col->action(QStringLiteral("delete_shortcut")); // see DolphinViewActionHandler
        QAction* showTarget              = col->action(QStringLiteral("show_target"));

        KFileItemListProperties capabilities(list);
        const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();

        renameAction->setEnabled(capabilities.supportsMoving());
        moveToTrashAction->setEnabled(enableMoveToTrash);
        deleteAction->setEnabled(capabilities.supportsDeleting());
        deleteWithTrashShortcut->setEnabled(capabilities.supportsDeleting() && !enableMoveToTrash);
        cutAction->setEnabled(capabilities.supportsMoving());
        showTarget->setEnabled(list.length() == 1 && list.at(0).isLink());
    }
}

void DolphinMainWindow::updateViewActions()
{
    m_actionHandler->updateViewActions();

    QAction* showFilterBarAction = actionCollection()->action(QStringLiteral("show_filter_bar"));
    showFilterBarAction->setChecked(m_activeViewContainer->isFilterBarVisible());

    updateSplitAction();

    QAction* editableLocactionAction = actionCollection()->action(QStringLiteral("editable_location"));
    const KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    editableLocactionAction->setChecked(urlNavigator->isUrlEditable());
}

void DolphinMainWindow::updateGoActions()
{
    QAction* goUpAction = actionCollection()->action(KStandardAction::name(KStandardAction::Up));
    const QUrl currentUrl = m_activeViewContainer->url();
    goUpAction->setEnabled(KIO::upUrl(currentUrl) != currentUrl);
}

void DolphinMainWindow::createControlButton()
{
    if (m_controlButton) {
        return;
    }
    Q_ASSERT(!m_controlButton);

    m_controlButton = new QToolButton(this);
    m_controlButton->setIcon(QIcon::fromTheme(QStringLiteral("application-menu")));
    m_controlButton->setText(i18nc("@action", "Control"));
    m_controlButton->setPopupMode(QToolButton::InstantPopup);
    m_controlButton->setToolButtonStyle(toolBar()->toolButtonStyle());

    QMenu* controlMenu = new QMenu(m_controlButton);
    connect(controlMenu, &QMenu::aboutToShow, this, &DolphinMainWindow::updateControlMenu);

    m_controlButton->setMenu(controlMenu);

    toolBar()->addWidget(m_controlButton);
    connect(toolBar(), &KToolBar::iconSizeChanged,
            m_controlButton, &QToolButton::setIconSize);
    connect(toolBar(), &KToolBar::toolButtonStyleChanged,
            m_controlButton, &QToolButton::setToolButtonStyle);

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
    foreach (const QWidget* widget, action->associatedWidgets()) {
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
        m_tabWidget->currentTabPage()->setSplitViewEnabled(splitView);
        updateSplitAction();
        updateWindowTitle();
    }

    emit settingsChanged();
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
            this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::goBack));
    connect(view, &DolphinView::goForwardRequested,
            this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::goForward));
    connect(view, &DolphinView::urlActivated,
            this, &DolphinMainWindow::handleUrl);

    const KUrlNavigator* navigator = container->urlNavigator();
    connect(navigator, &KUrlNavigator::urlChanged,
            this, &DolphinMainWindow::changeUrl);
    connect(navigator, &KUrlNavigator::historyChanged,
            this, &DolphinMainWindow::updateHistory);
    connect(navigator, &KUrlNavigator::editableStateChanged,
            this, &DolphinMainWindow::slotEditableStateChanged);
    connect(navigator, &KUrlNavigator::tabRequested,
            this, &DolphinMainWindow::openNewTab);
}

void DolphinMainWindow::updateSplitAction()
{
    QAction* splitAction = actionCollection()->action(QStringLiteral("split_view"));
    const DolphinTabPage* tabPage = m_tabWidget->currentTabPage();
    if (tabPage->splitViewEnabled()) {
        if (tabPage->primaryViewActive()) {
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

