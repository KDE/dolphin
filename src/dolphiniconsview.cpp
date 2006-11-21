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

#include "dolphiniconsview.h"
#include <qpainter.h>
//Added by qt3to4:
#include <QDragMoveEvent>
#include <QDropEvent>
#include <Q3ValueList>
#include <QPixmap>
#include <QMouseEvent>
#include <QDragEnterEvent>
#include <kiconeffect.h>
#include <kapplication.h>
#include <qobject.h>
#include <kglobalsettings.h>
#include <qclipboard.h>
#include <assert.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kfileitem.h>
#include <kactioncollection.h>

#include "dolphinview.h"
#include "viewproperties.h"
#include "dolphin.h"
#include "dolphinstatusbar.h"
#include "dolphinsettings.h"
#include "iconsmodesettings.h"

DolphinIconsView::DolphinIconsView(DolphinView* parent, LayoutMode layoutMode) :
    KFileIconView(parent, 0),
    m_previewIconSize(-1),
    m_layoutMode(layoutMode),
    m_dolphinView(parent)
{
    setAcceptDrops(true);
    setMode(K3IconView::Execute);
    setSelectionMode(KFile::Extended);
    Dolphin& dolphin = Dolphin::mainWin();

    connect(this, SIGNAL(onItem(Q3IconViewItem*)),
            this, SLOT(slotOnItem(Q3IconViewItem*)));
    connect(this, SIGNAL(onViewport()),
            this, SLOT(slotOnViewport()));
    connect(this, SIGNAL(contextMenuRequested(Q3IconViewItem*, const QPoint&)),
            this, SLOT(slotContextMenuRequested(Q3IconViewItem*, const QPoint&)));
    connect(this, SIGNAL(selectionChanged()),
            &dolphin, SLOT(slotSelectionChanged()));
    connect(&dolphin, SIGNAL(activeViewChanged()),
            this, SLOT(slotActivationUpdate()));
    connect(this, SIGNAL(itemRenamed(Q3IconViewItem*, const QString&)),
            this, SLOT(slotItemRenamed(Q3IconViewItem*, const QString&)));
    connect(this, SIGNAL(dropped(QDropEvent*, const KURL::List&, const KURL&)),
            parent, SLOT(slotURLListDropped(QDropEvent*, const KURL::List&, const KURL&)));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(slotUpdateDisabledItems()));

    // KFileIconView creates two actions for zooming, which are directly connected to the
    // slots KFileIconView::zoomIn() and KFileIconView::zoomOut(). As this behavior is not
    // wanted and the slots are not virtual, the actions are disabled here.
    KAction* zoomInAction = actionCollection()->action("zoomIn");
    assert(zoomInAction != 0);
    zoomInAction->setEnabled(false);

    KAction* zoomOutAction = actionCollection()->action("zoomOut");
    assert(zoomOutAction != 0);
    zoomOutAction->setEnabled(false);

    setItemsMovable(true);
    setWordWrapIconText(true);
    if (m_layoutMode == Previews) {
        showPreviews();
    }
    refreshSettings();
}

DolphinIconsView::~DolphinIconsView()
{
}

void DolphinIconsView::setLayoutMode(LayoutMode mode)
{
    if (m_layoutMode != mode) {
        m_layoutMode = mode;
        refreshSettings();
    }
}

void DolphinIconsView::beginItemUpdates()
{
}

void DolphinIconsView::endItemUpdates()
{
    arrangeItemsInGrid();

    // TODO: KFileIconView does not emit any signal when the preview
    // has been finished. Using a delay of 300 ms is a temporary workaround
    // until the DolphinIconsView will implement the previews by it's own in
    // future releases.
    QTimer::singleShot(300, this, SLOT(slotUpdateDisabledItems()));

    const KFileIconViewItem* item = static_cast<const KFileIconViewItem*>(firstItem());
    if (item != 0) {
        setCurrentItem(item->fileInfo());
    }

    int index = 0;
    const Q3ValueList<URLNavigator::HistoryElem> history = m_dolphinView->urlHistory(index);
    if (!history.isEmpty()) {
        KFileView* fileView = static_cast<KFileView*>(this);
        fileView->setCurrentItem(history[index].currentFileName());
        setContentsPos(history[index].contentsX(), history[index].contentsY());
    }
}

void DolphinIconsView::refreshSettings()
{
    const IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    assert(settings != 0);

    setIconSize(settings->iconSize());

    const Q3IconView::Arrangement arrangement = settings->arrangement() == "LeftToRight" ? // TODO: use enum directly in settings
                                               Q3IconView::LeftToRight : Q3IconView::TopToBottom;
    const Q3IconView::ItemTextPos textPos = (arrangement == Q3IconView::LeftToRight) ?
                                           Q3IconView::Bottom :
                                           Q3IconView::Right;
    setArrangement(arrangement);
    setItemTextPos(textPos);

    // TODO: tempory crash; will get changed anyway for KDE 4
    /*setGridX(settings->gridWidth());
    setGridY(settings->gridHeight());
    setSpacing(settings->gridSpacing());*/

    QFont adjustedFont(font());
    adjustedFont.setFamily(settings->fontFamily());
    adjustedFont.setPointSize(settings->fontSize());
    setFont(adjustedFont);
    setIconTextHeight(settings->numberOfTexlines());

    if (m_layoutMode == Previews) {
        // There is no getter method for the current size in KFileIconView. To
        // prevent a flickering the current size is stored in m_previewIconSize and
        // setPreviewSize is only invoked if the size really has changed.
        showPreviews();

        const int size = settings->previewSize();
        if (size != m_previewIconSize) {
            m_previewIconSize = size;
            setPreviewSize(size);
        }
    }
}

void DolphinIconsView::zoomIn()
{
    if (isZoomInPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
        const int textWidthHint = DolphinSettings::instance().textWidthHint(); // TODO: remove for KDE4

        const int iconSize = increasedIconSize(settings->iconSize());
        settings->setIconSize(iconSize);

        if (m_layoutMode == Previews) {
            const int previewSize = increasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
        }

        DolphinSettings::instance().calculateGridSize(textWidthHint); // TODO: remove for KDE4
        ItemEffectsManager::zoomIn();
    }
}

void DolphinIconsView::zoomOut()
{
    if (isZoomOutPossible()) {
        IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
        const int textWidthHint = DolphinSettings::instance().textWidthHint(); // TODO: remove for KDE4

        const int iconSize = decreasedIconSize(settings->iconSize());
        settings->setIconSize(iconSize);

        if (m_layoutMode == Previews) {
            const int previewSize = decreasedIconSize(settings->previewSize());
            settings->setPreviewSize(previewSize);
        }

        DolphinSettings::instance().calculateGridSize(textWidthHint); // TODO: remove for KDE4
        ItemEffectsManager::zoomOut();
    }
}

bool DolphinIconsView::isZoomInPossible() const
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    const int size = (m_layoutMode == Icons) ? settings->iconSize() : settings->previewSize();
    return size < K3Icon::SizeEnormous;
}

bool DolphinIconsView::isZoomOutPossible() const
{
    IconsModeSettings* settings = DolphinSettings::instance().iconsModeSettings();
    return settings->iconSize() > K3Icon::SizeSmall;
}

void DolphinIconsView::arrangeItemsInGrid( bool updated )
{

    KFileIconView::arrangeItemsInGrid(updated);

    if (m_layoutMode == Previews) {
        // The class KFileIconView has a bug when the size of the previews differs from the size
        // of the icons: For specific MIME types the y-position and the height is calculated in
        // a wrong manner. The following code bypasses this issue. No bugreport has been submitted
        // as this functionality is not used by any KDE3 application and the core developers are
        // busy enough for KDE4 now :-)

        KFileIconViewItem* item = static_cast<KFileIconViewItem*>(Q3IconView::firstItem());
        QString mimetype;
        while (item != 0) {
            mimetype = item->fileInfo()->mimetype();
            const bool fixSize = mimetype.contains("text") ||
                                 mimetype.contains("application/x-");
            if (fixSize) {
                item->setPixmapSize(QSize(m_previewIconSize, m_previewIconSize));
            }
            item = static_cast<KFileIconViewItem *>(item->nextItem());
        }
    }
}

void DolphinIconsView::setContextPixmap(void* context,
                                        const QPixmap& pixmap)
{
    reinterpret_cast<KFileIconViewItem*>(context)->setPixmap(pixmap);
}

const QPixmap* DolphinIconsView::contextPixmap(void* context)
{
    return reinterpret_cast<KFileIconViewItem*>(context)->pixmap();
}

void* DolphinIconsView::firstContext()
{
    return reinterpret_cast<void*>(firstItem());
}

void* DolphinIconsView::nextContext(void* context)
{
    KFileIconViewItem* iconViewItem = reinterpret_cast<KFileIconViewItem*>(context);
    return reinterpret_cast<void*>(iconViewItem->nextItem());
}

KFileItem* DolphinIconsView::contextFileInfo(void* context)
{
    return reinterpret_cast<KFileIconViewItem*>(context)->fileInfo();
}

void DolphinIconsView::contentsMousePressEvent(QMouseEvent* event)
{
    KFileIconView::contentsMousePressEvent(event);
    resetActivatedItem();
    emit signalRequestActivation();
    m_dolphinView->statusBar()->clear();
}

void DolphinIconsView::contentsMouseReleaseEvent(QMouseEvent* event)
{
    KFileIconView::contentsMouseReleaseEvent(event);

    // The KFileIconView does not send any selectionChanged signal if
    // a selection is done by using the "select-during-button-pressed" feature.
    // Hence inform Dolphin about the selection change manually:
    Dolphin::mainWin().slotSelectionChanged();
}

void DolphinIconsView::drawBackground(QPainter* painter, const QRect& rect)
{
    if (m_dolphinView->isActive()) {
        KFileIconView::drawBackground(painter, rect);
    }
    else {
        const QBrush brush(colorGroup().background());
        painter->fillRect(0, 0, width(), height(), brush);
    }
}

Q3DragObject* DolphinIconsView::dragObject()
{
    KUrl::List urls;
    QListIterator<KFileItem*> it(*KFileView::selectedItems());
    while (it.hasNext()) {
        KFileItem *item = it.next();
        urls.append(item->url());
    }

    QPixmap pixmap;
    if(urls.count() > 1) {
        pixmap = DesktopIcon("kmultiple", iconSize());
    }
    else {
        KFileIconViewItem* item = static_cast<KFileIconViewItem*>(currentItem());
        if ((item != 0) && (item->pixmap() != 0)) {
            pixmap = *(item->pixmap());
        }
    }

    if (pixmap.isNull()) {
        pixmap = currentFileItem()->pixmap(iconSize());
    }

    /* This should be ported to QMimeData
    Q3DragObject* dragObj = new KURLDrag(urls, widget());
    dragObj->setPixmap(pixmap);
    return dragObj;
    */
    return 0;
}

void DolphinIconsView::contentsDragEnterEvent(QDragEnterEvent* event)
{
    // TODO: The method KFileIconView::contentsDragEnterEvent() does
    // not allow drag and drop inside itself, which prevents the possability
    // to move a file into a directory. As the method KFileIconView::acceptDrag()
    // is not virtual, we must overwrite the method
    // KFileIconView::contentsDragEnterEvent() and do some cut/copy/paste for this
    // usecase. Corresponding to the documentation the method KFileIconView::acceptDrag()
    // will get virtual in KDE 4, which will simplify the code.

    if (event->source() != this) {
        KFileIconView::contentsDragEnterEvent(event);
        return;
    }

    KUrl::List uriList = KUrl::List::fromMimeData( event->mimeData() );
    const bool accept = !uriList.isEmpty() &&
                        (event->action() == QDropEvent::Copy ||
                         event->action() == QDropEvent::Move ||
                         event->action() == QDropEvent::Link );
    if (accept) {
        event->acceptAction();
    }
    else {
        event->ignore();
    }
}

void DolphinIconsView::contentsDragMoveEvent(QDragMoveEvent* event)
{
    KFileIconView::contentsDragMoveEvent(event);

    // If a dragging is done above a directory, show the icon as 'active' for
    // a visual feedback
    KFileIconViewItem* item = static_cast<KFileIconViewItem*>(findItem(contentsToViewport(event->pos())));

    bool showActive = false;
    if (item != 0) {
        const KFileItem* fileInfo = item->fileInfo();
        showActive = (fileInfo != 0) && fileInfo->isDir();
    }

    if (showActive) {
        slotOnItem(item);
    }
    else {
        slotOnViewport();
    }
}

void DolphinIconsView::contentsDropEvent(QDropEvent* event)
{
    // TODO: Most of the following code is a copy of
    // KFileIconView::contentsDropEvent. See comment in
    // DolphinIconsView::contentsDragEnterEvent for details.

    if (event->source() != this) {
        KFileIconView::contentsDropEvent(event);
        return;
    }

    KFileIconViewItem* item = static_cast<KFileIconViewItem*>(findItem(contentsToViewport(event->pos())));
    KUrl::List urls = KUrl::List::fromMimeData( event->mimeData() );
    const bool accept = !urls.isEmpty() &&
                        (event->action() == QDropEvent::Copy ||
                         event->action() == QDropEvent::Move ||
                         event->action() == QDropEvent::Link ) &&
                        (item != 0);
    if (!accept) {
        return;
    }

    KFileItem* fileItem = item->fileInfo();
    if (!fileItem->isDir()) {
        // the file is not a directory, hence don't accept any drop
        return;
    }
    emit dropped(event, fileItem);
    if (!urls.isEmpty()) {
        emit dropped(event, urls, fileItem != 0 ? fileItem->url() : KUrl());
        sig->dropURLs(fileItem, event, urls);
    }
}

void DolphinIconsView::slotOnItem(Q3IconViewItem* item)
{
    assert(item != 0);
    activateItem(reinterpret_cast<void*>(item));

    KFileItem* fileItem = static_cast<KFileIconViewItem*>(item)->fileInfo();
    m_dolphinView->requestItemInfo(fileItem->url());
}

void DolphinIconsView::slotOnViewport()
{
    resetActivatedItem();
    m_dolphinView->requestItemInfo(KUrl());
}

void DolphinIconsView::slotContextMenuRequested(Q3IconViewItem* item,
                                                const QPoint& pos)
{
    KFileItem* fileInfo = 0;
    if (item != 0) {
        fileInfo = static_cast<KFileIconViewItem*>(item)->fileInfo();
    }
    m_dolphinView->openContextMenu(fileInfo, pos);
}

void DolphinIconsView::slotItemRenamed(Q3IconViewItem* item,
                                       const QString& name)
{
    KFileItem* fileInfo = static_cast<KFileIconViewItem*>(item)->fileInfo();
    m_dolphinView->rename(KUrl(fileInfo->url()), name);
}

void DolphinIconsView::slotActivationUpdate()
{
    update();

    // TODO: there must be a simpler way to say
    // "update all children"
    const QList<QObject*> list = children();
    if (list.isEmpty()) {
        return;
    }

    QListIterator<QObject*> it(list);
    QObject* object = 0;
    while (it.hasNext()) {
        object = it.next();
        if (object->inherits("QWidget")) {
            QWidget* widget = static_cast<QWidget*>(object);
            widget->update();
        }
    }
}

void DolphinIconsView::slotUpdateDisabledItems()
{
    updateDisabledItems();
}

int DolphinIconsView::increasedIconSize(int size) const
{
    int incSize = 0;
    switch (size) {
        case K3Icon::SizeSmall:       incSize = K3Icon::SizeSmallMedium; break;
        case K3Icon::SizeSmallMedium: incSize = K3Icon::SizeMedium; break;
        case K3Icon::SizeMedium:      incSize = K3Icon::SizeLarge; break;
        case K3Icon::SizeLarge:       incSize = K3Icon::SizeHuge; break;
        case K3Icon::SizeHuge:        incSize = K3Icon::SizeEnormous; break;
        default: assert(false); break;
    }
    return incSize;
}

int DolphinIconsView::decreasedIconSize(int size) const
{
    int decSize = 0;
    switch (size) {
        case K3Icon::SizeSmallMedium: decSize = K3Icon::SizeSmall; break;
        case K3Icon::SizeMedium: decSize = K3Icon::SizeSmallMedium; break;
        case K3Icon::SizeLarge: decSize = K3Icon::SizeMedium; break;
        case K3Icon::SizeHuge: decSize = K3Icon::SizeLarge; break;
        case K3Icon::SizeEnormous: decSize = K3Icon::SizeHuge; break;
        default: assert(false); break;
    }
    return decSize;
}

#include "dolphiniconsview.moc"
