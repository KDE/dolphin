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

#include <kiconloader.h>
#include <kio/deletejob.h>
#include <kmenu.h>
#include <konqmimedata.h>
#include <konq_operations.h>
#include <klocale.h>
#include <kpropertiesdialog.h>

#include "renamedialog.h"

#include <QApplication>
#include <QClipboard>

TreeViewContextMenu::TreeViewContextMenu(QWidget* parent,
                                         KFileItem* fileInfo) :
    m_parent(parent),
    m_fileInfo(fileInfo)
{
}

TreeViewContextMenu::~TreeViewContextMenu()
{
}

void TreeViewContextMenu::open()
{
    Q_ASSERT(m_fileInfo != 0);

    KMenu* popup = new KMenu(m_parent);

    // insert 'Cut', 'Copy' and 'Paste'
    QAction* cutAction   = new QAction(KIcon("edit-cut"), i18n("Cut"), this);
    connect(cutAction, SIGNAL(triggered()), this, SLOT(cut()));

    QAction* copyAction  = new QAction(KIcon("edit-copy"), i18n("Copy"), this);
    connect(copyAction, SIGNAL(triggered()), this, SLOT(copy()));

    QAction* pasteAction = new QAction(KIcon("edit-paste"), i18n("Paste"), this);
    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    const KUrl::List pasteData = KUrl::List::fromMimeData(mimeData);
    pasteAction->setEnabled(!pasteData.isEmpty());
    connect(pasteAction, SIGNAL(triggered()), this, SLOT(paste()));

    popup->addAction(cutAction);
    popup->addAction(copyAction);
    popup->addAction(pasteAction);
    popup->addSeparator();

    // insert 'Rename'
    QAction* renameAction = new QAction(i18n("Rename..."), this);
    connect(renameAction, SIGNAL(triggered()), this, SLOT(rename()));
    popup->addAction(renameAction);

    // insert 'Move to Trash' and (optionally) 'Delete'
    const KSharedConfig::Ptr globalConfig = KSharedConfig::openConfig("kdeglobals", KConfig::NoGlobals);
    const KConfigGroup kdeConfig(globalConfig, "KDE");
    bool showDeleteCommand = kdeConfig.readEntry("ShowDeleteCommand", false);
    const KUrl& url = m_fileInfo->url();
    if (url.isLocalFile()) {
        QAction* moveToTrashAction = new QAction(KIcon("edit-trash"), i18n("Move To Trash"), this);
        connect(moveToTrashAction, SIGNAL(triggered()), this, SLOT(moveToTrash()));
        popup->addAction(moveToTrashAction);
    } else {
        showDeleteCommand = true;
    }

    if (showDeleteCommand) {
        QAction* deleteAction = new QAction(KIcon("edit-delete"), i18n("Delete"), this);
        connect(deleteAction, SIGNAL(triggered()), this, SLOT(deleteItem()));
        popup->addAction(deleteAction);
    }

    popup->addSeparator();

    // insert 'Properties' entry
    QAction* propertiesAction = new QAction(i18n("Properties"), this);
    connect(this, SIGNAL(triggered()), this, SLOT(showProperties()));
    popup->addAction(propertiesAction);

    popup->exec(QCursor::pos());
    popup->deleteLater();
}

void TreeViewContextMenu::cut()
{
    QMimeData* mimeData = new QMimeData();
    KUrl::List kdeUrls;
    kdeUrls.append(m_fileInfo->url());
    KonqMimeData::populateMimeData(mimeData, kdeUrls, KUrl::List(), true);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::copy()
{
    QMimeData* mimeData = new QMimeData();
    KUrl::List kdeUrls;
    kdeUrls.append(m_fileInfo->url());
    KonqMimeData::populateMimeData(mimeData, kdeUrls, KUrl::List(), false);
    QApplication::clipboard()->setMimeData(mimeData);
}

void TreeViewContextMenu::paste()
{
    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* mimeData = clipboard->mimeData();

    const KUrl::List source = KUrl::List::fromMimeData(mimeData);
    const KUrl& dest = m_fileInfo->url();
    if (KonqMimeData::decodeIsCutSelection(mimeData)) {
        KonqOperations::copy(m_parent, KonqOperations::MOVE, source, dest);
        clipboard->clear();
    } else {
        KonqOperations::copy(m_parent, KonqOperations::COPY, source, dest);
    }
}

void TreeViewContextMenu::rename()
{
    const KUrl& oldUrl = m_fileInfo->url();
    RenameDialog dialog(oldUrl);
    if (dialog.exec() == QDialog::Accepted) {
        const QString& newName = dialog.newName();
        if (!newName.isEmpty()) {
            KUrl newUrl = oldUrl;
            newUrl.setFileName(newName);
            KonqOperations::rename(m_parent, oldUrl, newUrl);
        }
    }
}

void TreeViewContextMenu::moveToTrash()
{
    KonqOperations::del(m_parent, KonqOperations::TRASH, m_fileInfo->url());
}

void TreeViewContextMenu::deleteItem()
{
    KonqOperations::del(m_parent, KonqOperations::DEL, m_fileInfo->url());
}

void TreeViewContextMenu::showProperties()
{
    new KPropertiesDialog(m_fileInfo->url());
}

#include "treeviewcontextmenu.moc"
