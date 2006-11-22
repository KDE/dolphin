/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#include "itemeffectsmanager.h"
#include <kiconeffect.h>
#include <kapplication.h>
#include <qobject.h>
//Added by qt3to4:
#include <QPixmap>
#include <kglobalsettings.h>
#include <qclipboard.h>
#include <klocale.h>

#include "dolphin.h"
#include "dolphinstatusbar.h"

ItemEffectsManager::ItemEffectsManager()
{
    m_pixmapCopy = new QPixmap();
}

ItemEffectsManager::~ItemEffectsManager()
{
    delete m_pixmapCopy;
    m_pixmapCopy = 0;

    m_highlightedUrl = 0;
}

void ItemEffectsManager::zoomIn()
{
    Dolphin::mainWin().refreshViews();
}

void ItemEffectsManager::zoomOut()
{
    Dolphin::mainWin().refreshViews();
}

void ItemEffectsManager::activateItem(void* context)
{
    KFileItem* fileInfo = contextFileInfo(context);
    const KUrl itemUrl(fileInfo->url());
    if (m_highlightedUrl == itemUrl) {
        // the item is already highlighted
        return;
    }

    resetActivatedItem();

    const QPixmap* itemPixmap = contextPixmap(context);
    if (itemPixmap != 0) {
        // remember the pixmap and item to be able to
        // restore it to the old state later
        *m_pixmapCopy = *itemPixmap;
        m_highlightedUrl = itemUrl;

        // apply an icon effect to the item below the mouse pointer
        KIconEffect iconEffect;
        QPixmap pixmap = iconEffect.apply(*itemPixmap,
                                          K3Icon::Desktop,
                                          K3Icon::ActiveState);
        setContextPixmap(context, pixmap);
    }

    if (!Dolphin::mainWin().activeView()->hasSelection()) {
        DolphinStatusBar* statusBar = Dolphin::mainWin().activeView()->statusBar();
        statusBar->setMessage(statusBarText(fileInfo), DolphinStatusBar::Default);
    }
}

void ItemEffectsManager::resetActivatedItem()
{
    if (m_highlightedUrl.isEmpty()) {
        return;
    }

    for (void* context = firstContext(); context != 0; context = nextContext(context)) {
        KUrl itemUrl(contextFileInfo(context)->url());
        if (itemUrl == m_highlightedUrl) {
            // the highlighted item has been found and is restored to the default state
            KIconEffect iconEffect;
            QPixmap pixmap = iconEffect.apply(*m_pixmapCopy,
                                              K3Icon::Desktop,
                                              K3Icon::DefaultState);

            // TODO: KFileIconView does not emit any signal when the preview has been finished.
            // Hence check the size to prevent that a preview is hidden by restoring a
            // non-preview pixmap.
            const QPixmap* highlightedPixmap = contextPixmap(context);
            const bool restore = (pixmap.width() == highlightedPixmap->width()) &&
                                 (pixmap.height() == highlightedPixmap->height());
            if (restore) {
                setContextPixmap(context, pixmap);
            }
            break;
        }
    }

    m_highlightedUrl = 0;

    DolphinStatusBar* statusBar = Dolphin::mainWin().activeView()->statusBar();
    statusBar->clear();
}

void ItemEffectsManager::updateDisabledItems()
{
    if (!m_disabledItems.isEmpty()) {
        // restore all disabled items with their original pixmap
        for (void* context = firstContext(); context != 0; context = nextContext(context)) {
            const KFileItem* fileInfo = contextFileInfo(context);
            const KUrl& fileUrl = fileInfo->url();
            Q3ValueListIterator<DisabledItem> it = m_disabledItems.begin();
            while (it != m_disabledItems.end()) {
                if (fileUrl == (*it).url) {
                    setContextPixmap(context, (*it).pixmap);
                }
                ++it;
            }
        }
        m_disabledItems.clear();
    }

    if (!Dolphin::mainWin().clipboardContainsCutData()) {
        return;
    }

    QClipboard* clipboard = QApplication::clipboard();
    const QMimeData* data = clipboard->mimeData();
    KUrl::List urls = KUrl::List::fromMimeData(data);
    if (urls.isEmpty()) {
        return;
    }

    // The clipboard contains items, which have been cutted. Change the pixmaps of all those
    // items to the disabled state.
    for (void* context = firstContext(); context != 0; context = nextContext(context)) {
        const KFileItem* fileInfo = contextFileInfo(context);
        const KUrl& fileUrl = fileInfo->url();
        for(KUrl::List::ConstIterator it = urls.begin(); it != urls.end(); ++it) {
            if (fileUrl == (*it)) {
                const QPixmap* itemPixmap = contextPixmap(context);
                if (itemPixmap != 0) {
                    // remember old pixmap
                    DisabledItem disabledItem;
                    disabledItem.url = fileUrl;
                    disabledItem.pixmap = *itemPixmap;
                    m_disabledItems.append(disabledItem);

                    KIconEffect iconEffect;
                    QPixmap disabledPixmap = iconEffect.apply(*itemPixmap,
                                                              K3Icon::Desktop,
                                                              K3Icon::DisabledState);
                    setContextPixmap(context, disabledPixmap);
                }
                break;
            }
        }
    }
}

QString ItemEffectsManager::statusBarText(KFileItem* fileInfo) const
{
    if (fileInfo->isDir()) {
        // KFileItem::getStatusBar() returns "MyDocuments/ Folder" as
        // status bar text for a folder 'MyDocuments'. This is adjusted
        // to "MyDocuments (Folder)" in Dolphin.
        return i18n("%1 (Folder)").arg(fileInfo->name());
    }

    return fileInfo->getStatusBarInfo();
}
