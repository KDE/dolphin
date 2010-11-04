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

#include <kfileitemlistproperties.h>
#include <konq_operations.h>

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <ktoggleaction.h>
#include <kio/netaccess.h>
#include <ktoolinvocation.h>
#include <kauthorized.h>
#include <knewfilemenu.h>
#include <kmenu.h>
#include <kinputdialog.h>
#include <kprotocolinfo.h>

#include "settings/dolphinsettings.h"
#include "views/dolphinview.h"
#include "views/dolphinviewactionhandler.h"
#include "views/dolphinsortfilterproxymodel.h"
#include "views/dolphinmodel.h"
#include "views/dolphinnewfilemenuobserver.h"
#include "views/dolphinremoteencoding.h"
#include "views/dolphindirlister.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>

K_PLUGIN_FACTORY(DolphinPartFactory, registerPlugin<DolphinPart>();)
K_EXPORT_PLUGIN(DolphinPartFactory("dolphinpart", "dolphin"))

DolphinPart::DolphinPart(QWidget* parentWidget, QObject* parent, const QVariantList& args)
    : KParts::ReadOnlyPart(parent)
{
    Q_UNUSED(args)
    setComponentData(DolphinPartFactory::componentData(), false);
    m_extension = new DolphinPartBrowserExtension(this);
    new DolphinPartFileInfoExtension(this);

    // make sure that other apps using this part find Dolphin's view-file-columns icons
    KIconLoader::global()->addAppDir("dolphin");

    m_dirLister = new DolphinDirLister;
    m_dirLister->setAutoUpdate(true);
    if (parentWidget) {
        m_dirLister->setMainWindow(parentWidget->window());
    }
    m_dirLister->setDelayedMimeTypes(true);

    connect(m_dirLister, SIGNAL(completed(KUrl)), this, SLOT(slotCompleted(KUrl)));
    connect(m_dirLister, SIGNAL(canceled(KUrl)), this, SLOT(slotCanceled(KUrl)));
    connect(m_dirLister, SIGNAL(percent(int)), this, SLOT(updateProgress(int)));
    connect(m_dirLister, SIGNAL(errorMessage(QString)), this, SLOT(slotErrorMessage(QString)));

    m_dolphinModel = new DolphinModel(this);
    m_dolphinModel->setDirLister(m_dirLister); // m_dolphinModel takes ownership of m_dirLister

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dolphinModel);

    m_view = new DolphinView(parentWidget, KUrl(), m_proxyModel);
    m_view->setTabsForFilesEnabled(true);
    setWidget(m_view);

    setXMLFile("dolphinpart.rc");

    connect(m_view, SIGNAL(infoMessage(QString)),
            this, SLOT(slotMessage(QString)));
    connect(m_view, SIGNAL(operationCompletedMessage(QString)),
            this, SLOT(slotMessage(QString)));
    connect(m_view, SIGNAL(errorMessage(QString)),
            this, SLOT(slotErrorMessage(QString)));
    connect(m_view, SIGNAL(itemTriggered(KFileItem)),
            this, SLOT(slotItemTriggered(KFileItem)));
    connect(m_view, SIGNAL(tabRequested(KUrl)),
            this, SLOT(createNewWindow(KUrl)));
    connect(m_view, SIGNAL(requestContextMenu(KFileItem,KUrl,QList<QAction*>)),
            this, SLOT(slotOpenContextMenu(KFileItem,KUrl,QList<QAction*>)));
    connect(m_view, SIGNAL(selectionChanged(KFileItemList)),
            m_extension, SIGNAL(selectionInfo(KFileItemList)));
    connect(m_view, SIGNAL(selectionChanged(KFileItemList)),
            this, SLOT(slotSelectionChanged(KFileItemList)));
    connect(m_view, SIGNAL(requestItemInfo(KFileItem)),
            this, SLOT(slotRequestItemInfo(KFileItem)));
    connect(m_view, SIGNAL(modeChanged()),
            this, SIGNAL(viewModeChanged())); // relay signal
    connect(m_view, SIGNAL(redirection(KUrl, KUrl)),
            this, SLOT(slotRedirection(KUrl, KUrl)));

    // Watch for changes that should result in updates to the
    // status bar text.
    connect(m_dirLister, SIGNAL(itemsDeleted(const KFileItemList&)),
            this, SLOT(updateStatusBar()));
    connect(m_dirLister, SIGNAL(clear()),
            this, SLOT(updateStatusBar()));
    connect(m_view,  SIGNAL(selectionChanged(const KFileItemList)),
            this, SLOT(updateStatusBar()));

    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    m_actionHandler->setCurrentView(m_view);
    connect(m_actionHandler, SIGNAL(createDirectory()), SLOT(createDirectory()));

    m_remoteEncoding = new DolphinRemoteEncoding(this, m_actionHandler);
    connect(this, SIGNAL(aboutToOpenURL()),
            m_remoteEncoding, SLOT(slotAboutToOpenUrl()));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));

    createActions();
    m_actionHandler->updateViewActions();
    slotSelectionChanged(KFileItemList()); // initially disable selection-dependent actions

    // TODO there was a "always open a new window" (when clicking on a directory) setting in konqueror
    // (sort of spacial navigation)

    loadPlugins(this, this, componentData());

}

DolphinPart::~DolphinPart()
{
    DolphinSettings::instance().save();
    DolphinNewFileMenuObserver::instance().detach(m_newFileMenu);
}

void DolphinPart::createActions()
{
    // Edit menu

    m_newFileMenu = new KNewFileMenu(actionCollection(), "new_menu", this);
    m_newFileMenu->setParentWidget(widget());
    DolphinNewFileMenuObserver::instance().attach(m_newFileMenu);
    connect(m_newFileMenu->menu(), SIGNAL(aboutToShow()),
            this, SLOT(updateNewMenu()));

    KAction *editMimeTypeAction = actionCollection()->addAction( "editMimeType" );
    editMimeTypeAction->setText( i18nc("@action:inmenu Edit", "&Edit File Type..." ) );
    connect(editMimeTypeAction, SIGNAL(triggered()), SLOT(slotEditMimeType()));

    KAction* selectItemsMatching = actionCollection()->addAction("select_items_matching");
    selectItemsMatching->setText(i18nc("@action:inmenu Edit", "Select Items Matching..."));
    selectItemsMatching->setShortcut(Qt::CTRL | Qt::Key_S);
    connect(selectItemsMatching, SIGNAL(triggered()), this, SLOT(slotSelectItemsMatchingPattern()));

    KAction* unselectItemsMatching = actionCollection()->addAction("unselect_items_matching");
    unselectItemsMatching->setText(i18nc("@action:inmenu Edit", "Unselect Items Matching..."));
    connect(unselectItemsMatching, SIGNAL(triggered()), this, SLOT(slotUnselectItemsMatchingPattern()));

    actionCollection()->addAction(KStandardAction::SelectAll, "select_all", m_view, SLOT(selectAll()));

    KAction* unselectAll = actionCollection()->addAction("unselect_all");
    unselectAll->setText(i18nc("@action:inmenu Edit", "Unselect All"));
    connect(unselectAll, SIGNAL(triggered()), m_view, SLOT(clearSelection()));

    KAction* invertSelection = actionCollection()->addAction("invert_selection");
    invertSelection->setText(i18nc("@action:inmenu Edit", "Invert Selection"));
    invertSelection->setShortcut(Qt::CTRL | Qt::SHIFT | Qt::Key_A);
    connect(invertSelection, SIGNAL(triggered()), m_view, SLOT(invertSelection()));

    // View menu: all done by DolphinViewActionHandler

    // Go menu

    QActionGroup* goActionGroup = new QActionGroup(this);
    connect(goActionGroup, SIGNAL(triggered(QAction*)),
            this, SLOT(slotGoTriggered(QAction*)));

    createGoAction("go_applications", "start-here-kde",
                   i18nc("@action:inmenu Go", "App&lications"), QString("programs:/"),
                   goActionGroup);
    createGoAction("go_network_folders", "folder-remote",
                   i18nc("@action:inmenu Go", "&Network Folders"), QString("remote:/"),
                   goActionGroup);
    createGoAction("go_settings", "preferences-system",
                   i18nc("@action:inmenu Go", "Sett&ings"), QString("settings:/"),
                   goActionGroup);
    createGoAction("go_trash", "user-trash",
                   i18nc("@action:inmenu Go", "Trash"), QString("trash:/"),
                   goActionGroup);
    createGoAction("go_autostart", "",
                   i18nc("@action:inmenu Go", "Autostart"), KGlobalSettings::autostartPath(),
                   goActionGroup);

    // Tools menu
    if (KAuthorized::authorizeKAction("shell_access")) {
        KAction* action = actionCollection()->addAction("open_terminal");
        action->setIcon(KIcon("utilities-terminal"));
        action->setText(i18nc("@action:inmenu Tools", "Open &Terminal"));
        connect(action, SIGNAL(triggered()), SLOT(slotOpenTerminal()));
        action->setShortcut(Qt::Key_F4);
    }

}

void DolphinPart::createGoAction(const char* name, const char* iconName,
                                 const QString& text, const QString& url,
                                 QActionGroup* actionGroup)
{
    KAction* action = actionCollection()->addAction(name);
    action->setIcon(KIcon(iconName));
    action->setText(text);
    action->setData(url);
    action->setActionGroup(actionGroup);
}

void DolphinPart::slotGoTriggered(QAction* action)
{
    const QString url = action->data().toString();
    emit m_extension->openUrlRequest(KUrl(url));
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
    return new KAboutData("dolphinpart", "dolphin", ki18nc("@title", "Dolphin Part"), "0.1");
}

bool DolphinPart::openUrl(const KUrl& url)
{
    bool reload = arguments().reload();
    // A bit of a workaround so that changing the namefilter works: force reload.
    // Otherwise DolphinView wouldn't relist the URL, so nothing would happen.
    if (m_nameFilter != m_dirLister->nameFilter())
        reload = true;
    if (m_view->url() == url && !reload) { // DolphinView won't do anything in that case, so don't emit started
        return true;
    }
    setUrl(url); // remember it at the KParts level
    KUrl visibleUrl(url);
    if (!m_nameFilter.isEmpty()) {
        visibleUrl.addPath(m_nameFilter);
    }
    QString prettyUrl = visibleUrl.pathOrUrl();
    emit setWindowCaption(prettyUrl);
    emit m_extension->setLocationBarUrl(prettyUrl);
    emit started(0); // get the wheel to spin
    m_dirLister->setNameFilter(m_nameFilter);
    m_view->setUrl(url);
    updatePasteAction();
    emit aboutToOpenURL();
    if (reload)
        m_view->reload();
    return true;
}

void DolphinPart::slotCompleted(const KUrl& url)
{
    Q_UNUSED(url)
    emit completed();
}

void DolphinPart::slotCanceled(const KUrl& url)
{
    slotCompleted(url);
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
        ReadOnlyPart::setStatusBarText(item.getStatusBarInfo());
    }
}

void DolphinPart::slotItemTriggered(const KFileItem& item)
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

void DolphinPart::createNewWindow(const KUrl& url)
{
    // TODO: Check issue N176832 for the missing QAIV signal; task 177399 - maybe this code
    // should be moved into DolphinPart::slotItemTriggered()
    emit m_extension->createNewWindow(url);
}

void DolphinPart::slotOpenContextMenu(const KFileItem& _item,
                                      const KUrl&,
                                      const QList<QAction*>& customActions)
{
    KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems
                                                      | KParts::BrowserExtension::ShowProperties
                                                      | KParts::BrowserExtension::ShowUrlOperations;

    KFileItem item(_item);

    if (item.isNull()) { // viewport context menu
        popupFlags |= KParts::BrowserExtension::ShowNavigationItems | KParts::BrowserExtension::ShowUp;
        item = m_dirLister->rootItem();
        if (item.isNull())
            item = KFileItem( S_IFDIR, (mode_t)-1, url() );
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
        bool supportsDeleting = capabilities.supportsDeleting();
        bool supportsMoving = capabilities.supportsMoving();

        if (!supportsDeleting) {
            popupFlags |= KParts::BrowserExtension::NoDeletion;
        }

        if (supportsMoving) {
            editActions.append(actionCollection()->action("rename"));
        }

        bool addTrash = capabilities.isLocal() && supportsMoving;
        bool addDel = false;
        if (supportsDeleting) {
            if ( !item.isLocalFile() )
                addDel = true;
            else if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                addTrash = false;
                addDel = true;
            }
            else {
                KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::IncludeGlobals);
                KConfigGroup configGroup(globalConfig, "KDE");
                addDel = configGroup.readEntry("ShowDeleteCommand", false);
            }
        }

        if (addTrash)
            editActions.append(actionCollection()->action("move_to_trash"));
        if (addDel)
            editActions.append(actionCollection()->action("delete"));

        // Normally KonqPopupMenu only shows the "Create new" submenu in the current view
        // since otherwise the created file would not be visible.
        // But in treeview mode we should allow it.
        if (m_view->itemsExpandable())
            popupFlags |= KParts::BrowserExtension::ShowCreateDirectory;

    }

    actionGroups.insert("editactions", editActions);

    emit m_extension->popupMenu(QCursor::pos(),
                                items,
                                KParts::OpenUrlArguments(),
                                KParts::BrowserArguments(),
                                popupFlags,
                                actionGroups);
}

void DolphinPart::slotRedirection(const KUrl& oldUrl, const KUrl& newUrl)
{
    //kDebug() << oldUrl << newUrl << "currentUrl=" << url();
    if (oldUrl.equals(url(), KUrl::CompareWithoutTrailingSlash /* #207572 */)) {
        KParts::ReadOnlyPart::setUrl(newUrl);
        const QString prettyUrl = newUrl.pathOrUrl();
        emit m_extension->setLocationBarUrl(prettyUrl);
    }
}


void DolphinPart::slotEditMimeType()
{
    const KFileItemList items = m_view->selectedItems();
    if (!items.isEmpty()) {
        KonqOperations::editMimeType(items.first().mimetype(), m_view);
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
        m_view->setItemSelectionEnabled(patternRegExp, selectItems);
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

    KUrl u(url());

    // If the given directory is not local, it can still be the URL of an
    // ioslave using UDS_LOCAL_PATH which to be converted first.
    u = KIO::NetAccess::mostLocalUrl(u, widget());

    //If the URL is local after the above conversion, set the directory.
    if (u.isLocalFile()) {
        dir = u.toLocalFile();
    }

    KToolInvocation::invokeTerminal(QString(), dir);
}

void DolphinPart::updateNewMenu()
{
    // As requested by KNewFileMenu :
    m_newFileMenu->checkUpToDate();
    m_newFileMenu->setViewShowsHiddenFiles(m_view->showHiddenFiles());
    // And set the files that the menu apply on :
    m_newFileMenu->setPopupFiles(url());
}

void DolphinPart::updateStatusBar()
{
    emit ReadOnlyPart::setStatusBarText(m_view->statusBarText());
}

void DolphinPart::updateProgress(int percent)
{
    m_extension->loadingProgress(percent);
}

void DolphinPart::createDirectory()
{
    m_newFileMenu->setViewShowsHiddenFiles(m_view->showHiddenFiles());
    m_newFileMenu->setPopupFiles(url());
    m_newFileMenu->createDirectory();
}

void DolphinPart::setFilesToSelect(const KUrl::List& files)
{
    m_view->markUrlsAsSelected(files);
}

////

void DolphinPartBrowserExtension::restoreState(QDataStream &stream)
{
    KParts::BrowserExtension::restoreState(stream);
    m_part->view()->restoreState(stream);
}

void DolphinPartBrowserExtension::saveState(QDataStream &stream)
{
    KParts::BrowserExtension::saveState(stream);
    m_part->view()->saveState(stream);
}

void DolphinPartBrowserExtension::cut()
{
    m_part->view()->cutSelectedItems();
}

void DolphinPartBrowserExtension::copy()
{
    m_part->view()->copySelectedItems();
}

void DolphinPartBrowserExtension::paste()
{
    m_part->view()->paste();
}

void DolphinPartBrowserExtension::pasteTo(const KUrl&)
{
    m_part->view()->pasteIntoFolder();
}

void DolphinPartBrowserExtension::reparseConfiguration()
{
    m_part->view()->refresh();
}

////

DolphinPartFileInfoExtension::DolphinPartFileInfoExtension(DolphinPart* part)
                             : KParts::FileInfoExtension(part)
{
}

DolphinPart* DolphinPartFileInfoExtension::part() const
{
    return static_cast<DolphinPart*>(parent());
}

bool DolphinPartFileInfoExtension::hasSelection() const
{
    return part()->view()->hasSelection();
}

KParts::FileInfoExtension::QueryModes DolphinPartFileInfoExtension::supportedQueryModes() const
{
    return (KParts::FileInfoExtension::AllItems | KParts::FileInfoExtension::SelectedItems);
}

KFileItemList DolphinPartFileInfoExtension::queryFor(KParts::FileInfoExtension::QueryMode mode) const
{
    KFileItemList list;

    if (mode == KParts::FileInfoExtension::None)
      return list;

    if (!(supportedQueryModes() & mode))
      return list;

    switch (mode) {
      case KParts::FileInfoExtension::SelectedItems:
          if (hasSelection())
              return part()->view()->selectedItems();
          break;
      case KParts::FileInfoExtension::AllItems:
          return part()->view()->items();
      default:
          break;
    }

    return list;
}

#include "dolphinpart.moc"
