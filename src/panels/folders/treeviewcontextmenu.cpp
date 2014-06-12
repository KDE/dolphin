/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "treeviewcontextmenu.h"

#include <KFileItem>
#include <KIconLoader>
#include <KIO/DeleteJob>
#include <KMenu>
#include <KIcon>
#include <KSharedConfig>
#include <KConfigGroup>
#include <konqmimedata.h>
#include <KFileItemListProperties>
#include <konq_operations.h>
#include <KLocale>
#include <KPropertiesDialog>

#include "folderspanel.h"

#include <QApplication>
#include <QClipboard>
#include <QMimeData>

TreeViewContextMenu::TreeViewContextMenu(FoldersPanel* parent,
                                         const KFileItem& fileInfo) :
    QObject(parent),
    m_parent(parent),
    m_fileItem(fileInfo)
{
}

TreeViewContextMenu::~TreeViewContextMenu()
{
}

void TreeViewContextMenu::open()
{
    KMenu* popup = new KMenu(m_parent);

    if (!m_fileItem.isNull()) {
        KFileItemListProperties capabilities(KFileItemList() << m_fileItem);

        // insert 'Cut', 'Copy' and 'Paste'
        QAction* cutAction = new QAction(KIcon("edit-cut"), i18nc("@action:inmenu", "Cut"), this);
        cutAction->setEnabled(capabilities.supportsMoving());
        connect(cutAction, &QAction::triggered, this, &TreeViewContextMenu::cut);

        QAction* copyAction = new QAction(KIcon("edit-copy"), i18nc("@action:inmenu", "Copy"), this);
        connect(copyAction, &QAction::triggered, this, &TreeViewContextMenu::copy);

        const QPair<bool, QString> pasteInfo = KonqOperations::pasteInfo(m_fileItem.url());
        QAction* pasteAction = new QAction(KIcon("edit-paste"), pasteInfo.second, this);
        connect(pasteAction, &QAction::triggered, this, &TreeViewContextMenu::paste);
        pasteAction->setEnabled(pasteInfo.first);

        popup->addAction(cutAction);
        popup->addAction(copyAction);
        popup->addAction(pasteAction);
        popup->addSeparator();

        // insert 'Rename'
        QAction* renameAction = new QAction(i18nc("@action:inmenu", "Rename..."), this);
        renameAction->setEnabled(capabilities.supportsMoving());
        renameAction->setIcon(KIcon("edit-rename"));
        connect(renameAction, &QAction::triggered, this, &TreeViewContextMenu::rename);
        popup->addAction(renameAction);

        // insert 'Move to Trash' and (optionally) 'Delete'
        KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::IncludeGlobals);
        KConfigGroup configGroup(globalConfig, "KDE");
        bool showDeleteCommand = configGroup.readEntry("ShowDeleteCommand", false);

        const KUrl url = m_fileItem.url();
        if (url.isLocalFile()) {
            QAction* moveToTrashAction = new QAction(KIcon("user-trash"),
                                                    i18nc("@action:inmenu", "Move to Trash"), this);
            const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();
            moveToTrashAction->setEnabled(enableMoveToTrash);
            connect(moveToTrashAction, &QAction::triggered, this, &TreeViewContextMenu::moveToTrash);
            popup->addAction(moveToTrashAction);
        } else {
            showDeleteCommand = true;
        }

        if (showDeleteCommand) {
            QAction* deleteAction = new QAction(KIcon("edit-delete"), i18nc("@action:inmenu", "Delete"), this);
            deleteAction->setEnabled(capabilities.supportsDeleting());
            connect(deleteAction, &QAction::triggered, this, &TreeViewContextMenu::deleteItem);
            popup->addAction(deleteAction);
        }

        popup->addSeparator();
    }

    // insert 'Show Hidden Files'
    QAction* showHiddenFilesAction = new QAction(i18nc("@action:inmenu", "Show Hidden Files"), this);
    showHiddenFilesAction->setCheckable(true);
    showHiddenFilesAction->setChecked(m_parent->showHiddenFiles());
    popup->addAction(showHiddenFilesAction);
    connect(showHiddenFilesAction, &QAction::toggled, this, &TreeViewContextMenu::setShowHiddenFiles);

    // insert 'Automatic Scrolling'
    QAction* autoScrollingAction = new QAction(i18nc("@action:inmenu", "Automatic Scrolling"), this);
    autoScrollingAction->setCheckable(true);
    autoScrollingAction->setChecked(m_parent->autoScrolling());
    // TODO: Temporary disabled. Horizontal autoscrolling will be implemented later either
    // in KItemViews or manually as part of the FoldersPanel
    //popup->addAction(autoScrollingAction);
    connect(autoScrollingAction, &QAction::toggled, this, &TreeViewContextMenu::setAutoScrolling);

    if (!m_fileItem.isNull()) {
        // insert 'Properties' entry
        QAction* propertiesAction = new QAction(i18nc("@action:inmenu", "Properties"), this);
        propertiesAction->setIcon(KIcon("document-properties"));
        connect(propertiesAction, &QAction::triggered, this, &TreeViewContextMenu::showProperties);
        popup->addAction(propertiesAction);
    }

    QList<QAction*> customActions = m_parent->customContextMenuActions();
    if (!customActions.isEmpty()) {
        popup->addSeparator();
        foreach (QAction* action, customActions) {
            popup->addAction(action);
        }
    }

    QWeakPointer<KMenu> popupPtr = popup;
    popup->exec(QCursor::pos());
    if (popupPtr.data()) {
        popupPtr.data()->deleteLater();
    }
}

void TreeViewContextMenu::populateMimeData(QMimeData* mimeData, bool cut)
{
    KUrl::List kdeUrls;
    kdeUrls.append(m_fileItem.url());
    KUrl::List mostLocalUrls;
    bool dummy;
    mostLocalUrls.append(m_fileItem.mostLocalUrl(dummy));
    KonqMimeData::populateMimeData(mimeData, kdeUrls, mostLocalUrls, cut);
}

void TreeViewContextMenu::cut()
{
    QMimeData* mimeData = new QMimeData();
    populateMimeData(mimeData, true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::copy()
{
    QMimeData* mimeData = new QMimeData();
    populateMimeData(mimeData, false);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::paste()
{
    KonqOperations::doPaste(m_parent, m_fileItem.url());
}

void TreeViewContextMenu::rename()
{
    m_parent->rename(m_fileItem);
}

void TreeViewContextMenu::moveToTrash()
{
    KonqOperations::del(m_parent, KonqOperations::TRASH, KUrl::List() << m_fileItem.url());
}

void TreeViewContextMenu::deleteItem()
{
    KonqOperations::del(m_parent, KonqOperations::DEL, KUrl::List() << m_fileItem.url());
}

void TreeViewContextMenu::showProperties()
{
    KPropertiesDialog* dialog = new KPropertiesDialog(m_fileItem.url(), m_parent);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void TreeViewContextMenu::setShowHiddenFiles(bool show)
{
    m_parent->setShowHiddenFiles(show);
}

void TreeViewContextMenu::setAutoScrolling(bool enable)
{
    m_parent->setAutoScrolling(enable);
}

#include "treeviewcontextmenu.moc"
