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
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"
#include "dolphinmodel.h"

#include <konq_operations.h>

#include <kactioncollection.h>
#include <kdirlister.h>
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
            this, SLOT(updateViewActions()));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(updatePasteAction()));

    createActions();
    updateViewActions();
    slotSelectionChanged(KFileItemList()); // initially disable selection-dependent actions

    // TODO provide the viewmode actions in the menu, merged with the existing view-mode-actions somehow
    // [Q_PROPERTY introspection?]

    // TODO sort_by_* actions
    // TODO show_*_info actions

    // TODO connect to urlsDropped

    // TODO there was a "always open a new window" (when clicking on a directory) setting in konqueror
    // (sort of spacial navigation)

    // TODO MMB-click should do something like KonqDirPart::mmbClicked
}

DolphinPart::~DolphinPart()
{
    delete m_dirLister;
}

void DolphinPart::createActions()
{
    QActionGroup* viewModeActions = new QActionGroup(this);
    viewModeActions->addAction(DolphinView::iconsModeAction(actionCollection()));
    viewModeActions->addAction(DolphinView::detailsModeAction(actionCollection()));
    viewModeActions->addAction(DolphinView::columnsModeAction(actionCollection()));
    connect(viewModeActions, SIGNAL(triggered(QAction*)), this, SLOT(slotViewModeActionTriggered(QAction*)));

    KAction* renameAction = DolphinView::createRenameAction(actionCollection());
    connect(renameAction, SIGNAL(triggered()), m_view, SLOT(renameSelectedItems()));

    KAction* moveToTrashAction = DolphinView::createMoveToTrashAction(actionCollection());
    connect(moveToTrashAction, SIGNAL(triggered(Qt::MouseButtons, Qt::KeyboardModifiers)),
            this, SLOT(slotTrashActivated(Qt::MouseButtons, Qt::KeyboardModifiers)));

    KAction* deleteAction = DolphinView::createDeleteAction(actionCollection());
    connect(deleteAction, SIGNAL(triggered()), m_view, SLOT(deleteSelectedItems()));

    // This action doesn't appear in the GUI, it's for the shortcut only.
    // KNewMenu takes care of the GUI stuff.
    KAction* newDirAction = actionCollection()->addAction( "create_dir" );
    newDirAction->setText( i18n("Create Folder..." ) );
    connect(newDirAction, SIGNAL(triggered()), SLOT(slotNewDir()));
    newDirAction->setShortcut(Qt::Key_F10);
    widget()->addAction(newDirAction);
}

void DolphinPart::slotSelectionChanged(const KFileItemList& selection)
{
    // Yes, DolphinMainWindow has very similar code :/
    const bool hasSelection = !selection.isEmpty();
    if (!hasSelection) {
        stateChanged("has_no_selection");
    } else {
        stateChanged("has_selection");
    }

    QStringList actions;
    actions << "rename" << "move_to_trash" << "delete";
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

void DolphinPart::updateViewActions()
{
    QAction* action = actionCollection()->action(m_view->currentViewModeActionName());
    if (action != 0) {
        KToggleAction* toggleAction = static_cast<KToggleAction*>(action);
        toggleAction->setChecked(true);
    }
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
    m_view->setUrl(url);
    if (reload)
        m_view->reload();
    emit started(0); // get the wheel to spin
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
    qDebug() << QApplication::mouseButtons();
    if (QApplication::mouseButtons() & Qt::MidButton) {
        qDebug() << "MMB!!" << item.mimetype();
        if (item.mimeTypePtr()->is("inode/directory")) {
            KParts::OpenUrlArguments args;
            args.setMimeType( item.mimetype() );
            emit m_extension->createNewWindow( item.url(), args );
        } else {
            qDebug() << "run()";
            item.run();
        }
    } else {
        // Left button. [Right button goes to slotOpenContextMenu before triggered can be emitted]
        qDebug() << "LMB";
        emit m_extension->openUrlRequest(item.url());
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

        KFileItemList items; items.append(item);
        emit m_extension->popupMenu(QCursor::pos(),
                                    items,
                                    KParts::OpenUrlArguments(),
                                    KParts::BrowserArguments(),
                                    popupFlags,
                                    actionGroups);
    }
}

void DolphinPart::slotViewModeActionTriggered(QAction* action)
{
    const DolphinView::Mode mode = action->data().value<DolphinView::Mode>();
    m_view->setMode(mode);
}

void DolphinPart::slotUrlChanged(const KUrl& url)
{
    if (m_view->url() != url) {
        // If the view URL is not equal to 'url', then an inner URL change has
        // been done (e. g. by activating an existing column in the column view).
        // From the hosts point of view this must be handled like changing the URL.
        emit m_extension->openUrlRequest(url);
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

void DolphinPart::slotTrashActivated(Qt::MouseButtons, Qt::KeyboardModifiers modifiers)
{
    // Note: kde3's konq_mainwindow.cpp used to check
    // reason == KAction::PopupMenuActivation && ...
    // but this isn't supported anymore
    if (modifiers & Qt::ShiftModifier)
        m_view->deleteSelectedItems();
    else
        m_view->trashSelectedItems();
}

void DolphinPart::slotNewDir()
{
    KonqOperations::newDir(widget(), url());
}

#include "dolphinpart.moc"
