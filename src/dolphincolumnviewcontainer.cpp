/***************************************************************************
 *   Copyright (C) 2007-2009 by Peter Penz <peter.penz@gmx.at>             *
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

#include "dolphincolumnviewcontainer.h"

#include "dolphincolumnview.h"
#include "dolphincontroller.h"
#include "dolphinsortfilterproxymodel.h"
#include "settings/dolphinsettings.h"

#include "dolphin_columnmodesettings.h"

#include <kfilepreviewgenerator.h>

#include <QPoint>
#include <QScrollBar>
#include <QTimeLine>

DolphinColumnViewContainer::DolphinColumnViewContainer(QWidget* parent, DolphinController* controller) :
    QScrollArea(parent),
    m_controller(controller),
    m_active(false),
    m_index(-1),
    m_contentX(0),
    m_columns(),
    m_emptyViewport(0),
    m_animation(0)
{
    Q_ASSERT(controller != 0);

    setAcceptDrops(true);
    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::NoFrame);
    setLayoutDirection(Qt::LeftToRight);

    connect(this, SIGNAL(viewportEntered()),
            controller, SLOT(emitViewportEntered()));
    connect(controller, SIGNAL(activationChanged(bool)),
            this, SLOT(updateColumnsBackground(bool)));

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(moveContentHorizontally(int)));

    m_animation = new QTimeLine(500, this);
    connect(m_animation, SIGNAL(frameChanged(int)), horizontalScrollBar(), SLOT(setValue(int)));

    DolphinColumnView* column = new DolphinColumnView(viewport(), this, m_controller->url());
    m_columns.append(column);
    setActiveColumnIndex(0);

    m_emptyViewport = new QFrame(viewport());
    m_emptyViewport->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    updateColumnsBackground(true);
}

DolphinColumnViewContainer::~DolphinColumnViewContainer()
{
}

KUrl DolphinColumnViewContainer::rootUrl() const
{
    return m_columns[0]->url();
}

QAbstractItemView* DolphinColumnViewContainer::activeColumn() const
{
    return m_columns[m_index];
}

bool DolphinColumnViewContainer::showColumn(const KUrl& url)
{
    if (!rootUrl().isParentOf(url)) {
        removeAllColumns();
        m_columns[0]->setUrl(url);
        return false;
    }

    int columnIndex = 0;
    foreach (DolphinColumnView* column, m_columns) {
        if (column->url() == url) {
            // the column represents already the requested URL, hence activate it
            requestActivation(column);
            layoutColumns();
            return false;
        } else if (!column->url().isParentOf(url)) {
            // the column is no parent of the requested URL, hence
            // just delete all remaining columns
            if (columnIndex > 0) {
                QList<DolphinColumnView*>::iterator start = m_columns.begin() + columnIndex;
                QList<DolphinColumnView*>::iterator end = m_columns.end();
                for (QList<DolphinColumnView*>::iterator it = start; it != end; ++it) {
                    deleteColumn(*it);
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

            DolphinColumnView* column = new DolphinColumnView(viewport(), this, childUrl);
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
    m_columns[m_index]->setActive(false);
    m_index = columnIndex;
    m_columns[m_index]->setActive(true);
    assureVisibleActiveColumn();

    return true;
}

void DolphinColumnViewContainer::mousePressEvent(QMouseEvent* event)
{
    m_controller->requestActivation();
    QScrollArea::mousePressEvent(event);
}

void DolphinColumnViewContainer::resizeEvent(QResizeEvent* event)
{
    QScrollArea::resizeEvent(event);
    layoutColumns();
    updateScrollBar();
    assureVisibleActiveColumn();
}

void DolphinColumnViewContainer::wheelEvent(QWheelEvent* event)
{
    // let Ctrl+wheel events propagate to the DolphinView for icon zooming
    if ((event->modifiers() & Qt::ControlModifier) == Qt::ControlModifier) {
        event->ignore();
    } else {
        QScrollArea::wheelEvent(event);
    }
}

void DolphinColumnViewContainer::moveContentHorizontally(int x)
{
    m_contentX = isRightToLeft() ? +x : -x;
    layoutColumns();
}

void DolphinColumnViewContainer::updateColumnsBackground(bool active)
{
    if (active == m_active) {
        return;
    }

    m_active = active;

    // dim the background of the viewport
    const QPalette::ColorRole role = viewport()->backgroundRole();
    QColor background = viewport()->palette().color(role);
    background.setAlpha(0);  // make background transparent

    QPalette palette = viewport()->palette();
    palette.setColor(role, background);
    viewport()->setPalette(palette);

    foreach (DolphinColumnView* column, m_columns) {
        column->updateBackground();
    }
}

void DolphinColumnViewContainer::setActiveColumnIndex(int index)
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

    assureVisibleActiveColumn();
}

void DolphinColumnViewContainer::layoutColumns()
{
    const int gap = 4;

    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int columnWidth = settings->columnWidth();

    QRect emptyViewportRect;
    if (isRightToLeft()) {
        int x = viewport()->width() - columnWidth + m_contentX;
        foreach (DolphinColumnView* column, m_columns) {
            column->setGeometry(QRect(x, 0, columnWidth - gap, viewport()->height()));
            x -= columnWidth;
        }
        emptyViewportRect = QRect(0, 0, x + columnWidth - gap, viewport()->height());
    } else {
        int x = m_contentX;
        foreach (DolphinColumnView* column, m_columns) {
            column->setGeometry(QRect(x, 0, columnWidth - gap, viewport()->height()));
            x += columnWidth;
        }
        emptyViewportRect = QRect(x, 0, viewport()->width() - x - gap, viewport()->height());
    }

    if (emptyViewportRect.isValid()) {
        m_emptyViewport->show();
        m_emptyViewport->setGeometry(emptyViewportRect);
    } else {
        m_emptyViewport->hide();
    }
}

void DolphinColumnViewContainer::updateScrollBar()
{
    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int contentWidth = m_columns.count() * settings->columnWidth();

    horizontalScrollBar()->setPageStep(contentWidth);
    horizontalScrollBar()->setRange(0, contentWidth - viewport()->width());
}

void DolphinColumnViewContainer::assureVisibleActiveColumn()
{
    const int viewportWidth = viewport()->width();
    const int x = activeColumn()->x();

    ColumnModeSettings* settings = DolphinSettings::instance().columnModeSettings();
    const int width = settings->columnWidth();

    if (x + width > viewportWidth) {
        const int newContentX = m_contentX - x - width + viewportWidth;
        if (isRightToLeft()) {
            m_animation->setFrameRange(m_contentX, newContentX);
        } else {
            m_animation->setFrameRange(-m_contentX, -newContentX);
        }
        if (m_animation->state() != QTimeLine::Running) {
           m_animation->start();
        }
    } else if (x < 0) {
        const int newContentX = m_contentX - x;
        if (isRightToLeft()) {
            m_animation->setFrameRange(m_contentX, newContentX);
        } else {
            m_animation->setFrameRange(-m_contentX, -newContentX);
        }
        if (m_animation->state() != QTimeLine::Running) {
           m_animation->start();
        }
    }
}

void DolphinColumnViewContainer::requestActivation(DolphinColumnView* column)
{
    m_controller->setItemView(column);
    if (column->isActive()) {
        assureVisibleActiveColumn();
    } else {
        int index = 0;
        foreach (DolphinColumnView* currColumn, m_columns) {
            if (currColumn == column) {
                setActiveColumnIndex(index);
                return;
            }
            ++index;
        }
    }
}

void DolphinColumnViewContainer::removeAllColumns()
{
    QList<DolphinColumnView*>::iterator start = m_columns.begin() + 1;
    QList<DolphinColumnView*>::iterator end = m_columns.end();
    for (QList<DolphinColumnView*>::iterator it = start; it != end; ++it) {
        deleteColumn(*it);
    }
    m_columns.erase(start, end);
    m_index = 0;
    m_columns[0]->setActive(true);
    assureVisibleActiveColumn();
}

QPoint DolphinColumnViewContainer::columnPosition(DolphinColumnView* column, const QPoint& point) const
{
    const QPoint topLeft = column->frameGeometry().topLeft();
    return QPoint(point.x() - topLeft.x(), point.y() - topLeft.y());
}

void DolphinColumnViewContainer::deleteColumn(DolphinColumnView* column)
{
    if (column != 0) {
        if (m_controller->itemView() == column) {
            m_controller->setItemView(0);
        }
        // deleteWhenNotDragSource(column) does not necessarily delete column,
        // and we want its preview generator destroyed immediately.
        column->m_previewGenerator->deleteLater();
        column->m_previewGenerator = 0;
        column->hide();
        // Prevent automatic destruction of column when this DolphinColumnViewContainer
        // is destroyed.
        column->setParent(0);
        column->disconnect();
        emit requestColumnDeletion(column);
    }
}

#include "dolphincolumnviewcontainer.moc"
