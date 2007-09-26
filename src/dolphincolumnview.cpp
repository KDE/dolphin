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

#include "dolphincolumnview.h"

#include "dolphinmodel.h"
#include "dolphincontroller.h"
#include "dolphinsettings.h"

#include "dolphin_columnmodesettings.h"

#include <kcolorutils.h>
#include <kcolorscheme.h>
#include <kdirlister.h>

#include <QAbstractProxyModel>
#include <QApplication>
#include <QPoint>
#include <QScrollBar>
#include <QTimer>
#include <QTimeLine>

/**
 * Represents one column inside the DolphinColumnView and has been
 * extended to respect view options and hovering information.
 */
class ColumnWidget : public QListView
{
public:
    ColumnWidget(QWidget* parent,
                 DolphinColumnView* columnView,
                 const KUrl& url);
    virtual ~ColumnWidget();

    /** Sets the size of the icons. */
    void setDecorationSize(const QSize& size);

    /**
     * An active column is defined as column, which shows the same URL
     * as indicated by the URL navigator. The active column is usually
     * drawn in a lighter color. All operations are applied to this column.
     */
    void setActive(bool active);
    bool isActive() const;

    /**
     * Sets the directory URL of the child column that is shown next to
     * this column. This property is only used for a visual indication
     * of the shown directory, it does not trigger a loading of the model.
     */
    void setChildUrl(const KUrl& url);
    const KUrl& childUrl() const;

    /** Sets the directory URL that is shown inside the column widget. */
    void setUrl(const KUrl& url);

    /** Returns the directory URL that is shown inside the column widget. */
    inline const KUrl& url() const;

protected:
    virtual QStyleOptionViewItem viewOptions() const;
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);
    virtual void dragMoveEvent(QDragMoveEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void paintEvent(QPaintEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void keyPressEvent(QKeyEvent* event);
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    /** Used by ColumnWidget::setActive(). */
    void activate();

    /** Used by ColumnWidget::setActive(). */
    void deactivate();

private:
    bool m_active;
    DolphinColumnView* m_view;
    KUrl m_url;      // URL of the directory that is shown
    KUrl m_childUrl; // URL of the next column that is shown
    QStyleOptionViewItem m_viewOptions;

    bool m_dragging;   // TODO: remove this property when the issue #160611 is solved in Qt 4.4
    QRect m_dropRect;  // TODO: remove this property when the issue #160611 is solved in Qt 4.4
};

ColumnWidget::ColumnWidget(QWidget* parent,
                           DolphinColumnView* columnView,
                           const KUrl& url) :
    QListView(parent),
    m_active(true),
    m_view(columnView),
    m_url(url),
    m_childUrl(),
    m_dragging(false),
    m_dropRect()
{
    setMouseTracking(true);
    viewport()->setAttribute(Qt::WA_Hover);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOn);
    // TODO: enable ScrollPerPixel again as soon as a Qt-patch
    // is supplied which fixes a possible crash
    //setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setSelectionBehavior(SelectItems);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setFocusPolicy(Qt::NoFocus);

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

    KFileItemDelegate* delegate = new KFileItemDelegate(this);
    setItemDelegate(delegate);

    activate();

    connect(this, SIGNAL(entered(const QModelIndex&)),
            m_view->m_controller, SLOT(emitItemEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            m_view->m_controller, SLOT(emitViewportEntered()));
}

ColumnWidget::~ColumnWidget()
{
}

void ColumnWidget::setDecorationSize(const QSize& size)
{
    m_viewOptions.decorationSize = size;
    doItemsLayout();
}

void ColumnWidget::setActive(bool active)
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

inline bool ColumnWidget::isActive() const
{
    return m_active;
}

inline void ColumnWidget::setChildUrl(const KUrl& url)
{
    m_childUrl = url;
}

inline const KUrl& ColumnWidget::childUrl() const
{
    return m_childUrl;
}

inline void ColumnWidget::setUrl(const KUrl& url)
{
    m_url = url;
}

const KUrl& ColumnWidget::url() const
{
    return m_url;
}

QStyleOptionViewItem ColumnWidget::viewOptions() const
{
    return m_viewOptions;
}

void ColumnWidget::dragEnterEvent(QDragEnterEvent* event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }

    m_dragging = true;
}

void ColumnWidget::dragLeaveEvent(QDragLeaveEvent* event)
{
    QListView::dragLeaveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    m_dragging = false;
    setDirtyRegion(m_dropRect);
}

void ColumnWidget::dragMoveEvent(QDragMoveEvent* event)
{
    QListView::dragMoveEvent(event);

    // TODO: remove this code when the issue #160611 is solved in Qt 4.4
    const QModelIndex index = indexAt(event->pos());
    setDirtyRegion(m_dropRect);
    m_dropRect = visualRect(index);
    setDirtyRegion(m_dropRect);
}

void ColumnWidget::dropEvent(QDropEvent* event)
{
    const KUrl::List urls = KUrl::List::fromMimeData(event->mimeData());
    if (!urls.isEmpty()) {
        event->acceptProposedAction();
        m_view->m_controller->indicateDroppedUrls(urls,
                                                  url(),
                                                  indexAt(event->pos()),
                                                  event->source());
    }
    QListView::dropEvent(event);
    m_dragging = false;
}

void ColumnWidget::paintEvent(QPaintEvent* event)
{
    if (!m_childUrl.isEmpty()) {
        // indicate the shown URL of the next column by highlighting the shown folder item
        const QModelIndex dirIndex = m_view->m_dolphinModel->indexForUrl(m_childUrl);
        const QModelIndex proxyIndex = m_view->m_proxyModel->mapFromSource(dirIndex);
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

void ColumnWidget::mousePressEvent(QMouseEvent* event)
{
    if (!m_active) {
        m_view->requestActivation(this);
    }

    QListView::mousePressEvent(event);
}

void ColumnWidget::keyPressEvent(QKeyEvent* event)
{
    QListView::keyPressEvent(event);

    const QItemSelectionModel* selModel = selectionModel();
    const QModelIndex currentIndex = selModel->currentIndex();
    const bool triggerItem = currentIndex.isValid()
                             && (event->key() == Qt::Key_Return)
                             && (selModel->selectedIndexes().count() <= 1);
    if (triggerItem) {
        m_view->m_controller->triggerItem(currentIndex);
    }
}

void ColumnWidget::contextMenuEvent(QContextMenuEvent* event)
{
    if (!m_active) {
        m_view->requestActivation(this);
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

void ColumnWidget::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    QListView::selectionChanged(selected, deselected);

    QItemSelectionModel* selModel = m_view->selectionModel();
    selModel->select(selected, QItemSelectionModel::Select);
    selModel->select(deselected, QItemSelectionModel::Deselect);
}

void ColumnWidget::activate()
{
    if (m_view->hasFocus()) {
        setFocus(Qt::OtherFocusReason);
    }
    m_view->setFocusProxy(this);

    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick'.
    if (KGlobalSettings::singleClick()) {
        connect(this, SIGNAL(clicked(const QModelIndex&)),
                m_view->m_controller, SLOT(triggerItem(const QModelIndex&)));
    } else {
        connect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                m_view->m_controller, SLOT(triggerItem(const QModelIndex&)));
    }

    const QColor bgColor = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
    QPalette palette = viewport()->palette();
    palette.setColor(viewport()->backgroundRole(), bgColor);
    viewport()->setPalette(palette);

    if (!m_childUrl.isEmpty()) {
        // assure that the current index is set on the index that represents
        // the child URL
        const QModelIndex dirIndex = m_view->m_dolphinModel->indexForUrl(m_childUrl);
        const QModelIndex proxyIndex = m_view->m_proxyModel->mapFromSource(dirIndex);
        selectionModel()->setCurrentIndex(proxyIndex, QItemSelectionModel::Current);
    }

    update();
}

void ColumnWidget::deactivate()
{
    // TODO: Connecting to the signal 'activated()' is not possible, as kstyle
    // does not forward the single vs. doubleclick to it yet (KDE 4.1?). Hence it is
    // necessary connecting the signal 'singleClick()' or 'doubleClick'.
    if (KGlobalSettings::singleClick()) {
        disconnect(this, SIGNAL(clicked(const QModelIndex&)),
                   m_view->m_controller, SLOT(triggerItem(const QModelIndex&)));
    } else {
        disconnect(this, SIGNAL(doubleClicked(const QModelIndex&)),
                   m_view->m_controller, SLOT(triggerItem(const QModelIndex&)));
    }

    const QPalette palette = m_view->viewport()->palette();
    viewport()->setPalette(palette);

    selectionModel()->clear();
    update();
}

// ---

DolphinColumnView::DolphinColumnView(QWidget* parent, DolphinController* controller) :
    QAbstractItemView(parent),
    m_controller(controller),
    m_restoreActiveColumnFocus(false),
    m_dirListerCompleted(false),
    m_index(-1),
    m_contentX(0),
    m_columns(),
    m_animation(0),
    m_dolphinModel(0),
    m_proxyModel(0)
{
    Q_ASSERT(controller != 0);

    setAcceptDrops(true);
    setDragDropMode(QAbstractItemView::DragDrop);
    setDropIndicatorShown(false);
    setSelectionMode(ExtendedSelection);

    connect(this, SIGNAL(entered(const QModelIndex&)),
            controller, SLOT(emitItemEntered(const QModelIndex&)));
    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(zoomIn()),
            this, SLOT(zoomIn()));
    connect(controller, SIGNAL(zoomOut()),
            this, SLOT(zoomOut()));
    connect(controller, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(showColumn(const KUrl&)));

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(moveContentHorizontally(int)));

    ColumnWidget* column = new ColumnWidget(viewport(), this, m_controller->url());
    m_columns.append(column);
    setActiveColumnIndex(0);

    updateDecorationSize();

    m_animation = new QTimeLine(500, this);
    connect(m_animation, SIGNAL(frameChanged(int)), horizontalScrollBar(), SLOT(setValue(int)));

    // dim the background of the viewport
    QColor bgColor = KColorScheme(QPalette::Active, KColorScheme::View).background().color();
    const QColor fgColor = KColorScheme(QPalette::Active, KColorScheme::View).foreground().color();
    bgColor = KColorUtils::mix(bgColor, fgColor, 0.04);

    QPalette palette = viewport()->palette();
    palette.setColor(viewport()->backgroundRole(), bgColor);
    viewport()->setPalette(palette);
}

DolphinColumnView::~DolphinColumnView()
{
}

QModelIndex DolphinColumnView::indexAt(const QPoint& point) const
{
    foreach (ColumnWidget* column, m_columns) {
        const QPoint topLeft = column->frameGeometry().topLeft();
        const QPoint adjustedPoint(point.x() - topLeft.x(), point.y() - topLeft.y());
        const QModelIndex index = column->indexAt(adjustedPoint);
        if (index.isValid()) {
            return index;
        }
    }

    return QModelIndex();
}

void DolphinColumnView::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    activeColumn()->scrollTo(index, hint);
}

QRect DolphinColumnView::visualRect(const QModelIndex& index) const
{
    return activeColumn()->visualRect(index);
}

void DolphinColumnView::setModel(QAbstractItemModel* model)
{
    if (m_dolphinModel != 0) {
        m_dolphinModel->disconnect(this);
    }

    m_proxyModel = static_cast<QAbstractProxyModel*>(model);
    m_dolphinModel = static_cast<DolphinModel*>(m_proxyModel->sourceModel());
    connect(m_dolphinModel, SIGNAL(expand(const QModelIndex&)),
            this, SLOT(triggerReloadColumns(const QModelIndex&)));

    KDirLister* dirLister = m_dolphinModel->dirLister();
    connect(dirLister, SIGNAL(started(const KUrl&)),
            this, SLOT(slotDirListerStarted(const KUrl&)));
    connect(dirLister, SIGNAL(completed()),
            this, SLOT(slotDirListerCompleted()));

    activeColumn()->setModel(model);
    QAbstractItemView::setModel(model);
}

void DolphinColumnView::invertSelection()
{
    // TODO: this approach of inverting the selection is quite slow. It should
    // be possible to speedup the implementation by using QItemSelection, but
    // all adempts have failed yet...

    ColumnWidget* column = activeColumn();
    QItemSelectionModel* selModel = column->selectionModel();

    KDirLister* dirLister = m_dolphinModel->dirLister();
    const KFileItemList list = dirLister->itemsForDir(column->url());
    foreach (KFileItem* item, list) {
        const QModelIndex index = m_dolphinModel->indexForUrl(item->url());
        selModel->select(m_proxyModel->mapFromSource(index), QItemSelectionModel::Toggle);
    }
}

void DolphinColumnView::reload()
{
    // Due to the reloading of the model all columns will be reset to show
    // the same content as the first column. As this is not wanted, all columns
    // except of the first column are temporary hidden until the root index can
    // be updated again.
    m_restoreActiveColumnFocus = false;
    QList<ColumnWidget*>::iterator start = m_columns.begin() + 1;
    QList<ColumnWidget*>::iterator end = m_columns.end();
    for (QList<ColumnWidget*>::iterator it = start; it != end; ++it) {
        ColumnWidget* column = (*it);
        if (column->isActive() && column->hasFocus()) {
            // because of hiding the column, it will lose the focus
            // -> remember that the focus should be restored after reloading
            m_restoreActiveColumnFocus = true;
        }
        column->hide();
    }

    // all columns are hidden, now reload the directory lister
    KDirLister* dirLister = m_dolphinModel->dirLister();
    connect(dirLister, SIGNAL(completed()),
            this, SLOT(expandToActiveUrl()));
    const KUrl& rootUrl = m_columns[0]->url();
    dirLister->openUrl(rootUrl, false, true);
    reloadColumns();
}

void DolphinColumnView::showColumn(const KUrl& url)
{
    const KUrl& rootUrl = m_columns[0]->url();
    if (!rootUrl.isParentOf(url)) {
        // the URL is no child URL of the column view, hence clear all columns
        // and reset the root column
        QList<ColumnWidget*>::iterator start = m_columns.begin() + 1;
        QList<ColumnWidget*>::iterator end = m_columns.end();
        for (QList<ColumnWidget*>::iterator it = start; it != end; ++it) {
            (*it)->deleteLater();
        }
        m_columns.erase(start, end);
        m_index = 0;
        m_columns[0]->setActive(true);
        m_columns[0]->setUrl(url);
        assureVisibleActiveColumn();
        return;
    }

    KDirLister* dirLister = m_dolphinModel->dirLister();
    const KUrl dirListerUrl = dirLister->url();
    if (dirListerUrl != rootUrl) {
        // It is possible that root URL of the directory lister is adjusted
        // after creating the column widget (e. g. when restoring the history
        // having a different root URL than the controller indicates).
        m_columns[0]->setUrl(dirListerUrl);
    }

    int columnIndex = 0;
    foreach (ColumnWidget* column, m_columns) {
        if (column->url() == url) {
            // the column represents already the requested URL, hence activate it
            requestActivation(column);
            return;
        } else if (!column->url().isParentOf(url)) {
            // the column is no parent of the requested URL, hence
            // just delete all remaining columns
            if (columnIndex > 0) {
                QList<ColumnWidget*>::iterator start = m_columns.begin() + columnIndex;
                QList<ColumnWidget*>::iterator end = m_columns.end();
                for (QList<ColumnWidget*>::iterator it = start; it != end; ++it) {
                    (*it)->deleteLater();
                }
                m_columns.erase(start, end);

                const int maxIndex = m_columns.count() - 1;
                Q_ASSERT(maxIndex >= 0);
                if (m_index > maxIndex) {
                    m_index = maxIndex;
                }
                break;
            }
        }
        ++columnIndex;
    }

    // Create missing columns. Assuming that the path is "/home/peter/Temp/" and
    // the target path is "/home/peter/Temp/a/b/c/", then the columns "a", "b" and
    // "c" will be created.
    const int lastIndex = m_columns.count() - 1;
    Q_ASSERT(lastIndex >= 0);

    const KUrl& activeUrl = m_columns[lastIndex]->url();
    Q_ASSERT(activeUrl.isParentOf(url));
    Q_ASSERT(activeUrl != url);

    QString path = activeUrl.url(KUrl::AddTrailingSlash);
    const QString targetPath = url.url(KUrl::AddTrailingSlash);

    columnIndex = lastIndex;
    int slashIndex = path.count('/');
    bool hasSubPath = (slashIndex >= 0);
    while (hasSubPath) {
        const QString subPath = targetPath.section('/', slashIndex, slashIndex);
        if (subPath.isEmpty()) {
            hasSubPath = false;
        } else {
            path += subPath + '/';
            ++slashIndex;

            const KUrl childUrl = KUrl(path);
            const QModelIndex dirIndex = m_dolphinModel->indexForUrl(KUrl(path));
            const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);

            m_columns[columnIndex]->setChildUrl(childUrl);
            columnIndex++;

            ColumnWidget* column = new ColumnWidget(viewport(), this, childUrl);
            column->setVerticalScrollMode(ColumnWidget::ScrollPerPixel);
            column->setHorizontalScrollMode(ColumnWidget::ScrollPerPixel);
            column->setModel(model());
            column->setRootIndex(proxyIndex);
            column->setActive(false);

            m_columns.append(column);

            // Before invoking layoutColumns() the column must be set visible temporary.
            // To prevent a flickering the initial geometry is set to a hidden position.
            column->setGeometry(QRect(-1, -1, 1, 1));
            column->show();
            layoutColumns();
            updateScrollBar();

            // the layout is finished, now let the column be invisible until it
            // gets a valid root index due to expandToActiveUrl()
            column->hide();
        }
    }

    // set the last column as active column without modifying the controller
    // and hence the history
    activeColumn()->setActive(false);
    m_index = columnIndex;
    activeColumn()->setActive(true);

    reloadColumns();

    // reloadColumns() is enough for simple use cases where only one column is added.
    // However when exchanging several columns a more complex synchronization must be
    // done by invoking synchronize(). The delay is an optimization for default use
    // cases and gives the directory lister the chance to be already finished when
    // synchronize() is invoked, which assures zero flickering.
    QTimer::singleShot(1000, this, SLOT(synchronize()));
}

void DolphinColumnView::selectAll()
{
    activeColumn()->selectAll();
}

bool DolphinColumnView::isIndexHidden(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return false;//activeColumn()->isIndexHidden(index);
}

QModelIndex DolphinColumnView::moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers)
{
    // Parts of this code have been taken from QColumnView::moveCursor().
    // Copyright (C) 1992-2007 Trolltech ASA.

    Q_UNUSED(modifiers);
    if (model() == 0) {
        return QModelIndex();
    }

    const QModelIndex current = currentIndex();
    if (isRightToLeft()) {
        if (cursorAction == MoveLeft) {
            cursorAction = MoveRight;
        } else if (cursorAction == MoveRight) {
            cursorAction = MoveLeft;
        }
    }

    switch (cursorAction) {
    case MoveLeft:
        if (m_index > 0) {
            setActiveColumnIndex(m_index - 1);
        }
        break;

    case MoveRight:
        if (m_index < m_columns.count() - 1) {
            setActiveColumnIndex(m_index + 1);
        }
        break;

    default:
        break;
    }

    return QModelIndex();
}

void DolphinColumnView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags)
{
    Q_UNUSED(rect);
    Q_UNUSED(flags);
    //activeColumn()->setSelection(rect, flags);
}

QRegion DolphinColumnView::visualRegionForSelection(const QItemSelection& selection) const
{
    Q_UNUSED(selection);
    return QRegion(); //activeColumn()->visualRegionForSelection(selection);
}

int DolphinColumnView::horizontalOffset() const
{
    return -m_contentX;
}

int DolphinColumnView::verticalOffset() const
{
    return 0;
}

void DolphinColumnView::mousePressEvent(QMouseEvent* event)
{
    m_controller->triggerActivation();
    QAbstractItemView::mousePressEvent(event);
}

void DolphinColumnView::resizeEvent(QResizeEvent* event)
{
    QAbstractItemView::resizeEvent(event);
    layoutColumns();
    updateScrollBar();
}

void DolphinColumnView::zoomIn()
{
    if (isZoomInPossible()) {
        ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
        // TODO: get rid of K3Icon sizes
        switch (settings->iconSize()) {
        case K3Icon::SizeSmall:  settings->setIconSize(K3Icon::SizeMedium); break;
        case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeLarge); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinColumnView::zoomOut()
{
    if (isZoomOutPossible()) {
        ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
        // TODO: get rid of K3Icon sizes
        switch (settings->iconSize()) {
        case K3Icon::SizeLarge:  settings->setIconSize(K3Icon::SizeMedium); break;
        case K3Icon::SizeMedium: settings->setIconSize(K3Icon::SizeSmall); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinColumnView::moveContentHorizontally(int x)
{
    m_contentX = -x;
    layoutColumns();
}

void DolphinColumnView::updateDecorationSize()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int iconSize = settings->iconSize();

    foreach (QObject* object, viewport()->children()) {
        if (object->inherits("QListView")) {
            ColumnWidget* widget = static_cast<ColumnWidget*>(object);
            widget->setDecorationSize(QSize(iconSize, iconSize));
        }
    }

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());

    doItemsLayout();
}

void DolphinColumnView::expandToActiveUrl()
{
    disconnect(m_dolphinModel->dirLister(), SIGNAL(completed()),
               this, SLOT(expandToActiveUrl()));

    const int lastIndex = m_columns.count() - 1;
    Q_ASSERT(lastIndex >= 0);
    const KUrl& activeUrl = m_columns[lastIndex]->url();
    const KUrl rootUrl = m_dolphinModel->dirLister()->url();
    const bool expand = rootUrl.isParentOf(activeUrl)
                        && !rootUrl.equals(activeUrl, KUrl::CompareWithoutTrailingSlash);
    if (expand) {
        Q_ASSERT(m_dirListerCompleted);
        m_dolphinModel->expandToUrl(activeUrl);
    }
    reloadColumns();
}

void DolphinColumnView::triggerReloadColumns(const QModelIndex& index)
{
    Q_UNUSED(index);
    // the reloading of the columns may not be done in the context of this slot
    QMetaObject::invokeMethod(this, "reloadColumns", Qt::QueuedConnection);
}

void DolphinColumnView::reloadColumns()
{
    const int end = m_columns.count() - 2; // next to last column
    for (int i = 0; i <= end; ++i) {
        ColumnWidget* nextColumn = m_columns[i + 1];

        const QModelIndex rootIndex = nextColumn->rootIndex();
        if (rootIndex.isValid()) {
            nextColumn->show();
        } else {
            const QModelIndex dirIndex = m_dolphinModel->indexForUrl(m_columns[i]->childUrl());
            const QModelIndex proxyIndex = m_proxyModel->mapFromSource(dirIndex);
            if (proxyIndex.isValid()) {
                nextColumn->setRootIndex(proxyIndex);
                nextColumn->show();
                if (nextColumn->isActive() && m_restoreActiveColumnFocus) {
                    nextColumn->setFocus();
                    m_restoreActiveColumnFocus = false;
                }
            }
        }
    }
    assureVisibleActiveColumn();
}

void DolphinColumnView::synchronize()
{
    if (m_dirListerCompleted) {
        // expanding the active URL may only be done if the directory lister
        // has been completed the loading
        expandToActiveUrl();
    } else {
        reload();
    }
}


void DolphinColumnView::slotDirListerStarted(const KUrl& url)
{
    Q_UNUSED(url);
    m_dirListerCompleted = false;
}

void DolphinColumnView::slotDirListerCompleted()
{
    m_dirListerCompleted = true;
}

bool DolphinColumnView::isZoomInPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() < K3Icon::SizeLarge;
}

bool DolphinColumnView::isZoomOutPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() > K3Icon::SizeSmall;
}

void DolphinColumnView::setActiveColumnIndex(int index)
{
    if (m_index == index) {
        return;
    }

    const bool hasActiveColumn = (m_index >= 0);
    if (hasActiveColumn) {
        m_columns[m_index]->setActive(false);
    }

    m_index = index;
    m_columns[m_index]->setActive(true);

    m_controller->setUrl(m_columns[m_index]->url());
}

void DolphinColumnView::layoutColumns()
{
    int x = m_contentX;
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int columnWidth = settings->columnWidth();
    foreach (ColumnWidget* column, m_columns) {
        column->setGeometry(QRect(x, 0, columnWidth, viewport()->height()));
        x += columnWidth;
    }
}

void DolphinColumnView::updateScrollBar()
{
    int contentWidth = 0;
    foreach (ColumnWidget* column, m_columns) {
        contentWidth += column->width();
    }

    horizontalScrollBar()->setPageStep(contentWidth);
    horizontalScrollBar()->setRange(0, contentWidth - viewport()->width());
}

void DolphinColumnView::assureVisibleActiveColumn()
{
    const int viewportWidth = viewport()->width();
    const int x = activeColumn()->x();
    const int width = activeColumn()->width();
    if (x + width > viewportWidth) {
        int newContentX = m_contentX - x - width + viewportWidth;
        if (newContentX > 0) {
            newContentX = 0;
        }
        m_animation->setFrameRange(-m_contentX, -newContentX);
        m_animation->start();
    } else if (x < 0) {
        const int newContentX = m_contentX - x;
        m_animation->setFrameRange(-m_contentX, -newContentX);
        m_animation->start();
    }
}

void DolphinColumnView::requestActivation(ColumnWidget* column)
{
    if (column->isActive()) {
        assureVisibleActiveColumn();
    } else {
        int index = 0;
        foreach (ColumnWidget* currColumn, m_columns) {
            if (currColumn == column) {
                setActiveColumnIndex(index);
                assureVisibleActiveColumn();
                return;
            }
            ++index;
        }
    }
}

#include "dolphincolumnview.moc"
