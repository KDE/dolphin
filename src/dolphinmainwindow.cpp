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

#include "dolphinapplication.h"
#include "dolphindockwidget.h"
#include "dolphincontextmenu.h"
#include "dolphinnewfilemenu.h"
#include "dolphinrecenttabsmenu.h"
#include "dolphinviewcontainer.h"
#include "dolphinviewsignaladapter.h"
#include "dolphintabpage.h"
#include "dolphintabwidget.h"
#include "panels/folders/folderspanel.h"
#include "panels/places/placespanel.h"
#include "panels/information/informationpanel.h"
#include "settings/dolphinsettingsdialog.h"
#include "statusbar/dolphinstatusbar.h"
#include "views/dolphinviewactionhandler.h"
#include "views/dolphinremoteencoding.h"
#include "views/draganddrophelper.h"
#include "views/viewproperties.h"
#include "views/dolphinnewfilemenuobserver.h"
#include "views/placespanelvisibilityobserver.h"

#ifndef Q_OS_WIN
#include "panels/terminal/terminalpanel.h"
#endif

#include "dolphin_generalsettings.h"

#include <KAcceleratorManager>
#include <KAction>
#include <KActionCollection>
#include <KActionMenu>
#include <KConfig>
#include <KDesktopFile>
#include <kdeversion.h>
#include <kdualaction.h>
#include <KFileDialog>
#include <KGlobal>
#include <KLineEdit>
#include <KToolBar>
#include <KIcon>
#include <KIconLoader>
#include <KIO/NetAccess>
#include <KIO/JobUiDelegate>
#include <KInputDialog>
#include <KLocale>
#include <KProtocolManager>
#include <KMenu>
#include <KMenuBar>
#include <KMessageBox>
#include <KFileItemListProperties>
#include <konqmimedata.h>
#include <KProtocolInfo>
#include <KRun>
#include <KShell>
#include <KStandardDirs>
#include <kstatusbar.h>
#include <KStandardAction>
#include <KToggleAction>
#include <KUrlNavigator>
#include <KUrl>
#include <KUrlComboBox>
#include <KToolInvocation>

#include <QDesktopWidget>
#include <QDBusMessage>
#include <QKeyEvent>
#include <QClipboard>
#include <QToolButton>

namespace {
    // Used for GeneralSettings::version() to determine whether
    // an updated version of Dolphin is running.
    const int CurrentDolphinVersion = 200;
};

DolphinMainWindow::DolphinMainWindow() :
    KXmlGuiWindow(0),
    m_viewSignalAdapter(0),
    m_tabWidget(0),
    m_newFileMenu(0),
    m_actionHandler(0),
    m_remoteEncoding(0),
    m_settingsDialog(),
    m_controlButton(0),
    m_updateToolBarTimer(0),
    m_lastHandleUrlStatJob(0)
{
    setObjectName("Dolphin#");

    connect(&DolphinNewFileMenuObserver::instance(), SIGNAL(errorMessage(QString)),
            this, SLOT(showErrorMessage(QString)));

    KIO::FileUndoManager* undoManager = KIO::FileUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface());

    connect(undoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));
    connect(undoManager, SIGNAL(undoTextChanged(QString)),
            this, SLOT(slotUndoTextChanged(QString)));
    connect(undoManager, SIGNAL(jobRecordingStarted(CommandType)),
            this, SLOT(clearStatusBar()));
    connect(undoManager, SIGNAL(jobRecordingFinished(CommandType)),
            this, SLOT(showCommand(CommandType)));

    GeneralSettings* generalSettings = GeneralSettings::self();
    const bool firstRun = (generalSettings->version() < 200);
    if (firstRun) {
        generalSettings->setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    m_tabWidget = new DolphinTabWidget(this);
    connect(m_tabWidget, SIGNAL(errorMessage(QString)),
            this, SLOT(showErrorMessage(QString)));
    connect(m_tabWidget, SIGNAL(activeViewChanged()),
            this, SLOT(activeViewChanged()));
    connect(m_tabWidget, SIGNAL(tabsCountChanged(int)),
            this, SLOT(slotTabsCountChanged(int)));
    setCentralWidget(m_tabWidget);

    m_viewSignalAdapter = new DolphinViewSignalAdapter(this);
    connect(m_viewSignalAdapter, SIGNAL(showFilterBarChanged(bool)),
            this, SLOT(updateFilterBarAction(bool)));
    connect(m_viewSignalAdapter, SIGNAL(writeStateChanged(bool)),
            this, SLOT(slotWriteStateChanged(bool)));
    connect(m_viewSignalAdapter, SIGNAL(selectionChanged(KFileItemList)),
            this, SLOT(slotSelectionChanged(KFileItemList)));
    connect(m_viewSignalAdapter, SIGNAL(requestItemInfo(KFileItem)),
            this, SLOT(slotRequestItemInfo(KFileItem)));
    connect(m_viewSignalAdapter, SIGNAL(tabRequested(KUrl)),
            tabWidget(), SLOT(openNewTab(KUrl)));
    connect(m_viewSignalAdapter, SIGNAL(requestContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)),
            this, SLOT(openContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)));
    connect(m_viewSignalAdapter, SIGNAL(directoryLoadingStarted()),
            this, SLOT(enableStopAction()));
    connect(m_viewSignalAdapter, SIGNAL(directoryLoadingCompleted()),
            this, SLOT(disableStopAction()));
    connect(m_viewSignalAdapter, SIGNAL(goBackRequested()),
            this, SLOT(goBack()));
    connect(m_viewSignalAdapter, SIGNAL(goForwardRequested()),
            this, SLOT(goForward()));
    connect(m_viewSignalAdapter, SIGNAL(urlChanged(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(m_viewSignalAdapter, SIGNAL(historyChanged()),
            this, SLOT(updateHistory()));
    connect(m_viewSignalAdapter, SIGNAL(editableStateChanged(bool)),
            this, SLOT(slotEditableStateChanged(bool)));

    setupActions();

    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    connect(m_actionHandler, SIGNAL(actionBeingHandled()), SLOT(clearStatusBar()));
    connect(m_actionHandler, SIGNAL(createDirectory()), SLOT(createDirectory()));

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, SIGNAL(urlChanged(KUrl)),
            m_remoteEncoding, SLOT(slotAboutToOpenUrl()));

    setupDockWidgets();

    setupGUI(Keys | Save | Create | ToolBar);
    stateChanged("new_file");

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));

    QAction* showFilterBarAction = actionCollection()->action("show_filter_bar");
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

    const KUrl homeUrl(generalSettings->homeUrl());
    tabWidget()->openNewActivatedTab(homeUrl);
}

DolphinMainWindow::~DolphinMainWindow()
{
}

DolphinViewContainer* DolphinMainWindow::activeViewContainer() const
{
    return tabWidget()->activeViewContainer();
}

void DolphinMainWindow::openDirectories(const QList<KUrl>& dirs)
{
    if (dirs.isEmpty()) {
        return;
    }

    if (dirs.count() == 1) {
        activeViewContainer()->setUrl(dirs.first());
        return;
    }

    const int oldOpenTabsCount = tabWidget()->count();

    const bool hasSplitView = GeneralSettings::splitView();

    // Open each directory inside a new tab. If the "split view" option has been enabled,
    // always show two directories within one tab.
    QList<KUrl>::const_iterator it = dirs.begin();
    while (it != dirs.end()) {
        tabWidget()->openNewTab(*it);
        ++it;

        if (hasSplitView && (it != dirs.end())) {
            const int tabIndex = tabWidget()->count() - 1;
            tabWidget()->tabPageAt(tabIndex)->setSplitViewEnabled(true, *it);
            ++it;
        }
    }

    // Remove the previously opened tabs
    for (int i = 0; i < oldOpenTabsCount; ++i) {
        tabWidget()->closeTab(0);
    }
}

void DolphinMainWindow::openFiles(const QList<KUrl>& files)
{
    if (files.isEmpty()) {
        return;
    }

    // Get all distinct directories from 'files' and open a tab
    // for each directory. If the "split view" option is enabled, two
    // directories are shown inside one tab (see openDirectories()).
    QList<KUrl> dirs;
    foreach (const KUrl& url, files) {
        const KUrl dir(url.directory());
        if (!dirs.contains(dir)) {
            dirs.append(dir);
        }
    }

    openDirectories(dirs);

    // Select the files. Although the files can be split between several
    // tabs, there is no need to split 'files' accordingly, as
    // the DolphinView will just ignore invalid selections.
    const int tabCount = tabWidget()->count();
    for (int i = 0; i < tabCount; ++i) {
        DolphinTabPage* tabPage = tabWidget()->tabPageAt(i);
        DolphinView* primaryView = tabPage->viewContainer(DolphinTabPage::PrimaryView)->view();
        primaryView->markUrlsAsSelected(files);
        primaryView->markUrlAsCurrent(files.first());
        if (tabPage->isSplitViewEnabled()) {
            DolphinView* secondaryView = tabPage->viewContainer(DolphinTabPage::SecondaryView)->view();
            secondaryView->markUrlsAsSelected(files);
            secondaryView->markUrlAsCurrent(files.first());
        }
    }
}

void DolphinMainWindow::showCommand(CommandType command)
{
    DolphinStatusBar* statusBar = activeViewContainer()->statusBar();
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
    activeViewContainer()->view()->pasteIntoFolder();
}

void DolphinMainWindow::changeUrl(const KUrl& url)
{
    if (!KProtocolManager::supportsListing(url)) {
        // The URL navigator only checks for validity, not
        // if the URL can be listed. An error message is
        // shown due to DolphinViewContainer::restoreView().
        return;
    }

    DolphinViewContainer* view = activeViewContainer();
    if (view) {
        view->setUrl(url);
        updateEditActions();
        updateViewActions();
        updateGoActions();
        setUrlAsCaption(url);
        emit urlChanged(url);
    }
}

void DolphinMainWindow::slotTerminalDirectoryChanged(const KUrl& url)
{
    activeViewContainer()->setAutoGrabFocus(false);
    changeUrl(url);
    activeViewContainer()->setAutoGrabFocus(true);
}

void DolphinMainWindow::slotEditableStateChanged(bool editable)
{
    KToggleAction* editableLocationAction =
        static_cast<KToggleAction*>(actionCollection()->action("editable_location"));
    editableLocationAction->setChecked(editable);
}

void DolphinMainWindow::slotSelectionChanged(const KFileItemList& selection)
{
    updateEditActions();

    const DolphinTabPage* tabPage = tabWidget()->currentTabPage();
    int selectedUrlsCount = tabPage->viewContainer(DolphinTabPage::PrimaryView)->view()->selectedItemsCount();
    if (tabPage->isSplitViewEnabled()) {
        selectedUrlsCount += tabPage->viewContainer(DolphinTabPage::SecondaryView)->view()->selectedItemsCount();
    }

    QAction* compareFilesAction = actionCollection()->action("compare_files");
    if (selectedUrlsCount == 2) {
        compareFilesAction->setEnabled(isKompareInstalled());
    } else {
        compareFilesAction->setEnabled(false);
    }

    emit selectionChanged(selection);
}

void DolphinMainWindow::slotRequestItemInfo(const KFileItem& item)
{
    emit requestItemInfo(item);
}

void DolphinMainWindow::updateHistory()
{
    const KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
    const int index = urlNavigator->historyIndex();

    QAction* backAction = actionCollection()->action("go_back");
    if (backAction) {
        backAction->setToolTip(i18nc("@info", "Go back"));
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction* forwardAction = actionCollection()->action("go_forward");
    if (forwardAction) {
        forwardAction->setToolTip(i18nc("@info", "Go forward"));
        forwardAction->setEnabled(index > 0);
    }
}

void DolphinMainWindow::updateFilterBarAction(bool show)
{
    QAction* showFilterBarAction = actionCollection()->action("show_filter_bar");
    showFilterBarAction->setChecked(show);
}

void DolphinMainWindow::openNewMainWindow()
{
    KRun::run("dolphin %u", KUrl::List(), this);
}

void DolphinMainWindow::openInNewTab()
{
    const KFileItemList& list = activeViewContainer()->view()->selectedItems();
    if (list.isEmpty()) {
        tabWidget()->openNewTab(activeViewContainer()->url());
    } else {
        foreach (const KFileItem& item, list) {
            const KUrl& url = DolphinView::openItemAsFolderUrl(item);
            if (!url.isEmpty()) {
                tabWidget()->openNewTab(url);
            }
        }
    }
}

void DolphinMainWindow::openInNewWindow()
{
    KUrl newWindowUrl;

    const KFileItemList list = activeViewContainer()->view()->selectedItems();
    if (list.isEmpty()) {
        newWindowUrl = activeViewContainer()->url();
    } else if (list.count() == 1) {
        const KFileItem& item = list.first();
        newWindowUrl = DolphinView::openItemAsFolderUrl(item);
    }

    if (!newWindowUrl.isEmpty()) {
        KRun::run("dolphin %u", KUrl::List() << newWindowUrl, this);
    }
}

void DolphinMainWindow::closeEvent(QCloseEvent* event)
{
    // Find out if Dolphin is closed directly by the user or
    // by the session manager because the session is closed
    bool closedByUser = true;
    DolphinApplication *application = qobject_cast<DolphinApplication*>(qApp);
    if (application && application->sessionSaving()) {
        closedByUser = false;
    }

    if (tabWidget()->count() > 1 && GeneralSettings::confirmClosingMultipleTabs() && closedByUser) {
        // Ask the user if he really wants to quit and close all tabs.
        // Open a confirmation dialog with 3 buttons:
        // KDialog::Yes    -> Quit
        // KDialog::No     -> Close only the current tab
        // KDialog::Cancel -> do nothing
        KDialog *dialog = new KDialog(this, Qt::Dialog);
        dialog->setCaption(i18nc("@title:window", "Confirmation"));
        dialog->setButtons(KDialog::Yes | KDialog::No | KDialog::Cancel);
        dialog->setModal(true);
        dialog->setButtonGuiItem(KDialog::Yes, KStandardGuiItem::quit());
        dialog->setButtonGuiItem(KDialog::No, KGuiItem(i18n("C&lose Current Tab"), KIcon("tab-close")));
        dialog->setButtonGuiItem(KDialog::Cancel, KStandardGuiItem::cancel());
        dialog->setDefaultButton(KDialog::Yes);

        bool doNotAskAgainCheckboxResult = false;

        const int result = KMessageBox::createKMessageBox(dialog,
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
            case KDialog::Yes:
                // Quit
                break;
            case KDialog::No:
                // Close only the current tab
                tabWidget()->closeTab();
            default:
                event->ignore();
                return;
        }
    }

    GeneralSettings::setVersion(CurrentDolphinVersion);
    GeneralSettings::self()->writeConfig();

    KXmlGuiWindow::closeEvent(event);
}

void DolphinMainWindow::saveProperties(KConfigGroup& group)
{
    tabWidget()->saveProperties(group);
}

void DolphinMainWindow::readProperties(const KConfigGroup& group)
{
    tabWidget()->readProperties(group);
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
    activeViewContainer()->showMessage(message, DolphinViewContainer::Error);
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
    activeViewContainer()->view()->cutSelectedItems();
}

void DolphinMainWindow::copy()
{
    activeViewContainer()->view()->copySelectedItems();
}

void DolphinMainWindow::paste()
{
    activeViewContainer()->view()->paste();
}

void DolphinMainWindow::find()
{
    activeViewContainer()->setSearchModeEnabled(true);
}

void DolphinMainWindow::updatePasteAction()
{
    QAction* pasteAction = actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
    QPair<bool, QString> pasteInfo = activeViewContainer()->view()->pasteInfo();
    pasteAction->setEnabled(pasteInfo.first);
    pasteAction->setText(pasteInfo.second);
}

void DolphinMainWindow::selectAll()
{
    clearStatusBar();

    // if the URL navigator is editable and focused, select the whole
    // URL instead of all items of the view

    KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
    QLineEdit* lineEdit = urlNavigator->editor()->lineEdit(); // krazy:exclude=qclasses
    const bool selectUrl = urlNavigator->isUrlEditable() &&
                           lineEdit->hasFocus();
    if (selectUrl) {
        lineEdit->selectAll();
    } else {
        activeViewContainer()->view()->selectAll();
    }
}

void DolphinMainWindow::invertSelection()
{
    clearStatusBar();
    activeViewContainer()->view()->invertSelection();
}

void DolphinMainWindow::reloadView()
{
    clearStatusBar();
    activeViewContainer()->view()->reload();
}

void DolphinMainWindow::stopLoading()
{
    activeViewContainer()->view()->stopLoading();
}

void DolphinMainWindow::enableStopAction()
{
    actionCollection()->action("stop")->setEnabled(true);
}

void DolphinMainWindow::disableStopAction()
{
    actionCollection()->action("stop")->setEnabled(false);
}

void DolphinMainWindow::showFilterBar()
{
    activeViewContainer()->setFilterBarVisible(true);
}

void DolphinMainWindow::toggleEditLocation()
{
    clearStatusBar();

    QAction* action = actionCollection()->action("editable_location");
    KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
    urlNavigator->setUrlEditable(action->isChecked());
}

void DolphinMainWindow::replaceLocation()
{
    KUrlNavigator* navigator = activeViewContainer()->urlNavigator();
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

void DolphinMainWindow::goBack()
{
    KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
    urlNavigator->goBack();

    if (urlNavigator->locationState().isEmpty()) {
        // An empty location state indicates a redirection URL,
        // which must be skipped too
        urlNavigator->goBack();
    }
}

void DolphinMainWindow::goForward()
{
    activeViewContainer()->urlNavigator()->goForward();
}

void DolphinMainWindow::goUp()
{
    activeViewContainer()->urlNavigator()->goUp();
}

void DolphinMainWindow::goHome()
{
    activeViewContainer()->urlNavigator()->goHome();
}

void DolphinMainWindow::goBack(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goBack().
    if (buttons == Qt::MidButton) {
        KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
        const int index = urlNavigator->historyIndex() + 1;
        tabWidget()->openNewTab(urlNavigator->locationUrl(index));
    }
}

void DolphinMainWindow::goForward(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goForward().
    if (buttons == Qt::MidButton) {
        KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
        const int index = urlNavigator->historyIndex() - 1;
        tabWidget()->openNewTab(urlNavigator->locationUrl(index));
    }
}

void DolphinMainWindow::goUp(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goUp().
    if (buttons == Qt::MidButton) {
        tabWidget()->openNewTab(activeViewContainer()->url().upUrl());
    }
}

void DolphinMainWindow::goHome(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goHome().
    if (buttons == Qt::MidButton) {
        tabWidget()->openNewTab(GeneralSettings::self()->homeUrl());
    }
}

void DolphinMainWindow::compareFiles()
{
    // The method is only invoked if exactly 2 files have
    // been selected. The selected files may be:
    // - both in the primary view
    // - both in the secondary view
    // - one in the primary view and the other in the secondary
    //   view

    KUrl urlA;
    KUrl urlB;

    DolphinTabPage* tabPage = tabWidget()->currentTabPage();
    DolphinView* primaryView = tabPage->viewContainer(DolphinTabPage::PrimaryView)->view();
    KFileItemList items = primaryView->selectedItems();

    switch (items.count()) {
    case 0: {
        DolphinView* secondaryView = tabPage->viewContainer(DolphinTabPage::SecondaryView)->view();
        items = secondaryView->selectedItems();
        Q_ASSERT(items.count() == 2);
        urlA = items[0].url();
        urlB = items[1].url();
        break;
    }

    case 1: {
        urlA = items[0].url();
        DolphinView* secondaryView = tabPage->viewContainer(DolphinTabPage::SecondaryView)->view();
        items = secondaryView->selectedItems();
        Q_ASSERT(items.count() == 1);
        urlB = items[0].url();
        break;
    }

    case 2: {
        urlA = items[0].url();
        urlB = items[1].url();
        break;
    }

    default: {
        // may not happen: compareFiles may only get invoked if 2
        // files are selected
        Q_ASSERT(false);
    }
    }

    QString command("kompare -c \"");
    command.append(urlA.pathOrUrl());
    command.append("\" \"");
    command.append(urlB.pathOrUrl());
    command.append('\"');
    KRun::runCommand(command, "Kompare", "kompare", this);
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
    KUrl url = KIO::NetAccess::mostLocalUrl(activeViewContainer()->url(), this);

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

        const KUrl url = container->url();
        DolphinSettingsDialog* settingsDialog = new DolphinSettingsDialog(url, this);
        connect(settingsDialog, SIGNAL(settingsChanged()), this, SLOT(refreshViews()));
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        settingsDialog->show();
        m_settingsDialog = settingsDialog;
    } else {
        m_settingsDialog.data()->raise();
    }
}

void DolphinMainWindow::handleUrl(const KUrl& url)
{
    delete m_lastHandleUrlStatJob;
    m_lastHandleUrlStatJob = 0;

    if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isDir()) {
        activeViewContainer()->setUrl(url);
    } else if (KProtocolManager::supportsListing(url)) {
        // stat the URL to see if it is a dir or not
        m_lastHandleUrlStatJob = KIO::stat(url, KIO::HideProgressInfo);
        if (m_lastHandleUrlStatJob->ui()) {
            m_lastHandleUrlStatJob->ui()->setWindow(this);
        }
        connect(m_lastHandleUrlStatJob, SIGNAL(result(KJob*)),
                this, SLOT(slotHandleUrlStatFinished(KJob*)));

    } else {
        new KRun(url, this); // Automatically deletes itself after being finished
    }
}

void DolphinMainWindow::slotHandleUrlStatFinished(KJob* job)
{
    m_lastHandleUrlStatJob = 0;
    const KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
    const KUrl url = static_cast<KIO::StatJob*>(job)->url();
    if (entry.isDir()) {
        activeViewContainer()->setUrl(url);
    } else {
        new KRun(url, this);  // Automatically deletes itself after being finished
    }
}

void DolphinMainWindow::slotWriteStateChanged(bool isFolderWritable)
{
    newFileMenu()->setEnabled(isFolderWritable);
}

void DolphinMainWindow::openContextMenu(const QPoint& pos,
                                        const KFileItem& item,
                                        const KUrl& url,
                                        const QList<QAction*>& customActions)
{
    QWeakPointer<DolphinContextMenu> contextMenu = new DolphinContextMenu(this, pos, item, url);
    contextMenu.data()->setCustomActions(customActions);
    const DolphinContextMenu::Command command = contextMenu.data()->open();

    switch (command) {
    case DolphinContextMenu::OpenParentFolderInNewWindow: {
        KRun::run("dolphin %u", KUrl::List() << item.url().upUrl(), this);
        break;
    }

    case DolphinContextMenu::OpenParentFolderInNewTab:
        tabWidget()->openNewTab(item.url().upUrl());
        break;

    case DolphinContextMenu::None:
    default:
        break;
    }

    delete contextMenu.data();
}

void DolphinMainWindow::updateControlMenu()
{
    KMenu* menu = qobject_cast<KMenu*>(sender());
    Q_ASSERT(menu);

    // All actions get cleared by KMenu::clear(). The sub-menus are deleted
    // by connecting to the aboutToHide() signal from the parent-menu.
    menu->clear();

    KActionCollection* ac = actionCollection();

    // Add "Edit" actions
    bool added = addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Undo)), menu) |
                 addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Find)), menu) |
                 addActionToMenu(ac->action("select_all"), menu) |
                 addActionToMenu(ac->action("invert_selection"), menu);

    if (added) {
        menu->addSeparator();
    }

    // Add "View" actions
    if (!GeneralSettings::showZoomSlider()) {
        addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ZoomIn)), menu);
        addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ZoomOut)), menu);
        menu->addSeparator();
    }

    added = addActionToMenu(ac->action("view_mode"), menu) |
            addActionToMenu(ac->action("sort"), menu) |
            addActionToMenu(ac->action("additional_info"), menu) |
            addActionToMenu(ac->action("show_preview"), menu) |
            addActionToMenu(ac->action("show_in_groups"), menu) |
            addActionToMenu(ac->action("show_hidden_files"), menu);

    if (added) {
        menu->addSeparator();
    }

    added = addActionToMenu(ac->action("split_view"), menu) |
            addActionToMenu(ac->action("reload"), menu) |
            addActionToMenu(ac->action("view_properties"), menu);
    if (added) {
        menu->addSeparator();
    }

    addActionToMenu(ac->action("panels"), menu);
    KMenu* locationBarMenu = new KMenu(i18nc("@action:inmenu", "Location Bar"), menu);
    locationBarMenu->addAction(ac->action("editable_location"));
    locationBarMenu->addAction(ac->action("replace_location"));
    menu->addMenu(locationBarMenu);

    menu->addSeparator();

    // Add "Go" menu
    KMenu* goMenu = new KMenu(i18nc("@action:inmenu", "Go"), menu);
    connect(menu, SIGNAL(aboutToHide()), goMenu, SLOT(deleteLater()));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Back)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Forward)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Up)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Home)));
    goMenu->addAction(ac->action("closed_tabs"));
    menu->addMenu(goMenu);

    // Add "Tool" menu
    KMenu* toolsMenu = new KMenu(i18nc("@action:inmenu", "Tools"), menu);
    connect(menu, SIGNAL(aboutToHide()), toolsMenu, SLOT(deleteLater()));
    toolsMenu->addAction(ac->action("show_filter_bar"));
    toolsMenu->addAction(ac->action("compare_files"));
    toolsMenu->addAction(ac->action("open_terminal"));
    toolsMenu->addAction(ac->action("change_remote_encoding"));
    menu->addMenu(toolsMenu);

    // Add "Settings" menu entries
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::KeyBindings)), menu);
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::ConfigureToolbars)), menu);
    addActionToMenu(ac->action(KStandardAction::name(KStandardAction::Preferences)), menu);

    // Add "Help" menu
    KMenu* helpMenu = new KMenu(i18nc("@action:inmenu", "Help"), menu);
    connect(menu, SIGNAL(aboutToHide()), helpMenu, SLOT(deleteLater()));
    helpMenu->addAction(ac->action(KStandardAction::name(KStandardAction::HelpContents)));
    helpMenu->addAction(ac->action(KStandardAction::name(KStandardAction::WhatsThis)));
    helpMenu->addSeparator();
    helpMenu->addAction(ac->action(KStandardAction::name(KStandardAction::ReportBug)));
    helpMenu->addSeparator();
    helpMenu->addAction(ac->action(KStandardAction::name(KStandardAction::SwitchApplicationLanguage)));
    helpMenu->addSeparator();
    helpMenu->addAction(ac->action(KStandardAction::name(KStandardAction::AboutApp)));
    helpMenu->addAction(ac->action(KStandardAction::name(KStandardAction::AboutKDE)));
    menu->addMenu(helpMenu);

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
    m_controlButton = 0;
    m_updateToolBarTimer->start();
}

void DolphinMainWindow::slotPanelErrorMessage(const QString& error)
{
    activeViewContainer()->showMessage(error, DolphinViewContainer::Error);
}

void DolphinMainWindow::activeViewChanged()
{
    DolphinViewContainer* container = activeViewContainer();
    Q_ASSERT(container);

    m_actionHandler->setCurrentView(container->view());
    m_viewSignalAdapter->setViewContainer(container);

    updateHistory();
    updateEditActions();
    updateViewActions();
    updateGoActions();

    const KUrl& url = container->url();
    setUrlAsCaption(url);

    emit urlChanged(url);
}

void DolphinMainWindow::slotTabsCountChanged(int count)
{
    actionCollection()->action("close_tab")->setEnabled(count > 1);
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newFileMenu = new DolphinNewFileMenu(actionCollection(), this);
    KMenu* menu = m_newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(KIcon("document-new"));
    m_newFileMenu->setDelayed(false);
    connect(menu, SIGNAL(aboutToShow()),
            this, SLOT(updateNewMenu()));

    KAction* newWindow = actionCollection()->addAction("new_window");
    newWindow->setIcon(KIcon("window-new"));
    newWindow->setText(i18nc("@action:inmenu File", "New &Window"));
    newWindow->setShortcut(Qt::CTRL | Qt::Key_N);
    connect(newWindow, SIGNAL(triggered()), this, SLOT(openNewMainWindow()));

    KAction* newTab = actionCollection()->addAction("new_tab");
    newTab->setIcon(KIcon("tab-new"));
    newTab->setText(i18nc("@action:inmenu File", "New Tab"));
    newTab->setShortcut(KShortcut(Qt::CTRL | Qt::Key_T, Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    connect(newTab, SIGNAL(triggered()), tabWidget(), SLOT(openNewTab()));

    KAction* closeTab = actionCollection()->addAction("close_tab");
    closeTab->setIcon(KIcon("tab-close"));
    closeTab->setText(i18nc("@action:inmenu File", "Close Tab"));
    closeTab->setShortcut(Qt::CTRL | Qt::Key_W);
    closeTab->setEnabled(false);
    connect(closeTab, SIGNAL(triggered()), tabWidget(), SLOT(closeTab()));

    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    // setup 'Edit' menu
    KStandardAction::undo(this,
                          SLOT(undo()),
                          actionCollection());

    // need to remove shift+del from cut action, else the shortcut for deletejob
    // doesn't work
    KAction* cut = KStandardAction::cut(this, SLOT(cut()), actionCollection());
    KShortcut cutShortcut = cut->shortcut();
    cutShortcut.remove(Qt::SHIFT | Qt::Key_Delete, KShortcut::KeepEmpty);
    cut->setShortcut(cutShortcut);
    KStandardAction::copy(this, SLOT(copy()), actionCollection());
    KAction* paste = KStandardAction::paste(this, SLOT(paste()), actionCollection());
    // The text of the paste-action is modified dynamically by Dolphin
    // (e. g. to "Paste One Folder"). To prevent that the size of the toolbar changes
    // due to the long text, the text "Paste" is used:
    paste->setIconText(i18nc("@action:inmenu Edit", "Paste"));

    KStandardAction::find(this, SLOT(find()), actionCollection());

    KAction* selectAll = actionCollection()->addAction("select_all");
    selectAll->setText(i18nc("@action:inmenu Edit", "Select All"));
    selectAll->setShortcut(Qt::CTRL | Qt::Key_A);
    connect(selectAll, SIGNAL(triggered()), this, SLOT(selectAll()));

    KAction* invertSelection = actionCollection()->addAction("invert_selection");
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    invertSelection->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, SIGNAL(triggered()), this, SLOT(invertSelection()));

    // setup 'View' menu
    // (note that most of it is set up in DolphinViewActionHandler)

    KAction* split = actionCollection()->addAction("split_view");
    split->setShortcut(Qt::Key_F3);
    connect(split, SIGNAL(triggered()), tabWidget(), SLOT(toggleSplitView()));

    KAction* reload = actionCollection()->addAction("reload");
    reload->setText(i18nc("@action:inmenu View", "Reload"));
    reload->setShortcut(Qt::Key_F5);
    reload->setIcon(KIcon("view-refresh"));
    connect(reload, SIGNAL(triggered()), this, SLOT(reloadView()));

    KAction* stop = actionCollection()->addAction("stop");
    stop->setText(i18nc("@action:inmenu View", "Stop"));
    stop->setToolTip(i18nc("@info", "Stop loading"));
    stop->setIcon(KIcon("process-stop"));
    connect(stop, SIGNAL(triggered()), this, SLOT(stopLoading()));

    KToggleAction* editableLocation = actionCollection()->add<KToggleAction>("editable_location");
    editableLocation->setText(i18nc("@action:inmenu Navigation Bar", "Editable Location"));
    editableLocation->setShortcut(Qt::Key_F6);
    connect(editableLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    KAction* replaceLocation = actionCollection()->addAction("replace_location");
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    replaceLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(replaceLocation, SIGNAL(triggered()), this, SLOT(replaceLocation()));

    // setup 'Go' menu
    KAction* backAction = KStandardAction::back(this, SLOT(goBack()), actionCollection());
    connect(backAction, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this, SLOT(goBack(Qt::MouseButtons)));
    KShortcut backShortcut = backAction->shortcut();
    backShortcut.setAlternate(Qt::Key_Backspace);
    backAction->setShortcut(backShortcut);

    DolphinRecentTabsMenu* recentTabsMenu = new DolphinRecentTabsMenu(this);
    actionCollection()->addAction("closed_tabs", recentTabsMenu);
    connect(tabWidget(), SIGNAL(rememberClosedTab(ClosedTab)),
            recentTabsMenu, SLOT(rememberClosedTab(ClosedTab)));
    connect(recentTabsMenu, SIGNAL(restoreClosedTab(ClosedTab)),
            tabWidget(), SLOT(restoreClosedTab(ClosedTab)));

    KAction* forwardAction = KStandardAction::forward(this, SLOT(goForward()), actionCollection());
    connect(forwardAction, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this, SLOT(goForward(Qt::MouseButtons)));

    KAction* upAction = KStandardAction::up(this, SLOT(goUp()), actionCollection());
    connect(upAction, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this, SLOT(goUp(Qt::MouseButtons)));

    KAction* homeAction = KStandardAction::home(this, SLOT(goHome()), actionCollection());
    connect(homeAction, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this, SLOT(goHome(Qt::MouseButtons)));

    // setup 'Tools' menu
    KAction* showFilterBar = actionCollection()->addAction("show_filter_bar");
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Show Filter Bar"));
    showFilterBar->setIcon(KIcon("view-filter"));
    showFilterBar->setShortcut(Qt::CTRL | Qt::Key_I);
    connect(showFilterBar, SIGNAL(triggered()), this, SLOT(showFilterBar()));

    KAction* compareFiles = actionCollection()->addAction("compare_files");
    compareFiles->setText(i18nc("@action:inmenu Tools", "Compare Files"));
    compareFiles->setIcon(KIcon("kompare"));
    compareFiles->setEnabled(false);
    connect(compareFiles, SIGNAL(triggered()), this, SLOT(compareFiles()));

    KAction* openTerminal = actionCollection()->addAction("open_terminal");
    openTerminal->setText(i18nc("@action:inmenu Tools", "Open Terminal"));
    openTerminal->setIcon(KIcon("utilities-terminal"));
    openTerminal->setShortcut(Qt::SHIFT | Qt::Key_F4);
    connect(openTerminal, SIGNAL(triggered()), this, SLOT(openTerminal()));

    // setup 'Settings' menu
    KToggleAction* showMenuBar = KStandardAction::showMenubar(0, 0, actionCollection());
    connect(showMenuBar, SIGNAL(triggered(bool)),                   // Fixes #286822
            this, SLOT(toggleShowMenuBar()), Qt::QueuedConnection);
    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());

    // not in menu actions
    QList<QKeySequence> nextTabKeys;
    nextTabKeys.append(KStandardShortcut::tabNext().primary());
    nextTabKeys.append(QKeySequence(Qt::CTRL | Qt::Key_Tab));

    QList<QKeySequence> prevTabKeys;
    prevTabKeys.append(KStandardShortcut::tabPrev().primary());
    prevTabKeys.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));

    KAction* activateNextTab = actionCollection()->addAction("activate_next_tab");
    activateNextTab->setText(i18nc("@action:inmenu", "Activate Next Tab"));
    connect(activateNextTab, SIGNAL(triggered()), tabWidget(), SLOT(activateNextTab()));
    activateNextTab->setShortcuts(QApplication::isRightToLeft() ? prevTabKeys : nextTabKeys);

    KAction* activatePrevTab = actionCollection()->addAction("activate_prev_tab");
    activatePrevTab->setText(i18nc("@action:inmenu", "Activate Previous Tab"));
    connect(activatePrevTab, SIGNAL(triggered()), tabWidget(), SLOT(activatePrevTab()));
    activatePrevTab->setShortcuts(QApplication::isRightToLeft() ? nextTabKeys : prevTabKeys);

    // for context menu
    KAction* openInNewTab = actionCollection()->addAction("open_in_new_tab");
    openInNewTab->setText(i18nc("@action:inmenu", "Open in New Tab"));
    openInNewTab->setIcon(KIcon("tab-new"));
    connect(openInNewTab, SIGNAL(triggered()), this, SLOT(openInNewTab()));

    KAction* openInNewTabs = actionCollection()->addAction("open_in_new_tabs");
    openInNewTabs->setText(i18nc("@action:inmenu", "Open in New Tabs"));
    openInNewTabs->setIcon(KIcon("tab-new"));
    connect(openInNewTabs, SIGNAL(triggered()), this, SLOT(openInNewTab()));

    KAction* openInNewWindow = actionCollection()->addAction("open_in_new_window");
    openInNewWindow->setText(i18nc("@action:inmenu", "Open in New Window"));
    openInNewWindow->setIcon(KIcon("window-new"));
    connect(openInNewWindow, SIGNAL(triggered()), this, SLOT(openInNewWindow()));
}

void DolphinMainWindow::setupDockWidgets()
{
    const bool lock = GeneralSettings::lockPanels();

    KDualAction* lockLayoutAction = actionCollection()->add<KDualAction>("lock_panels");
    lockLayoutAction->setActiveText(i18nc("@action:inmenu Panels", "Unlock Panels"));
    lockLayoutAction->setActiveIcon(KIcon("object-unlocked"));
    lockLayoutAction->setInactiveText(i18nc("@action:inmenu Panels", "Lock Panels"));
    lockLayoutAction->setInactiveIcon(KIcon("object-locked"));
    lockLayoutAction->setActive(lock);
    connect(lockLayoutAction, SIGNAL(triggered()), this, SLOT(togglePanelLockState()));

    // Setup "Information"
    DolphinDockWidget* infoDock = new DolphinDockWidget(i18nc("@title:window", "Information"));
    infoDock->setLocked(lock);
    infoDock->setObjectName("infoDock");
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    Panel* infoPanel = new InformationPanel(infoDock);
    infoPanel->setCustomContextMenuActions(QList<QAction*>() << lockLayoutAction);
    connect(infoPanel, SIGNAL(urlActivated(KUrl)), this, SLOT(handleUrl(KUrl)));
    infoDock->setWidget(infoPanel);

    QAction* infoAction = infoDock->toggleViewAction();
    createPanelAction(KIcon("dialog-information"), Qt::Key_F11, infoAction, "show_information_panel");

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            infoPanel, SLOT(setUrl(KUrl)));
    connect(this, SIGNAL(selectionChanged(KFileItemList)),
            infoPanel, SLOT(setSelection(KFileItemList)));
    connect(this, SIGNAL(requestItemInfo(KFileItem)),
            infoPanel, SLOT(requestDelayedItemInfo(KFileItem)));

    // Setup "Folders"
    DolphinDockWidget* foldersDock = new DolphinDockWidget(i18nc("@title:window", "Folders"));
    foldersDock->setLocked(lock);
    foldersDock->setObjectName("foldersDock");
    foldersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    FoldersPanel* foldersPanel = new FoldersPanel(foldersDock);
    foldersPanel->setCustomContextMenuActions(QList<QAction*>() << lockLayoutAction);
    foldersDock->setWidget(foldersPanel);

    QAction* foldersAction = foldersDock->toggleViewAction();
    createPanelAction(KIcon("folder"), Qt::Key_F7, foldersAction, "show_folders_panel");

    addDockWidget(Qt::LeftDockWidgetArea, foldersDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            foldersPanel, SLOT(setUrl(KUrl)));
    connect(foldersPanel, SIGNAL(folderActivated(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(foldersPanel, SIGNAL(folderMiddleClicked(KUrl)),
            tabWidget(), SLOT(openNewTab(KUrl)));
    connect(foldersPanel, SIGNAL(errorMessage(QString)),
            this, SLOT(slotPanelErrorMessage(QString)));

    // Setup "Terminal"
#ifndef Q_OS_WIN
    DolphinDockWidget* terminalDock = new DolphinDockWidget(i18nc("@title:window Shell terminal", "Terminal"));
    terminalDock->setLocked(lock);
    terminalDock->setObjectName("terminalDock");
    terminalDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    Panel* terminalPanel = new TerminalPanel(terminalDock);
    terminalPanel->setCustomContextMenuActions(QList<QAction*>() << lockLayoutAction);
    terminalDock->setWidget(terminalPanel);

    connect(terminalPanel, SIGNAL(hideTerminalPanel()), terminalDock, SLOT(hide()));
    connect(terminalPanel, SIGNAL(changeUrl(KUrl)), this, SLOT(slotTerminalDirectoryChanged(KUrl)));
    connect(terminalDock, SIGNAL(visibilityChanged(bool)),
            terminalPanel, SLOT(dockVisibilityChanged()));

    QAction* terminalAction = terminalDock->toggleViewAction();
    createPanelAction(KIcon("utilities-terminal"), Qt::Key_F4, terminalAction, "show_terminal_panel");

    addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            terminalPanel, SLOT(setUrl(KUrl)));
#endif

    if (GeneralSettings::version() < 200) {
        infoDock->hide();
        foldersDock->hide();
#ifndef Q_OS_WIN
        terminalDock->hide();
#endif
    }

    // Setup "Places"
    DolphinDockWidget* placesDock = new DolphinDockWidget(i18nc("@title:window", "Places"));
    placesDock->setLocked(lock);
    placesDock->setObjectName("placesDock");
    placesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    PlacesPanel* placesPanel = new PlacesPanel(placesDock);
    placesPanel->setCustomContextMenuActions(QList<QAction*>() << lockLayoutAction);
    placesDock->setWidget(placesPanel);

    QAction* placesAction = placesDock->toggleViewAction();
    createPanelAction(KIcon("bookmarks"), Qt::Key_F9, placesAction, "show_places_panel");

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(placesPanel, SIGNAL(placeActivated(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(placesPanel, SIGNAL(placeMiddleClicked(KUrl)),
            tabWidget(), SLOT(openNewTab(KUrl)));
    connect(placesPanel, SIGNAL(errorMessage(QString)),
            this, SLOT(slotPanelErrorMessage(QString)));
    connect(this, SIGNAL(urlChanged(KUrl)),
            placesPanel, SLOT(setUrl(KUrl)));
    connect(placesDock, SIGNAL(visibilityChanged(bool)),
            &PlacesPanelVisibilityObserver::instance(), SLOT(placesPanelVisibilityChanged(bool)));
    connect(this, SIGNAL(settingsChanged()),
	    placesPanel, SLOT(readSettings()));

    // Add actions into the "Panels" menu
    KActionMenu* panelsMenu = new KActionMenu(i18nc("@action:inmenu View", "Panels"), this);
    actionCollection()->addAction("panels", panelsMenu);
    panelsMenu->setDelayed(false);
    const KActionCollection* ac = actionCollection();
    panelsMenu->addAction(ac->action("show_places_panel"));
    panelsMenu->addAction(ac->action("show_information_panel"));
    panelsMenu->addAction(ac->action("show_folders_panel"));
#ifndef Q_OS_WIN
    panelsMenu->addAction(ac->action("show_terminal_panel"));
#endif
    panelsMenu->addSeparator();
    panelsMenu->addAction(lockLayoutAction);
}

void DolphinMainWindow::updateEditActions()
{
    const KFileItemList list = activeViewContainer()->view()->selectedItems();
    if (list.isEmpty()) {
        stateChanged("has_no_selection");
    } else {
        stateChanged("has_selection");

        KActionCollection* col = actionCollection();
        QAction* renameAction      = col->action("rename");
        QAction* moveToTrashAction = col->action("move_to_trash");
        QAction* deleteAction      = col->action("delete");
        QAction* cutAction         = col->action(KStandardAction::name(KStandardAction::Cut));
        QAction* deleteWithTrashShortcut = col->action("delete_shortcut"); // see DolphinViewActionHandler

        KFileItemListProperties capabilities(list);
        const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();

        renameAction->setEnabled(capabilities.supportsMoving());
        moveToTrashAction->setEnabled(enableMoveToTrash);
        deleteAction->setEnabled(capabilities.supportsDeleting());
        deleteWithTrashShortcut->setEnabled(capabilities.supportsDeleting() && !enableMoveToTrash);
        cutAction->setEnabled(capabilities.supportsMoving());
    }
    updatePasteAction();
}

void DolphinMainWindow::updateViewActions()
{
    m_actionHandler->updateViewActions();

    QAction* showFilterBarAction = actionCollection()->action("show_filter_bar");
    showFilterBarAction->setChecked(activeViewContainer()->isFilterBarVisible());

    updateSplitAction();

    QAction* editableLocactionAction = actionCollection()->action("editable_location");
    const KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
    editableLocactionAction->setChecked(urlNavigator->isUrlEditable());
}

void DolphinMainWindow::updateGoActions()
{
    QAction* goUpAction = actionCollection()->action(KStandardAction::name(KStandardAction::Up));
    const KUrl currentUrl = activeViewContainer()->url();
    goUpAction->setEnabled(currentUrl.upUrl() != currentUrl);
}

void DolphinMainWindow::createControlButton()
{
    if (m_controlButton) {
        return;
    }
    Q_ASSERT(!m_controlButton);

    m_controlButton = new QToolButton(this);
    m_controlButton->setIcon(KIcon("applications-system"));
    m_controlButton->setText(i18nc("@action", "Control"));
    m_controlButton->setPopupMode(QToolButton::InstantPopup);
    m_controlButton->setToolButtonStyle(toolBar()->toolButtonStyle());

    KMenu* controlMenu = new KMenu(m_controlButton);
    connect(controlMenu, SIGNAL(aboutToShow()), this, SLOT(updateControlMenu()));

    m_controlButton->setMenu(controlMenu);

    toolBar()->addWidget(m_controlButton);
    connect(toolBar(), SIGNAL(iconSizeChanged(QSize)),
            m_controlButton, SLOT(setIconSize(QSize)));
    connect(toolBar(), SIGNAL(toolButtonStyleChanged(Qt::ToolButtonStyle)),
            m_controlButton, SLOT(setToolButtonStyle(Qt::ToolButtonStyle)));

    // The added widgets are owned by the toolbar and may get deleted when e.g. the toolbar
    // gets edited. In this case we must add them again. The adding is done asynchronously by
    // m_updateToolBarTimer.
    connect(m_controlButton, SIGNAL(destroyed()), this, SLOT(slotControlButtonDeleted()));
    m_updateToolBarTimer = new QTimer(this);
    m_updateToolBarTimer->setInterval(500);
    connect(m_updateToolBarTimer, SIGNAL(timeout()), this, SLOT(updateToolBar()));
}

void DolphinMainWindow::deleteControlButton()
{
    delete m_controlButton;
    m_controlButton = 0;

    delete m_updateToolBarTimer;
    m_updateToolBarTimer = 0;
}

bool DolphinMainWindow::addActionToMenu(QAction* action, KMenu* menu)
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
    const int tabCount = tabWidget()->count();
    for (int i = 0; i < tabCount; ++i) {
        tabWidget()->tabPageAt(i)->refresh();
    }

    if (GeneralSettings::modifiedStartupSettings()) {
        // The startup settings have been changed by the user (see bug #254947).
        // Synchronize the split-view setting with the active view:
        const bool splitView = GeneralSettings::splitView();
        tabWidget()->currentTabPage()->setSplitViewEnabled(splitView);
    }

    emit settingsChanged();
}

void DolphinMainWindow::clearStatusBar()
{
    activeViewContainer()->statusBar()->resetToDefaultText();
}

void DolphinMainWindow::updateSplitAction()
{
    QAction* splitAction = actionCollection()->action("split_view");
    DolphinTabPage* tabPage = tabWidget()->currentTabPage();
    if (tabPage->isSplitViewEnabled()) {
        if (tabPage->activeView() == DolphinTabPage::SecondaryView) {
            splitAction->setText(i18nc("@action:intoolbar Close right view", "Close"));
            splitAction->setToolTip(i18nc("@info", "Close right view"));
            splitAction->setIcon(KIcon("view-right-close"));
        } else {
            splitAction->setText(i18nc("@action:intoolbar Close left view", "Close"));
            splitAction->setToolTip(i18nc("@info", "Close left view"));
            splitAction->setIcon(KIcon("view-left-close"));
        }
    } else {
        splitAction->setText(i18nc("@action:intoolbar Split view", "Split"));
        splitAction->setToolTip(i18nc("@info", "Split view"));
        splitAction->setIcon(KIcon("view-right-new"));
    }
}

bool DolphinMainWindow::isKompareInstalled() const
{
    static bool initialized = false;
    static bool installed = false;
    if (!initialized) {
        // TODO: maybe replace this approach later by using a menu
        // plugin like kdiff3plugin.cpp
        installed = !KGlobal::dirs()->findExe("kompare").isEmpty();
        initialized = true;
    }
    return installed;
}

void DolphinMainWindow::setUrlAsCaption(const KUrl& url)
{
    QString caption;
    if (!url.isLocalFile()) {
        caption.append(url.protocol() + " - ");
        if (url.hasHost()) {
            caption.append(url.host() + " - ");
        }
    }

    const QString fileName = url.fileName().isEmpty() ? "/" : url.fileName();
    caption.append(fileName);

    setCaption(caption);
}

void DolphinMainWindow::createPanelAction(const KIcon& icon,
                                          const QKeySequence& shortcut,
                                          QAction* dockAction,
                                          const QString& actionName)
{
    KAction* panelAction = actionCollection()->addAction(actionName);
    panelAction->setCheckable(true);
    panelAction->setChecked(dockAction->isChecked());
    panelAction->setText(dockAction->text());
    panelAction->setIcon(icon);
    panelAction->setShortcut(shortcut);

    connect(panelAction, SIGNAL(triggered()), dockAction, SLOT(trigger()));
    connect(dockAction, SIGNAL(toggled(bool)), panelAction, SLOT(setChecked(bool)));
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

#include "dolphinmainwindow.moc"
