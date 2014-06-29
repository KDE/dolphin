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
#include <KDialog>
#include <KJobWidgets>
#include <KLineEdit>
#include <KToolBar>
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
#include <ktabbar.h>
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
#include <QSplitter>
#include <QTimer>
#include <QPushButton>

namespace {
    // Used for GeneralSettings::version() to determine whether
    // an updated version of Dolphin is running.
    const int CurrentDolphinVersion = 200;
};

DolphinMainWindow::DolphinMainWindow() :
    KXmlGuiWindow(0),
    m_newFileMenu(0),
    m_tabBar(0),
    m_activeViewContainer(0),
    m_centralWidgetLayout(0),
    m_tabIndex(0),
    m_viewTab(),
    m_actionHandler(0),
    m_remoteEncoding(0),
    m_settingsDialog(),
    m_controlButton(0),
    m_updateToolBarTimer(0),
    m_lastHandleUrlStatJob(0)
{
    setObjectName("Dolphin#");

    m_viewTab.append(ViewTab());
    ViewTab& viewTab = m_viewTab[m_tabIndex];
    viewTab.wasActive = true; // The first opened tab is automatically active

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

    viewTab.splitter = new QSplitter(this);
    viewTab.splitter->setChildrenCollapsible(false);

    setupActions();

    const KUrl homeUrl(generalSettings->homeUrl());
    setUrlAsCaption(homeUrl);
    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    connect(m_actionHandler, &DolphinViewActionHandler::actionBeingHandled, this, &DolphinMainWindow::clearStatusBar);
    connect(m_actionHandler, &DolphinViewActionHandler::createDirectory, this, &DolphinMainWindow::createDirectory);

    viewTab.primaryView = createViewContainer(homeUrl, viewTab.splitter);

    m_activeViewContainer = viewTab.primaryView;
    connectViewSignals(m_activeViewContainer);
    DolphinView* view = m_activeViewContainer->view();
    m_activeViewContainer->show();
    m_actionHandler->setCurrentView(view);

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, &DolphinMainWindow::urlChanged,
            m_remoteEncoding, &DolphinRemoteEncoding::slotAboutToOpenUrl);

    m_tabBar = new KTabBar(this);
    m_tabBar->setMovable(true);
    m_tabBar->setTabsClosable(true);
    connect(m_tabBar, &KTabBar::currentChanged,
            this, &DolphinMainWindow::setActiveTab);
    connect(m_tabBar, &KTabBar::tabCloseRequested,
            this, static_cast<void(DolphinMainWindow::*)(int)>(&DolphinMainWindow::closeTab));
    connect(m_tabBar, &KTabBar::contextMenu,
            this, &DolphinMainWindow::openTabContextMenu);
    connect(m_tabBar, &KTabBar::newTabRequest,
            this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::openNewTab));
    connect(m_tabBar, &KTabBar::testCanDecode,
            this, &DolphinMainWindow::slotTestCanDecode);
    connect(m_tabBar, &KTabBar::mouseMiddleClick,
            this, static_cast<void(DolphinMainWindow::*)(int)>(&DolphinMainWindow::closeTab));
    connect(m_tabBar, &KTabBar::tabMoved,
            this, &DolphinMainWindow::slotTabMoved);
    connect(m_tabBar, &KTabBar::receivedDropEvent,
            this, &DolphinMainWindow::tabDropEvent);

    m_tabBar->blockSignals(true);  // signals get unblocked after at least 2 tabs are open

    QWidget* centralWidget = new QWidget(this);
    m_centralWidgetLayout = new QVBoxLayout(centralWidget);
    m_centralWidgetLayout->setSpacing(0);
    m_centralWidgetLayout->setMargin(0);
    m_centralWidgetLayout->addWidget(m_tabBar);
    m_centralWidgetLayout->addWidget(viewTab.splitter, 1);

    setCentralWidget(centralWidget);
    setupDockWidgets();
    emit urlChanged(homeUrl);

    setupGUI(Keys | Save | Create | ToolBar);
    stateChanged("new_file");

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, &QClipboard::dataChanged,
            this, &DolphinMainWindow::updatePasteAction);

    if (generalSettings->splitView()) {
        toggleSplitView();
    }
    updateEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();

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
}

DolphinMainWindow::~DolphinMainWindow()
{
}

void DolphinMainWindow::openDirectories(const QList<KUrl>& dirs)
{
    if (dirs.isEmpty()) {
        return;
    }

    if (dirs.count() == 1) {
        m_activeViewContainer->setUrl(dirs.first());
        return;
    }

    const int oldOpenTabsCount = m_viewTab.count();

    const bool hasSplitView = GeneralSettings::splitView();

    // Open each directory inside a new tab. If the "split view" option has been enabled,
    // always show two directories within one tab.
    QList<KUrl>::const_iterator it = dirs.constBegin();
    while (it != dirs.constEnd()) {
        openNewTab(*it);
        ++it;

        if (hasSplitView && (it != dirs.constEnd())) {
            const int tabIndex = m_viewTab.count() - 1;
            m_viewTab[tabIndex].secondaryView->setUrl(*it);
            ++it;
        }
    }

    // Remove the previously opened tabs
    for (int i = 0; i < oldOpenTabsCount; ++i) {
        closeTab(0);
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
    const int tabCount = m_viewTab.count();
    for (int i = 0; i < tabCount; ++i) {
        m_viewTab[i].primaryView->view()->markUrlsAsSelected(files);
        m_viewTab[i].primaryView->view()->markUrlAsCurrent(files.at(0));
        if (m_viewTab[i].secondaryView) {
            m_viewTab[i].secondaryView->view()->markUrlsAsSelected(files);
            m_viewTab[i].secondaryView->view()->markUrlAsCurrent(files.at(0));
        }
    }
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
        updatePasteAction();
        updateViewActions();
        updateGoActions();
        setUrlAsCaption(url);
        if (m_viewTab.count() > 1) {
            m_tabBar->setTabText(m_tabIndex, squeezedText(tabName(m_activeViewContainer->url())));
        }
        const QString iconName = KIO::iconNameForUrl(url);
        m_tabBar->setTabIcon(m_tabIndex, QIcon::fromTheme(iconName));
        emit urlChanged(url);
    }
}

void DolphinMainWindow::slotTerminalDirectoryChanged(const KUrl& url)
{
    m_activeViewContainer->setAutoGrabFocus(false);
    changeUrl(url);
    m_activeViewContainer->setAutoGrabFocus(true);
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

    Q_ASSERT(m_viewTab[m_tabIndex].primaryView);
    int selectedUrlsCount = m_viewTab[m_tabIndex].primaryView->view()->selectedItemsCount();
    if (m_viewTab[m_tabIndex].secondaryView) {
        selectedUrlsCount += m_viewTab[m_tabIndex].secondaryView->view()->selectedItemsCount();
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
    const KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
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

void DolphinMainWindow::openNewTab()
{
    const bool isUrlEditable =  m_activeViewContainer->urlNavigator()->isUrlEditable();

    openNewTab(m_activeViewContainer->url());
    m_tabBar->setCurrentIndex(m_viewTab.count() - 1);

    // The URL navigator of the new tab should have the same editable state
    // as the current tab
    KUrlNavigator* navigator = m_activeViewContainer->urlNavigator();
    navigator->setUrlEditable(isUrlEditable);

    if (isUrlEditable) {
        // If a new tab is opened and the URL is editable, assure that
        // the user can edit the URL without manually setting the focus
        navigator->setFocus();
    }
}

void DolphinMainWindow::openNewTab(const KUrl& url)
{
    QWidget* focusWidget = QApplication::focusWidget();

    if (m_viewTab.count() == 1) {
        // Only one view is open currently and hence no tab is shown at
        // all. Before creating a tab for 'url', provide a tab for the current URL.
        const KUrl currentUrl = m_activeViewContainer->url();
        m_tabBar->addTab(QIcon::fromTheme(KIO::iconNameForUrl(currentUrl)),
                         squeezedText(tabName(currentUrl)));
        m_tabBar->blockSignals(false);
    }

    m_tabBar->addTab(QIcon::fromTheme(KIO::iconNameForUrl(url)),
                     squeezedText(tabName(url)));

    ViewTab viewTab;
    viewTab.splitter = new QSplitter(this);
    viewTab.splitter->setChildrenCollapsible(false);
    viewTab.primaryView = createViewContainer(url, viewTab.splitter);
    viewTab.primaryView->setActive(false);
    connectViewSignals(viewTab.primaryView);

    m_viewTab.append(viewTab);

    actionCollection()->action("close_tab")->setEnabled(true);
    actionCollection()->action("activate_prev_tab")->setEnabled(true);
    actionCollection()->action("activate_next_tab")->setEnabled(true);

    // Provide a split view, if the startup settings are set this way
    if (GeneralSettings::splitView()) {
        const int newTabIndex = m_viewTab.count() - 1;
        createSecondaryView(newTabIndex);
        m_viewTab[newTabIndex].secondaryView->setActive(true);
        m_viewTab[newTabIndex].isPrimaryViewActive = false;
    }

    if (focusWidget) {
        // The DolphinViewContainer grabbed the keyboard focus. As the tab is opened
        // in background, assure that the previous focused widget gets the focus back.
        focusWidget->setFocus();
    }
}

void DolphinMainWindow::openNewActivatedTab(const KUrl& url)
{
    openNewTab(url);
    m_tabBar->setCurrentIndex(m_viewTab.count() - 1);
}

void DolphinMainWindow::activateNextTab()
{
    if (m_viewTab.count() >= 2) {
        const int tabIndex = (m_tabBar->currentIndex() + 1) % m_tabBar->count();
        m_tabBar->setCurrentIndex(tabIndex);
    }
}

void DolphinMainWindow::activatePrevTab()
{
    if (m_viewTab.count() >= 2) {
        int tabIndex = m_tabBar->currentIndex() - 1;
        if (tabIndex == -1) {
            tabIndex = m_tabBar->count() - 1;
        }
        m_tabBar->setCurrentIndex(tabIndex);
    }
}

void DolphinMainWindow::openInNewTab()
{
    const KFileItemList& list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        openNewTab(m_activeViewContainer->url());
    } else {
        foreach (const KFileItem& item, list) {
            const KUrl& url = DolphinView::openItemAsFolderUrl(item);
            if (!url.isEmpty()) {
                openNewTab(url);
            }
        }
    }
}

void DolphinMainWindow::openInNewWindow()
{
    KUrl newWindowUrl;

    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        newWindowUrl = m_activeViewContainer->url();
    } else if (list.count() == 1) {
        const KFileItem& item = list.first();
        newWindowUrl = DolphinView::openItemAsFolderUrl(item);
    }

    if (!newWindowUrl.isEmpty()) {
        KRun::run("dolphin %u", QList<QUrl>() << newWindowUrl, this);
    }
}

void DolphinMainWindow::toggleActiveView()
{
    if (!m_viewTab[m_tabIndex].secondaryView) {
        // only one view is available
        return;
    }

    Q_ASSERT(m_activeViewContainer);
    Q_ASSERT(m_viewTab[m_tabIndex].primaryView);

    DolphinViewContainer* left  = m_viewTab[m_tabIndex].primaryView;
    DolphinViewContainer* right = m_viewTab[m_tabIndex].secondaryView;
    setActiveViewContainer(m_activeViewContainer == right ? left : right);
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
    DolphinApplication *application = qobject_cast<DolphinApplication*>(qApp);
    if (application && application->sessionSaving()) {
        closedByUser = false;
    }

    if (m_viewTab.count() > 1 && GeneralSettings::confirmClosingMultipleTabs() && closedByUser) {
        // Ask the user if he really wants to quit and close all tabs.
        // Open a confirmation dialog with 3 buttons:
        // KDialog::Yes    -> Quit
        // KDialog::No     -> Close only the current tab
        // KDialog::Cancel -> do nothing
        QDialog *dialog = new QDialog(this, Qt::Dialog);
        dialog->setWindowTitle(i18nc("@title:window", "Confirmation"));
        dialog->setModal(true);
        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Yes | QDialogButtonBox::No | QDialogButtonBox::Cancel);
        KGuiItem::assign(buttons->button(QDialogButtonBox::Yes), KStandardGuiItem::quit());
        KGuiItem::assign(buttons->button(QDialogButtonBox::No), KGuiItem(i18n("C&lose Current Tab"), QIcon::fromTheme("tab-close")));
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
                closeTab();
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
    const int tabCount = m_viewTab.count();
    group.writeEntry("Tab Count", tabCount);
    group.writeEntry("Active Tab Index", m_tabBar->currentIndex());

    for (int i = 0; i < tabCount; ++i) {
        const DolphinViewContainer* cont = m_viewTab[i].primaryView;
        group.writeEntry(tabProperty("Primary URL", i), cont->url().url());
        group.writeEntry(tabProperty("Primary Editable", i),
                         cont->urlNavigator()->isUrlEditable());

        cont = m_viewTab[i].secondaryView;
        if (cont) {
            group.writeEntry(tabProperty("Secondary URL", i), cont->url().url());
            group.writeEntry(tabProperty("Secondary Editable", i),
                             cont->urlNavigator()->isUrlEditable());
        }
    }
}

void DolphinMainWindow::readProperties(const KConfigGroup& group)
{
    const int tabCount = group.readEntry("Tab Count", 1);
    for (int i = 0; i < tabCount; ++i) {
        DolphinViewContainer* cont = m_viewTab[i].primaryView;

        cont->setUrl(group.readEntry(tabProperty("Primary URL", i)));
        const bool editable = group.readEntry(tabProperty("Primary Editable", i), false);
        cont->urlNavigator()->setUrlEditable(editable);

        cont = m_viewTab[i].secondaryView;
        const QString secondaryUrl = group.readEntry(tabProperty("Secondary URL", i));
        if (!secondaryUrl.isEmpty()) {
            if (!cont) {
                // a secondary view should be shown, but no one is available
                // currently -> create a new view
                toggleSplitView();
                cont = m_viewTab[i].secondaryView;
                Q_ASSERT(cont);
            }

            // The right view must be activated before the URL is set. Changing
            // the URL in the right view will emit the right URL navigator's
            // urlChanged(KUrl) signal, which is connected to the changeUrl(KUrl)
            // slot. That slot will change the URL in the left view if it is still
            // active. See https://bugs.kde.org/show_bug.cgi?id=330047.
            setActiveViewContainer(cont);

            cont->setUrl(secondaryUrl);
            const bool editable = group.readEntry(tabProperty("Secondary Editable", i), false);
            cont->urlNavigator()->setUrlEditable(editable);
        } else if (cont) {
            // no secondary view should be shown, but the default setting shows
            // one already -> close the view
            toggleSplitView();
        }

        // openNewTab() needs to be called only tabCount - 1 times
        if (i != tabCount - 1) {
            openNewTab();
        }
    }

    const int index = group.readEntry("Active Tab Index", 0);
    m_tabBar->setCurrentIndex(index);
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
    if (!m_viewTab[m_tabIndex].secondaryView) {
        createSecondaryView(m_tabIndex);
        setActiveViewContainer(m_viewTab[m_tabIndex].secondaryView);
    } else if (m_activeViewContainer == m_viewTab[m_tabIndex].secondaryView) {
        // remove secondary view
        m_viewTab[m_tabIndex].secondaryView->close();
        m_viewTab[m_tabIndex].secondaryView->deleteLater();
        m_viewTab[m_tabIndex].secondaryView = 0;

        setActiveViewContainer(m_viewTab[m_tabIndex].primaryView);
    } else {
        // The primary view is active and should be closed. Hence from a users point of view
        // the content of the secondary view should be moved to the primary view.
        // From an implementation point of view it is more efficient to close
        // the primary view and exchange the internal pointers afterwards.

        m_viewTab[m_tabIndex].primaryView->close();
        m_viewTab[m_tabIndex].primaryView->deleteLater();
        m_viewTab[m_tabIndex].primaryView = m_viewTab[m_tabIndex].secondaryView;
        m_viewTab[m_tabIndex].secondaryView = 0;

        setActiveViewContainer(m_viewTab[m_tabIndex].primaryView);
    }

    updateViewActions();
}

void DolphinMainWindow::reloadView()
{
    clearStatusBar();
    m_activeViewContainer->view()->reload();
}

void DolphinMainWindow::stopLoading()
{
    m_activeViewContainer->view()->stopLoading();
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
    m_activeViewContainer->setFilterBarVisible(true);
}

void DolphinMainWindow::toggleEditLocation()
{
    clearStatusBar();

    QAction* action = actionCollection()->action("editable_location");
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

void DolphinMainWindow::slotPlacesPanelVisibilityChanged(bool visible)
{
    const int tabCount = m_viewTab.count();
    for (int i = 0; i < tabCount; ++i) {
        ViewTab& tab = m_viewTab[i];
        Q_ASSERT(tab.primaryView);
        tab.primaryView->urlNavigator()->setPlacesSelectorVisible(!visible);
        if (tab.secondaryView) {
            tab.secondaryView->urlNavigator()->setPlacesSelectorVisible(!visible);
        }
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

void DolphinMainWindow::goBack(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goBack().
    if (buttons == Qt::MidButton) {
        KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
        const int index = urlNavigator->historyIndex() + 1;
        openNewTab(urlNavigator->locationUrl(index));
    }
}

void DolphinMainWindow::goForward(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goForward().
    if (buttons == Qt::MidButton) {
        KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
        const int index = urlNavigator->historyIndex() - 1;
        openNewTab(urlNavigator->locationUrl(index));
    }
}

void DolphinMainWindow::goUp(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goUp().
    if (buttons == Qt::MidButton) {
        openNewTab(activeViewContainer()->url().upUrl());
    }
}

void DolphinMainWindow::goHome(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goHome().
    if (buttons == Qt::MidButton) {
        openNewTab(GeneralSettings::self()->homeUrl());
    }
}

void DolphinMainWindow::compareFiles()
{
    const DolphinViewContainer* primaryViewContainer = m_viewTab[m_tabIndex].primaryView;
    Q_ASSERT(primaryViewContainer);
    KFileItemList items = primaryViewContainer->view()->selectedItems();

    const DolphinViewContainer* secondaryViewContainer = m_viewTab[m_tabIndex].secondaryView;
    if (secondaryViewContainer) {
        items.append(secondaryViewContainer->view()->selectedItems());
    }

    if (items.count() != 2) {
        // The action is disabled in this case, but it could have been triggered
        // via D-Bus, see https://bugs.kde.org/show_bug.cgi?id=325517
        return;
    }

    KUrl urlA = items.at(0).url();
    KUrl urlB = items.at(1).url();

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
    KUrl url = KIO::NetAccess::mostLocalUrl(m_activeViewContainer->url(), this);

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
        connect(settingsDialog, &DolphinSettingsDialog::settingsChanged, this, &DolphinMainWindow::refreshViews);
        settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        settingsDialog->show();
        m_settingsDialog = settingsDialog;
    } else {
        m_settingsDialog.data()->raise();
    }
}

void DolphinMainWindow::setActiveTab(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < m_viewTab.count());
    if (index == m_tabIndex) {
        return;
    }

    // hide current tab content
    ViewTab& hiddenTab = m_viewTab[m_tabIndex];
    hiddenTab.isPrimaryViewActive = hiddenTab.primaryView->isActive();
    hiddenTab.primaryView->setActive(false);
    if (hiddenTab.secondaryView) {
        hiddenTab.secondaryView->setActive(false);
    }
    QSplitter* splitter = m_viewTab[m_tabIndex].splitter;
    splitter->hide();
    m_centralWidgetLayout->removeWidget(splitter);

    // show active tab content
    m_tabIndex = index;

    ViewTab& viewTab = m_viewTab[index];
    m_centralWidgetLayout->addWidget(viewTab.splitter, 1);
    viewTab.primaryView->show();
    if (viewTab.secondaryView) {
        viewTab.secondaryView->show();
    }
    viewTab.splitter->show();

    if (!viewTab.wasActive) {
        viewTab.wasActive = true;

        // If the tab has not been activated yet the size of the KItemListView is
        // undefined and results in an unwanted animation. To prevent this a
        // reloading of the directory gets triggered.
        viewTab.primaryView->view()->reload();
        if (viewTab.secondaryView) {
            viewTab.secondaryView->view()->reload();
        }
    }

    setActiveViewContainer(viewTab.isPrimaryViewActive ? viewTab.primaryView :
                                                         viewTab.secondaryView);
}

void DolphinMainWindow::closeTab()
{
    closeTab(m_tabBar->currentIndex());
}

void DolphinMainWindow::closeTab(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < m_viewTab.count());
    if (m_viewTab.count() == 1) {
        // the last tab may never get closed
        return;
    }

    if (index == m_tabIndex) {
        // The tab that should be closed is the active tab. Activate the
        // previous tab before closing the tab.
        m_tabBar->setCurrentIndex((index > 0) ? index - 1 : 1);
    }

    const KUrl primaryUrl(m_viewTab[index].primaryView->url());
    const KUrl secondaryUrl(m_viewTab[index].secondaryView ? m_viewTab[index].secondaryView->url() : KUrl());
    emit rememberClosedTab(primaryUrl, secondaryUrl);

    // delete tab
    m_viewTab[index].primaryView->deleteLater();
    if (m_viewTab[index].secondaryView) {
        m_viewTab[index].secondaryView->deleteLater();
    }
    m_viewTab[index].splitter->deleteLater();
    m_viewTab.erase(m_viewTab.begin() + index);

    m_tabBar->blockSignals(true);
    m_tabBar->removeTab(index);

    if (m_tabIndex > index) {
        m_tabIndex--;
        Q_ASSERT(m_tabIndex >= 0);
    }

    // if only one tab is left, also remove the tab entry so that
    // closing the last tab is not possible
    if (m_viewTab.count() == 1) {
        m_tabBar->removeTab(0);
        actionCollection()->action("close_tab")->setEnabled(false);
        actionCollection()->action("activate_prev_tab")->setEnabled(false);
        actionCollection()->action("activate_next_tab")->setEnabled(false);
    } else {
        m_tabBar->blockSignals(false);
    }
}

void DolphinMainWindow::openTabContextMenu(int index, const QPoint& pos)
{
    KMenu menu(this);

    QAction* newTabAction = menu.addAction(QIcon::fromTheme("tab-new"), i18nc("@action:inmenu", "New Tab"));
    newTabAction->setShortcut(actionCollection()->action("new_tab")->shortcut());

    QAction* detachTabAction = menu.addAction(QIcon::fromTheme("tab-detach"), i18nc("@action:inmenu", "Detach Tab"));

    QAction* closeOtherTabsAction = menu.addAction(QIcon::fromTheme("tab-close-other"), i18nc("@action:inmenu", "Close Other Tabs"));

    QAction* closeTabAction = menu.addAction(QIcon::fromTheme("tab-close"), i18nc("@action:inmenu", "Close Tab"));
    closeTabAction->setShortcut(actionCollection()->action("close_tab")->shortcut());
    QAction* selectedAction = menu.exec(pos);
    if (selectedAction == newTabAction) {
        const ViewTab& tab = m_viewTab[index];
        Q_ASSERT(tab.primaryView);
        const KUrl url = tab.secondaryView && tab.secondaryView->isActive() ?
                         tab.secondaryView->url() : tab.primaryView->url();
        openNewTab(url);
        m_tabBar->setCurrentIndex(m_viewTab.count() - 1);
    } else if (selectedAction == detachTabAction) {
        const QString separator(QLatin1Char(' '));
        QString command = QLatin1String("dolphin");

        const ViewTab& tab = m_viewTab[index];
        Q_ASSERT(tab.primaryView);

        command += separator + tab.primaryView->url().url();
        if (tab.secondaryView) {
            command += separator + tab.secondaryView->url().url();
            command += separator + QLatin1String("-split");
        }

        KRun::runCommand(command, this);

        closeTab(index);
    } else if (selectedAction == closeOtherTabsAction) {
        const int count = m_tabBar->count();
        for (int i = 0; i < index; ++i) {
            closeTab(0);
        }
        for (int i = index + 1; i < count; ++i) {
            closeTab(1);
        }
    } else if (selectedAction == closeTabAction) {
        closeTab(index);
    }
}

void DolphinMainWindow::slotTabMoved(int from, int to)
{
    m_viewTab.move(from, to);
    m_tabIndex = m_tabBar->currentIndex();
}

void DolphinMainWindow::slotTestCanDecode(const QDragMoveEvent* event, bool& canDecode)
{
    canDecode = KUrl::List::canDecode(event->mimeData());
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
    m_lastHandleUrlStatJob = 0;
    const KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
    const KUrl url = static_cast<KIO::StatJob*>(job)->url();
    if (entry.isDir()) {
        activeViewContainer()->setUrl(url);
    } else {
        new KRun(url, this);  // Automatically deletes itself after being finished
    }
}

void DolphinMainWindow::tabDropEvent(int tab, QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty() && tab != -1) {
        const ViewTab& viewTab = m_viewTab[tab];
        const DolphinView* view = viewTab.isPrimaryViewActive ? viewTab.primaryView->view()
                                                              : viewTab.secondaryView->view();
        QString error;
        DragAndDropHelper::dropUrls(view->rootItem(), view->url(), event, error);
        if (!error.isEmpty()) {
            activeViewContainer()->showMessage(error, DolphinViewContainer::Error);
        }
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

        KRun::run("dolphin %u", QList<QUrl>() << KIO::upUrl(item.url()), this);
        break;
    }

    case DolphinContextMenu::OpenParentFolderInNewTab:
        openNewTab(KIO::upUrl(item.url()));
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
    connect(menu, &KMenu::aboutToHide, goMenu, &KMenu::deleteLater);
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Back)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Forward)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Up)));
    goMenu->addAction(ac->action(KStandardAction::name(KStandardAction::Home)));
    goMenu->addAction(ac->action("closed_tabs"));
    menu->addMenu(goMenu);

    // Add "Tool" menu
    KMenu* toolsMenu = new KMenu(i18nc("@action:inmenu", "Tools"), menu);
    connect(menu, &KMenu::aboutToHide, toolsMenu, &KMenu::deleteLater);
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
    connect(menu, &KMenu::aboutToHide, helpMenu, &KMenu::deleteLater);
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

void DolphinMainWindow::slotPlaceActivated(const KUrl& url)
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

void DolphinMainWindow::restoreClosedTab(const KUrl& primaryUrl, const KUrl& secondaryUrl)
{
    openNewActivatedTab(primaryUrl);

    if (!secondaryUrl.isEmpty() && secondaryUrl.isValid()) {
        const int index = m_tabBar->currentIndex();
        createSecondaryView(index);
        setActiveViewContainer(m_viewTab[index].secondaryView);
        m_viewTab[index].secondaryView->setUrl(secondaryUrl);
    }
}

void DolphinMainWindow::setActiveViewContainer(DolphinViewContainer* viewContainer)
{
    Q_ASSERT(viewContainer);
    Q_ASSERT((viewContainer == m_viewTab[m_tabIndex].primaryView) ||
             (viewContainer == m_viewTab[m_tabIndex].secondaryView));
    if (m_activeViewContainer == viewContainer) {
        return;
    }

    m_activeViewContainer->setActive(false);
    m_activeViewContainer = viewContainer;

    // Activating the view container might trigger a recursive setActiveViewContainer() call
    // inside DolphinMainWindow::toggleActiveView() when having a split view. Temporary
    // disconnect the activated() signal in this case:
    disconnect(m_activeViewContainer->view(), &DolphinView::activated, this, &DolphinMainWindow::toggleActiveView);
    m_activeViewContainer->setActive(true);
    connect(m_activeViewContainer->view(), &DolphinView::activated, this, &DolphinMainWindow::toggleActiveView);

    m_actionHandler->setCurrentView(viewContainer->view());

    updateHistory();
    updateEditActions();
    updatePasteAction();
    updateViewActions();
    updateGoActions();

    const KUrl url = m_activeViewContainer->url();
    setUrlAsCaption(url);
    if (m_viewTab.count() > 1) {
        m_tabBar->setTabText(m_tabIndex, tabName(url));
        m_tabBar->setTabIcon(m_tabIndex, QIcon::fromTheme(KIO::iconNameForUrl(url)));
    }

    emit urlChanged(url);
}

DolphinViewContainer* DolphinMainWindow::createViewContainer(const KUrl& url, QWidget* parent)
{
    DolphinViewContainer* container = new DolphinViewContainer(url, parent);

    // The places-selector from the URL navigator should only be shown
    // if the places dock is invisible
    QDockWidget* placesDock = findChild<QDockWidget*>("placesDock");
    container->urlNavigator()->setPlacesSelectorVisible(!placesDock || !placesDock->isVisible());

    return container;
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newFileMenu = new DolphinNewFileMenu(actionCollection(), this);
    QMenu* menu = m_newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(QIcon::fromTheme("document-new"));
    m_newFileMenu->setDelayed(false);
    connect(menu, &QMenu::aboutToShow,
            this, &DolphinMainWindow::updateNewMenu);

    QAction* newWindow = actionCollection()->addAction("new_window");
    newWindow->setIcon(QIcon::fromTheme("window-new"));
    newWindow->setText(i18nc("@action:inmenu File", "New &Window"));
    newWindow->setShortcut(Qt::CTRL | Qt::Key_N);
    connect(newWindow, &QAction::triggered, this, &DolphinMainWindow::openNewMainWindow);

    QAction* newTab = actionCollection()->addAction("new_tab");
    newTab->setIcon(QIcon::fromTheme("tab-new"));
    newTab->setText(i18nc("@action:inmenu File", "New Tab"));
    newTab->setShortcuts(QList<QKeySequence>() << QKeySequence(Qt::CTRL | Qt::Key_T) << QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N));
    connect(newTab, &QAction::triggered, this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::openNewTab));

    QAction* closeTab = actionCollection()->addAction("close_tab");
    closeTab->setIcon(QIcon::fromTheme("tab-close"));
    closeTab->setText(i18nc("@action:inmenu File", "Close Tab"));
    closeTab->setShortcut(Qt::CTRL | Qt::Key_W);
    closeTab->setEnabled(false);
    connect(closeTab, &QAction::triggered, this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::closeTab));

    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    // setup 'Edit' menu
    KStandardAction::undo(this,
                          SLOT(undo()),
                          actionCollection());

    // need to remove shift+del from cut action, else the shortcut for deletejob
    // doesn't work
    QAction* cut = KStandardAction::cut(this, SLOT(cut()), actionCollection());
    auto cutShortcuts = cut->shortcuts();
    cutShortcuts.removeAll(QKeySequence(Qt::SHIFT | Qt::Key_Delete));
    cut->setShortcuts(cutShortcuts);
    KStandardAction::copy(this, SLOT(copy()), actionCollection());
    QAction* paste = KStandardAction::paste(this, SLOT(paste()), actionCollection());
    // The text of the paste-action is modified dynamically by Dolphin
    // (e. g. to "Paste One Folder"). To prevent that the size of the toolbar changes
    // due to the long text, the text "Paste" is used:
    paste->setIconText(i18nc("@action:inmenu Edit", "Paste"));

    KStandardAction::find(this, SLOT(find()), actionCollection());

    QAction* selectAll = actionCollection()->addAction("select_all");
    selectAll->setText(i18nc("@action:inmenu Edit", "Select All"));
    selectAll->setShortcut(Qt::CTRL | Qt::Key_A);
    connect(selectAll, &QAction::triggered, this, &DolphinMainWindow::selectAll);

    QAction* invertSelection = actionCollection()->addAction("invert_selection");
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    invertSelection->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, &QAction::triggered, this, &DolphinMainWindow::invertSelection);

    // setup 'View' menu
    // (note that most of it is set up in DolphinViewActionHandler)

    QAction* split = actionCollection()->addAction("split_view");
    split->setShortcut(Qt::Key_F3);
    updateSplitAction();
    connect(split, &QAction::triggered, this, &DolphinMainWindow::toggleSplitView);

    QAction* reload = actionCollection()->addAction("reload");
    reload->setText(i18nc("@action:inmenu View", "Reload"));
    reload->setShortcut(Qt::Key_F5);
    reload->setIcon(QIcon::fromTheme("view-refresh"));
    connect(reload, &QAction::triggered, this, &DolphinMainWindow::reloadView);

    QAction* stop = actionCollection()->addAction("stop");
    stop->setText(i18nc("@action:inmenu View", "Stop"));
    stop->setToolTip(i18nc("@info", "Stop loading"));
    stop->setIcon(QIcon::fromTheme("process-stop"));
    connect(stop, &QAction::triggered, this, &DolphinMainWindow::stopLoading);

    KToggleAction* editableLocation = actionCollection()->add<KToggleAction>("editable_location");
    editableLocation->setText(i18nc("@action:inmenu Navigation Bar", "Editable Location"));
    editableLocation->setShortcut(Qt::Key_F6);
    connect(editableLocation, &KToggleAction::triggered, this, &DolphinMainWindow::toggleEditLocation);

    QAction* replaceLocation = actionCollection()->addAction("replace_location");
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    replaceLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(replaceLocation, &QAction::triggered, this, &DolphinMainWindow::replaceLocation);

    // setup 'Go' menu
    QAction* backAction = KStandardAction::back(this, SLOT(goBack()), actionCollection());
    auto backShortcuts = backAction->shortcuts();
    backShortcuts.append(QKeySequence(Qt::Key_Backspace));
    backAction->setShortcuts(backShortcuts);

    DolphinRecentTabsMenu* recentTabsMenu = new DolphinRecentTabsMenu(this);
    actionCollection()->addAction("closed_tabs", recentTabsMenu);
    connect(this, SIGNAL(rememberClosedTab(KUrl,KUrl)),
            recentTabsMenu, SLOT(rememberClosedTab(KUrl,KUrl)));
    connect(recentTabsMenu, SIGNAL(restoreClosedTab(KUrl,KUrl)),
            this, SLOT(restoreClosedTab(KUrl,KUrl)));

    KStandardAction::forward(this, SLOT(goForward()), actionCollection());
    KStandardAction::up(this, SLOT(goUp()), actionCollection());
    KStandardAction::home(this, SLOT(goHome()), actionCollection());

    // setup 'Tools' menu
    QAction* showFilterBar = actionCollection()->addAction("show_filter_bar");
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Show Filter Bar"));
    showFilterBar->setIcon(QIcon::fromTheme("view-filter"));
    showFilterBar->setShortcut(Qt::CTRL | Qt::Key_I);
    connect(showFilterBar, &QAction::triggered, this, &DolphinMainWindow::showFilterBar);

    QAction* compareFiles = actionCollection()->addAction("compare_files");
    compareFiles->setText(i18nc("@action:inmenu Tools", "Compare Files"));
    compareFiles->setIcon(QIcon::fromTheme("kompare"));
    compareFiles->setEnabled(false);
    connect(compareFiles, &QAction::triggered, this, &DolphinMainWindow::compareFiles);

    QAction* openTerminal = actionCollection()->addAction("open_terminal");
    openTerminal->setText(i18nc("@action:inmenu Tools", "Open Terminal"));
    openTerminal->setIcon(QIcon::fromTheme("utilities-terminal"));
    openTerminal->setShortcut(Qt::SHIFT | Qt::Key_F4);
    connect(openTerminal, &QAction::triggered, this, &DolphinMainWindow::openTerminal);

    // setup 'Settings' menu
    KToggleAction* showMenuBar = KStandardAction::showMenubar(0, 0, actionCollection());
    connect(showMenuBar, &KToggleAction::triggered,                   // Fixes #286822
            this, &DolphinMainWindow::toggleShowMenuBar, Qt::QueuedConnection);
    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());

    // not in menu actions
    QList<QKeySequence> nextTabKeys;
    nextTabKeys.append(KStandardShortcut::tabNext().first()); //TODO: is this correct
    nextTabKeys.append(QKeySequence(Qt::CTRL | Qt::Key_Tab));

    QList<QKeySequence> prevTabKeys;
    prevTabKeys.append(KStandardShortcut::tabPrev().first()); //TODO: is this correct
    prevTabKeys.append(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_Tab));

    QAction* activateNextTab = actionCollection()->addAction("activate_next_tab");
    activateNextTab->setIconText(i18nc("@action:inmenu", "Next Tab"));
    activateNextTab->setText(i18nc("@action:inmenu", "Activate Next Tab"));
    activateNextTab->setEnabled(false);
    connect(activateNextTab, &QAction::triggered, this, &DolphinMainWindow::activateNextTab);
    activateNextTab->setShortcuts(QApplication::isRightToLeft() ? prevTabKeys : nextTabKeys);

    QAction* activatePrevTab = actionCollection()->addAction("activate_prev_tab");
    activatePrevTab->setIconText(i18nc("@action:inmenu", "Previous Tab"));
    activatePrevTab->setText(i18nc("@action:inmenu", "Activate Previous Tab"));
    activatePrevTab->setEnabled(false);
    connect(activatePrevTab, &QAction::triggered, this, &DolphinMainWindow::activatePrevTab);
    activatePrevTab->setShortcuts(QApplication::isRightToLeft() ? nextTabKeys : prevTabKeys);

    // for context menu
    QAction* openInNewTab = actionCollection()->addAction("open_in_new_tab");
    openInNewTab->setText(i18nc("@action:inmenu", "Open in New Tab"));
    openInNewTab->setIcon(QIcon::fromTheme("tab-new"));
    connect(openInNewTab, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction* openInNewTabs = actionCollection()->addAction("open_in_new_tabs");
    openInNewTabs->setText(i18nc("@action:inmenu", "Open in New Tabs"));
    openInNewTabs->setIcon(QIcon::fromTheme("tab-new"));
    connect(openInNewTabs, &QAction::triggered, this, &DolphinMainWindow::openInNewTab);

    QAction* openInNewWindow = actionCollection()->addAction("open_in_new_window");
    openInNewWindow->setText(i18nc("@action:inmenu", "Open in New Window"));
    openInNewWindow->setIcon(QIcon::fromTheme("window-new"));
    connect(openInNewWindow, &QAction::triggered, this, &DolphinMainWindow::openInNewWindow);
}

void DolphinMainWindow::setupDockWidgets()
{
    const bool lock = GeneralSettings::lockPanels();

    KDualAction* lockLayoutAction = actionCollection()->add<KDualAction>("lock_panels");
    lockLayoutAction->setActiveText(i18nc("@action:inmenu Panels", "Unlock Panels"));
    lockLayoutAction->setActiveIcon(QIcon::fromTheme("object-unlocked"));
    lockLayoutAction->setInactiveText(i18nc("@action:inmenu Panels", "Lock Panels"));
    lockLayoutAction->setInactiveIcon(QIcon::fromTheme("object-locked"));
    lockLayoutAction->setActive(lock);
    connect(lockLayoutAction, &KDualAction::triggered, this, &DolphinMainWindow::togglePanelLockState);

    // Setup "Information"
    DolphinDockWidget* infoDock = new DolphinDockWidget(i18nc("@title:window", "Information"));
    infoDock->setLocked(lock);
    infoDock->setObjectName("infoDock");
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    InformationPanel* infoPanel = new InformationPanel(infoDock);
    infoPanel->setCustomContextMenuActions(QList<QAction*>() << lockLayoutAction);
    connect(infoPanel, &InformationPanel::urlActivated, this, &DolphinMainWindow::handleUrl);
    infoDock->setWidget(infoPanel);

    QAction* infoAction = infoDock->toggleViewAction();
    createPanelAction(QIcon::fromTheme("dialog-information"), Qt::Key_F11, infoAction, "show_information_panel");

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, &DolphinMainWindow::urlChanged,
            infoPanel, &InformationPanel::setUrl);
    connect(this, &DolphinMainWindow::selectionChanged,
            infoPanel, &InformationPanel::setSelection);
    connect(this, &DolphinMainWindow::requestItemInfo,
            infoPanel, &InformationPanel::requestDelayedItemInfo);

    // Setup "Folders"
    DolphinDockWidget* foldersDock = new DolphinDockWidget(i18nc("@title:window", "Folders"));
    foldersDock->setLocked(lock);
    foldersDock->setObjectName("foldersDock");
    foldersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    FoldersPanel* foldersPanel = new FoldersPanel(foldersDock);
    foldersPanel->setCustomContextMenuActions(QList<QAction*>() << lockLayoutAction);
    foldersDock->setWidget(foldersPanel);

    QAction* foldersAction = foldersDock->toggleViewAction();
    createPanelAction(QIcon::fromTheme("folder"), Qt::Key_F7, foldersAction, "show_folders_panel");

    addDockWidget(Qt::LeftDockWidgetArea, foldersDock);
    connect(this, &DolphinMainWindow::urlChanged,
            foldersPanel, &FoldersPanel::setUrl);
    connect(foldersPanel, &FoldersPanel::folderActivated,
            this, &DolphinMainWindow::changeUrl);
    connect(foldersPanel, &FoldersPanel::folderMiddleClicked,
            this, static_cast<void(DolphinMainWindow::*)(const KUrl&)>(&DolphinMainWindow::openNewTab));
    connect(foldersPanel, &FoldersPanel::errorMessage,
            this, &DolphinMainWindow::slotPanelErrorMessage);

    // Setup "Terminal"
#ifndef Q_OS_WIN
    DolphinDockWidget* terminalDock = new DolphinDockWidget(i18nc("@title:window Shell terminal", "Terminal"));
    terminalDock->setLocked(lock);
    terminalDock->setObjectName("terminalDock");
    terminalDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    TerminalPanel* terminalPanel = new TerminalPanel(terminalDock);
    terminalPanel->setCustomContextMenuActions(QList<QAction*>() << lockLayoutAction);
    terminalDock->setWidget(terminalPanel);

    connect(terminalPanel, &TerminalPanel::hideTerminalPanel, terminalDock, &DolphinDockWidget::hide);
    connect(terminalPanel, &TerminalPanel::changeUrl, this, &DolphinMainWindow::slotTerminalDirectoryChanged);
    connect(terminalDock, &DolphinDockWidget::visibilityChanged,
            terminalPanel, &TerminalPanel::dockVisibilityChanged);

    QAction* terminalAction = terminalDock->toggleViewAction();
    createPanelAction(QIcon::fromTheme("utilities-terminal"), Qt::Key_F4, terminalAction, "show_terminal_panel");

    addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
    connect(this, &DolphinMainWindow::urlChanged,
            terminalPanel, &TerminalPanel::setUrl);
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
    createPanelAction(QIcon::fromTheme("bookmarks"), Qt::Key_F9, placesAction, "show_places_panel");

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(placesPanel, &PlacesPanel::placeActivated,
            this, &DolphinMainWindow::slotPlaceActivated);
    connect(placesPanel, &PlacesPanel::placeMiddleClicked,
            this, static_cast<void(DolphinMainWindow::*)(const KUrl&)>(&DolphinMainWindow::openNewTab));
    connect(placesPanel, &PlacesPanel::errorMessage,
            this, &DolphinMainWindow::slotPanelErrorMessage);
    connect(this, &DolphinMainWindow::urlChanged,
            placesPanel, &PlacesPanel::setUrl);
    connect(placesDock, &DolphinDockWidget::visibilityChanged,
            this, &DolphinMainWindow::slotPlacesPanelVisibilityChanged);
    connect(this, &DolphinMainWindow::settingsChanged,
	    placesPanel, &PlacesPanel::readSettings);

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
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
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
}

void DolphinMainWindow::updateViewActions()
{
    m_actionHandler->updateViewActions();

    QAction* showFilterBarAction = actionCollection()->action("show_filter_bar");
    showFilterBarAction->setChecked(m_activeViewContainer->isFilterBarVisible());

    updateSplitAction();

    QAction* editableLocactionAction = actionCollection()->action("editable_location");
    const KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    editableLocactionAction->setChecked(urlNavigator->isUrlEditable());
}

void DolphinMainWindow::updateGoActions()
{
    QAction* goUpAction = actionCollection()->action(KStandardAction::name(KStandardAction::Up));
    const KUrl currentUrl = m_activeViewContainer->url();
    goUpAction->setEnabled(currentUrl.upUrl() != currentUrl);
}

void DolphinMainWindow::createControlButton()
{
    if (m_controlButton) {
        return;
    }
    Q_ASSERT(!m_controlButton);

    m_controlButton = new QToolButton(this);
    m_controlButton->setIcon(QIcon::fromTheme("applications-system"));
    m_controlButton->setText(i18nc("@action", "Control"));
    m_controlButton->setPopupMode(QToolButton::InstantPopup);
    m_controlButton->setToolButtonStyle(toolBar()->toolButtonStyle());

    KMenu* controlMenu = new KMenu(m_controlButton);
    connect(controlMenu, &KMenu::aboutToShow, this, &DolphinMainWindow::updateControlMenu);

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
    Q_ASSERT(m_viewTab[m_tabIndex].primaryView);

    // remember the current active view, as because of
    // the refreshing the active view might change to
    // the secondary view
    DolphinViewContainer* activeViewContainer = m_activeViewContainer;

    const int tabCount = m_viewTab.count();
    for (int i = 0; i < tabCount; ++i) {
        m_viewTab[i].primaryView->readSettings();
        if (m_viewTab[i].secondaryView) {
            m_viewTab[i].secondaryView->readSettings();
        }
    }

    setActiveViewContainer(activeViewContainer);

    if (GeneralSettings::modifiedStartupSettings()) {
        // The startup settings have been changed by the user (see bug #254947).
        // Synchronize the split-view setting with the active view:
        const bool splitView = GeneralSettings::splitView();
        const ViewTab& activeTab = m_viewTab[m_tabIndex];
        const bool toggle =    ( splitView && !activeTab.secondaryView)
                            || (!splitView &&  activeTab.secondaryView);
        if (toggle) {
            toggleSplitView();
        }
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

    DolphinView* view = container->view();
    connect(view, &DolphinView::selectionChanged,
            this, &DolphinMainWindow::slotSelectionChanged);
    connect(view, &DolphinView::requestItemInfo,
            this, &DolphinMainWindow::slotRequestItemInfo);
    connect(view, &DolphinView::activated,
            this, &DolphinMainWindow::toggleActiveView);
    connect(view, &DolphinView::tabRequested,
            this, static_cast<void(DolphinMainWindow::*)(const KUrl&)>(&DolphinMainWindow::openNewTab));
    connect(view, &DolphinView::requestContextMenu,
            this, &DolphinMainWindow::openContextMenu);
    connect(view, &DolphinView::directoryLoadingStarted,
            this, &DolphinMainWindow::enableStopAction);
    connect(view, &DolphinView::directoryLoadingCompleted,
            this, &DolphinMainWindow::disableStopAction);
    connect(view, &DolphinView::goBackRequested,
            this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::goBack));
    connect(view, &DolphinView::goForwardRequested,
            this, static_cast<void(DolphinMainWindow::*)()>(&DolphinMainWindow::goForward));

    const KUrlNavigator* navigator = container->urlNavigator();
    connect(navigator, &KUrlNavigator::urlChanged,
            this, &DolphinMainWindow::changeUrl);
    connect(navigator, &KUrlNavigator::historyChanged,
            this, &DolphinMainWindow::updateHistory);
    connect(navigator, &KUrlNavigator::editableStateChanged,
            this, &DolphinMainWindow::slotEditableStateChanged);
    connect(navigator, &KUrlNavigator::tabRequested,
            this, static_cast<void(DolphinMainWindow::*)(const KUrl&)>(&DolphinMainWindow::openNewTab));
}

void DolphinMainWindow::updateSplitAction()
{
    QAction* splitAction = actionCollection()->action("split_view");
    if (m_viewTab[m_tabIndex].secondaryView) {
        if (m_activeViewContainer == m_viewTab[m_tabIndex].secondaryView) {
            splitAction->setText(i18nc("@action:intoolbar Close right view", "Close"));
            splitAction->setToolTip(i18nc("@info", "Close right view"));
            splitAction->setIcon(QIcon::fromTheme("view-right-close"));
        } else {
            splitAction->setText(i18nc("@action:intoolbar Close left view", "Close"));
            splitAction->setToolTip(i18nc("@info", "Close left view"));
            splitAction->setIcon(QIcon::fromTheme("view-left-close"));
        }
    } else {
        splitAction->setText(i18nc("@action:intoolbar Split view", "Split"));
        splitAction->setToolTip(i18nc("@info", "Split view"));
        splitAction->setIcon(QIcon::fromTheme("view-right-new"));
    }
}

QString DolphinMainWindow::tabName(const KUrl& url) const
{
    QString name;
    if (url.equals(KUrl("file:///"))) {
        name = '/';
    } else {
        name = url.fileName();
        if (name.isEmpty()) {
            name = url.protocol();
        } else {
            // Make sure that a '&' inside the directory name is displayed correctly
            // and not misinterpreted as a keyboard shortcut in QTabBar::setTabText()
            name.replace('&', "&&");
        }
    }
    return name;
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

void DolphinMainWindow::createSecondaryView(int tabIndex)
{
    ViewTab& viewTab = m_viewTab[tabIndex];

    QSplitter* splitter = viewTab.splitter;
    const int newWidth = (viewTab.primaryView->width() - splitter->handleWidth()) / 2;

    const DolphinView* view = viewTab.primaryView->view();
    // The final parent of the new view container will be set by adding it
    // to the splitter. However, we must make sure that the DolphinMainWindow
    // is a parent of the view container already when it is constructed
    // because this enables the container's KFileItemModel to assign its
    // dir lister to the right main window. The dir lister can then cache
    // authentication data.
    viewTab.secondaryView = createViewContainer(view->url(), this);
    splitter->addWidget(viewTab.secondaryView);
    splitter->setSizes(QList<int>() << newWidth << newWidth);

    connectViewSignals(viewTab.secondaryView);
    viewTab.secondaryView->setActive(false);
    viewTab.secondaryView->resize(newWidth, viewTab.primaryView->height());
    viewTab.secondaryView->show();
}

QString DolphinMainWindow::tabProperty(const QString& property, int tabIndex) const
{
    return "Tab " + QString::number(tabIndex) + ' ' + property;
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

QString DolphinMainWindow::squeezedText(const QString& text) const
{
    const QFontMetrics fm = fontMetrics();
    return fm.elidedText(text, Qt::ElideMiddle, fm.maxWidth() * 10);
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
    panelAction->setShortcut(shortcut);

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

