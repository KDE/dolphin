/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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
#include "dolphinviewactionhandler.h"
#include "dolphindropcontroller.h"

#include <config-nepomuk.h>

#include "dolphinapplication.h"
#include "dolphinfileplacesview.h"
#include "dolphinnewmenu.h"
#include "dolphinsettings.h"
#include "dolphinsettingsdialog.h"
#include "dolphinstatusbar.h"
#include "dolphinviewcontainer.h"
#include "infosidebarpage.h"
#include "metadatawidget.h"
#include "mainwindowadaptor.h"
#include "treeviewsidebarpage.h"
#include "viewpropertiesdialog.h"
#include "viewproperties.h"

#ifndef Q_OS_WIN
#include "terminalsidebarpage.h"
#endif

#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <kconfig.h>
#include <kdesktopfile.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kfileplacesmodel.h>
#include <kglobal.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kmenu.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kurlnavigator.h>
#include <konqmimedata.h>
#include <kpropertiesdialog.h>
#include <kprotocolinfo.h>
#include <ktoggleaction.h>
#include <krun.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <ktabbar.h>
#include <kurl.h>
#include <kurlcombobox.h>

#include <QKeyEvent>
#include <QClipboard>
#include <QLineEdit>
#include <QSplitter>
#include <QDockWidget>

DolphinMainWindow::DolphinMainWindow(int id) :
    KXmlGuiWindow(0),
    m_newMenu(0),
    m_showMenuBar(0),
    m_tabBar(0),
    m_activeViewContainer(0),
    m_centralWidgetLayout(0),
    m_id(id),
    m_tabIndex(0),
    m_viewTab(),
    m_actionHandler(0)
{
    setObjectName("Dolphin#");

    m_viewTab.append(ViewTab());

    new MainWindowAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QString("/dolphin/MainWindow%1").arg(m_id), this);

    KonqFileUndoManager::incRef();

    KonqFileUndoManager* undoManager = KonqFileUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface(this));

    connect(undoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));
    connect(undoManager, SIGNAL(undoTextChanged(const QString&)),
            this, SLOT(slotUndoTextChanged(const QString&)));
    connect(DolphinSettings::instance().placesModel(), SIGNAL(errorMessage(const QString&)),
            this, SLOT(slotHandlePlacesError(const QString&)));
}

DolphinMainWindow::~DolphinMainWindow()
{
    KonqFileUndoManager::decRef();
    DolphinApplication::app()->removeMainWindow(this);
}

void DolphinMainWindow::toggleViews()
{
    if (m_viewTab[m_tabIndex].primaryView == 0) {
        return;
    }

    // move secondary view from the last position of the splitter
    // to the first position
    m_viewTab[m_tabIndex].splitter->insertWidget(0, m_viewTab[m_tabIndex].secondaryView);

    DolphinViewContainer* container = m_viewTab[m_tabIndex].primaryView;
    m_viewTab[m_tabIndex].primaryView = m_viewTab[m_tabIndex].secondaryView;
    m_viewTab[m_tabIndex].secondaryView = container;
}

void DolphinMainWindow::slotDoingOperation(KonqFileUndoManager::CommandType commandType)
{
    clearStatusBar();
    m_undoCommandTypes.append(commandType);
}

void DolphinMainWindow::refreshViews()
{
    Q_ASSERT(m_viewTab[m_tabIndex].primaryView != 0);

    // remember the current active view, as because of
    // the refreshing the active view might change to
    // the secondary view
    DolphinViewContainer* activeViewContainer = m_activeViewContainer;

    m_viewTab[m_tabIndex].primaryView->view()->refresh();
    if (m_viewTab[m_tabIndex].secondaryView != 0) {
        m_viewTab[m_tabIndex].secondaryView->view()->refresh();
    }

    setActiveViewContainer(activeViewContainer);
}

void DolphinMainWindow::dropUrls(const KUrl::List& urls,
                                 const KUrl& destination)
{
    DolphinDropController dropController(this);
    connect(&dropController, SIGNAL(doingOperation(KonqFileUndoManager::CommandType)),
            this, SLOT(slotDoingOperation(KonqFileUndoManager::CommandType)));
    dropController.dropUrls(urls, destination);
}

void DolphinMainWindow::pasteIntoFolder()
{
    m_activeViewContainer->view()->pasteIntoFolder();
}

void DolphinMainWindow::changeUrl(const KUrl& url)
{
    DolphinViewContainer* view = activeViewContainer();
    if (view != 0) {
        view->setUrl(url);
        updateEditActions();
        updateViewActions();
        updateGoActions();
        const QString caption = url.fileName();
        setCaption(caption);
        if (m_viewTab.count() > 1) {
            m_tabBar->setTabText(m_tabIndex, caption);
        }
        emit urlChanged(url);
    }
}

void DolphinMainWindow::changeSelection(const KFileItemList& selection)
{
    activeViewContainer()->view()->changeSelection(selection);
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

    Q_ASSERT(m_viewTab[m_tabIndex].primaryView != 0);
    int selectedUrlsCount = m_viewTab[m_tabIndex].primaryView->view()->selectedUrls().count();
    if (m_viewTab[m_tabIndex].secondaryView != 0) {
        selectedUrlsCount += m_viewTab[m_tabIndex].secondaryView->view()->selectedUrls().count();
    }

    QAction* compareFilesAction = actionCollection()->action("compare_files");
    if (selectedUrlsCount == 2) {
        const bool kompareInstalled = !KGlobal::dirs()->findExe("kompare").isEmpty();
        compareFilesAction->setEnabled(selectedUrlsCount == 2 && kompareInstalled);
    } else {
        compareFilesAction->setEnabled(false);
    }

    m_activeViewContainer->updateStatusBar();

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
    if (backAction != 0) {
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction* forwardAction = actionCollection()->action("go_forward");
    if (forwardAction != 0) {
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
    DolphinApplication::app()->createMainWindow()->show();
}

void DolphinMainWindow::openNewTab()
{
    openNewTab(m_activeViewContainer->url());
    m_tabBar->setCurrentIndex(m_viewTab.count() - 1);
}

void DolphinMainWindow::openNewTab(const KUrl& url)
{
    if (m_viewTab.count() == 1) {
        // Only one view is open currently and hence no tab is shown at
        // all. Before creating a tab for 'url', provide a tab for the current URL.
        m_tabBar->addTab(KIcon("folder"), m_activeViewContainer->url().fileName());
    }

    m_tabBar->addTab(KIcon("folder"), url.fileName());

    ViewTab viewTab;
    viewTab.splitter = new QSplitter(this);
    viewTab.primaryView = new DolphinViewContainer(this, viewTab.splitter, url);
    connectViewSignals(viewTab.primaryView);
    viewTab.primaryView->view()->reload();

    m_viewTab.append(viewTab);
}

void DolphinMainWindow::toggleActiveView()
{
    if (m_viewTab[m_tabIndex].secondaryView == 0) {
        // only one view is available
        return;
    }

    Q_ASSERT(m_activeViewContainer != 0);
    Q_ASSERT(m_viewTab[m_tabIndex].primaryView != 0);

    DolphinViewContainer* left  = m_viewTab[m_tabIndex].primaryView;
    DolphinViewContainer* right = m_viewTab[m_tabIndex].secondaryView;
    setActiveViewContainer(m_activeViewContainer == right ? left : right);
}

void DolphinMainWindow::closeEvent(QCloseEvent* event)
{
    DolphinSettings& settings = DolphinSettings::instance();
    GeneralSettings* generalSettings = settings.generalSettings();
    generalSettings->setFirstRun(false);

    settings.save();

    KXmlGuiWindow::closeEvent(event);
}

void DolphinMainWindow::saveProperties(KConfigGroup& group)
{
    // TODO: remember tabs
    DolphinViewContainer* cont = m_viewTab[m_tabIndex].primaryView;
    group.writeEntry("Primary Url", cont->url().url());
    group.writeEntry("Primary Editable Url", cont->isUrlEditable());

    cont = m_viewTab[m_tabIndex].secondaryView;
    if (cont != 0) {
        group.writeEntry("Secondary Url", cont->url().url());
        group.writeEntry("Secondary Editable Url", cont->isUrlEditable());
    }
}

void DolphinMainWindow::readProperties(const KConfigGroup& group)
{
    // TODO: read tabs
    DolphinViewContainer* cont = m_viewTab[m_tabIndex].primaryView;

    cont->setUrl(group.readEntry("Primary Url"));
    bool editable = group.readEntry("Primary Editable Url", false);
    cont->urlNavigator()->setUrlEditable(editable);

    cont = m_viewTab[m_tabIndex].secondaryView;
    const QString secondaryUrl = group.readEntry("Secondary Url");
    if (!secondaryUrl.isEmpty()) {
        if (cont == 0) {
            // a secondary view should be shown, but no one is available
            // currently -> create a new view
            toggleSplitView();
            cont = m_viewTab[m_tabIndex].secondaryView;
            Q_ASSERT(cont != 0);
        }

        cont->setUrl(secondaryUrl);
        bool editable = group.readEntry("Secondary Editable Url", false);
        cont->urlNavigator()->setUrlEditable(editable);
    } else if (cont != 0) {
        // no secondary view should be shown, but the default setting shows
        // one already -> close the view
        toggleSplitView();
    }
}

void DolphinMainWindow::updateNewMenu()
{
    m_newMenu->slotCheckUpToDate();
    m_newMenu->setPopupFiles(activeViewContainer()->url());
}

void DolphinMainWindow::properties()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();

    KPropertiesDialog *dialog = new KPropertiesDialog(list, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void DolphinMainWindow::quit()
{
    close();
}

void DolphinMainWindow::slotHandlePlacesError(const QString &message)
{
    if (!message.isEmpty()) {
        DolphinStatusBar* statusBar = m_activeViewContainer->statusBar();
        statusBar->setMessage(message, DolphinStatusBar::Error);
    }
}

void DolphinMainWindow::slotUndoAvailable(bool available)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setEnabled(available);
    }

    if (available && (m_undoCommandTypes.count() > 0)) {
        const KonqFileUndoManager::CommandType command = m_undoCommandTypes.takeFirst();
        DolphinStatusBar* statusBar = m_activeViewContainer->statusBar();
        switch (command) {
        case KonqFileUndoManager::COPY:
            statusBar->setMessage(i18nc("@info:status", "Copy operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqFileUndoManager::MOVE:
            statusBar->setMessage(i18nc("@info:status", "Move operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqFileUndoManager::LINK:
            statusBar->setMessage(i18nc("@info:status", "Link operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqFileUndoManager::TRASH:
            statusBar->setMessage(i18nc("@info:status", "Move to trash operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;
        case KonqFileUndoManager::RENAME:
            statusBar->setMessage(i18nc("@info:status", "Renaming operation completed."),
                                  DolphinStatusBar::OperationCompleted);
            break;

        case KonqFileUndoManager::MKDIR:
            statusBar->setMessage(i18nc("@info:status", "Created folder."),
                                  DolphinStatusBar::OperationCompleted);
            break;

        default:
            break;
        }

    }
}

void DolphinMainWindow::slotUndoTextChanged(const QString& text)
{
    QAction* undoAction = actionCollection()->action(KStandardAction::name(KStandardAction::Undo));
    if (undoAction != 0) {
        undoAction->setText(text);
    }
}

void DolphinMainWindow::undo()
{
    clearStatusBar();
    KonqFileUndoManager::self()->undo();
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

void DolphinMainWindow::updatePasteAction()
{
    QAction* pasteAction = actionCollection()->action(KStandardAction::name(KStandardAction::Paste));
    if (pasteAction == 0) {
        return;
    }

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
    if (m_viewTab[m_tabIndex].secondaryView == 0) {
        // create a secondary view
        QSplitter* splitter = m_viewTab[m_tabIndex].splitter;
        const int newWidth = (m_viewTab[m_tabIndex].primaryView->width() - splitter->handleWidth()) / 2;

        const DolphinView* view = m_viewTab[m_tabIndex].primaryView->view();
        m_viewTab[m_tabIndex].secondaryView = new DolphinViewContainer(this, 0, view->rootUrl());
        connectViewSignals(m_viewTab[m_tabIndex].secondaryView);
        splitter->addWidget(m_viewTab[m_tabIndex].secondaryView);
        splitter->setSizes(QList<int>() << newWidth << newWidth);
        m_viewTab[m_tabIndex].secondaryView->view()->reload();
        m_viewTab[m_tabIndex].secondaryView->setActive(false);
        m_viewTab[m_tabIndex].secondaryView->show();
    } else if (m_activeViewContainer == m_viewTab[m_tabIndex].primaryView) {
        // remove secondary view
        m_viewTab[m_tabIndex].secondaryView->close();
        m_viewTab[m_tabIndex].secondaryView->deleteLater();
        m_viewTab[m_tabIndex].secondaryView = 0;
    } else {
        // The secondary view is active, hence from a users point of view
        // the content of the secondary view should be moved to the primary view.
        // From an implementation point of view it is more efficient to close
        // the primary view and exchange the internal pointers afterwards.

        m_viewTab[m_tabIndex].primaryView->close();
        m_viewTab[m_tabIndex].primaryView->deleteLater();
        m_viewTab[m_tabIndex].primaryView = m_viewTab[m_tabIndex].secondaryView;
        m_viewTab[m_tabIndex].secondaryView = 0;
    }

    setActiveViewContainer(m_viewTab[m_tabIndex].primaryView);
    updateViewActions();
}

void DolphinMainWindow::reloadView()
{
    clearStatusBar();
    m_activeViewContainer->view()->reload();
}

void DolphinMainWindow::stopLoading()
{
}

void DolphinMainWindow::toggleFilterBarVisibility(bool show)
{
    m_activeViewContainer->showFilterBar(show);
}

void DolphinMainWindow::toggleEditLocation()
{
    clearStatusBar();

    QAction* action = actionCollection()->action("editable_location");
    KUrlNavigator* urlNavigator = m_activeViewContainer->urlNavigator();
    urlNavigator->setUrlEditable(action->isChecked());
}

void DolphinMainWindow::editLocation()
{
    KUrlNavigator* navigator = m_activeViewContainer->urlNavigator();
    navigator->setUrlEditable(true);
    navigator->setFocus();

    // select the whole text of the combo box editor
    QLineEdit* lineEdit = navigator->editor()->lineEdit();
    const QString text = lineEdit->text();
    lineEdit->setSelection(0, text.length());
}

void DolphinMainWindow::adjustViewProperties()
{
    clearStatusBar();
    ViewPropertiesDialog dlg(m_activeViewContainer->view());
    dlg.exec();
}

void DolphinMainWindow::goBack()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goBack();
}

void DolphinMainWindow::goForward()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goForward();
}

void DolphinMainWindow::goUp()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goUp();
}

void DolphinMainWindow::goHome()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goHome();
}

void DolphinMainWindow::findFile()
{
    KRun::run("kfind", m_activeViewContainer->url(), this);
}

void DolphinMainWindow::compareFiles()
{
    // The method is only invoked if exactly 2 files have
    // been selected. The selected files may be:
    // - both in the primary view
    // - both in the secondary view
    // - one in the primary view and the other in the secondary
    //   view
    Q_ASSERT(m_viewTab[m_tabIndex].primaryView != 0);

    KUrl urlA;
    KUrl urlB;
    KUrl::List urls = m_viewTab[m_tabIndex].primaryView->view()->selectedUrls();

    switch (urls.count()) {
    case 0: {
        Q_ASSERT(m_viewTab[m_tabIndex].secondaryView != 0);
        urls = m_viewTab[m_tabIndex].secondaryView->view()->selectedUrls();
        Q_ASSERT(urls.count() == 2);
        urlA = urls[0];
        urlB = urls[1];
        break;
    }

    case 1: {
        urlA = urls[0];
        Q_ASSERT(m_viewTab[m_tabIndex].secondaryView != 0);
        urls = m_viewTab[m_tabIndex].secondaryView->view()->selectedUrls();
        Q_ASSERT(urls.count() == 1);
        urlB = urls[0];
        break;
    }

    case 2: {
        urlA = urls[0];
        urlB = urls[1];
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
}

void DolphinMainWindow::editSettings()
{
    DolphinSettingsDialog dialog(this);
    dialog.exec();
}

void DolphinMainWindow::setActiveTab(int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < m_viewTab.count());
    if (index == m_tabIndex) {
        return;
    }

    // hide current tab content
    QSplitter* splitter = m_viewTab[m_tabIndex].splitter;
    m_centralWidgetLayout->removeWidget(splitter);
    splitter->hide();

    // show active tab content
    m_tabIndex = index;

    ViewTab& viewTab = m_viewTab[index];
    m_centralWidgetLayout->addWidget(viewTab.splitter);
    viewTab.splitter->show();
    viewTab.primaryView->show();
    if (viewTab.secondaryView != 0) {
        viewTab.secondaryView->show();
    }

    setActiveViewContainer(viewTab.primaryView);
}

void DolphinMainWindow::init()
{
    DolphinSettings& settings = DolphinSettings::instance();

    // Check whether Dolphin runs the first time. If yes then
    // a proper default window size is given at the end of DolphinMainWindow::init().
    GeneralSettings* generalSettings = settings.generalSettings();
    const bool firstRun = generalSettings->firstRun();
    if (firstRun) {
        generalSettings->setViewPropsTimestamp(QDateTime::currentDateTime());
    }

    setAcceptDrops(true);

    m_viewTab[m_tabIndex].splitter = new QSplitter(this);

    setupActions();

    const KUrl& homeUrl = generalSettings->homeUrl();
    setCaption(homeUrl.fileName());
    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    connect(m_actionHandler, SIGNAL(actionBeingHandled()), SLOT(clearStatusBar()));
    ViewProperties props(homeUrl);
    m_viewTab[m_tabIndex].primaryView = new DolphinViewContainer(this,
                                                                 m_viewTab[m_tabIndex].splitter,
                                                                 homeUrl);

    m_activeViewContainer = m_viewTab[m_tabIndex].primaryView;
    connectViewSignals(m_activeViewContainer);
    DolphinView* view = m_activeViewContainer->view();
    view->reload();
    m_activeViewContainer->show();
    m_actionHandler->setCurrentView(view);

    m_tabBar = new KTabBar(this);
    m_tabBar->setHoverCloseButton(true);
    connect(m_tabBar, SIGNAL(currentChanged(int)),
            this, SLOT(setActiveTab(int)));

    QWidget* centralWidget = new QWidget(this);
    m_centralWidgetLayout = new QVBoxLayout(centralWidget);
    m_centralWidgetLayout->setSpacing(0);
    m_centralWidgetLayout->setMargin(0);
    m_centralWidgetLayout->addWidget(m_tabBar);
    m_centralWidgetLayout->addWidget(m_viewTab[m_tabIndex].splitter);


    setCentralWidget(centralWidget);
    setupDockWidgets();

    setupGUI(Keys | Save | Create | ToolBar);
    createGUI();

    stateChanged("new_file");
    setAutoSaveSettings();

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));
    updatePasteAction();
    updateGoActions();

    if (generalSettings->splitView()) {
        toggleSplitView();
    }
    updateViewActions();

    if (firstRun) {
        // assure a proper default size if Dolphin runs the first time
        resize(750, 500);
    }

    emit urlChanged(homeUrl);
}

void DolphinMainWindow::setActiveViewContainer(DolphinViewContainer* viewContainer)
{
    Q_ASSERT(viewContainer != 0);
    Q_ASSERT((viewContainer == m_viewTab[m_tabIndex].primaryView) ||
             (viewContainer == m_viewTab[m_tabIndex].secondaryView));
    if (m_activeViewContainer == viewContainer) {
        return;
    }

    m_activeViewContainer->setActive(false);
    m_activeViewContainer = viewContainer;
    m_activeViewContainer->setActive(true);

    m_actionHandler->setCurrentView(viewContainer->view());

    updateHistory();
    updateEditActions();
    updateViewActions();
    updateGoActions();

    const KUrl& url = m_activeViewContainer->url();
    const QString caption = url.fileName();
    setCaption(caption);
    if (m_viewTab.count() > 1) {
        m_tabBar->setTabText(m_tabIndex, caption);
    }

    emit urlChanged(url);
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newMenu = new DolphinNewMenu(this);
    KMenu* menu = m_newMenu->menu();
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
    newTab->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_N);
    connect(newTab, SIGNAL(triggered()), this, SLOT(openNewTab()));

    KAction* properties = actionCollection()->addAction("properties");
    properties->setText(i18nc("@action:inmenu File", "Properties"));
    properties->setShortcut(Qt::ALT | Qt::Key_Return);
    connect(properties, SIGNAL(triggered()), this, SLOT(properties()));

    KStandardAction::quit(this, SLOT(quit()), actionCollection());

    // setup 'Edit' menu
    KStandardAction::undo(this,
                          SLOT(undo()),
                          actionCollection());

    // need to remove shift+del from cut action, else the shortcut for deletejob
    // doesn't work
    KAction* cut = KStandardAction::cut(this, SLOT(cut()), actionCollection());
    KShortcut cutShortcut = cut->shortcut();
    cutShortcut.remove(Qt::SHIFT + Qt::Key_Delete, KShortcut::KeepEmpty);
    cut->setShortcut(cutShortcut);
    KStandardAction::copy(this, SLOT(copy()), actionCollection());
    KStandardAction::paste(this, SLOT(paste()), actionCollection());

    KAction* selectAll = actionCollection()->addAction("select_all");
    selectAll->setText(i18nc("@action:inmenu Edit", "Select All"));
    selectAll->setShortcut(Qt::CTRL + Qt::Key_A);
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
    stop->setIcon(KIcon("process-stop"));
    connect(stop, SIGNAL(triggered()), this, SLOT(stopLoading()));

    // TODO: the naming "Show full Location" is currently confusing...
    KToggleAction* showFullLocation = actionCollection()->add<KToggleAction>("editable_location");
    showFullLocation->setText(i18nc("@action:inmenu Navigation Bar", "Show Full Location"));
    showFullLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(showFullLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    KAction* editLocation = actionCollection()->addAction("edit_location");
    editLocation->setText(i18nc("@action:inmenu Navigation Bar", "Edit Location"));
    editLocation->setShortcut(Qt::Key_F6);
    connect(editLocation, SIGNAL(triggered()), this, SLOT(editLocation()));

    KAction* adjustViewProps = actionCollection()->addAction("view_properties");
    adjustViewProps->setText(i18nc("@action:inmenu View", "Adjust View Properties..."));
    connect(adjustViewProps, SIGNAL(triggered()), this, SLOT(adjustViewProperties()));

    // setup 'Go' menu
    KAction* backAction = KStandardAction::back(this, SLOT(goBack()), actionCollection());
    KShortcut backShortcut = backAction->shortcut();
    backShortcut.setAlternate(Qt::Key_Backspace);
    backAction->setShortcut(backShortcut);

    KStandardAction::forward(this, SLOT(goForward()), actionCollection());
    KStandardAction::up(this, SLOT(goUp()), actionCollection());
    KStandardAction::home(this, SLOT(goHome()), actionCollection());

    // setup 'Tools' menu
    QAction* findFile = actionCollection()->addAction("find_file");
    findFile->setText(i18nc("@action:inmenu Tools", "Find File..."));
    findFile->setShortcut(Qt::CTRL | Qt::Key_F);
    findFile->setIcon(KIcon("edit-find"));
    connect(findFile, SIGNAL(triggered()), this, SLOT(findFile()));

    KToggleAction* showFilterBar = actionCollection()->add<KToggleAction>("show_filter_bar");
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Show Filter Bar"));
    showFilterBar->setShortcut(Qt::CTRL | Qt::Key_I);
    connect(showFilterBar, SIGNAL(triggered(bool)), this, SLOT(toggleFilterBarVisibility(bool)));

    KAction* compareFiles = actionCollection()->addAction("compare_files");
    compareFiles->setText(i18nc("@action:inmenu Tools", "Compare Files"));
    compareFiles->setIcon(KIcon("kompare"));
    compareFiles->setEnabled(false);
    connect(compareFiles, SIGNAL(triggered()), this, SLOT(compareFiles()));

    // setup 'Settings' menu
    m_showMenuBar = KStandardAction::showMenubar(this, SLOT(toggleShowMenuBar()), actionCollection());
    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());
}

void DolphinMainWindow::setupDockWidgets()
{
    // setup "Information"
    QDockWidget* infoDock = new QDockWidget(i18nc("@title:window", "Information"));
    infoDock->setObjectName("infoDock");
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    SidebarPage* infoWidget = new InfoSidebarPage(infoDock);
    infoDock->setWidget(infoWidget);

    infoDock->toggleViewAction()->setText(i18nc("@title:window", "Information"));
    infoDock->toggleViewAction()->setShortcut(Qt::Key_F11);
    actionCollection()->addAction("show_info_panel", infoDock->toggleViewAction());

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            infoWidget, SLOT(setUrl(KUrl)));
    connect(this, SIGNAL(selectionChanged(KFileItemList)),
            infoWidget, SLOT(setSelection(KFileItemList)));
    connect(this, SIGNAL(requestItemInfo(KFileItem)),
            infoWidget, SLOT(requestDelayedItemInfo(KFileItem)));

    // setup "Tree View"
    QDockWidget* treeViewDock = new QDockWidget(i18nc("@title:window", "Folders"));
    treeViewDock->setObjectName("treeViewDock");
    treeViewDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    TreeViewSidebarPage* treeWidget = new TreeViewSidebarPage(treeViewDock);
    treeViewDock->setWidget(treeWidget);

    treeViewDock->toggleViewAction()->setText(i18nc("@title:window", "Folders"));
    treeViewDock->toggleViewAction()->setShortcut(Qt::Key_F7);
    actionCollection()->addAction("show_folders_panel", treeViewDock->toggleViewAction());

    addDockWidget(Qt::LeftDockWidgetArea, treeViewDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            treeWidget, SLOT(setUrl(KUrl)));
    connect(treeWidget, SIGNAL(changeUrl(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(treeWidget, SIGNAL(changeSelection(KFileItemList)),
            this, SLOT(changeSelection(KFileItemList)));
    connect(treeWidget, SIGNAL(urlsDropped(KUrl::List, KUrl)),
            this, SLOT(dropUrls(KUrl::List, KUrl)));

    // setup "Terminal"
#ifndef Q_OS_WIN
    QDockWidget* terminalDock = new QDockWidget(i18nc("@title:window Shell terminal", "Terminal"));
    terminalDock->setObjectName("terminalDock");
    terminalDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    SidebarPage* terminalWidget = new TerminalSidebarPage(terminalDock);
    terminalDock->setWidget(terminalWidget);

    connect(terminalWidget, SIGNAL(hideTerminalSidebarPage()), terminalDock, SLOT(hide()));

    terminalDock->toggleViewAction()->setText(i18nc("@title:window Shell terminal", "Terminal"));
    terminalDock->toggleViewAction()->setShortcut(Qt::Key_F4);
    actionCollection()->addAction("show_terminal_panel", terminalDock->toggleViewAction());

    addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            terminalWidget, SLOT(setUrl(KUrl)));
#endif

    const bool firstRun = DolphinSettings::instance().generalSettings()->firstRun();
    if (firstRun) {
        treeViewDock->hide();
#ifndef Q_OS_WIN
        terminalDock->hide();
#endif
    }

    QDockWidget* placesDock = new QDockWidget(i18nc("@title:window", "Places"));
    placesDock->setObjectName("placesDock");
    placesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    DolphinFilePlacesView* placesView = new DolphinFilePlacesView(placesDock);
    placesDock->setWidget(placesView);
    placesView->setModel(DolphinSettings::instance().placesModel());
    placesView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    placesDock->toggleViewAction()->setText(i18nc("@title:window", "Places"));
    placesDock->toggleViewAction()->setShortcut(Qt::Key_F9);
    actionCollection()->addAction("show_places_panel", placesDock->toggleViewAction());

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(placesView, SIGNAL(urlChanged(KUrl)),
            this, SLOT(changeUrl(KUrl)));
    connect(this, SIGNAL(urlChanged(KUrl)),
            placesView, SLOT(setUrl(KUrl)));
}

void DolphinMainWindow::updateEditActions()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if (list.isEmpty()) {
        stateChanged("has_no_selection");
    } else {
        stateChanged("has_selection");

        QAction* renameAction = actionCollection()->action("rename");
        if (renameAction != 0) {
            renameAction->setEnabled(true);
        }

        bool enableMoveToTrash = true;

        KFileItemList::const_iterator it = list.begin();
        const KFileItemList::const_iterator end = list.end();
        while (it != end) {
            const KUrl& url = (*it).url();
            // only enable the 'Move to Trash' action for local files
            if (!url.isLocalFile()) {
                enableMoveToTrash = false;
            }
            ++it;
        }

        QAction* moveToTrashAction = actionCollection()->action("move_to_trash");
        moveToTrashAction->setEnabled(enableMoveToTrash);
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
    const KUrl& currentUrl = m_activeViewContainer->url();
    goUpAction->setEnabled(currentUrl.upUrl() != currentUrl);
}

void DolphinMainWindow::clearStatusBar()
{
    m_activeViewContainer->statusBar()->clear();
}

void DolphinMainWindow::connectViewSignals(DolphinViewContainer* container)
{
    connect(container, SIGNAL(showFilterBarChanged(bool)),
            this, SLOT(updateFilterBarAction(bool)));

    DolphinView* view = container->view();
    connect(view, SIGNAL(selectionChanged(KFileItemList)),
            this, SLOT(slotSelectionChanged(KFileItemList)));
    connect(view, SIGNAL(requestItemInfo(KFileItem)),
            this, SLOT(slotRequestItemInfo(KFileItem)));
    connect(view, SIGNAL(activated()),
            this, SLOT(toggleActiveView()));
    connect(view, SIGNAL(doingOperation(KonqFileUndoManager::CommandType)),
            this, SLOT(slotDoingOperation(KonqFileUndoManager::CommandType)));
    connect(view, SIGNAL(tabRequested(const KUrl&)),
            this, SLOT(openNewTab(const KUrl&)));

    const KUrlNavigator* navigator = container->urlNavigator();
    connect(navigator, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(changeUrl(const KUrl&)));
    connect(navigator, SIGNAL(historyChanged()),
            this, SLOT(updateHistory()));
    connect(navigator, SIGNAL(editableStateChanged(bool)),
            this, SLOT(slotEditableStateChanged(bool)));
}

void DolphinMainWindow::updateSplitAction()
{
    QAction* splitAction = actionCollection()->action("split_view");
    if (m_viewTab[m_tabIndex].secondaryView != 0) {
        if (m_activeViewContainer == m_viewTab[m_tabIndex].primaryView) {
            splitAction->setText(i18nc("@action:intoolbar Close right view", "Close"));
            splitAction->setIcon(KIcon("view-right-close"));
        } else {
            splitAction->setText(i18nc("@action:intoolbar Close left view", "Close"));
            splitAction->setIcon(KIcon("view-left-close"));
        }
    } else {
        splitAction->setText(i18nc("@action:intoolbar Split view", "Split"));
        splitAction->setIcon(KIcon("view-right-new"));
    }
}

DolphinMainWindow::UndoUiInterface::UndoUiInterface(DolphinMainWindow* mainWin) :
    KonqFileUndoManager::UiInterface(mainWin),
    m_mainWin(mainWin)
{
    Q_ASSERT(m_mainWin != 0);
}

DolphinMainWindow::UndoUiInterface::~UndoUiInterface()
{
}

void DolphinMainWindow::UndoUiInterface::jobError(KIO::Job* job)
{
    DolphinStatusBar* statusBar = m_mainWin->activeViewContainer()->statusBar();
    statusBar->setMessage(job->errorString(), DolphinStatusBar::Error);
}

#include "dolphinmainwindow.moc"
