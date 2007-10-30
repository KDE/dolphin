/***************************************************************************
 *   Copyright (C) 2007 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "dolphincolumnwidget.h"

#include "dolphinmodel.h"
#include "dolphincolumnview.h"
#include "dolphincontroller.h"
#include "dolphindirlister.h"
#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinsettings.h"

#include "dolphin_columnmodesettings.h"

#include <kcolorutils.h>
#include <kcolorscheme.h>
#include <kdirlister.h>
#include <kfileitem.h>
#include <kio/previewjob.h>
#include <kiconeffect.h>
#include <kmimetyperesolver.h>
#include <konqmimedata.h>

#include <QAbstractProxyModel>
#include <QApplication>
#include <QClipboard>
#include <QPoint>
#include <QScrollBar>
#include <QTimer>
#include <QTimeLine>

DolphinColumnWidget::DolphinColumnWidget(QWidget* parent,
                                         DolphinColumnView* columnView,
                                         const KUrl& url) :
    QListView(parent),
    m_active(true),
    m_showPreview(false),
    m_view(columnView),
    m_url(url),
    m_childUrl(),
    m_viewOptions(),
    m_dirLister(0),
    m_dolphinModel(0),
    m_proxyModel(0),
    m_dragging(false),
    m_dropRect()
{
    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    setSelectionBehavior(SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);

// TODO: Remove this check when 4.3.2 is released and KDE requires it... this
//       check avoids a division by zero happening on versions before 4.3.1.
//       Right now KDE in theory can be shipped with Qt 4.3.0 and above.
//       ereslibre
#if (QT_VERSION >= QT_VERSION_CHECK(4, 3, 2) || defined(QT_KDE_QT_COPY))
    setVerticalScrollMode(QListView::ScrollPerPixel);
    setHorizontalScrollMode(QListView::ScrollPerPixel);
#endif

    // apply the column mode settings to the widget
    const ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    Q_ASSERT(settings != 0);

    m_viewOptions = QListView::viewOptions();

    QFont font(settings->fontFamily(), settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_viewOptions.font = font;

    const int iconSize = settings->iconSize();
    m_viewOptions.decorationSize = QSize(iconSize, iconSize);

    m_viewOptions.showDecorationSelected = true;

    KFileItemDelegate* delegate = new KFileItemDelegate(this);
    setItemDelegate(delegate);

    activate();

    connect(this, SIGNAL(viewportEntered()),
            m_view->m_controller, SLOT(emitViewportEntered()));
    connect(this, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(slotEntered(const QModelIndex&)));

    //m_dirLister = new DolphinDirLister(); TODO
    m_dirLister = new KDirLister();
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(this);
    m_dirLister->setDelayedMimeTypes(true);
    const bool showHiddenFiles = m_view->m_controller->dolphinView()->showHiddenFiles();
    m_dirLister->setShowingDotFiles(showHiddenFiles);
    connect(m_dirLister, SIGNAL(newItems(const KFileItemList&)),
            this, SLOT(generatePreviews(const KFileItemList&)));

    m_dolphinModel = new DolphinModel(this);
    m_dolphinModel->setDirLister(m_dirLister);
    m_dolphinModel->setDropsAllowed(DolphinModel::DropOnDirectory);

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dolphinModel);

    setModel(m_proxyModel);
    new KMimeTypeResolver(this, m_dolphinModel);

    m_dirLister->openUrl(url, KDirLister::NoFlags);
}

DolphinColumnWidget::~DolphinColumnWidget()
{
    delete m_dirLister;
    m_dirLister = 0;
}

void DolphinColumnWidget::setDecorationSize(const QSize& size)
{
    m_viewOptions.decorationSize = size;
    doItemsLayout();
}

void DolphinColumnWidget::setActive(bool active)
{
    if (m_active == active) {
        return;
    }

    m_active = active;

    if (active) {
        activate();
    } else {
        deactivate();
    }
}

void DolphinColumnWidget::reload()
{
    m_dirLister->stop();
    m_dirLister->openUrl(m_url, KDirLister::Reload);
}

void DolphinColumnWidget::setShowHiddenFiles(bool show)
{
    if (show != m_dirLister->showingDotFiles()) {
        m_dirLister->setShowingDotFiles(show);
        m_dirLister->stop();
        m_dirLister->openUrl(m_url, KDirLister::Reload);
    }
}

void DolphinColumnWidget::setShowPreview(bool show)
{
    if (show != m_showPreview) {
        m_dirLister->stop();
        m_dirLister->openUrl(m_url, KDirLister::Reload);
    }
}

void DolphinColumnWidget::updateBackground()
{
    QColor color = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
    if (!m_active || !m_view->m_active) {
        color.setAlpha(150);
    }
    QPalette palette = viewport()->palette();
    palette.setColor(viewport()->backgroundRole(), color);
    viewport()->setPalette(palette);

    update();
}

void DolphinColumnWidget::setNameFilter(const QString& nameFilter)
{
    m_proxyModel->setFilterRegExp(nameFilter);
}

void DolphinColumnWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }

    m_dragging = true;
}

void DolphinColumnWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    QListView::dragLeaveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    m_dragging = false;
    setDirtyRegion(m_dropRect);
}

void DolphinColumnWidget::dragMoveEvent(QDragMoveEvent* event)
{
    QListView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);
    if (itemForIndex(index).isDir()) {
        m_dropRect = visualRect(index);
    } else {
        m_dropRect.setSize(QSize()); // set as invalid
    }
    setDirtyRegion(m_dropRect);
}

void DolphinColumnWidget::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        const QModelIndex index = indexAt(event->pos());
        const KFileItem item = itemForIndex(index);
        m_view->m_controller->indicateDroppedUrls(urls,
                                                  url(),
                                                  item);
        event->acceptProposedAction();
    }
    QListView::dropEvent(event);
    m_dragging = false;
}

void DolphinColumnWidget::paintEvent(QPaintEvent* event)
{
    if (!m_childUrl.isEmpty()) {
        // indicate the shown URL of the next column by highlighting the shown folder item
        const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_childUrl);
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
        if (proxyIndex.isValid() && !selectionModel()->isSelected(proxyIndex)) {
            const QRect itemRect = visualRect(proxyIndex);
            QPainter painter(viewport());
            painter.save();

            QColor color = KColorScheme(QPalette::Active, KColorScheme::View).foreground().color();
            color.setAlpha(32);
            painter.setPen(Qt::NoPen);
            painter.setBrush(color);
            painter.drawRect(itemRect);

            painter.restore();
        }
    }

    QListView::paintEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    if (m_dragging) {
        const QBrush& brush = m_viewOptions.palette.brush(QPalette::Normal, QPalette::Highlight);
        DolphinController::drawHoverIndication(viewport(), m_dropRect, brush);
    }
}

void DolphinColumnWidget::mousePressEvent(QMouseEvent* event)
{
    m_view->m_controller->requestActivation();
    if (!m_active) {
        m_view->requestActivation(this);
        m_view->m_controller->triggerUrlChangeRequest(m_url);
    }

    QListView::mousePressEvent(event);
}

void DolphinColumnWidget::keyPressEvent(QKeyEvent* event)
{
    QListView::keyPressEvent(event);

    const QItemSelectionModel* selModel = selectionModel();
    const QModelIndex currentIndex = selModel->currentIndex();
    const bool trigger = currentIndex.isValid()
                         && (event->key() == Qt::Key_Return)
                         && (selModel->selectedIndexes().count() <= 1);
    if (trigger) {
        triggerItem(currentIndex);
    }
}

void DolphinColumnWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (!m_active) {
        m_view->requestActivation(this);
        m_view->m_controller->triggerUrlChangeRequest(m_url);
    }

    QListView::contextMenuEvent(event);

    const QModelIndex index = indexAt(event->pos());
    if (index.isValid() || m_active) {
        // Only open a context menu above an item or if the mouse is above
        // the active column.
        const QPoint pos = m_view->viewport()->mapFromGlobal(event->globalPos());
        m_view->m_controller->triggerContextMenuRequest(pos);
    }
}

void DolphinColumnWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QListView::selectionChanged(selected, deselected);

    QItemSelectionModel* selModel = m_view->selectionModel();
    selModel->select(selected, QItemSelectionModel::Select);
    selModel->select(deselected, QItemSelectionModel::Deselect);
}
void DolphinColumnWidget::triggerItem(const QModelIndex& index)
{
    const KFileItem item = itemForIndex(index);
    m_view->m_controller->triggerItem(item);
}

void DolphinColumnWidget::generatePreviews(const KFileItemList& items)
{
    // TODO: same implementation as in DolphinView; create helper class
    // for generatePreviews(), showPreview() and isCutItem()

    if (m_view->m_controller->dolphinView()->showPreview()) {
        KIO::PreviewJob* job = KIO::filePreview(items, 128);
        connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
                this, SLOT(showPreview(const KFileItem&, const QPixmap&)));
    }
}

void DolphinColumnWidget::showPreview(const KFileItem& item, const QPixmap& pixmap)
{
    // TODO: same implementation as in DolphinView; create helper class
    // for generatePreviews(), showPreview() and isCutItem()

    Q_ASSERT(!item.isNull());
    if (item.url().directory() != m_dirLister->url().path()) {
        // the preview job is still working on items of an older URL, hence
        // the item is not part of the directory model anymore
        return;
    }

    const QModelIndex idx = m_dolphinModel->indexForItem(item);
    if (idx.isValid() && (idx.column() == 0)) {
        const QMimeData* mimeData = QApplication::clipboard()->mimeData();
        if (KonqMimeData::decodeIsCutSelection(mimeData) && isCutItem(item)) {
            KIconEffect iconEffect;
            const QPixmap cutPixmap = iconEffect.apply(pixmap, KIconLoader::Desktop, KIconLoader::DisabledState);
            m_dolphinModel->setData(idx, QIcon(cutPixmap), Qt::DecorationRole);
        } else {
            m_dolphinModel->setData(idx, QIcon(pixmap), Qt::DecorationRole);
        }
    }
}

void DolphinColumnWidget::slotEntered(const QModelIndex& index)
{
    const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
    const KFileItem item = m_dolphinModel->itemForIndex(dirIndex);
    m_view->m_controller->emitItemEntered(item);
}

void DolphinColumnWidget::activate()
{
    setFocus(Qt::OtherFocusReason);

    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick'.
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                this, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                this, SLOT(triggerItem(const QModelIndex&)));
    }

    if (!m_childUrl.isEmpty()) {
        // assure that the current index is set on the index that represents
        // the child URL
        const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_childUrl);
        const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
        selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::Current);
    }

    updateBackground();
}

void DolphinColumnWidget::deactivate()
{
    clearFocus();

    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick'.
    if (KGlobalSettings::singleClick()) {
        disconnect(this, SIGNAL(clicked(const QModelIndex&)),
                   this, SLOT(triggerItem(const QModelIndex&)));
    } else {
        disconnect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                   this, SLOT(triggerItem(const QModelIndex&)));
    }

    selectionModel()->clear();
    updateBackground();
}

bool DolphinColumnWidget::isCutItem(const KFileItem& item) const
{
    // TODO: same implementation as in DolphinView; create helper class
    // for generatePreviews(), showPreview() and isCutItem()

    const QMimeData* mimeData = QApplication::clipboard()->mimeData();
    const KUrl::List cutUrls = KUrl::List::fromMimeData(mimeData);

    const KUrl& itemUrl = item.url();
    KUrl::List::const_iterator it = cutUrls.begin();
    const KUrl::List::const_iterator end = cutUrls.end();
    while (it != end) {
        if (*it == itemUrl) {
            return true;
        }
        ++it;
    }

    return false;
}

KFileItem DolphinColumnWidget::itemForIndex(const QModelIndex& index) const
{
    const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
    return m_dolphinModel->itemForIndex(dirIndex);
}


#include "dolphincolumnwidget.moc"
