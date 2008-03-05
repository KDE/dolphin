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

#include <konq_operations.h>

#include <kpropertiesdialog.h>
#include <kglobalsettings.h>
#include <kactioncollection.h>
#include <kdirlister.h>
#include <kiconloader.h>
#include <kmessagebox.h>
#include <kparts/genericfactory.h>
#include <ktoggleaction.h>
#include <kconfiggroup.h>

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>

typedef KParts::GenericFactory<DolphinPart> DolphinPartFactory;
K_EXPORT_COMPONENT_FACTORY(dolphinpart, DolphinPartFactory)

DolphinPart::DolphinPart(QWidget* parentWidget, QObject* parent, const QStringList& args)
    : KParts::ReadOnlyPart(parent)
{
    Q_UNUSED(args)
    setComponentData( DolphinPartFactory::componentData() );
    m_extension = new DolphinPartBrowserExtension(this);

    // make sure that other apps using this part find Dolphin's view-file-columns icons
    KIconLoader::global()->addAppDir("dolphin");

    m_dirLister = new KDirLister;
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(parentWidget->topLevelWidget());
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
    setWidget(m_view);

    setXMLFile("dolphinpart.rc");

    connect(m_view, SIGNAL(infoMessage(QString)),
            this, SLOT(slotInfoMessage(QString)));
    connect(m_view, SIGNAL(errorMessage(QString)),
            this, SLOT(slotErrorMessage(QString)));
    connect(m_view, SIGNAL(itemTriggered(KFileItem)),
            this, SLOT(slotItemTriggered(KFileItem)));
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
    if (!hasSelection) {
        stateChanged("has_no_selection");
    } else {
        stateChanged("has_selection");
    }

    QStringList actions;
    actions << "rename" << "move_to_trash" << "delete" << "editMimeType" << "properties";
    foreach(const QString& actionName, actions) {
        QAction* action = actionCollection()->action(actionName);
        Q_ASSERT(action);
        if (action) {
            action->setEnabled(hasSelection);
        }
    }

    emit m_extension->enableAction("cut", hasSelection);
    emit m_extension->enableAction("copy", hasSelection);
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
    const bool reload = arguments().reload();
    if (m_view->url() == url && !reload) { // DolphinView won't do anything in that case, so don't emit started
        return true;
    }
    setUrl(url); // remember it at the KParts level
    const QString prettyUrl = url.pathOrUrl();
    emit setWindowCaption(prettyUrl);
    emit m_extension->setLocationBarUrl(prettyUrl);
    emit started(0); // get the wheel to spin
    m_view->setUrl(url);
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

    // MMB click support.
    // TODO: this doesn't work, mouseButtons() is always 0.
    // Issue N176832 for the missing QAIV signal; task 177399
    kDebug() << QApplication::mouseButtons();
    if (QApplication::mouseButtons() & Qt::MidButton) {
        kDebug() << "MMB!!" << item.mimetype();
        if (item.mimeTypePtr()->is("inode/directory")) {
            emit m_extension->createNewWindow(item.url(), args);
        } else {
            kDebug() << "run()";
            item.run();
        }
    } else {
        // Left button. [Right button goes to slotOpenContextMenu before triggered can be emitted]
        kDebug() << "LMB";
        emit m_extension->openUrlRequest(item.url(), args, browserArgs);
    }
}

void DolphinPart::slotOpenContextMenu(const KFileItem& _item, const KUrl&)
{
    KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems
                                                      | KParts::BrowserExtension::ShowProperties
                                                      | KParts::BrowserExtension::ShowUrlOperations;
    // TODO KonqKfmIconView had if ( !rootItem->isWritable() )
    //            popupFlags |= KParts::BrowserExtension::NoDeletion;

    KFileItem item(_item);

    if (item.isNull()) { // viewport context menu
        popupFlags |= KParts::BrowserExtension::ShowNavigationItems | KParts::BrowserExtension::ShowUp;
        // TODO get m_dirLister->rootItem if possible. or via kdirmodel?
        // and use this as fallback:
        item = KFileItem( S_IFDIR, (mode_t)-1, url() );
    }

    KParts::BrowserExtension::ActionGroupMap actionGroups;
    QList<QAction *> editActions;

    if (!item.isNull()) { // only for context menu on one or more items
        // TODO if ( sMoving )
        editActions.append(actionCollection()->action("rename"));

        bool addTrash = false;
        bool addDel = false;

        // TODO if ( sMoving && !isIntoTrash && !isTrashLink )
        addTrash = true;

        /* TODO if ( sDeleting ) */ {
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
}

void DolphinPart::slotUrlChanged(const KUrl& url)
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

#include "dolphinpart.moc"
