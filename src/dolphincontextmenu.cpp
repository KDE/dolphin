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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "dolphincontextmenu.h"

#include <kactioncollection.h>
#include <kbookmarkmanager.h>
#include <kbookmark.h>
#include <kmimetypetrader.h>
#include <klocale.h>
#include <krun.h>
#include <qdir.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <kiconloader.h>
#include <kaction.h>
#include <kpropertiesdialog.h>
#include <kdesktopfile.h>
#include <assert.h>
#include <kio/netaccess.h>
#include <kmenu.h>
#include <kstdaction.h>

#include "dolphin.h"
#include "dolphinview.h"
#include "editbookmarkdialog.h"
#include "dolphinsettings.h"


DolphinContextMenu::DolphinContextMenu(DolphinView* parent,
                                       KFileItem* fileInfo,
                                       const QPoint& pos) :
   m_dolphinView(parent),
   m_fileInfo(fileInfo),
   m_pos(pos)
{
}

void DolphinContextMenu::open()
{
    if (m_fileInfo == 0) {
        openViewportContextMenu();
    }
    else {
        openItemContextMenu();
    }
}

DolphinContextMenu::~DolphinContextMenu()
{
}

void DolphinContextMenu::openViewportContextMenu()
{
    // Parts of the following code have been taken
    // from the class KonqOperations located in
    // libqonq/konq_operations.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>)

    assert(m_fileInfo == 0);

    KMenu* popup = new KMenu(m_dolphinView);
    Dolphin& dolphin = Dolphin::mainWin();

    // setup 'Create New' menu
    KMenu* createNewMenu = new KMenu();

    KAction* createFolderAction = dolphin.actionCollection()->action("create_folder");
    if (createFolderAction != 0) {
        createFolderAction->plug(createNewMenu);
    }

    createNewMenu->insertSeparator();

    KAction* action = 0;

    Q3PtrListIterator<KAction> fileGrouptIt(dolphin.fileGroupActions());
    while ((action = fileGrouptIt.current()) != 0) {
        action->plug(createNewMenu);
        ++fileGrouptIt;
    }

    // TODO: not used yet. See documentation of Dolphin::linkGroupActions()
    // and Dolphin::linkToDeviceActions() in the header file for details.
    //
    //createNewMenu->insertSeparator();
    //
    //QPtrListIterator<KAction> linkGroupIt(dolphin.linkGroupActions());
    //while ((action = linkGroupIt.current()) != 0) {
    //    action->plug(createNewMenu);
    //    ++linkGroupIt;
    //}
    //
    //KMenu* linkToDeviceMenu = new KMenu();
    //QPtrListIterator<KAction> linkToDeviceIt(dolphin.linkToDeviceActions());
    //while ((action = linkToDeviceIt.current()) != 0) {
    //    action->plug(linkToDeviceMenu);
    //    ++linkToDeviceIt;
    //}
    //
    //createNewMenu->insertItem(i18n("Link to Device"), linkToDeviceMenu);

    popup->insertItem(SmallIcon("filenew"), i18n("Create New"), createNewMenu);
    popup->insertSeparator();

    KAction* pasteAction = dolphin.actionCollection()->action(KStdAction::stdName(KStdAction::Paste));
    pasteAction->plug(popup);

    // setup 'View Mode' menu
    KMenu* viewModeMenu = new KMenu();

    KAction* iconsMode = dolphin.actionCollection()->action("icons");
    iconsMode->plug(viewModeMenu);

    KAction* detailsMode = dolphin.actionCollection()->action("details");
    detailsMode->plug(viewModeMenu);

    KAction* previewsMode = dolphin.actionCollection()->action("previews");
    previewsMode->plug(viewModeMenu);

    popup->insertItem(i18n("View Mode"), viewModeMenu);
    popup->insertSeparator();

    QAction *bookmarkAction = popup->addAction(i18n("Bookmark this folder"));
    popup->insertSeparator();

    QAction *propertiesAction = popup->addAction(i18n("Properties..."));

    QAction *activatedAction = popup->exec(m_pos);
    if (activatedAction == propertiesAction) {
        new KPropertiesDialog(dolphin.activeView()->url());
    }
    else if (activatedAction == bookmarkAction) {
        const KUrl& url = dolphin.activeView()->url();
        KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add folder as bookmark"),
                                                             url.fileName(),
                                                             url,
                                                             "bookmark");
        if (!bookmark.isNull()) {
            KBookmarkManager* manager = DolphinSettings::instance().bookmarkManager();
            KBookmarkGroup root = manager->root();
            root.addBookmark(manager, bookmark);
            manager->emitChanged(root);
        }
    }

    popup->deleteLater();
}

void DolphinContextMenu::openItemContextMenu()
{
    // Parts of the following code have been taken
    // from the class KonqOperations located in
    // libqonq/konq_operations.h of Konqueror.
    // (Copyright (C) 2000  David Faure <faure@kde.org>)

    assert(m_fileInfo != 0);

    KMenu* popup = new KMenu(m_dolphinView);
    Dolphin& dolphin = Dolphin::mainWin();
    const KUrl::List urls = m_dolphinView->selectedURLs();

    // insert 'Cut', 'Copy' and 'Paste'
    const KStdAction::StdAction actionNames[] = { KStdAction::Cut, KStdAction::Copy, KStdAction::Paste };
    const int count = sizeof(actionNames) / sizeof(KStdAction::StdAction);
    for (int i = 0; i < count; ++i) {
        KAction* action = dolphin.actionCollection()->action(KStdAction::stdName(actionNames[i]));
        if (action != 0) {
            action->plug(popup);
        }
    }
    popup->insertSeparator();

    // insert 'Rename'
    KAction* renameAction = dolphin.actionCollection()->action("rename");
    renameAction->plug(popup);

    // insert 'Move to Trash' for local URLs, otherwise insert 'Delete'
    const KUrl& url = dolphin.activeView()->url();
    if (url.isLocalFile()) {
        KAction* moveToTrashAction = dolphin.actionCollection()->action("move_to_trash");
        moveToTrashAction->plug(popup);
    }
    else {
        KAction* deleteAction = dolphin.actionCollection()->action("delete");
        deleteAction->plug(popup);
    }

    // insert 'Bookmark this folder...' entry
    // urls is a list of selected items, so insert boolmark menu if
    // urls contains only one item, i.e. no multiple selection made
    QAction *bookmarkAction = 0;
    if (m_fileInfo->isDir() && (urls.count() == 1)) {
        bookmarkAction = popup->addAction(i18n("Bookmark this folder"));
    }

    popup->insertSeparator();

    // Insert 'Open With...' sub menu
    Q3ValueVector<KService::Ptr> openWithVector;
    const QList<QAction*> openWithActions = insertOpenWithItems(popup, openWithVector);

    // Insert 'Actions' sub menu
    Q3ValueVector<KDEDesktopMimeType::Service> actionsVector;
    const QList<QAction*> serviceActions = insertActionItems(popup, actionsVector);

    // insert 'Properties...' entry
    popup->insertSeparator();
    KAction* propertiesAction = dolphin.actionCollection()->action("properties");
    propertiesAction->plug(popup);

    QAction *activatedAction = popup->exec(m_pos);

    if (bookmarkAction!=0 && activatedAction == bookmarkAction) {
        const KUrl selectedURL(m_fileInfo->url());
        KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add folder as bookmark"),
                                                             selectedURL.fileName(),
                                                             selectedURL,
                                                             "bookmark");
        if (!bookmark.isNull()) {
            KBookmarkManager* manager = DolphinSettings::instance().bookmarkManager();
            KBookmarkGroup root = manager->root();
            root.addBookmark(manager, bookmark);
            manager->emitChanged(root);
        }
    }
    else if (serviceActions.contains(activatedAction)) {
        // one of the 'Actions' items has been selected
        int id = serviceActions.indexOf(activatedAction);
        KDEDesktopMimeType::executeService(urls, actionsVector[id]);
    }
    else if (openWithActions.contains(activatedAction)) {
        // one of the 'Open With' items has been selected
        if (openWithActions.last()==activatedAction) {
            // the item 'Other...' has been selected
            KRun::displayOpenWithDialog(urls, m_dolphinView);
        }
        else {
            int id = openWithActions.indexOf(activatedAction);
            KService::Ptr servicePtr = openWithVector[id];
            KRun::run(*servicePtr, urls, m_dolphinView);
        }
    }

    openWithVector.clear();
    actionsVector.clear();
    popup->deleteLater();
}

QList<QAction*> DolphinContextMenu::insertOpenWithItems(KMenu* popup,
                                                        Q3ValueVector<KService::Ptr>& openWithVector)
{
    // Prepare 'Open With' sub menu. Usually a sub menu is created, where all applications
    // are listed which are registered to open the item. As last entry "Other..." will be
    // attached which allows to select a custom application. If no applications are registered
    // no sub menu is created at all, only "Open With..." will be offered.
    const KFileItemList* list = m_dolphinView->selectedItems();
    assert(list != 0);

    bool insertOpenWithItems = true;
    const QString contextMimeType(m_fileInfo->mimetype());
    QListIterator<KFileItem*> mimeIt(*list);
    while (insertOpenWithItems && mimeIt.hasNext()) {
        KFileItem* item = mimeIt.next();
        insertOpenWithItems = (contextMimeType == item->mimetype());
    }

    QList<QAction*> openWithActions;

    if (insertOpenWithItems) {
        // fill the 'Open with' sub menu with application types
        const KMimeType::Ptr mimePtr = KMimeType::findByUrl(m_fileInfo->url());
        KService::List offers = KMimeTypeTrader::self()->query(mimePtr->name(),
                                                               "Application",
                                                               "Type == 'Application'");
        if (offers.count() > 0) {
            KService::List::Iterator it;
            KMenu* openWithMenu = new KMenu();
            for(it = offers.begin(); it != offers.end(); ++it) {
                // The offer list from the KTrader returns duplicate
                // application entries. Although this seems to be a configuration
                // problem outside the scope of Dolphin, duplicated entries just
                // will be skipped here.
                const QString appName((*it)->name());
                if (!containsEntry(openWithMenu, appName)) {
                    QAction *action = openWithMenu->addAction((*it)->pixmap(K3Icon::Small),
                                                              appName);
                    openWithVector.append(*it);
                    openWithActions << action;
                }
            }

            openWithMenu->insertSeparator();
            QAction *action = openWithMenu->addAction(i18n("&Other..."));
            openWithActions << action;
            popup->insertItem(i18n("Open With"), openWithMenu);
        }
        else {
            // No applications are registered, hence just offer
            // a "Open With..." item instead of a sub menu containing
            // only one entry.
            QAction *action = popup->addAction(i18n("Open With..."));
            openWithActions << action;
        }
    }
    else {
        // At least one of the selected items has a different MIME type. In this case
        // just show a disabled "Open With..." entry.
        QAction *action = popup->addAction(i18n("Open With..."));
        action->setEnabled(false);
    }

    return openWithActions;
}

QList<QAction*> DolphinContextMenu::insertActionItems(KMenu* popup,
                                                      Q3ValueVector<KDEDesktopMimeType::Service>& actionsVector)
{
    KMenu* actionsMenu = new KMenu();

    QList<QAction*> serviceActions;

    QStringList dirs = KGlobal::dirs()->findDirs("data", "dolphin/servicemenus/");

    KMenu* menu = 0;
    for (QStringList::ConstIterator dirIt = dirs.begin(); dirIt != dirs.end(); ++dirIt) {
        QDir dir(*dirIt);
        QStringList entries = dir.entryList("*.desktop", QDir::Files);

        for (QStringList::ConstIterator entryIt = entries.begin(); entryIt != entries.end(); ++entryIt) {
            KSimpleConfig cfg(*dirIt + *entryIt, true);
            cfg.setDesktopGroup();
            if ((cfg.hasKey("Actions") || cfg.hasKey("X-KDE-GetActionMenu")) && cfg.hasKey("ServiceTypes")) {
                const QStringList types = cfg.readListEntry("ServiceTypes");
                for (QStringList::ConstIterator it = types.begin(); it != types.end(); ++it) {
                    // check whether the mime type is equal or whether the
                    // mimegroup (e. g. image/*) is supported

                    bool insert = false;
                    if ((*it) == "all/allfiles") {
                        // The service type is valid for all files, but not for directories.
                        // Check whether the selected items only consist of files...
                        const KFileItemList* list = m_dolphinView->selectedItems();
                        assert(list != 0);

                        QListIterator<KFileItem*> mimeIt(*list);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            insert = !item->isDir();
                        }
                    }

                    if (!insert) {
                        // Check whether the MIME types of all selected files match
                        // to the mimetype of the service action. As soon as one MIME
                        // type does not match, no service menu is shown at all.
                        const KFileItemList* list = m_dolphinView->selectedItems();
                        assert(list != 0);

                        QListIterator<KFileItem*> mimeIt(*list);
                        insert = true;
                        while (insert && mimeIt.hasNext()) {
                            KFileItem* item = mimeIt.next();
                            const QString mimeType(item->mimetype());
                            const QString mimeGroup(mimeType.left(mimeType.find('/')));

                            insert  = (*it == mimeType) ||
                                      ((*it).right(1) == "*") &&
                                      ((*it).left((*it).find('/')) == mimeGroup);
                        }
                    }

                    if (insert) {
                        menu = actionsMenu;

                        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                        if (!submenuName.isEmpty()) {
                            menu = new KMenu();
                            actionsMenu->insertItem(submenuName, menu, submenuID);
                        }

                        Q3ValueList<KDEDesktopMimeType::Service> userServices =
                            KDEDesktopMimeType::userDefinedServices(*dirIt + *entryIt, true);

                        Q3ValueList<KDEDesktopMimeType::Service>::Iterator serviceIt;
                        for (serviceIt = userServices.begin(); serviceIt != userServices.end(); ++serviceIt) {
                            KDEDesktopMimeType::Service service = (*serviceIt);
                            if (!service.m_strIcon.isEmpty()) {
                                QAction *action = menu->addAction(SmallIcon(service.m_strIcon),
                                                                  service.m_strName);
                                serviceActions << action;
                            }
                            else {
                                QAction *action = menu->addAction(service.m_strName);
                                serviceActions << action;
                            }
                            actionsVector.append(service);
                        }
                    }
                }
            }
        }
    }

    const int itemsCount = actionsMenu->count();
    if (itemsCount == 0) {
        // no actions are available at all, hence show the "Actions"
        // submenu disabled
        actionsMenu->setEnabled(false);
    }

    if (itemsCount == 1) {
        // Exactly one item is available. Instead of showing a sub menu with
        // only one item, show the item directly in the root menu.
        if (menu == actionsMenu) {
            // The item is an action, hence show the action in the root menu.
            const int id = actionsMenu->idAt(0);
            const QString text(actionsMenu->text(id));
            QIcon iconSet = actionsMenu->iconSet(id);
            if (iconSet.isNull()) {
                QAction *action = popup->addAction(text);
                serviceActions.clear();
                serviceActions << action;
            }
            else {
                QAction *action = popup->addAction(iconSet, text);
                serviceActions.clear();
                serviceActions << action;
            }
        }
        else {
            // The item is a sub menu, hence show the sub menu in the root menu.
            popup->insertItem(actionsMenu->text(submenuID), menu);
        }
        actionsMenu->deleteLater();
        actionsMenu = 0;
    }
    else {
        popup->insertItem(i18n("Actions"), actionsMenu);
    }

    return serviceActions;
}

bool DolphinContextMenu::containsEntry(const KMenu* menu,
                                       const QString& entryName) const
{
    assert(menu != 0);

    const uint count = menu->count();
    for (uint i = 0; i < count; ++i) {
        const int id = menu->idAt(i);
        if (menu->text(id) == entryName) {
            return true;
        }
    }

    return false;
}
