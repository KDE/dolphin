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
#include "dolphinremoteencoding.h"

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
#include "search/dolphinsearchoptionsconfigurator.h"
#endif

#include "dolphinapplication.h"
#include "dolphinnewmenu.h"
#include "search/dolphinsearchbox.h"
#include "settings/dolphinsettings.h"
#include "settings/dolphinsettingsdialog.h"
#include "dolphinviewcontainer.h"
#include "panels/folders/folderspanel.h"
#include "panels/places/placespanel.h"
#include "panels/information/informationpanel.h"
#include "mainwindowadaptor.h"
#include "statusbar/dolphinstatusbar.h"
#include "viewproperties.h"

#ifndef Q_OS_WIN
#include "panels/terminal/terminalpanel.h"
#endif

#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "draganddrophelper.h"

#include <kaction.h>
#include <kactioncollection.h>
#include <kconfig.h>
#include <kdesktopfile.h>
#include <kdeversion.h>
#include <kfiledialog.h>
#include <kfileplacesmodel.h>
#include <kglobal.h>
#include <klineedit.h>
#include <ktoolbar.h>
#include <kicon.h>
#include <kiconloader.h>
#include <kio/netaccess.h>
#include <kinputdialog.h>
#include <klocale.h>
#include <kprotocolmanager.h>
#include <kmenu.h>
#include <kmenubar.h>
#include <kmessagebox.h>
#include <kfileitemlistproperties.h>
#include <konqmimedata.h>
#include <kprotocolinfo.h>
#include <krun.h>
#include <kshell.h>
#include <kstandarddirs.h>
#include <kstatusbar.h>
#include <kstandardaction.h>
#include <ktabbar.h>
#include <ktoggleaction.h>
#include <kurlnavigator.h>
#include <kurl.h>
#include <kurlcombobox.h>
#include <ktoolinvocation.h>

#include <QDBusMessage>
#include <QKeyEvent>
#include <QClipboard>
#include <QSplitter>
#include <QDockWidget>
#include <kacceleratormanager.h>

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

DolphinMainWindow::DolphinMainWindow(int id) :
    KXmlGuiWindow(0),
    m_newMenu(0),
    m_showMenuBar(0),
    m_tabBar(0),
    m_activeViewContainer(0),
    m_centralWidgetLayout(0),
    m_searchBox(0),
#ifdef HAVE_NEPOMUK
    m_searchOptionsConfigurator(0),
#endif
    m_id(id),
    m_tabIndex(0),
    m_viewTab(),
    m_actionHandler(0),
    m_remoteEncoding(0),
    m_settingsDialog(0),
    m_captionStatJob(0)
{
    setObjectName("Dolphin#");

    m_viewTab.append(ViewTab());

    new MainWindowAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QString("/dolphin/MainWindow%1").arg(m_id), this);

    KIO::FileUndoManager* undoManager = KIO::FileUndoManager::self();
    undoManager->setUiInterface(new UndoUiInterface());

    connect(undoManager, SIGNAL(undoAvailable(bool)),
            this, SLOT(slotUndoAvailable(bool)));
    connect(undoManager, SIGNAL(undoTextChanged(const QString&)),
            this, SLOT(slotUndoTextChanged(const QString&)));
    connect(undoManager, SIGNAL(jobRecordingStarted(CommandType)),
            this, SLOT(clearStatusBar()));
    connect(undoManager, SIGNAL(jobRecordingFinished(CommandType)),
            this, SLOT(showCommand(CommandType)));
    connect(DolphinSettings::instance().placesModel(), SIGNAL(errorMessage(const QString&)),
            this, SLOT(showErrorMessage(const QString&)));
    connect(&DragAndDropHelper::instance(), SIGNAL(errorMessage(const QString&)),
            this, SLOT(showErrorMessage(const QString&)));
}

DolphinMainWindow::~DolphinMainWindow()
{
    DolphinApplication::app()->removeMainWindow(this);
}

void DolphinMainWindow::openDirectories(const QList<KUrl>& dirs)
{
    if (dirs.isEmpty()) {
        return;
    }

    const int oldOpenTabsCount = m_viewTab.count();

    const GeneralSettings* generalSettings = DolphinSettings::instance().generalSettings();
    const bool hasSplitView = generalSettings->splitView();

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

    // remove the previously opened tabs
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
        if (m_viewTab[i].secondaryView != 0) {
            m_viewTab[i].secondaryView->view()->markUrlsAsSelected(files);
        }
    }
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

void DolphinMainWindow::showCommand(CommandType command)
{
    DolphinStatusBar* statusBar = m_activeViewContainer->statusBar();
    switch (command) {
    case KIO::FileUndoManager::Copy:
        statusBar->setMessage(i18nc("@info:status", "Successfully copied."),
                              DolphinStatusBar::OperationCompleted);
        break;
    case KIO::FileUndoManager::Move:
        statusBar->setMessage(i18nc("@info:status", "Successfully moved."),
                              DolphinStatusBar::OperationCompleted);
        break;
    case KIO::FileUndoManager::Link:
        statusBar->setMessage(i18nc("@info:status", "Successfully linked."),
                              DolphinStatusBar::OperationCompleted);
        break;
    case KIO::FileUndoManager::Trash:
        statusBar->setMessage(i18nc("@info:status", "Successfully moved to trash."),
                              DolphinStatusBar::OperationCompleted);
        break;
    case KIO::FileUndoManager::Rename:
        statusBar->setMessage(i18nc("@info:status", "Successfully renamed."),
                              DolphinStatusBar::OperationCompleted);
        break;

    case KIO::FileUndoManager::Mkdir:
        statusBar->setMessage(i18nc("@info:status", "Created folder."),
                              DolphinStatusBar::OperationCompleted);
        break;

    default:
        break;
    }
}

void DolphinMainWindow::refreshViews()
{
    Q_ASSERT(m_viewTab[m_tabIndex].primaryView != 0);

    // remember the current active view, as because of
    // the refreshing the active view might change to
    // the secondary view
    DolphinViewContainer* activeViewContainer = m_activeViewContainer;

    const int tabCount = m_viewTab.count();
    for (int i = 0; i < tabCount; ++i) {
        m_viewTab[i].primaryView->refresh();
        if (m_viewTab[i].secondaryView != 0) {
            m_viewTab[i].secondaryView->refresh();
        }
    }

    setActiveViewContainer(activeViewContainer);
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
    if (view != 0) {
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

    Q_ASSERT(m_viewTab[m_tabIndex].primaryView != 0);
    int selectedUrlsCount = m_viewTab[m_tabIndex].primaryView->view()->selectedItemsCount();
    if (m_viewTab[m_tabIndex].secondaryView != 0) {
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

void DolphinMainWindow::slotWheelMoved(int wheelDelta)
{
    if (wheelDelta > 0) {
        activatePrevTab();
    } else {
        activateNextTab();
    }
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
    backAction->setToolTip(i18nc("@info", "Go back"));
    if (backAction != 0) {
        backAction->setEnabled(index < urlNavigator->historySize() - 1);
    }

    QAction* forwardAction = actionCollection()->action("go_forward");
    forwardAction->setToolTip(i18nc("@info", "Go forward"));
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

    KUrlNavigator* navigator = m_activeViewContainer->urlNavigator();
    if (navigator->isUrlEditable()) {
        // if a new tab is opened and the URL is editable, assure that
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
    viewTab.primaryView = new DolphinViewContainer(this, viewTab.splitter, url);
    viewTab.primaryView->setActive(false);
    connectViewSignals(viewTab.primaryView);
    viewTab.primaryView->view()->reload();

    m_viewTab.append(viewTab);

    actionCollection()->action("close_tab")->setEnabled(true);

    // provide a split view, if the startup settings are set this way
    const GeneralSettings* generalSettings = DolphinSettings::instance().generalSettings();
    if (generalSettings->splitView()) {
        const int tabIndex = m_viewTab.count() - 1;
        createSecondaryView(tabIndex);
        m_viewTab[tabIndex].secondaryView->setActive(true);
        m_viewTab[tabIndex].isPrimaryViewActive = false;
    }

    if (focusWidget != 0) {
        // The DolphinViewContainer grabbed the keyboard focus. As the tab is opened
        // in background, assure that the previous focused widget gets the focus back.
        focusWidget->setFocus();
    }
}

void DolphinMainWindow::activateNextTab()
{
    if ((m_viewTab.count() == 1) || (m_tabBar->count() < 2)) {
        return;
    }

    const int tabIndex = (m_tabBar->currentIndex() + 1) % m_tabBar->count();
    m_tabBar->setCurrentIndex(tabIndex);
}

void DolphinMainWindow::activatePrevTab()
{
    if ((m_viewTab.count() == 1) || (m_tabBar->count() < 2)) {
        return;
    }

    int tabIndex = m_tabBar->currentIndex() - 1;
    if (tabIndex == -1) {
        tabIndex = m_tabBar->count() - 1;
    }
    m_tabBar->setCurrentIndex(tabIndex);
}

void DolphinMainWindow::openInNewTab()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if ((list.count() == 1) && list[0].isDir()) {
        openNewTab(m_activeViewContainer->view()->selectedUrls()[0]);
    }
}

void DolphinMainWindow::openInNewWindow()
{
    const KFileItemList list = m_activeViewContainer->view()->selectedItems();
    if ((list.count() == 1) && list[0].isDir()) {
        DolphinMainWindow* window = DolphinApplication::app()->createMainWindow();
        window->changeUrl(m_activeViewContainer->view()->selectedUrls()[0]);
        window->show();
    }
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

void DolphinMainWindow::showEvent(QShowEvent* event)
{
    KXmlGuiWindow::showEvent(event);
    if (!event->spontaneous()) {
        m_activeViewContainer->view()->setFocus();
    }
}

void DolphinMainWindow::closeEvent(QCloseEvent* event)
{
    DolphinSettings& settings = DolphinSettings::instance();
    GeneralSettings* generalSettings = settings.generalSettings();

    // Find out if Dolphin is closed directly by the user or
    // by the session manager because the session is closed
    bool closedByUser = true;
    DolphinApplication *application = qobject_cast<DolphinApplication*>(qApp);
    if (application && application->sessionSaving()) {
        closedByUser = false;
    }

    if ((m_viewTab.count() > 1) && generalSettings->confirmClosingMultipleTabs() && closedByUser) {
        // Ask the user if he really wants to quit and close all tabs.
        // Open a confirmation dialog with 3 buttons:
        // KDialog::Yes    -> Quit
        // KDialog::No     -> Close only the current tab
        // KDialog::Cancel -> do nothing
        KDialog *dialog = new KDialog(this, Qt::Dialog);
        dialog->setCaption(i18nc("@title:window", "Confirmation"));
        dialog->setButtons(KDialog::Yes | KDialog::No | KDialog::Cancel);
        dialog->setModal(true);
        dialog->showButtonSeparator(true);
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
            generalSettings->setConfirmClosingMultipleTabs(false);
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

    generalSettings->setFirstRun(false);

    settings.save();

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
        group.writeEntry(tabProperty("Primary Editable", i), cont->isUrlEditable());

        cont = m_viewTab[i].secondaryView;
        if (cont != 0) {
            group.writeEntry(tabProperty("Secondary URL", i), cont->url().url());
            group.writeEntry(tabProperty("Secondary Editable", i), cont->isUrlEditable());
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
            if (cont == 0) {
                // a secondary view should be shown, but no one is available
                // currently -> create a new view
                toggleSplitView();
                cont = m_viewTab[i].secondaryView;
                Q_ASSERT(cont != 0);
            }

            cont->setUrl(secondaryUrl);
            const bool editable = group.readEntry(tabProperty("Secondary Editable", i), false);
            cont->urlNavigator()->setUrlEditable(editable);
        } else if (cont != 0) {
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
    m_newMenu->setViewShowsHiddenFiles(activeViewContainer()->view()->showHiddenFiles());
    m_newMenu->slotCheckUpToDate();
    m_newMenu->setPopupFiles(activeViewContainer()->url());
}

void DolphinMainWindow::createDirectory()
{
    m_newMenu->setViewShowsHiddenFiles(activeViewContainer()->view()->showHiddenFiles());
    m_newMenu->setPopupFiles(activeViewContainer()->url());
    m_newMenu->createDirectory();
}

void DolphinMainWindow::quit()
{
    close();
}

void DolphinMainWindow::showErrorMessage(const QString& message)
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
    if (undoAction != 0) {
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
    if (m_viewTab[m_tabIndex].secondaryView == 0) {
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

void DolphinMainWindow::replaceLocation()
{
    KUrlNavigator* navigator = m_activeViewContainer->urlNavigator();
    navigator->setUrlEditable(true);
    navigator->setFocus();

    // select the whole text of the combo box editor
    QLineEdit* lineEdit = navigator->editor()->lineEdit();  // krazy:exclude=qclasses
    const QString text = lineEdit->text();
    lineEdit->setSelection(0, text.length());
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

void DolphinMainWindow::goBack(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goBack().
    if (buttons == Qt::MidButton) {
        KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
        openNewTab(urlNavigator->historyUrl(urlNavigator->historyIndex() + 1));
    }
}

void DolphinMainWindow::goForward(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goForward().
    if (buttons == Qt::MidButton) {
        KUrlNavigator* urlNavigator = activeViewContainer()->urlNavigator();
        openNewTab(urlNavigator->historyUrl(urlNavigator->historyIndex() - 1));
    }
}

void DolphinMainWindow::goUp(Qt::MouseButtons buttons)
{
    // The default case (left button pressed) is handled in goUp().
    if (buttons == Qt::MidButton) {
        openNewTab(activeViewContainer()->url().upUrl());
    }
}

void DolphinMainWindow::goHome()
{
    clearStatusBar();
    m_activeViewContainer->urlNavigator()->goHome();
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
    if (m_settingsDialog == 0) {
        const KUrl& url = activeViewContainer()->url();
        m_settingsDialog = new DolphinSettingsDialog(url, this);
        m_settingsDialog->setAttribute(Qt::WA_DeleteOnClose);
        m_settingsDialog->show();
    } else {
        m_settingsDialog->raise();
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
    if (hiddenTab.secondaryView != 0) {
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
    if (viewTab.secondaryView != 0) {
        viewTab.secondaryView->show();
    }
    viewTab.splitter->show();

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
    if (m_viewTab[index].secondaryView != 0) {
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

    QAction* closeOtherTabsAction = menu.addAction(KIcon("tab-close-other"), i18nc("@action:inmenu", "Close Other Tabs"));

    QAction* closeTabAction = menu.addAction(KIcon("tab-close"), i18nc("@action:inmenu", "Close Tab"));
    closeTabAction->setShortcut(actionCollection()->action("close_tab")->shortcut());
    QAction* selectedAction = menu.exec(pos);
    if (selectedAction == newTabAction) {
        const ViewTab& tab = m_viewTab[index];
        Q_ASSERT(tab.primaryView != 0);
        const KUrl url = (tab.secondaryView != 0) && tab.secondaryView->isActive() ?
                         tab.secondaryView->url() : tab.primaryView->url();
        openNewTab(url);
        m_tabBar->setCurrentIndex(m_viewTab.count() - 1);
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

void DolphinMainWindow::handlePlacesClick(const KUrl& url, Qt::MouseButtons buttons)
{
    if (buttons & Qt::MidButton) {
        openNewTab(url);
        m_tabBar->setCurrentIndex(m_viewTab.count() - 1);
    } else {
        changeUrl(url);
    }
}

void DolphinMainWindow::slotTestCanDecode(const QDragMoveEvent* event, bool& canDecode)
{
    canDecode = KUrl::List::canDecode(event->mimeData());
}

void DolphinMainWindow::searchItems()
{
#ifdef HAVE_NEPOMUK
    const KUrl nepomukSearchUrl = m_searchOptionsConfigurator->nepomukSearchUrl();
    m_activeViewContainer->setUrl(nepomukSearchUrl);
#endif
}

void DolphinMainWindow::slotTabMoved(int from, int to)
{
    m_viewTab.move(from, to);
    m_tabIndex = m_tabBar->currentIndex();
}

void DolphinMainWindow::showSearchOptions()
{
#ifdef HAVE_NEPOMUK
    m_searchOptionsConfigurator->show();
#endif
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
    m_viewTab[m_tabIndex].splitter->setChildrenCollapsible(false);

    setupActions();

    const KUrl& homeUrl = generalSettings->homeUrl();
    setUrlAsCaption(homeUrl);
    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    connect(m_actionHandler, SIGNAL(actionBeingHandled()), SLOT(clearStatusBar()));
    connect(m_actionHandler, SIGNAL(createDirectory()), SLOT(createDirectory()));
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

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, SIGNAL(urlChanged(const KUrl&)),
            m_remoteEncoding, SLOT(slotAboutToOpenUrl()));

#ifdef HAVE_NEPOMUK
    m_searchOptionsConfigurator = new DolphinSearchOptionsConfigurator(this);
    m_searchOptionsConfigurator->hide();
    connect(m_searchOptionsConfigurator, SIGNAL(searchOptionsChanged()),
            this, SLOT(searchItems()));
    connect(this, SIGNAL(urlChanged(KUrl)), m_searchOptionsConfigurator, SLOT(setDirectory(KUrl)));
#endif

    m_tabBar = new KTabBar(this);
    m_tabBar->setMovable(true);
    m_tabBar->setTabsClosable(true);
    connect(m_tabBar, SIGNAL(currentChanged(int)),
            this, SLOT(setActiveTab(int)));
    connect(m_tabBar, SIGNAL(tabCloseRequested(int)),
            this, SLOT(closeTab(int)));
    connect(m_tabBar, SIGNAL(contextMenu(int, const QPoint&)),
            this, SLOT(openTabContextMenu(int, const QPoint&)));
    connect(m_tabBar, SIGNAL(newTabRequest()),
            this, SLOT(openNewTab()));
    connect(m_tabBar, SIGNAL(testCanDecode(const QDragMoveEvent*, bool&)),
            this, SLOT(slotTestCanDecode(const QDragMoveEvent*, bool&)));
    connect(m_tabBar, SIGNAL(wheelDelta(int)),
	    this, SLOT(slotWheelMoved(int)));
    connect(m_tabBar, SIGNAL(mouseMiddleClick(int)),
            this, SLOT(closeTab(int)));
    connect(m_tabBar, SIGNAL(tabMoved(int, int)),
            this, SLOT(slotTabMoved(int, int)));

    m_tabBar->blockSignals(true);  // signals get unblocked after at least 2 tabs are open

    QWidget* centralWidget = new QWidget(this);
    m_centralWidgetLayout = new QVBoxLayout(centralWidget);
    m_centralWidgetLayout->setSpacing(0);
    m_centralWidgetLayout->setMargin(0);
#ifdef HAVE_NEPOMUK
    m_centralWidgetLayout->addWidget(m_searchOptionsConfigurator);
#endif
    m_centralWidgetLayout->addWidget(m_tabBar);
    m_centralWidgetLayout->addWidget(m_viewTab[m_tabIndex].splitter, 1);

    setCentralWidget(centralWidget);
    setupDockWidgets();
    emit urlChanged(homeUrl);

    setupGUI(Keys | Save | Create | ToolBar);

    m_searchBox->setParent(toolBar("searchToolBar"));
    m_searchBox->show();
    connect(m_searchBox, SIGNAL(requestSearchOptions()),
            this, SLOT(showSearchOptions()));
#ifdef HAVE_NEPOMUK
    connect(m_searchBox, SIGNAL(searchTextChanged(QString)),
            m_searchOptionsConfigurator, SLOT(setCustomSearchQuery(QString)));
#endif

    stateChanged("new_file");

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));
    updatePasteAction();
    updateGoActions();

    if (generalSettings->splitView()) {
        toggleSplitView();
    }
    updateViewActions();

    QAction* showFilterBarAction = actionCollection()->action("show_filter_bar");
    showFilterBarAction->setChecked(generalSettings->filterBar());

    if (firstRun) {
        // assure a proper default size if Dolphin runs the first time
        resize(750, 500);
    }

    m_showMenuBar->setChecked(!menuBar()->isHidden());  // workaround for bug #171080
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

    const KUrl& url = m_activeViewContainer->url();
    setUrlAsCaption(url);
    if (m_viewTab.count() > 1 && m_viewTab[m_tabIndex].secondaryView != 0) {
        m_tabBar->setTabText(m_tabIndex, tabName(url));
        m_tabBar->setTabIcon(m_tabIndex, KIcon(KMimeType::iconNameForUrl(url)));
    }

    emit urlChanged(url);
}

void DolphinMainWindow::setupActions()
{
    // setup 'File' menu
    m_newMenu = new DolphinNewMenu(this, this);
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
    cutShortcut.remove(Qt::SHIFT + Qt::Key_Delete, KShortcut::KeepEmpty);
    cut->setShortcut(cutShortcut);
    KStandardAction::copy(this, SLOT(copy()), actionCollection());
    KAction* paste = KStandardAction::paste(this, SLOT(paste()), actionCollection());
    // The text of the paste-action is modified dynamically by Dolphin
    // (e. g. to "Paste One Folder"). To prevent that the size of the toolbar changes
    // due to the long text, the text "Paste" is used:
    paste->setIconText(i18nc("@action:inmenu Edit", "Paste"));

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
    stop->setToolTip(i18nc("@info", "Stop loading"));
    stop->setIcon(KIcon("process-stop"));
    connect(stop, SIGNAL(triggered()), this, SLOT(stopLoading()));

    KToggleAction* showFullLocation = actionCollection()->add<KToggleAction>("editable_location");
    showFullLocation->setText(i18nc("@action:inmenu Navigation Bar", "Editable Location"));
    showFullLocation->setShortcut(Qt::CTRL | Qt::Key_L);
    connect(showFullLocation, SIGNAL(triggered()), this, SLOT(toggleEditLocation()));

    KAction* replaceLocation = actionCollection()->addAction("replace_location");
    replaceLocation->setText(i18nc("@action:inmenu Navigation Bar", "Replace Location"));
    replaceLocation->setShortcut(Qt::Key_F6);
    connect(replaceLocation, SIGNAL(triggered()), this, SLOT(replaceLocation()));

    // setup 'Go' menu
    KAction* backAction = KStandardAction::back(this, SLOT(goBack()), actionCollection());
    connect(backAction, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)), this, SLOT(goBack(Qt::MouseButtons)));
    KShortcut backShortcut = backAction->shortcut();
    backShortcut.setAlternate(Qt::Key_Backspace);
    backAction->setShortcut(backShortcut);

    m_recentTabsMenu = new KActionMenu(i18n("Recently Closed Tabs"), this);
    m_recentTabsMenu->setIcon(KIcon("edit-undo"));
    actionCollection()->addAction("closed_tabs", m_recentTabsMenu);
    connect(m_recentTabsMenu->menu(), SIGNAL(triggered(QAction *)),
            this, SLOT(restoreClosedTab(QAction *)));

    QAction* action = new QAction("Empty Recently Closed Tabs", m_recentTabsMenu);
    action->setIcon(KIcon("edit-clear-list"));
    action->setData(QVariant::fromValue(true));
    m_recentTabsMenu->addAction(action);
    m_recentTabsMenu->addSeparator();
    m_recentTabsMenu->setEnabled(false);

    KAction* forwardAction = KStandardAction::forward(this, SLOT(goForward()), actionCollection());
    connect(forwardAction, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)), this, SLOT(goForward(Qt::MouseButtons)));

    KAction* upAction = KStandardAction::up(this, SLOT(goUp()), actionCollection());
    connect(upAction, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)), this, SLOT(goUp(Qt::MouseButtons)));

    KStandardAction::home(this, SLOT(goHome()), actionCollection());

    // setup 'Tools' menu
    KToggleAction* showSearchBar = actionCollection()->add<KToggleAction>("show_search_bar");
    showSearchBar->setText(i18nc("@action:inmenu Tools", "Show Search Bar"));
    showSearchBar->setShortcut(Qt::CTRL | Qt::Key_S);
    connect(showSearchBar, SIGNAL(triggered(bool)), this, SLOT(toggleFilterBarVisibility(bool)));

    KToggleAction* showFilterBar = actionCollection()->add<KToggleAction>("show_filter_bar");
    showFilterBar->setText(i18nc("@action:inmenu Tools", "Show Filter Bar"));
    showFilterBar->setIcon(KIcon("view-filter"));
    showFilterBar->setShortcut(Qt::CTRL | Qt::Key_I);
    connect(showFilterBar, SIGNAL(triggered(bool)), this, SLOT(toggleFilterBarVisibility(bool)));

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
    m_showMenuBar = KStandardAction::showMenubar(this, SLOT(toggleShowMenuBar()), actionCollection());
    KStandardAction::preferences(this, SLOT(editSettings()), actionCollection());

    // not in menu actions
    KAction* activateNextTab = actionCollection()->addAction("activate_next_tab");
    activateNextTab->setText(i18nc("@action:inmenu", "Activate Next Tab"));
    connect(activateNextTab, SIGNAL(triggered()), SLOT(activateNextTab()));
    activateNextTab->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabPrev() :
                                                                  KStandardShortcut::tabNext());

    KAction* activatePrevTab = actionCollection()->addAction("activate_prev_tab");
    activatePrevTab->setText(i18nc("@action:inmenu", "Activate Previous Tab"));
    connect(activatePrevTab, SIGNAL(triggered()), SLOT(activatePrevTab()));
    activatePrevTab->setShortcuts(QApplication::isRightToLeft() ? KStandardShortcut::tabNext() :
                                                                  KStandardShortcut::tabPrev());

    // for context menu
    KAction* openInNewTab = actionCollection()->addAction("open_in_new_tab");
    openInNewTab->setText(i18nc("@action:inmenu", "Open in New Tab"));
    openInNewTab->setIcon(KIcon("tab-new"));
    connect(openInNewTab, SIGNAL(triggered()), this, SLOT(openInNewTab()));

    KAction* openInNewWindow = actionCollection()->addAction("open_in_new_window");
    openInNewWindow->setText(i18nc("@action:inmenu", "Open in New Window"));
    openInNewWindow->setIcon(KIcon("window-new"));
    connect(openInNewWindow, SIGNAL(triggered()), this, SLOT(openInNewWindow()));

    // 'Search' toolbar
    m_searchBox = new DolphinSearchBox(this);
    connect(m_searchBox, SIGNAL(search(QString)), this, SLOT(searchItems()));

    KAction* search = new KAction(this);
    actionCollection()->addAction("search_bar", search);
    search->setText(i18nc("@action:inmenu", "Search Bar"));
    search->setDefaultWidget(m_searchBox);
    search->setShortcutConfigurable(false);
}

void DolphinMainWindow::setupDockWidgets()
{
    // setup "Information"
    QDockWidget* infoDock = new QDockWidget(i18nc("@title:window", "Information"));
    infoDock->setObjectName("infoDock");
    infoDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    Panel* infoPanel = new InformationPanel(infoDock);
    connect(infoPanel, SIGNAL(urlActivated(KUrl)), this, SLOT(handleUrl(KUrl)));
    infoDock->setWidget(infoPanel);

    KAction* infoAction = new KAction(this);
    infoAction->setText(i18nc("@title:window", "Information"));
    infoAction->setShortcut(Qt::Key_F11);
    infoAction->setIcon(KIcon("dialog-information"));
    actionCollection()->addAction("show_info_panel", infoAction);
    connect(infoAction, SIGNAL(triggered()), infoDock->toggleViewAction(), SLOT(trigger()));

    addDockWidget(Qt::RightDockWidgetArea, infoDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            infoPanel, SLOT(setUrl(KUrl)));
    connect(this, SIGNAL(selectionChanged(KFileItemList)),
            infoPanel, SLOT(setSelection(KFileItemList)));
    connect(this, SIGNAL(requestItemInfo(KFileItem)),
            infoPanel, SLOT(requestDelayedItemInfo(KFileItem)));

    // setup "Folders"
    QDockWidget* foldersDock = new QDockWidget(i18nc("@title:window", "Folders"));
    foldersDock->setObjectName("foldersDock");
    foldersDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    FoldersPanel* foldersPanel = new FoldersPanel(foldersDock);
    foldersDock->setWidget(foldersPanel);

    KAction* foldersAction = new KAction(this);
    foldersAction->setText(i18nc("@title:window", "Folders"));
    foldersAction->setShortcut(Qt::Key_F7);
    foldersAction->setIcon(KIcon("folder"));
    actionCollection()->addAction("show_folders_panel", foldersAction);
    connect(foldersAction, SIGNAL(triggered()), foldersDock->toggleViewAction(), SLOT(trigger()));

    addDockWidget(Qt::LeftDockWidgetArea, foldersDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            foldersPanel, SLOT(setUrl(KUrl)));
    connect(foldersPanel, SIGNAL(changeUrl(KUrl, Qt::MouseButtons)),
            this, SLOT(handlePlacesClick(KUrl, Qt::MouseButtons)));

    // setup "Terminal"
#ifndef Q_OS_WIN
    QDockWidget* terminalDock = new QDockWidget(i18nc("@title:window Shell terminal", "Terminal"));
    terminalDock->setObjectName("terminalDock");
    terminalDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    Panel* terminalPanel = new TerminalPanel(terminalDock);
    terminalDock->setWidget(terminalPanel);

    connect(terminalPanel, SIGNAL(hideTerminalPanel()), terminalDock, SLOT(hide()));

    KAction* terminalAction = new KAction(this);
    terminalAction->setText(i18nc("@title:window Shell terminal", "Terminal"));
    terminalAction->setShortcut(Qt::Key_F4);
    terminalAction->setIcon(KIcon("utilities-terminal"));
    actionCollection()->addAction("show_terminal_panel", terminalAction);
    connect(terminalAction, SIGNAL(triggered()), terminalDock->toggleViewAction(), SLOT(trigger()));

    addDockWidget(Qt::BottomDockWidgetArea, terminalDock);
    connect(this, SIGNAL(urlChanged(KUrl)),
            terminalPanel, SLOT(setUrl(KUrl)));
#endif

    const bool firstRun = DolphinSettings::instance().generalSettings()->firstRun();
    if (firstRun) {
        foldersDock->hide();
#ifndef Q_OS_WIN
        terminalDock->hide();
#endif
    }

    // setup "Places"
    QDockWidget* placesDock = new QDockWidget(i18nc("@title:window", "Places"));
    placesDock->setObjectName("placesDock");
    placesDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    PlacesPanel* placesPanel = new PlacesPanel(placesDock);
    placesDock->setWidget(placesPanel);
    placesPanel->setModel(DolphinSettings::instance().placesModel());
    placesPanel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    KAction* placesAction = new KAction(this);
    placesAction->setText(i18nc("@title:window", "Places"));
    placesAction->setShortcut(Qt::Key_F9);
    placesAction->setIcon(KIcon("bookmarks"));
    actionCollection()->addAction("show_places_panel", placesAction);
    connect(placesAction, SIGNAL(triggered()), placesDock->toggleViewAction(), SLOT(trigger()));

    addDockWidget(Qt::LeftDockWidgetArea, placesDock);
    connect(placesPanel, SIGNAL(urlChanged(KUrl, Qt::MouseButtons)),
            this, SLOT(handlePlacesClick(KUrl, Qt::MouseButtons)));
    connect(this, SIGNAL(urlChanged(KUrl)),
            placesPanel, SLOT(setUrl(KUrl)));
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
    const KUrl& currentUrl = m_activeViewContainer->url();
    goUpAction->setEnabled(currentUrl.upUrl() != currentUrl);
}

void DolphinMainWindow::rememberClosedTab(int index)
{
    KMenu* tabsMenu = m_recentTabsMenu->menu();

    const QString primaryPath = m_viewTab[index].primaryView->url().path();
    const QString iconName = KMimeType::iconNameForUrl(primaryPath);

    QAction* action = new QAction(squeezedText(primaryPath), tabsMenu);

    ClosedTab closedTab;
    closedTab.primaryUrl = m_viewTab[index].primaryView->url();

    if (m_viewTab[index].secondaryView != 0) {
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
    QSplitter* splitter = m_viewTab[tabIndex].splitter;
    const int newWidth = (m_viewTab[tabIndex].primaryView->width() - splitter->handleWidth()) / 2;

    const DolphinView* view = m_viewTab[tabIndex].primaryView->view();
    m_viewTab[tabIndex].secondaryView = new DolphinViewContainer(this, 0, view->rootUrl());
    splitter->addWidget(m_viewTab[tabIndex].secondaryView);
    splitter->setSizes(QList<int>() << newWidth << newWidth);
    connectViewSignals(m_viewTab[tabIndex].secondaryView);
    m_viewTab[tabIndex].secondaryView->view()->reload();
    m_viewTab[tabIndex].secondaryView->setActive(false);
    m_viewTab[tabIndex].secondaryView->show();
}

QString DolphinMainWindow::tabProperty(const QString& property, int tabIndex) const
{
    return "Tab " + QString::number(tabIndex) + ' ' + property;
}

void DolphinMainWindow::setUrlAsCaption(const KUrl& url)
{
    delete m_captionStatJob;
    m_captionStatJob = 0;

    if (url.protocol() == QLatin1String("file")) {
        QString caption;
        if (url.equals(KUrl("file:///"))) {
            caption = '/';
        } else {
            caption = url.fileName();
            if (caption.isEmpty()) {
                caption = url.protocol();
            }
        }

        setCaption(caption);
    } else {
        m_captionStatJob = KIO::stat(url, KIO::HideProgressInfo);
        connect(m_captionStatJob, SIGNAL(result(KJob*)),
                this, SLOT(slotCaptionStatFinished(KJob*)));
    }
}

void DolphinMainWindow::handleUrl(const KUrl& url)
{
    if (KProtocolManager::supportsListing(url)) {
        activeViewContainer()->setUrl(url);
    } else {
        new KRun(url, this);
    }
}

void DolphinMainWindow::slotCaptionStatFinished(KJob* job)
{  
    m_captionStatJob = 0;
    const KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
    const QString name = entry.stringValue(KIO::UDSEntry::UDS_DISPLAY_NAME);
    setCaption(name);
}

QString DolphinMainWindow::squeezedText(const QString& text) const
{
    const QFontMetrics fm = fontMetrics();
    return fm.elidedText(text, Qt::ElideMiddle, fm.maxWidth() * 10);
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
        DolphinStatusBar* statusBar = mainWin->activeViewContainer()->statusBar();
        statusBar->setMessage(job->errorString(), DolphinStatusBar::Error);
    } else {
        KIO::FileUndoManager::UiInterface::jobError(job);
    }
}

#include "dolphinmainwindow.moc"
