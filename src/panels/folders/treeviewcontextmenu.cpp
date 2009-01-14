/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   Cvetoslav Ludmiloff                                                   *
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

#include "dolphin_folderspanelsettings.h"

#include <kfileitem.h>
#include <kiconloader.h>
#include <kio/deletejob.h>
#include <kmenu.h>
#include <konqmimedata.h>
#include <konq_fileitemcapabilities.h>
#include <konq_operations.h>
#include <klocale.h>
#include <kpropertiesdialog.h>

#include "treeviewsidebarpage.h"

#include <QtGui/QApplication>
#include <QtGui/QClipboard>

TreeViewContextMenu::TreeViewContextMenu(TreeViewSidebarPage* parent,
                                         const KFileItem& fileInfo) :
    QObject(parent),
    m_parent(parent),
    m_fileInfo(fileInfo)
{
}

TreeViewContextMenu::~TreeViewContextMenu()
{
}

void TreeViewContextMenu::open()
{
    KMenu* popup = new KMenu(m_parent);

    if (!m_fileInfo.isNull()) {
        KonqFileItemCapabilities capabilities(KFileItemList() << m_fileInfo);

        // insert 'Cut', 'Copy' and 'Paste'
        QAction* cutAction = new QAction(KIcon("edit-cut"), i18nc("@action:inmenu", "Cut"), this);
        cutAction->setEnabled(capabilities.supportsMoving());
        connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));

        QAction* copyAction = new QAction(KIcon("edit-copy"), i18nc("@action:inmenu", "Copy"), this);
        connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

        QAction* pasteAction = new QAction(KIcon("edit-paste"), i18nc("@action:inmenu", "Paste"), this);
        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        const KUrl::List pasteData = KUrl::List::fromMimeData(mimeData);
        connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));
        pasteAction->setEnabled(!pasteData.isEmpty() && capabilities.supportsWriting());

        popup->addAction(cutAction);
        popup->addAction(copyAction);
        popup->addAction(pasteAction);
        popup->addSeparator();

        // insert 'Rename'
        QAction* renameAction = new QAction(i18nc("@action:inmenu", "Rename..."), this);
        renameAction->setEnabled(capabilities.supportsMoving());
        connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));
        popup->addAction(renameAction);

        // insert 'Move to Trash' and (optionally) 'Delete'
        KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::IncludeGlobals);
        KConfigGroup configGroup(globalConfig, "KDE");
        bool showDeleteCommand = configGroup.readEntry("ShowDeleteCommand", false);

        const KUrl& url = m_fileInfo.url();
        if (url.isLocalFile()) {
            QAction* moveToTrashAction = new QAction(KIcon("user-trash"),
                                                    i18nc("@action:inmenu", "Move To Trash"), this);
            const bool enableMoveToTrash = capabilities.isLocal() && capabilities.supportsMoving();
            moveToTrashAction->setEnabled(enableMoveToTrash);
            connect(moveToTrashAction, SIGNAL(triggered()), this, SLOT(moveToTrash()));
            popup->addAction(moveToTrashAction);
        } else {
            showDeleteCommand = true;
        }

        if (showDeleteCommand) {
            QAction* deleteAction = new QAction(KIcon("edit-delete"), i18nc("@action:inmenu", "Delete"), this);
            deleteAction->setEnabled(capabilities.supportsDeleting());
            connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteItem()));
            popup->addAction(deleteAction);
        }

        popup->addSeparator();

        // insert 'Properties' entry
        QAction* propertiesAction = new QAction(i18nc("@action:inmenu", "Properties"), this);
        connect(propertiesAction, SIGNAL(triggered()), this, SLOT(showProperties()));
        popup->addAction(propertiesAction);

        popup->addSeparator();
    }

    QAction* showHiddenFilesAction = new QAction(i18nc("@action:inmenu", "Show Hidden Files"), this);
    showHiddenFilesAction->setCheckable(true);
    showHiddenFilesAction->setChecked(FoldersPanelSettings::showHiddenFiles());
    popup->addAction(showHiddenFilesAction);

    connect(showHiddenFilesAction, SIGNAL(toggled(bool)), this, SLOT(setShowHiddenFiles(bool)));

    popup->exec(QCursor::pos());
    popup->deleteLater();
}

void TreeViewContextMenu::populateMimeData(QMimeData* mimeData, bool cut)
{
    KUrl::List kdeUrls;
    kdeUrls.append(m_fileInfo.url());
    KUrl::List mostLocalUrls;
    bool dummy;
    mostLocalUrls.append(m_fileInfo.mostLocalUrl(dummy));
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
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    const KUrl::List source = KUrl::List::fromMimeData(mimeData);
    const KUrl& dest = m_fileInfo.url();
    if (KonqMimeData::decodeIsCutSelection(mimeData)) {
        KonqOperations::copy(m_parent, KonqOperations::MOVE, source, dest);
        clipboard->clear();
    } else {
        KonqOperations::copy(m_parent, KonqOperations::COPY, source, dest);
    }
}

void TreeViewContextMenu::rename()
{
    m_parent->rename(m_fileInfo);
}

void TreeViewContextMenu::moveToTrash()
{
    KonqOperations::del(m_parent, KonqOperations::TRASH, m_fileInfo.url());
}

void TreeViewContextMenu::deleteItem()
{
    KonqOperations::del(m_parent, KonqOperations::DEL, m_fileInfo.url());
}

void TreeViewContextMenu::showProperties()
{
    KPropertiesDialog dialog(m_fileInfo.url(), m_parent);
    dialog.exec();
}

void TreeViewContextMenu::setShowHiddenFiles(bool show)
{
    m_parent->setShowHiddenFiles(show);
}

#include "treeviewcontextmenu.moc"
