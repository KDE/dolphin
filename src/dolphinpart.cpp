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
#include "dolphinviewactionhandler.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "dolphinmodel.h"

#include <konq_fileitemcapabilities.h>
#include <konq_operations.h>

#include <kaboutdata.h>
#include <kactioncollection.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobalsettings.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpluginfactory.h>
#include <kpropertiesdialog.h>
#include <ktoggleaction.h>
#include <kio/netaccess.h>
#include <kauthorized.h>
#include <kshell.h>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>

K_PLUGIN_FACTORY(DolphinPartFactory, registerPlugin<DolphinPart>();)
// The componentdata name must be dolphinpart so that dolphinpart.rc is found
// Alternatively we would have to install it as dolphin/dolphinpart.rc
K_EXPORT_PLUGIN(DolphinPartFactory("dolphinpart"))

DolphinPart::DolphinPart(QWidget* parentWidget, QObject* parent, const QVariantList& args)
    : KParts::ReadOnlyPart(parent)
{
    Q_UNUSED(args)
    setComponentData(DolphinPartFactory::componentData(), false);
    m_extension = new DolphinPartBrowserExtension(this);

    // make sure that other apps using this part find Dolphin's view-file-columns icons
    KIconLoader::global()->addAppDir("dolphin");

    m_dirLister = new KDirLister;
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(parentWidget->window());
    m_dirLister->setDelayedMimeTypes(true);

    //connect(m_dirLister, SIGNAL(started(KUrl)), this, SLOT(slotStarted()));
    connect(m_dirLister, SIGNAL(completed(KUrl)), this, SLOT(slotCompleted(KUrl)));
    connect(m_dirLister, SIGNAL(canceled(KUrl)), this, SLOT(slotCanceled(KUrl)));

    m_dolphinModel = new DolphinModel(this);
    m_dolphinModel->setDirLister(m_dirLister);

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dolphinModel);

    m_view = new DolphinView(parentWidget,
                             KUrl(),
                             m_dirLister,
                             m_dolphinModel,
                             m_proxyModel);
    m_view->setTabsForFilesEnabled(true);
    setWidget(m_view);

    setXMLFile("dolphinpart.rc");

    connect(m_view, SIGNAL(infoMessage(QString)),
            this, SLOT(slotInfoMessage(QString)));
    connect(m_view, SIGNAL(errorMessage(QString)),
            this, SLOT(slotErrorMessage(QString)));
    connect(m_view, SIGNAL(itemTriggered(KFileItem)),
            this, SLOT(slotItemTriggered(KFileItem)));
    connect(m_view, SIGNAL(tabRequested(KUrl)),
            this, SLOT(createNewWindow(KUrl)));
    connect(m_view, SIGNAL(requestContextMenu(KFileItem,KUrl)),
            this, SLOT(slotOpenContextMenu(KFileItem,KUrl)));
    connect(m_view, SIGNAL(selectionChanged(KFileItemList)),
            m_extension, SIGNAL(selectionInfo(KFileItemList)));
    connect(m_view, SIGNAL(selectionChanged(KFileItemList)),
            this, SLOT(slotSelectionChanged(KFileItemList)));
    connect(m_view, SIGNAL(requestItemInfo(KFileItem)),
            this, SLOT(slotRequestItemInfo(KFileItem)));
    connect(m_view, SIGNAL(urlChanged(KUrl)),
            this, SLOT(slotUrlChanged(KUrl)));
    connect(m_view, SIGNAL(requestUrlChange(KUrl)),
            this, SLOT(slotRequestUrlChange(KUrl)));
    connect(m_view, SIGNAL(modeChanged()),
            this, SIGNAL(viewModeChanged())); // relay signal

    m_actionHandler = new DolphinViewActionHandler(actionCollection(), this);
    m_actionHandler->setCurrentView(m_view);

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));

    createActions();
    m_actionHandler->updateViewActions();
    slotSelectionChanged(KFileItemList()); // initially disable selection-dependent actions

    // TODO sort_by_* actions

    // TODO there was a "always open a new window" (when clicking on a directory) setting in konqueror
    // (sort of spacial navigation)

    loadPlugins(this, this, componentData());
}

DolphinPart::~DolphinPart()
{
    delete m_dirLister;
}

void DolphinPart::createActions()
{
    KAction *editMimeTypeAction = actionCollection()->addAction( "editMimeType" );
    editMimeTypeAction->setText( i18nc("@action:inmenu Edit", "&Edit File Type..." ) );
    connect(editMimeTypeAction, SIGNAL(triggered()), SLOT(slotEditMimeType()));

    KAction *propertiesAction = actionCollection()->addAction( "properties" );
    propertiesAction->setText( i18nc("@action:inmenu Edit", "Properties") );
    propertiesAction->setShortcut(Qt::ALT+Qt::Key_Return);
    connect(propertiesAction, SIGNAL(triggered()), SLOT(slotProperties()));

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

    if (!hasSelection) {
        stateChanged("has_no_selection");

        emit m_extension->enableAction("cut", false);
        emit m_extension->enableAction("copy", false);
        renameAction->setEnabled(false);
        moveToTrashAction->setEnabled(false);
        deleteAction->setEnabled(false);
        editMimeTypeAction->setEnabled(false);
        propertiesAction->setEnabled(false);
    } else {
        stateChanged("has_selection");

        KonqFileItemCapabilities capabilities(selection);
        const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();

        renameAction->setEnabled(capabilities.supportsMoving());
        moveToTrashAction->setEnabled(enableMoveToTrash);
        deleteAction->setEnabled(capabilities.supportsDeleting());
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

void DolphinPart::slotInfoMessage(const QString& msg)
{
    emit setStatusBarText(msg);
}

void DolphinPart::slotErrorMessage(const QString& msg)
{
    KMessageBox::error(m_view, msg);
}

void DolphinPart::slotRequestItemInfo(const KFileItem& item)
{
    emit m_extension->mouseOverInfo(item);
}

void DolphinPart::slotItemTriggered(const KFileItem& item)
{
    KParts::OpenUrlArguments args;
    args.setMimeType(item.mimetype());

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
    KFileItem item(S_IFDIR, (mode_t)-1, url);
    KParts::OpenUrlArguments args;
    args.setMimeType(item.mimetype());
    emit m_extension->createNewWindow(url, args);
}

void DolphinPart::slotOpenContextMenu(const KFileItem& _item, const KUrl&)
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
    }

    KParts::BrowserExtension::ActionGroupMap actionGroups;
    QList<QAction *> editActions;

    if (!_item.isNull()) { // only for context menu on one or more items
        bool sDeleting = true;
        bool sMoving = true;

        // If the parent directory of the selected item is writable, moving
        // and deleting are possible.
        KFileItem parentDir = m_dirLister->rootItem();
        if (!parentDir.isWritable()) {
            popupFlags |= KParts::BrowserExtension::NoDeletion;
            sDeleting = false;
            sMoving = false;
        }

        if ( sMoving )
            editActions.append(actionCollection()->action("rename"));

        bool addTrash = false;
        bool addDel = false;

        bool isIntoTrash = _item.url().protocol() == "trash";

        if ( sMoving && !isIntoTrash && item.isLocalFile() )
            addTrash = true;

        if ( sDeleting ) {
            if ( !item.isLocalFile() )
                addDel = true;
            else if (QApplication::keyboardModifiers() & Qt::ShiftModifier) {
                addTrash = false;
                addDel = true;
            }
            else {
                KConfigGroup configGroup( KGlobal::config(), "KDE" );
                if ( configGroup.readEntry( "ShowDeleteCommand", false) )
                    addDel = true;
            }
        }

        if (addTrash)
            editActions.append(actionCollection()->action("move_to_trash"));
        if (addDel)
            editActions.append(actionCollection()->action("delete"));
        actionGroups.insert("editactions", editActions);
    }

    // TODO: We should change the signature of the slots (and signals) for being able
    //       to tell for which items we want a popup.
    KFileItemList items = (m_view->selectedItems().count() ? m_view->selectedItems()
                           : KFileItemList() << item);
    emit m_extension->popupMenu(QCursor::pos(),
                                items,
                                KParts::OpenUrlArguments(),
                                KParts::BrowserArguments(),
                                popupFlags,
                                actionGroups);
}

void DolphinPart::slotUrlChanged(const KUrl& url)
{
    QString prettyUrl = url.pathOrUrl();
    emit m_extension->setLocationBarUrl(prettyUrl);
}

void DolphinPart::slotRequestUrlChange(const KUrl& url)
{
    if (m_view->url() != url) {
        // If the view URL is not equal to 'url', then an inner URL change has
        // been done (e. g. by activating an existing column in the column view).
        openUrl(url);
        emit m_extension->openUrlNotify();
    }
}

////

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

void DolphinPartBrowserExtension::reparseConfiguration()
{
    m_part->view()->refresh();
}

////

void DolphinPart::slotEditMimeType()
{
    const KFileItemList items = m_view->selectedItems();
    if (!items.isEmpty()) {
        KonqOperations::editMimeType(items.first().mimetype(), m_view);
    }
}

void DolphinPart::slotProperties()
{
    const KFileItemList items = m_view->selectedItems();
    if (!items.isEmpty()) {
        KPropertiesDialog dialog(items.first().url(), m_view);
        dialog.exec();
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
    KConfigGroup confGroup(KGlobal::config(), "General"); // set by componentchooser kcm
    const QString preferredTerminal = confGroup.readPathEntry("TerminalApplication", "konsole");

    QString dir(QDir::homePath());

    KUrl u(url());

    // If the given directory is not local, it can still be the URL of an
    // ioslave using UDS_LOCAL_PATH which to be converted first.
    u = KIO::NetAccess::mostLocalUrl(u, widget());

    //If the URL is local after the above conversion, set the directory.
    if (u.isLocalFile()) {
        dir = u.path();
    }

    // Compensate for terminal having arguments.
    QStringList args = KShell::splitArgs(preferredTerminal);
    if (args.isEmpty()) {
        return;
    }
    const QString prog = args.takeFirst();
    if (prog == "konsole") {
        args << "--workdir";
        args << dir;
    }
    QProcess::startDetached(prog, args);
}

#include "dolphinpart.moc"
