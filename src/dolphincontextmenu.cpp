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
#include <ktrader.h>
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
#include <ksortablevaluelist.h>
#include <kio/netaccess.h>

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
    const int propertiesID = 100;
	const int bookmarkID = 101;

    KPopupMenu* popup = new KPopupMenu(m_dolphinView);
    Dolphin& dolphin = Dolphin::mainWin();

    // setup 'Create New' menu
    KPopupMenu* createNewMenu = new KPopupMenu();

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
    //KPopupMenu* linkToDeviceMenu = new KPopupMenu();
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
    KPopupMenu* viewModeMenu = new KPopupMenu();

    KAction* iconsMode = dolphin.actionCollection()->action("icons");
    iconsMode->plug(viewModeMenu);

    KAction* detailsMode = dolphin.actionCollection()->action("details");
    detailsMode->plug(viewModeMenu);

    KAction* previewsMode = dolphin.actionCollection()->action("previews");
    previewsMode->plug(viewModeMenu);

    popup->insertItem(i18n("View Mode"), viewModeMenu);
    popup->insertSeparator();

    popup->insertItem(i18n("Bookmark this folder"), bookmarkID);
    popup->insertSeparator();

    popup->insertItem(i18n("Properties..."), propertiesID);

    int id = popup->exec(m_pos);
    if (id == propertiesID) {
        new KPropertiesDialog(dolphin.activeView()->url());
    }
    else if (id == bookmarkID) {
        const KURL& url = dolphin.activeView()->url();
        KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add folder as bookmark"),
                                                             url.filename(),
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

    KPopupMenu* popup = new KPopupMenu(m_dolphinView);
    Dolphin& dolphin = Dolphin::mainWin();
    const KURL::List urls = m_dolphinView->selectedURLs();

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
    const KURL& url = dolphin.activeView()->url();
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
    if (m_fileInfo->isDir() && (urls.count() == 1)) {
        popup->insertItem(i18n("Bookmark this folder"), bookmarkID);
    }

    popup->insertSeparator();

    // Insert 'Open With...' sub menu
    Q3ValueVector<KService::Ptr> openWithVector;
    const int openWithID = insertOpenWithItems(popup, openWithVector);

    // Insert 'Actions' sub menu
    Q3ValueVector<KDEDesktopMimeType::Service> actionsVector;
    insertActionItems(popup, actionsVector);

    // insert 'Properties...' entry
    popup->insertSeparator();
    KAction* propertiesAction = dolphin.actionCollection()->action("properties");
    propertiesAction->plug(popup);

    int id = popup->exec(m_pos);

    if (id == bookmarkID) {
        const KURL selectedURL(m_fileInfo->url());
        KBookmark bookmark = EditBookmarkDialog::getBookmark(i18n("Add folder as bookmark"),
                                                             selectedURL.filename(),
                                                             selectedURL,
                                                             "bookmark");
        if (!bookmark.isNull()) {
            KBookmarkManager* manager = DolphinSettings::instance().bookmarkManager();
            KBookmarkGroup root = manager->root();
            root.addBookmark(manager, bookmark);
            manager->emitChanged(root);
        }
    }
    else if (id >= actionsIDStart) {
        // one of the 'Actions' items has been selected
        KDEDesktopMimeType::executeService(urls, actionsVector[id - actionsIDStart]);
    }
    else if (id >= openWithIDStart) {
        // one of the 'Open With' items has been selected
        if (id == openWithID) {
            // the item 'Other...' has been selected
            KRun::displayOpenWithDialog(urls);
        }
        else {
            KService::Ptr servicePtr = openWithVector[id - openWithIDStart];
            KRun::run(*servicePtr, urls);
        }
    }

    openWithVector.clear();
    actionsVector.clear();
    popup->deleteLater();
}

int DolphinContextMenu::insertOpenWithItems(KPopupMenu* popup,
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
    KFileItemListIterator mimeIt(*list);
    KFileItem* item = 0;
    while (insertOpenWithItems && ((item = mimeIt.current()) != 0)) {
        insertOpenWithItems = (contextMimeType == item->mimetype());
        ++mimeIt;
    }

    int openWithID = -1;

    if (insertOpenWithItems) {
        // fill the 'Open with' sub menu with application types
        const KMimeType::Ptr mimePtr = KMimeType::findByURL(m_fileInfo->url());
        KTrader::OfferList offers = KTrader::self()->query(mimePtr->name(),
                                                           "Type == 'Application'");
        int index = openWithIDStart;
        if (offers.count() > 0) {
            KTrader::OfferList::Iterator it;
            KPopupMenu* openWithMenu = new KPopupMenu();
            for(it = offers.begin(); it != offers.end(); ++it) {
                // The offer list from the KTrader returns duplicate
                // application entries. Although this seems to be a configuration
                // problem outside the scope of Dolphin, duplicated entries just
                // will be skipped here.
                const QString appName((*it)->name());
                if (!containsEntry(openWithMenu, appName)) {
                    openWithMenu->insertItem((*it)->pixmap(KIcon::Small),
                                            appName, index);
                    openWithVector.append(*it);
                    ++index;
                }
            }

            openWithMenu->insertSeparator();
            openWithMenu->insertItem(i18n("&Other..."), index);
            popup->insertItem(i18n("Open With"), openWithMenu);
        }
        else {
            // No applications are registered, hence just offer
            // a "Open With..." item instead of a sub menu containing
            // only one entry.
            popup->insertItem(i18n("Open With..."), openWithIDStart);
        }
        openWithID = index;
    }
    else {
        // At least one of the selected items has a different MIME type. In this case
        // just show a disabled "Open With..." entry.
        popup->insertItem(i18n("Open With..."), openWithIDStart);
        popup->setItemEnabled(openWithIDStart, false);
    }

    popup->setItemEnabled(openWithID, insertOpenWithItems);

    return openWithID;
}

void DolphinContextMenu::insertActionItems(KPopupMenu* popup,
                                           Q3ValueVector<KDEDesktopMimeType::Service>& actionsVector)
{
    KPopupMenu* actionsMenu = new KPopupMenu();

    int actionsIndex = 0;

    QStringList dirs = KGlobal::dirs()->findDirs("data", "dolphin/servicemenus/");

    KPopupMenu* menu = 0;
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

                        KFileItemListIterator mimeIt(*list);
                        KFileItem* item = 0;
                        insert = true;
                        while (insert && ((item = mimeIt.current()) != 0)) {
                            insert = !item->isDir();
                            ++mimeIt;
                        }
                    }

                    if (!insert) {
                        // Check whether the MIME types of all selected files match
                        // to the mimetype of the service action. As soon as one MIME
                        // type does not match, no service menu is shown at all.
                        const KFileItemList* list = m_dolphinView->selectedItems();
                        assert(list != 0);

                        KFileItemListIterator mimeIt(*list);
                        KFileItem* item = 0;
                        insert = true;
                        while (insert && ((item = mimeIt.current()) != 0)) {
                            const QString mimeType((*mimeIt)->mimetype());
                            const QString mimeGroup(mimeType.left(mimeType.find('/')));

                            insert  = (*it == mimeType) ||
                                      ((*it).right(1) == "*") &&
                                      ((*it).left((*it).find('/')) == mimeGroup);
                            ++mimeIt;
                        }
                    }

                    if (insert) {
                        menu = actionsMenu;

                        const QString submenuName = cfg.readEntry( "X-KDE-Submenu" );
                        if (!submenuName.isEmpty()) {
                            menu = new KPopupMenu();
                            actionsMenu->insertItem(submenuName, menu, submenuID);
                        }

                        Q3ValueList<KDEDesktopMimeType::Service> userServices =
                            KDEDesktopMimeType::userDefinedServices(*dirIt + *entryIt, true);

                        Q3ValueList<KDEDesktopMimeType::Service>::Iterator serviceIt;
                        for (serviceIt = userServices.begin(); serviceIt != userServices.end(); ++serviceIt) {
                            KDEDesktopMimeType::Service service = (*serviceIt);
                            if (!service.m_strIcon.isEmpty()) {
                                menu->insertItem(SmallIcon(service.m_strIcon),
                                                 service.m_strName,
                                                 actionsIDStart + actionsIndex);
                            }
                            else {
                                menu->insertItem(service.m_strName,
                                                 actionsIDStart + actionsIndex);
                            }
                            actionsVector.append(service);
                            ++actionsIndex;
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
            const QIcon* iconSet = actionsMenu->iconSet(id);
            if (iconSet == 0) {
                popup->insertItem(text, id);
            }
            else {
                popup->insertItem(*iconSet, text, id);
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
}

bool DolphinContextMenu::containsEntry(const KPopupMenu* menu,
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
