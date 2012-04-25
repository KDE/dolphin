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
#include <KFilePlacesModel>
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
#include <ktabbar.h>
#include <KToggleAction>
#include <KUrlNavigator>
#include <KUrl>
#include <KUrlComboBox>
#include <KToolInvocation>

#include "views/dolphinplacesmodel.h"

#include <QDesktopWidget>
#include <QDBusMessage>
#include <QKeyEvent>
#include <QClipboard>
#include <QToolButton>
#include <QSplitter>

namespace {
    // Used for GeneralSettings::version() to determine whether
    // an updated version of Dolphin is running.
    const int CurrentDolphinVersion = 200;
};

/*
 * Remembers the tab configuration if a tab has been closed.
 * Each closed tab can be restored by the menu
 * "Go -> Recently Closed Tabs".
 */
struct ClosedTab
{
    KUrl primaryUrl;
    KUrl secondaryUrl;
    bool isSplit;
};
Q_DECLARE_METATYPE(ClosedTab)

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
    DolphinPlacesModel::setModel(new KFilePlacesModel(this));
    connect(DolphinPlacesModel::instance(), SIGNAL(errorMessage(QString)),
            this, SLOT(showErrorMessage(QString)));

    // Workaround for a X11-issue in combination with KModifierInfo
    // (see DolphinContextMenu::initializeModifierKeyInfo() for
    // more information):
    DolphinContextMenu::initializeModifierKeyInfo();

    setObjectName("Dolphin#");

    m_viewTab.append(ViewTab());
    ViewTab& viewTab = m_viewTab[m_tabIndex];
    viewTab.wasActive = true; // The first opened tab is automatically active

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

    viewTab.splitter = new QSplitter(this);
    viewTab.splitter->setChildrenCollapsible(false);

    setupActions();

    const KUrl homeUrl(generalSettings->homeUrl());
    setUrlAsCaption(homeUrl);
    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    connect(m_actionHandler, SIGNAL(actionBeingHandled()), SLOT(clearStatusBar()));
    connect(m_actionHandler, SIGNAL(createDirectory()), SLOT(createDirectory()));

    viewTab.primaryView = createViewContainer(homeUrl, viewTab.splitter);

    m_activeViewContainer = viewTab.primaryView;
    connectViewSignals(m_activeViewContainer);
    DolphinView* view = m_activeViewContainer->view();
    m_activeViewContainer->show();
    m_actionHandler->setCurrentView(view);

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, SIGNAL(urlChanged(KUrl)),
            m_remoteEncoding, SLOT(slotAboutToOpenUrl()));

    m_tabBar = new KTabBar(this);
    m_tabBar->setMovable(true);
    m_tabBar->setTabsClosable(true);
    connect(m_tabBar, SIGNAL(currentChanged(int)),
            this, SLOT(setActiveTab(int)));
    connect(m_tabBar, SIGNAL(tabCloseRequested(int)),
            this, SLOT(closeTab(int)));
    connect(m_tabBar, SIGNAL(contextMenu(int,QPoint)),
            this, SLOT(openTabContextMenu(int,QPoint)));
    connect(m_tabBar, SIGNAL(newTabRequest()),
            this, SLOT(openNewTab()));
    connect(m_tabBar, SIGNAL(testCanDecode(const QDragMoveEvent*,bool&)),
            this, SLOT(slotTestCanDecode(const QDragMoveEvent*,bool&)));
    connect(m_tabBar, SIGNAL(mouseMiddleClick(int)),
            this, SLOT(closeTab(int)));
    connect(m_tabBar, SIGNAL(tabMoved(int,int)),
            this, SLOT(slotTabMoved(int,int)));
    connect(m_tabBar, SIGNAL(receivedDropEvent(int,QDropEvent*)),
            this, SLOT(tabDropEvent(int,QDropEvent*)));

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
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));

    if (generalSettings->splitView()) {
        toggleSplitView();
    }
    updateEditActions();
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
    QList<KUrl>::const_iterator it = dirs.begin();
    while (it != dirs.end()) {
        openNewTab(*it);
        ++it;

        if (hasSplitView && (it != dirs.end())) {
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
        updateViewActions();
        updateGoActions();
        setUrlAsCaption(url);
        if (m_viewTab.count() > 1) {
            m_tabBar->setTabText(m_tabIndex, squeezedText(tabName(m_activeViewContainer->url())));
        }
        const QString iconName = KMimeType::iconNameForUrl(url);
        m_tabBar->setTabIcon(m_tabIndex, KIcon(iconName));
        emit urlChanged(url);
    }
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
    KRun::run("dolphin", KUrl::List(), this);
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
        m_tabBar->addTab(KIcon(KMimeType::iconNameForUrl(currentUrl)),
                         squeezedText(tabName(currentUrl)));
        m_tabBar->blockSignals(false);
    }

    m_tabBar->addTab(KIcon(KMimeType::iconNameForUrl(url)),
                     squeezedText(tabName(url)));

    ViewTab viewTab;
    viewTab.splitter = new QSplitter(this);
    viewTab.splitter->setChildrenCollapsible(false);
    viewTab.primaryView = createViewContainer(url, viewTab.splitter);
    viewTab.primaryView->setActive(false);
    connectViewSignals(viewTab.primaryView);

    m_viewTab.append(viewTab);

    actionCollection()->action("close_tab")->setEnabled(true);

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
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        openNewTab(m_activeViewContainer->url());
    } else if ((list.count() == 1) && list[0].isDir()) {
        openNewTab(list[0].url());
    }
}

void DolphinMainWindow::openInNewWindow()
{
    KUrl newWindowUrl;

    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        newWindowUrl = m_activeViewContainer->url();
    } else if ((list.count() == 1) && list[0].isDir()) {
        newWindowUrl = list[0].url();
    }

    if (!newWindowUrl.isEmpty()) {
        KRun::run("dolphin", KUrl::List() << newWindowUrl, this);
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

void DolphinMainWindow::restoreClosedTab(QAction* action)
{
    if (action->data().toBool()) {
        // clear all actions except the "Empty Recently Closed Tabs"
        // action and the separator
        QList<QAction*> actions = m_recentTabsMenu->menu()->actions();
        const int count = actions.size();
        for (int i = 2; i < count; ++i) {
            m_recentTabsMenu->menu()->removeAction(actions.at(i));
        }
    } else {
        const ClosedTab closedTab = action->data().value<ClosedTab>();
        openNewTab(closedTab.primaryUrl);
        m_tabBar->setCurrentIndex(m_viewTab.count() - 1);

        if (closedTab.isSplit) {
            // create secondary view
            toggleSplitView();
            m_viewTab[m_tabIndex].secondaryView->setUrl(closedTab.secondaryUrl);
        }

        m_recentTabsMenu->removeAction(action);
    }

    if (m_recentTabsMenu->menu()->actions().count() == 2) {
        m_recentTabsMenu->setEnabled(false);
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
    // The method is only invoked if exactly 2 files have
    // been selected. The selected files may be:
    // - both in the primary view
    // - both in the secondary view
    // - one in the primary view and the other in the secondary
    //   view
    Q_ASSERT(m_viewTab[m_tabIndex].primaryView);

    KUrl urlA;
    KUrl urlB;

    KFileItemList items = m_viewTab[m_tabIndex].primaryView->view()->selectedItems();

    switch (items.count()) {
    case 0: {
        Q_ASSERT(m_viewTab[m_tabIndex].secondaryView);
        items = m_viewTab[m_tabIndex].secondaryView->view()->selectedItems();
        Q_ASSERT(items.count() == 2);
        urlA = items[0].url();
        urlB = items[1].url();
        break;
    }

    case 1: {
        urlA = items[0].url();
        Q_ASSERT(m_viewTab[m_tabIndex].secondaryView);
        items = m_viewTab[m_tabIndex].secondaryView->view()->selectedItems();
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
        connect(settingsDialog, SIGNAL(settingsChanged()), this, SLOT(refreshViews()));
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
    rememberClosedTab(index);

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
    } else {
        m_tabBar->blockSignals(false);
    }
}

void DolphinMainWindow::openTabContextMenu(int index, const QPoint& pos)
{
    KMenu menu(this);

    QAction* newTabAction = menu.addAction(KIcon("tab-new"), i18nc("@action:inmenu", "New Tab"));
    newTabAction->setShortcut(actionCollection()->action("new_tab")->shortcut());

    QAction* detachTabAction = menu.addAction(KIcon("tab-detach"), i18nc("@action:inmenu", "Detach Tab"));

    QAction* closeOtherTabsAction = menu.addAction(KIcon("tab-close-other"), i18nc("@action:inmenu", "Close Other Tabs"));

    QAction* closeTabAction = menu.addAction(KIcon("tab-close"), i18nc("@action:inmenu", "Close Tab"));
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

void DolphinMainWindow::tabDropEvent(int tab, QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty() && tab != -1) {
        const ViewTab& viewTab = m_viewTab[tab];
        const DolphinView* view = viewTab.isPrimaryViewActive ? viewTab.primaryView->view()
                                                              : viewTab.secondaryView->view();
        const QString error = DragAndDropHelper::dropUrls(view->rootItem(), view->url(), event);
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
        KRun::run("dolphin", KUrl::List() << item.url().upUrl(), this);
        break;
    }

    case DolphinContextMenu::OpenParentFolderInNewTab:
        openNewTab(item.url().upUrl());
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
    disconnect(m_activeViewContainer->view(), SIGNAL(activated()), this, SLOT(toggleActiveView()));
    m_activeViewContainer->setActive(true);
    connect(m_activeViewContainer->view(), SIGNAL(activated()), this, SLOT(toggleActiveView()));

    m_actionHandler->setCurrentView(viewContainer->view());

    updateHistory();
    updateEditActions();
    updateViewActions();
    updateGoActions();

    const KUrl url = m_activeViewContainer->url();
    setUrlAsCaption(url);
    if (m_viewTab.count() > 1) {
        m_tabBar->setTabText(m_tabIndex, tabName(url));
        m_tabBar->setTabIcon(m_tabIndex, KIcon(KMimeType::iconNameForUrl(url)));
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
    m_newFileMenu = new DolphinNewFileMenu(this);
    KMenu* menu = m_newFileMenu->menu();
    menu->setTitle(i18nc("@title:menu Create new folder, file, link, etc.", "Create New"));
    menu->setIcon(KIcon("document-new"));
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
    connect(newTab, SIGNAL(triggered()), this, SLOT(openNewTab()));

    KAction* closeTab = actionCollection()->addAction("close_tab");
    closeTab->setIcon(KIcon("tab-close"));
    closeTab->setText(i18nc("@action:inmenu File", "Close Tab"));
    closeTab->setShortcut(Qt::CTRL | Qt::Key_W);
    closeTab->setEnabled(false);
    connect(closeTab, SIGNAL(triggered()), this, SLOT(closeTab()));

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
    updateSplitAction();
    connect(split, SIGNAL(triggered()), this, SLOT(toggleSplitView()));

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
    editableLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(editableLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    KAction* replaceLocation = actionCollection()->addAction("replace_location");
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    replaceLocation->setShortcut(Qt::Key_F6);
    connect(replaceLocation, SIGNAL(triggered()), this, SLOT(replaceLocation()));

    // setup 'Go' menu
    KAction* backAction = KStandardAction::back(this, SLOT(goBack()), actionCollection());
    connect(backAction, SIGNAL(triggered(Qt::MouseButtons,Qt::KeyboardModifiers)), this, SLOT(goBack(Qt::MouseButtons)));
    KShortcut backShortcut = backAction->shortcut();
    backShortcut.setAlternate(Qt::Key_Backspace);
    backAction->setShortcut(backShortcut);

    m_recentTabsMenu = new KActionMenu(i18n("Recently Closed Tabs"), this);
    m_recentTabsMenu->setIcon(KIcon("edit-undo"));
    actionCollection()->addAction("closed_tabs", m_recentTabsMenu);
    connect(m_recentTabsMenu->menu(), SIGNAL(triggered(QAction*)),
            this, SLOT(restoreClosedTab(QAction*)));

    QAction* action = new QAction(i18n("Empty Recently Closed Tabs"), m_recentTabsMenu);
    action->setIcon(KIcon("edit-clear-list"));
    action->setData(QVariant::fromValue(true));
    m_recentTabsMenu->addAction(action);
    m_recentTabsMenu->addSeparator();
    m_recentTabsMenu->setEnabled(false);

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
    connect(activateNextTab, SIGNAL(triggered()), SLOT(activateNextTab()));
    activateNextTab->setShortcuts(QApplication::isRightToLeft() ? prevTabKeys : nextTabKeys);

    KAction* activatePrevTab = actionCollection()->addAction("activate_prev_tab");
    activatePrevTab->setText(i18nc("@action:inmenu", "Activate Previous Tab"));
    connect(activatePrevTab, SIGNAL(triggered()), SLOT(activatePrevTab()));
    activatePrevTab->setShortcuts(QApplication::isRightToLeft() ? nextTabKeys : prevTabKeys);

    // for context menu
    KAction* openInNewTab = actionCollection()->addAction("open_in_new_tab");
    openInNewTab->setText(i18nc("@action:inmenu", "Open in New Tab"));
    openInNewTab->setIcon(KIcon("tab-new"));
    connect(openInNewTab, SIGNAL(triggered()), this, SLOT(openInNewTab()));

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
            this, SLOT(openNewActivatedTab(KUrl)));

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
    QAction* separator = new QAction(placesPanel);
    separator->setSeparator(true);
    QList<QAction*> placesActions;
    placesActions.append(separator);
    placesActions.append(lockLayoutAction);
    //placesPanel->addActions(placesActions);
    //placesPanel->setModel(DolphinPlacesModel::instance());
    //placesPanel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    placesDock->setWidget(placesPanel);

    QAction* placesAction = placesDock->toggleViewAction();
    createPanelAction(KIcon("bookmarks"), Qt::Key_F9, placesAction, "show_places_panel");

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(placesPanel, SIGNAL(placeActivated(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(placesPanel, SIGNAL(placeMiddleClicked(KUrl)),
            this, SLOT(openNewActivatedTab(KUrl)));
    connect(this, SIGNAL(urlChanged(KUrl)),
            placesPanel, SLOT(setUrl(KUrl)));
    connect(placesDock, SIGNAL(visibilityChanged(bool)),
            this, SLOT(slotPlacesPanelVisibilityChanged(bool)));

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
    updatePasteAction();
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

void DolphinMainWindow::rememberClosedTab(int index)
{
    KMenu* tabsMenu = m_recentTabsMenu->menu();

    const QString primaryPath = m_viewTab[index].primaryView->url().path();
    const QString iconName = KMimeType::iconNameForUrl(primaryPath);

    QAction* action = new QAction(squeezedText(primaryPath), tabsMenu);

    ClosedTab closedTab;
    closedTab.primaryUrl = m_viewTab[index].primaryView->url();

    if (m_viewTab[index].secondaryView) {
        closedTab.secondaryUrl = m_viewTab[index].secondaryView->url();
        closedTab.isSplit = true;
    } else {
        closedTab.isSplit = false;
    }

    action->setData(QVariant::fromValue(closedTab));
    action->setIcon(KIcon(iconName));

    // add the closed tab menu entry after the separator and
    // "Empty Recently Closed Tabs" entry
    if (tabsMenu->actions().size() == 2) {
        tabsMenu->addAction(action);
    } else {
        tabsMenu->insertAction(tabsMenu->actions().at(2), action);
    }

    // assure that only up to 8 closed tabs are shown in the menu
    if (tabsMenu->actions().size() > 8) {
        tabsMenu->removeAction(tabsMenu->actions().last());
    }
    actionCollection()->action("closed_tabs")->setEnabled(true);
    KAcceleratorManager::manage(tabsMenu);
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
}

void DolphinMainWindow::clearStatusBar()
{
    m_activeViewContainer->statusBar()->resetToDefaultText();
}

void DolphinMainWindow::connectViewSignals(DolphinViewContainer* container)
{
    connect(container, SIGNAL(showFilterBarChanged(bool)),
            this, SLOT(updateFilterBarAction(bool)));
    connect(container, SIGNAL(writeStateChanged(bool)),
            this, SLOT(slotWriteStateChanged(bool)));

    DolphinView* view = container->view();
    connect(view, SIGNAL(selectionChanged(KFileItemList)),
            this, SLOT(slotSelectionChanged(KFileItemList)));
    connect(view, SIGNAL(requestItemInfo(KFileItem)),
            this, SLOT(slotRequestItemInfo(KFileItem)));
    connect(view, SIGNAL(activated()),
            this, SLOT(toggleActiveView()));
    connect(view, SIGNAL(tabRequested(KUrl)),
            this, SLOT(openNewTab(KUrl)));
    connect(view, SIGNAL(requestContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)),
            this, SLOT(openContextMenu(QPoint,KFileItem,KUrl,QList<QAction*>)));
    connect(view, SIGNAL(directoryLoadingStarted()),
            this, SLOT(enableStopAction()));
    connect(view, SIGNAL(directoryLoadingCompleted()),
            this, SLOT(disableStopAction()));
    connect(view, SIGNAL(goBackRequested()),
            this, SLOT(goBack()));
    connect(view, SIGNAL(goForwardRequested()),
            this, SLOT(goForward()));

    const KUrlNavigator* navigator = container->urlNavigator();
    connect(navigator, SIGNAL(urlChanged(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(navigator, SIGNAL(historyChanged()),
            this, SLOT(updateHistory()));
    connect(navigator, SIGNAL(editableStateChanged(bool)),
            this, SLOT(slotEditableStateChanged(bool)));
    connect(navigator, SIGNAL(tabRequested(KUrl)),
            this, SLOT(openNewTab(KUrl)));
}

void DolphinMainWindow::updateSplitAction()
{
    QAction* splitAction = actionCollection()->action("split_view");
    if (m_viewTab[m_tabIndex].secondaryView) {
        if (m_activeViewContainer == m_viewTab[m_tabIndex].secondaryView) {
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
    viewTab.secondaryView = createViewContainer(view->url(), 0);
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
