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

#include "dolphindetailsview.h"

#include <qpainter.h>
#include <qobject.h>
#include <q3header.h>
#include <qclipboard.h>
#include <qpainter.h>
//Added by qt3to4:
#include <Q3ValueList>
#include <QPixmap>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QEvent>
#include <QPaintEvent>
#include <QStyleOptionFocusRect>
#include <klocale.h>
#include <kglobalsettings.h>
#include <kicontheme.h>
#include <qscrollbar.h>
#include <qcursor.h>
#include <qstyle.h>
#include <assert.h>

#include "dolphinview.h"
#include "viewproperties.h"
#include "dolphin.h"
#include "kiconeffect.h"
#include "dolphinsettings.h"
#include "dolphinstatusbar.h"
#include "detailsmodesettings.h"

DolphinDetailsView::DolphinDetailsView(DolphinView* parent) :
    KFileDetailView(parent),
    m_dolphinView(parent),
    m_resizeTimer(0),
    m_scrollTimer(0),
    m_rubber(0)
{
    m_resizeTimer = new QTimer(this);
    connect(m_resizeTimer, SIGNAL(timeout()),
            this, SLOT(updateColumnsWidth()));

    setAcceptDrops(true);
    setSelectionMode(KFile::Extended);
    setHScrollBarMode(Q3ScrollView::AlwaysOff);

    setColumnAlignment(SizeColumn, Qt::AlignRight);
    for (int i = DateColumn; i <= GroupColumn; ++i) {
        setColumnAlignment(i, Qt::AlignHCenter);
    }

    Dolphin& dolphin = Dolphin::mainWin();

    connect(this, SIGNAL(onItem(Q3ListViewItem*)),
            this, SLOT(slotOnItem(Q3ListViewItem*)));
    connect(this, SIGNAL(onViewport()),
            this, SLOT(slotOnViewport()));
    connect(this, SIGNAL(contextMenuRequested(Q3ListViewItem*, const QPoint&, int)),
            this, SLOT(slotContextMenuRequested(Q3ListViewItem*, const QPoint&, int)));
    connect(this, SIGNAL(selectionChanged()),
            &dolphin, SLOT(slotSelectionChanged()));
    connect(&dolphin, SIGNAL(activeViewChanged()),
            this, SLOT(slotActivationUpdate()));
    connect(this, SIGNAL(itemRenamed(Q3ListViewItem*, const QString&, int)),
            this, SLOT(slotItemRenamed(Q3ListViewItem*, const QString&, int)));
    connect(this, SIGNAL(dropped(QDropEvent*, const KUrl::List&, const KUrl&)),
            parent, SLOT(slotUrlListDropped(QDropEvent*, const KUrl::List&, const KUrl&)));

    QClipboard* clipboard = QApplication::clipboard();
    connect(clipboard, SIGNAL(dataChanged()),
            this, SLOT(slotUpdateDisabledItems()));

    Q3Header* viewHeader = header();
    viewHeader->setResizeEnabled(false);
    viewHeader->setMovingEnabled(false);
    connect(viewHeader, SIGNAL(clicked(int)),
            this, SLOT(slotHeaderClicked(int)));

    setMouseTracking(true);
    setDefaultRenameAction(Q3ListView::Accept);

    refreshSettings();
}

DolphinDetailsView::~DolphinDetailsView()
{
    delete m_rubber;
    m_rubber = 0;
}

void DolphinDetailsView::beginItemUpdates()
{
}

void DolphinDetailsView::endItemUpdates()
{
    updateDisabledItems();

    // Restore the current item. Use the information stored in the history if
    // available. Otherwise use the first item as current item.

    const KFileListViewItem* item = static_cast<const KFileListViewItem*>(firstChild());
    if (item != 0) {
        setCurrentItem(item->fileInfo());
    }

    int index = 0;
    const Q3ValueList<UrlNavigator::HistoryElem> history = m_dolphinView->urlHistory(index);
    if (!history.isEmpty()) {
        KFileView* fileView = static_cast<KFileView*>(this);
        fileView->setCurrentItem(history[index].currentFileName());
        setContentsPos(history[index].contentsX(), history[index].contentsY());
    }

    updateColumnsWidth();
}

void DolphinDetailsView::insertItem(KFileItem* fileItem)
{
    KFileView::insertItem(fileItem);

    DolphinListViewItem* item = new DolphinListViewItem(static_cast<Q3ListView*>(this), fileItem);

    QDir::SortFlags spec = KFileView::sorting();
    if (spec & QDir::Time) {
        item->setKey(sortingKey(fileItem->time(KIO::UDS_MODIFICATION_TIME),
                                fileItem->isDir(),
                                spec));
    }
    else if (spec & QDir::Size) {
       item->setKey(sortingKey(fileItem->size(), fileItem->isDir(), spec));
    }
    else {
       item->setKey(sortingKey(fileItem->text(), fileItem->isDir(), spec));
    }

    fileItem->setExtraData(this, item);
}

bool DolphinDetailsView::isOnFilename(const Q3ListViewItem* item, const QPoint& pos) const
{
    const QPoint absPos(mapToGlobal(QPoint(0, 0)));
    return (pos.x() - absPos.x()) <= filenameWidth(item);
}

void DolphinDetailsView::refreshSettings()
{
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    assert(settings != 0);

    if (!settings->showGroup()) {
        removeColumn(GroupColumn);
    }
    if (!settings->showOwner()) {
        removeColumn(OwnerColumn);
    }
    if (!settings->showPermissions()) {
        removeColumn(PermissionsColumn);
    }
    if (!settings->showDate()) {
        removeColumn(DateColumn);
    }

    QFont adjustedFont(font());
    adjustedFont.setFamily(settings->fontFamily());
    adjustedFont.setPointSize(settings->fontSize());
    setFont(adjustedFont);

    updateView(true);
}

void DolphinDetailsView::zoomIn()
{
    if (isZoomInPossible()) {
        DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
        switch (settings->iconSize()) {
            case K3Icon::SizeSmall:  settings->setIconSize(K3Icon::SizeMedium); break;
            case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeLarge); break;
            default: assert(false); break;
        }
        ItemEffectsManager::zoomIn();
    }
}

void DolphinDetailsView::zoomOut()
{
    if (isZoomOutPossible()) {
        DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
        switch (settings->iconSize()) {
            case K3Icon::SizeLarge:  settings->setIconSize(K3Icon::SizeMedium); break;
            case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeSmall); break;
            default: assert(false); break;
        }
        ItemEffectsManager::zoomOut();
    }
}

bool DolphinDetailsView::isZoomInPossible() const
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    return settings->iconSize() < K3Icon::SizeLarge;
}

bool DolphinDetailsView::isZoomOutPossible() const
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    return settings->iconSize() > K3Icon::SizeSmall;
}

void DolphinDetailsView::resizeContents(int width, int height)
{
    KFileDetailView::resizeContents(width, height);

    // When loading several 1000 items a punch of resize events
    // drops in. As updating the column width is a quite expensive
    // operation, this operation will be postponed until there is
    // no resize event for at least 50 milliseconds.
    m_resizeTimer->stop();
    m_resizeTimer->start(50, true);
}

void DolphinDetailsView::slotOnItem(Q3ListViewItem* item)
{
    if (isOnFilename(item, QCursor::pos())) {
        activateItem(item);
        KFileItem* fileItem = static_cast<KFileListViewItem*>(item)->fileInfo();
        m_dolphinView->requestItemInfo(fileItem->url());
    }
    else {
        resetActivatedItem();
    }
}

void DolphinDetailsView::slotOnViewport()
{
    resetActivatedItem();
    m_dolphinView->requestItemInfo(KUrl());
}

void DolphinDetailsView::setContextPixmap(void* context,
                                        const QPixmap& pixmap)
{
    reinterpret_cast<KFileListViewItem*>(context)->setPixmap(0, pixmap);
}

const QPixmap* DolphinDetailsView::contextPixmap(void* context)
{
    return reinterpret_cast<KFileListViewItem*>(context)->pixmap(0);
}

void* DolphinDetailsView::firstContext()
{
    return reinterpret_cast<void*>(firstChild());
}

void* DolphinDetailsView::nextContext(void* context)
{
    KFileListViewItem* listViewItem = reinterpret_cast<KFileListViewItem*>(context);
    return reinterpret_cast<void*>(listViewItem->nextSibling());
}

KFileItem* DolphinDetailsView::contextFileInfo(void* context)
{
    return reinterpret_cast<KFileListViewItem*>(context)->fileInfo();
}


void DolphinDetailsView::contentsDragMoveEvent(QDragMoveEvent* event)
{
    KFileDetailView::contentsDragMoveEvent(event);

    // If a dragging is done above a directory, show the icon as 'active' for
    // a visual feedback
    KFileListViewItem* item = static_cast<KFileListViewItem*>(itemAt(event->pos()));

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

void DolphinDetailsView::resizeEvent(QResizeEvent* event)
{
    KFileDetailView::resizeEvent(event);

    // When loading several 1000 items a punch of resize events
    // drops in. As updating the column width is a quite expensive
    // operation, this operation will be postponed until there is
    // no resize event for at least 50 milliseconds.
    m_resizeTimer->stop();
    m_resizeTimer->start(50, true);
}

bool DolphinDetailsView::acceptDrag(QDropEvent* event) const
{
    KUrl::List uriList = KUrl::List::fromMimeData( event->mimeData() );
    bool accept = !uriList.isEmpty() &&
                  (event->action() == QDropEvent::Copy ||
                   event->action() == QDropEvent::Move ||
                   event->action() == QDropEvent::Link);
    if (accept) {
        if (static_cast<const QWidget*>(event->source()) == this) {
            KFileListViewItem* item = static_cast<KFileListViewItem*>(itemAt(event->pos()));
            accept = (item != 0);
            if (accept) {
                KFileItem* fileItem = item->fileInfo();
                accept = fileItem->isDir();
            }
        }
    }

    return accept;
}

void DolphinDetailsView::contentsDropEvent(QDropEvent* event)
{
    // KFileDetailView::contentsDropEvent does not care whether the mouse
    // cursor is above a filename or not, the destination Url is always
    // the Url of the item. This is fixed here in a way that the destination
    // Url is only the Url of the item if the cursor is above the filename.
    const QPoint pos(QCursor::pos());
    const QPoint viewportPos(viewport()->mapToGlobal(QPoint(0, 0)));
    Q3ListViewItem* item = itemAt(QPoint(pos.x() - viewportPos.x(), pos.y() - viewportPos.y()));
    if ((item == 0) || ((item != 0) && isOnFilename(item, pos))) {
        // dropping is done on the viewport or directly above a filename
        KFileDetailView::contentsDropEvent(event);
        return;
    }

    // Dropping is done above an item, but the mouse cursor is not above the file name.
    // In this case the signals of the base implementation will be blocked and send
    // in a corrected manner afterwards.
    assert(item != 0);
    const bool block = signalsBlocked();
    blockSignals(true);
    KFileDetailView::contentsDropEvent(event);
    blockSignals(block);

    if (!acceptDrag(event)) {
        return;
    }

    emit dropped(event, 0);
    KUrl::List urls = KUrl::List::fromMimeData( event->mimeData() );
    if (!urls.isEmpty()) {
        emit dropped(event, urls, KUrl());
        sig->dropURLs(0, event, urls);
    }
}

void DolphinDetailsView::contentsMousePressEvent(QMouseEvent* event)
{
    if (m_rubber != 0) {
        drawRubber();
        delete m_rubber;
        m_rubber = 0;
    }

    // Swallow the base implementation of the mouse press event
    // if the mouse cursor is not above the filename. This prevents
    // that the item gets selected and simulates an equal usability
    // like in the icon view.
    const QPoint pos(QCursor::pos());
    const QPoint viewportPos(viewport()->mapToGlobal(QPoint(0, 0)));
    Q3ListViewItem* item = itemAt(QPoint(pos.x() - viewportPos.x(), pos.y() - viewportPos.y()));
    if ((item != 0) && isOnFilename(item, pos)) {
        KFileDetailView::contentsMousePressEvent(event);
    }
    else if (event->button() == Qt::LeftButton) {
        const Qt::KeyboardModifiers keyboardState = QApplication::keyboardModifiers();
        const bool isSelectionActive = (keyboardState & Qt::ShiftModifier) ||
                                       (keyboardState & Qt::ControlModifier);
        if (!isSelectionActive) {
            clearSelection();
        }

        assert(m_rubber == 0);
        m_rubber = new QRect(event->x(), event->y(), 0, 0);
    }

    resetActivatedItem();
    emit signalRequestActivation();

    m_dolphinView->statusBar()->clear();
}

void DolphinDetailsView::contentsMouseMoveEvent(QMouseEvent* event)
{
    if (m_rubber != 0) {
        slotAutoScroll();
        return;
    }

    KFileDetailView::contentsMouseMoveEvent(event);

    const QPoint& pos = event->globalPos();
    const QPoint viewportPos = viewport()->mapToGlobal(QPoint(0, 0));
    Q3ListViewItem* item = itemAt(QPoint(pos.x() - viewportPos.x(), pos.y() - viewportPos.y()));
    if ((item != 0) && isOnFilename(item, pos)) {
        activateItem(item);
    }
    else {
        resetActivatedItem();
    }
}

void DolphinDetailsView::contentsMouseReleaseEvent(QMouseEvent* event)
{
    if (m_rubber != 0) {
        drawRubber();
        delete m_rubber;
        m_rubber = 0;
    }

    if (m_scrollTimer != 0) {
        disconnect(m_scrollTimer, SIGNAL(timeout()),
                    this, SLOT(slotAutoScroll()));
        m_scrollTimer->stop();
        delete m_scrollTimer;
        m_scrollTimer = 0;
    }

    KFileDetailView::contentsMouseReleaseEvent(event);
}

void DolphinDetailsView::paintEmptyArea(QPainter* painter, const QRect& rect)
{
    if (m_dolphinView->isActive()) {
        KFileDetailView::paintEmptyArea(painter, rect);
    }
    else {
        const QBrush brush(colorGroup().background());
        painter->fillRect(rect, brush);
    }
}

void DolphinDetailsView::drawRubber()
{
    // Parts of the following code have been taken
    // from the class KonqBaseListViewWidget located in
    // konqueror/listview/konq_listviewwidget.h of Konqueror.
    // (Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
    //  2001, 2002, 2004 Michael Brade <brade@kde.org>)
    if (m_rubber == 0) {
        return;
    }

    QPainter p;
    p.begin(viewport());
    //p.setRasterOp(NotROP);
    p.setPen(QPen(Qt::color0, 1));
    p.setBrush(Qt::NoBrush);

    QPoint point(m_rubber->x(), m_rubber->y());
    point = contentsToViewport(point);
    QStyleOptionFocusRect option;
    option.initFrom(this);
    option.rect = QRect(point.x(), point.y(), m_rubber->width(), m_rubber->height());
    style()->drawPrimitive(QStyle::PE_FrameFocusRect, &option, &p);
    p.end();
}

void DolphinDetailsView::viewportPaintEvent(QPaintEvent* paintEvent)
{
    drawRubber();
    KFileDetailView::viewportPaintEvent(paintEvent);
    drawRubber();
}

void DolphinDetailsView::leaveEvent(QEvent* event)
{
    KFileDetailView::leaveEvent(event);
    slotOnViewport();
}

void DolphinDetailsView::slotActivationUpdate()
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

void DolphinDetailsView::slotContextMenuRequested(Q3ListViewItem* item,
                                                  const QPoint& pos,
                                                  int /* col */)
{
    KFileItem* fileInfo = 0;
    if ((item != 0) && isOnFilename(item, pos)) {
        fileInfo = static_cast<KFileListViewItem*>(item)->fileInfo();
    }
    m_dolphinView->openContextMenu(fileInfo, pos);

}

void DolphinDetailsView::slotUpdateDisabledItems()
{
    updateDisabledItems();
}

void DolphinDetailsView::slotAutoScroll()
{
    // Parts of the following code have been taken
    // from the class KonqBaseListViewWidget located in
    // konqueror/listview/konq_listviewwidget.h of Konqueror.
    // (Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
    //  2001, 2002, 2004 Michael Brade <brade@kde.org>)

    const QPoint pos(viewport()->mapFromGlobal(QCursor::pos()));
    const QPoint vc(viewportToContents(pos));

    if (vc == m_rubber->bottomRight()) {
        return;
    }

    drawRubber();

    m_rubber->setBottomRight(vc);

    Q3ListViewItem* item = itemAt(QPoint(0,0));

    const bool block = signalsBlocked();
    blockSignals(true);

    const QRect rubber(m_rubber->normalize());
    const int bottom = contentsY() + visibleHeight() - 1;

    // select all items which intersect with the rubber, deselect all others
    bool bottomReached = false;
    while ((item != 0) && !bottomReached) {
        QRect rect(itemRect(item));
        rect.setWidth(filenameWidth(item));
        rect = QRect(viewportToContents(rect.topLeft()),
                     viewportToContents(rect.bottomRight()));
        if (rect.isValid() && (rect.top() <= bottom)) {
            const KFileItem* fileItem = static_cast<KFileListViewItem*>(item)->fileInfo();
            setSelected(fileItem, rect.intersects(rubber));
            item = item->itemBelow();
        }
        else {
            bottomReached = true;
        }
    }

    blockSignals(block);
    emit selectionChanged();

    drawRubber();

    // scroll the viewport if the top or bottom margin is reached
    const int scrollMargin = 40;
    ensureVisible(vc.x(), vc.y(), scrollMargin, scrollMargin);
    const bool scroll = !QRect(scrollMargin,
                               scrollMargin,
                               viewport()->width()  - 2 * scrollMargin,
                               viewport()->height() - 2 * scrollMargin).contains(pos);
    if (scroll) {
        if (m_scrollTimer == 0) {
            m_scrollTimer = new QTimer( this );
            connect(m_scrollTimer, SIGNAL(timeout()),
                    this, SLOT(slotAutoScroll()));
            m_scrollTimer->start(100, false);
        }
    }
    else if (m_scrollTimer != 0) {
        disconnect(m_scrollTimer, SIGNAL(timeout()),
                   this, SLOT(slotAutoScroll()));
        m_scrollTimer->stop();
        delete m_scrollTimer;
        m_scrollTimer = 0;
    }
}

void DolphinDetailsView::updateColumnsWidth()
{
    const int columnCount = columns();
    int requiredWidth = 0;
    for (int i = 1; i < columnCount; ++i) {
        // When a directory contains no items, a minimum width for
        // the column must be available, so that the header is readable.
        // TODO: use header data instead of the hardcoded 64 value...
        int columnWidth = 64;
        QFontMetrics fontMetrics(font());
        for (Q3ListViewItem* item = firstChild(); item != 0; item = item->nextSibling()) {
            const int width = item->width(fontMetrics, this, i);
            if (width > columnWidth) {
                columnWidth = width;
            }
        }
        columnWidth += 16;    // add custom margin
        setColumnWidth(i, columnWidth);
        requiredWidth += columnWidth;
    }

    // resize the first column in a way that the
    // whole available width is used
    int firstColumnWidth = visibleWidth() - requiredWidth;
    if (firstColumnWidth < 128) {
        firstColumnWidth = 128;
    }
    setColumnWidth(0, firstColumnWidth);
}

void DolphinDetailsView::slotItemRenamed(Q3ListViewItem* item,
                                         const QString& name,
                                         int /* column */)
{
    KFileItem* fileInfo = static_cast<KFileListViewItem*>(item)->fileInfo();
    m_dolphinView->rename(KUrl(fileInfo->url()), name);
}

void DolphinDetailsView::slotHeaderClicked(int /* section */)
{
    // The sorting has already been changed in QListView if this slot is
    // invoked, but Dolphin was not informed about this (no signal is available
    // which indicates a change of the sorting). This is bypassed by changing
    // the sorting and sort order to a temporary other value and readjust it again.
    const int column = sortColumn();
    if (column <= DateColumn) {
        DolphinView::Sorting sorting = DolphinView::SortByName;
        switch (column) {
            case SizeColumn: sorting = DolphinView::SortBySize; break;
            case DateColumn: sorting = DolphinView::SortByDate; break;
            case NameColumn:
            default: break;
        }

        const Qt::SortOrder currSortOrder = sortOrder();

        // temporary adjust the sorting and sort order to different values...
        const DolphinView::Sorting tempSorting = (sorting == DolphinView::SortByName) ?
                                                 DolphinView::SortBySize :
                                                 DolphinView::SortByName;
        m_dolphinView->setSorting(tempSorting);
        const Qt::SortOrder tempSortOrder = (currSortOrder == Qt::Ascending) ?
                                            Qt::Descending : Qt::Ascending;
        m_dolphinView->setSortOrder(tempSortOrder);

        // ... so that setting them again results in storing the new setting.
        m_dolphinView->setSorting(sorting);
        m_dolphinView->setSortOrder(currSortOrder);
    }
}

DolphinDetailsView::DolphinListViewItem::DolphinListViewItem(Q3ListView* parent,
                                                             KFileItem* fileItem) :
    KFileListViewItem(parent, fileItem)
{
    const int iconSize = DolphinSettings::instance().detailsModeSettings()->iconSize();
    KFileItem* info = fileInfo();
    setPixmap(DolphinDetailsView::NameColumn, info->pixmap(iconSize));

    // The base class KFileListViewItem represents the column 'Size' only as byte values.
    // Adjust those values in a way that a mapping to GBytes, MBytes, KBytes and Bytes
    // is done. As the file size for directories is useless (only the size of the directory i-node
    // is given), it is removed completely.
    if (fileItem->isDir()) {
        setText(SizeColumn, " - ");
    }
    else {
        QString sizeText(KIO::convertSize(fileItem->size()));
        sizeText.append(" ");
        setText(SizeColumn, sizeText);
    }

    // Dolphin allows to remove specific columns, but the base class KFileListViewItem
    // is not aware about this (or at least the class KFileDetailView does not react on
    // QListView::remove()). Therefore the columns are rearranged here.
    const DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    assert(settings != 0);

    int column_idx = DateColumn;    // the columns for 'name' and 'size' cannot get removed
    for (int i = DolphinDetailsView::DateColumn; i <= DolphinDetailsView::GroupColumn; ++i) {
        if (column_idx < i) {
            setText(column_idx, text(i));
        }

        bool inc = false;
        switch (i) {
            case DateColumn:        inc = settings->showDate(); break;
            case PermissionsColumn: inc = settings->showPermissions(); break;
            case OwnerColumn:       inc = settings->showOwner(); break;
            case GroupColumn:       inc = settings->showGroup(); break;
            default: break;
        }

        if (inc) {
            ++column_idx;
        }
    }
}

DolphinDetailsView::DolphinListViewItem::~DolphinListViewItem()
{
}

void DolphinDetailsView::DolphinListViewItem::paintCell(QPainter* painter,
                                                        const QColorGroup& colorGroup,
                                                        int column,
                                                        int cellWidth,
                                                        int alignment)
{
    const Q3ListView* view = listView();
    const bool isActive = view->parent() == Dolphin::mainWin().activeView();
    if (isSelected()) {
        // Per default the selection is drawn above the whole width of the item. As a consistent
        // behavior with the icon view is wanted, only the the column containing the file name
        // should be shown as selected.
        QColorGroup defaultColorGroup(colorGroup);
        const QColor highlightColor(isActive ? backgroundColor(column) : view->colorGroup().background());
        defaultColorGroup.setColor(QColorGroup::Highlight , highlightColor);
        defaultColorGroup.setColor(QColorGroup::HighlightedText, colorGroup.color(QColorGroup::Text));
        KFileListViewItem::paintCell(painter, defaultColorGroup, column, cellWidth, alignment);

        if (column == 0) {
            // draw the selection only on the first column
            Q3ListView* parent = listView();
            const int itemWidth = width(parent->fontMetrics(), parent, 0);
            if (isActive) {
                KFileListViewItem::paintCell(painter, colorGroup, column, itemWidth, alignment);
            }
            else {
                Q3ListViewItem::paintCell(painter, colorGroup, column, itemWidth, alignment);
            }
        }
    }
    else {
        if (isActive) {
            KFileListViewItem::paintCell(painter, colorGroup, column, cellWidth, alignment);
        }
        else {
            Q3ListViewItem::paintCell(painter, colorGroup, column, cellWidth, alignment);
        }
    }

    if (column < listView()->columns() - 1) {
        // draw a separator between columns
        painter->setPen(KGlobalSettings::buttonBackground());
        painter->drawLine(cellWidth - 1, 0, cellWidth - 1, height() - 1);
    }
}

void DolphinDetailsView::DolphinListViewItem::paintFocus(QPainter* painter,
                                                         const QColorGroup& colorGroup,
                                                         const QRect& rect)
{
    // draw the focus consistently with the selection (see implementation notes
    // in DolphinListViewItem::paintCell)
    Q3ListView* parent = listView();
    int visibleWidth = width(parent->fontMetrics(), parent, 0);
    const int colWidth = parent->columnWidth(0);
    if (visibleWidth > colWidth) {
        visibleWidth = colWidth;
    }

    QRect focusRect(rect);
    focusRect.setWidth(visibleWidth);

    KFileListViewItem::paintFocus(painter, colorGroup, focusRect);
}

int DolphinDetailsView::filenameWidth(const Q3ListViewItem* item) const
{
    assert(item != 0);

    int visibleWidth = item->width(fontMetrics(), this, 0);
    const int colWidth = columnWidth(0);
    if (visibleWidth > colWidth) {
        visibleWidth = colWidth;
    }

    return visibleWidth;
}
#include "dolphindetailsview.moc"
