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
#include "dolphincolumnwidget.h"
#include "dolphincontroller.h"
#include "dolphindirlister.h"
#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"
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

DolphinColumnView::DolphinColumnView(QWidget* parent, DolphinController* controller) :
    QAbstractItemView(parent),
    m_controller(controller),
    m_restoreActiveColumnFocus(false),
    m_index(-1),
    m_contentX(0),
    m_columns(),
    m_animation(0)
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
    connect(controller, SIGNAL(showHiddenFilesChanged(bool)),
            this, SLOT(slotShowHiddenFilesChanged(bool)));
    connect(controller, SIGNAL(showPreviewChanged(bool)),
            this, SLOT(slotShowPreviewChanged(bool)));

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(moveContentHorizontally(int)));

    m_animation = new QTimeLine(500, this);
    connect(m_animation, SIGNAL(frameChanged(int)), horizontalScrollBar(), SLOT(setValue(int)));

    DolphinColumnWidget* column = new DolphinColumnWidget(viewport(), this, m_controller->url());
    m_columns.append(column);
    setActiveColumnIndex(0);

    updateDecorationSize();

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
    foreach (DolphinColumnWidget* column, m_columns) {
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

void DolphinColumnView::invertSelection()
{
    QItemSelectionModel* selectionModel = activeColumn()->selectionModel();
    const QAbstractItemModel* itemModel = selectionModel->model();

    const QModelIndex topLeft = itemModel->index(0, 0);
    const QModelIndex bottomRight = itemModel->index(itemModel->rowCount() - 1,
                                                     itemModel->columnCount() - 1);

    const QItemSelection selection(topLeft, bottomRight);
    selectionModel->select(selection, QItemSelectionModel::Toggle);
}

void DolphinColumnView::reload()
{
    foreach (DolphinColumnWidget* column, m_columns) {
        column->reload();
    }
}

void DolphinColumnView::setRootUrl(const KUrl& url)
{
    removeAllColumns();
    m_columns[0]->setUrl(url);
}

KUrl DolphinColumnView::rootUrl() const
{
    return m_columns[0]->url();
}

void DolphinColumnView::showColumn(const KUrl& url)
{
    if (!rootUrl().isParentOf(url)) {
        setRootUrl(url);
        return;
    }

    int columnIndex = 0;
    foreach (DolphinColumnWidget* column, m_columns) {
        if (column->url() == url) {
            // the column represents already the requested URL, hence activate it
            requestActivation(column);
            return;
        } else if (!column->url().isParentOf(url)) {
            // the column is no parent of the requested URL, hence
            // just delete all remaining columns
            if (columnIndex > 0) {
                QList<DolphinColumnWidget*>::iterator start = m_columns.begin() + columnIndex;
                QList<DolphinColumnWidget*>::iterator end = m_columns.end();
                for (QList<DolphinColumnWidget*>::iterator it = start; it != end; ++it) {
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
            m_columns[columnIndex]->setChildUrl(childUrl);
            columnIndex++;

            DolphinColumnWidget* column = new DolphinColumnWidget(viewport(), this, childUrl);
            column->setActive(false);

            m_columns.append(column);

            // Before invoking layoutColumns() the column must be set visible temporary.
            // To prevent a flickering the initial geometry is set to a hidden position.
            column->setGeometry(QRect(-1, -1, 1, 1));
            column->show();
            layoutColumns();
            updateScrollBar();
        }
    }

    // set the last column as active column without modifying the controller
    // and hence the history
    activeColumn()->setActive(false);
    m_index = columnIndex;
    activeColumn()->setActive(true);
    assureVisibleActiveColumn();
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
        switch (settings->iconSize()) {
        case KIconLoader::SizeSmall:  settings->setIconSize(KIconLoader::SizeMedium); break;
        case KIconLoader::SizeMedium: settings->setIconSize(KIconLoader::SizeLarge); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinColumnView::zoomOut()
{
    if (isZoomOutPossible()) {
        ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
        switch (settings->iconSize()) {
        case KIconLoader::SizeLarge:  settings->setIconSize(KIconLoader::SizeMedium); break;
        case KIconLoader::SizeMedium: settings->setIconSize(KIconLoader::SizeSmall); break;
        default: Q_ASSERT(false); break;
        }
        updateDecorationSize();
    }
}

void DolphinColumnView::moveContentHorizontally(int x)
{
    m_contentX = isRightToLeft() ? +x : -x;
    layoutColumns();
}

void DolphinColumnView::updateDecorationSize()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int iconSize = settings->iconSize();

    foreach (QObject* object, viewport()->children()) {
        if (object->inherits("QListView")) {
            DolphinColumnWidget* widget = static_cast<DolphinColumnWidget*>(object);
            widget->setDecorationSize(QSize(iconSize, iconSize));
        }
    }

    m_controller->setZoomInPossible(isZoomInPossible());
    m_controller->setZoomOutPossible(isZoomOutPossible());

    doItemsLayout();
}

void DolphinColumnView::slotShowHiddenFilesChanged(bool show)
{
    foreach (DolphinColumnWidget* column, m_columns) {
        column->setShowHiddenFiles(show);
    }
}

void DolphinColumnView::slotShowPreviewChanged(bool show)
{
    foreach (DolphinColumnWidget* column, m_columns) {
        column->setShowPreview(show);
    }
}

bool DolphinColumnView::isZoomInPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() < KIconLoader::SizeLarge;
}

bool DolphinColumnView::isZoomOutPossible() const
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    return settings->iconSize() > KIconLoader::SizeSmall;
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

    assureVisibleActiveColumn();
}

void DolphinColumnView::layoutColumns()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int columnWidth = settings->columnWidth();
    if (isRightToLeft()) {
        int x = viewport()->width() - columnWidth + m_contentX;
        foreach (DolphinColumnWidget* column, m_columns) {
            column->setGeometry(QRect(x, 0, columnWidth, viewport()->height()));
            x -= columnWidth;
        }
    } else {
        int x = m_contentX;
        foreach (DolphinColumnWidget* column, m_columns) {
            column->setGeometry(QRect(x, 0, columnWidth, viewport()->height()));
            x += columnWidth;
        }
    }
}

void DolphinColumnView::updateScrollBar()
{
    int contentWidth = 0;
    foreach (DolphinColumnWidget* column, m_columns) {
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
        const int newContentX = m_contentX - x - width + viewportWidth;
        if (isRightToLeft()) {
            m_animation->setFrameRange(m_contentX, newContentX);
        } else {
            m_animation->setFrameRange(-m_contentX, -newContentX);
        }
        m_animation->start();
    } else if (x < 0) {
        const int newContentX = m_contentX - x;
        if (isRightToLeft()) {
            m_animation->setFrameRange(m_contentX, newContentX);
        } else {
            m_animation->setFrameRange(-m_contentX, -newContentX);
        }
        m_animation->start();
    }
}

void DolphinColumnView::requestActivation(DolphinColumnWidget* column)
{
    if (column->isActive()) {
        assureVisibleActiveColumn();
    } else {
        int index = 0;
        foreach (DolphinColumnWidget* currColumn, m_columns) {
            if (currColumn == column) {
                setActiveColumnIndex(index);
                return;
            }
            ++index;
        }
    }
}

void DolphinColumnView::removeAllColumns()
{
    QList<DolphinColumnWidget*>::iterator start = m_columns.begin() + 1;
    QList<DolphinColumnWidget*>::iterator end = m_columns.end();
    for (QList<DolphinColumnWidget*>::iterator it = start; it != end; ++it) {
        (*it)->deleteLater();
    }
    m_columns.erase(start, end);
    m_index = 0;
    m_columns[0]->setActive(true);
    assureVisibleActiveColumn();
}

#include "dolphincolumnview.moc"
