/***************************************************************************
 *   Copyright (C) 2007-2009 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "dolphin_columnmodesettings.h"

#include "dolphincolumnview.h"
#include "dolphinviewcontroller.h"
#include "dolphinsortfilterproxymodel.h"
#include "draganddrophelper.h"
#include "settings/dolphinsettings.h"
#include "viewmodecontroller.h"

#include <QPoint>
#include <QScrollBar>
#include <QTimeLine>
#include <QTimer>

DolphinColumnViewContainer::DolphinColumnViewContainer(QWidget* parent,
                                                       DolphinViewController* dolphinViewController,
                                                       const ViewModeController* viewModeController) :
    QScrollArea(parent),
    m_dolphinViewController(dolphinViewController),
    m_viewModeController(viewModeController),
    m_active(false),
    m_index(-1),
    m_contentX(0),
    m_columns(),
    m_emptyViewport(0),
    m_animation(0),
    m_dragSource(0),
    m_activeUrlTimer(0),
    m_assureVisibleActiveColumnTimer(0)
{
    Q_ASSERT(dolphinViewController != 0);
    Q_ASSERT(viewModeController != 0);

    setAcceptDrops(true);
    setFocusPolicy(Qt::NoFocus);
    setFrameShape(QFrame::NoFrame);
    setLayoutDirection(Qt::LeftToRight);

    connect(viewModeController, SIGNAL(activationChanged(bool)),
            this, SLOT(updateColumnsBackground(bool)));

    connect(horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(moveContentHorizontally(int)));

    m_animation = new QTimeLine(500, this);
    connect(m_animation, SIGNAL(frameChanged(int)), horizontalScrollBar(), SLOT(setValue(int)));

    m_activeUrlTimer = new QTimer(this);
    m_activeUrlTimer->setSingleShot(true);
    m_activeUrlTimer->setInterval(200);
    connect(m_activeUrlTimer, SIGNAL(timeout()),
            this, SLOT(updateActiveUrl()));

    // Assuring that the active column gets fully visible is done with a small delay. This
    // prevents that for temporary activations an animation gets started (e. g. when clicking
    // on any folder of the parent column, the child column gets activated).
    m_assureVisibleActiveColumnTimer = new QTimer(this);
    m_assureVisibleActiveColumnTimer->setSingleShot(true);
    m_assureVisibleActiveColumnTimer->setInterval(200);
    connect(m_assureVisibleActiveColumnTimer, SIGNAL(timeout()),
            this, SLOT(slotAssureVisibleActiveColumn()));

    DolphinColumnView* column = new DolphinColumnView(viewport(), this, viewModeController->url());
    m_columns.append(column);
    requestActivation(column);

    m_emptyViewport = new QFrame(viewport());
    m_emptyViewport->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);

    updateColumnsBackground(true);
}

DolphinColumnViewContainer::~DolphinColumnViewContainer()
{
    delete m_dragSource;
    m_dragSource = 0;
}

KUrl DolphinColumnViewContainer::rootUrl() const
{
    return m_columns[0]->url();
}

QAbstractItemView* DolphinColumnViewContainer::activeColumn() const
{
    return m_columns[m_index];
}

void DolphinColumnViewContainer::showColumn(const KUrl& url)
{
    if (!rootUrl().isParentOf(url)) {
        removeAllColumns();
        m_columns[0]->setUrl(url);
        return;
    }

    int columnIndex = 0;
    foreach (DolphinColumnView* column, m_columns) {
        if (column->url().equals(url, KUrl::CompareWithoutTrailingSlash)) {
            // the column represents already the requested URL, hence activate it
            requestActivation(column);
            layoutColumns();
            return;
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
            m_columns.append(column);

            // Before invoking layoutColumns() the column must be set visible temporary.
            // To prevent a flickering the initial geometry is set to a hidden position.
            column->setGeometry(QRect(-1, -1, 1, 1));
            column->show();
            layoutColumns();
        }
    }

    requestActivation(m_columns[columnIndex]);
}

void DolphinColumnViewContainer::mousePressEvent(QMouseEvent* event)
{
    m_dolphinViewController->requestActivation();
    QScrollArea::mousePressEvent(event);
}

void DolphinColumnViewContainer::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Left) {
        if (m_index > 0) {
            requestActivation(m_columns[m_index - 1]);
        }
    } else {
        QScrollArea::keyPressEvent(event);
    }
}

void DolphinColumnViewContainer::resizeEvent(QResizeEvent* event)
{
    QScrollArea::resizeEvent(event);
    layoutColumns();
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

void DolphinColumnViewContainer::updateActiveUrl()
{
    const KUrl activeUrl = m_columns[m_index]->url();
    m_dolphinViewController->requestUrlChange(activeUrl);
}

void DolphinColumnViewContainer::slotAssureVisibleActiveColumn()
{
    const int viewportWidth = viewport()->width();
    const int x = activeColumn()->x();

    // When a column that is partly visible gets activated,
    // it is useful to also assure that the neighbor column is partly visible.
    // This allows the user to scroll to the first/last column without using the
    // scrollbar and drag & drop operations to invisible columns.
    const int neighborColumnGap = 3 * style()->pixelMetric(QStyle::PM_ScrollBarExtent, 0, verticalScrollBar());

    const int width = activeColumn()->maximumWidth();
    if (x + width > viewportWidth) {
        const int newContentX = m_contentX - x - width + viewportWidth;
        if (isRightToLeft()) {
            m_animation->setFrameRange(m_contentX, newContentX + neighborColumnGap);
        } else {
            m_animation->setFrameRange(-m_contentX, -newContentX + neighborColumnGap);
        }
        if (m_animation->state() != QTimeLine::Running) {
           m_animation->start();
        }
    } else if (x < 0) {
        const int newContentX = m_contentX - x;
        if (isRightToLeft()) {
            m_animation->setFrameRange(m_contentX, newContentX - neighborColumnGap);
        } else {
            m_animation->setFrameRange(-m_contentX, -newContentX - neighborColumnGap);
        }
        if (m_animation->state() != QTimeLine::Running) {
           m_animation->start();
        }
    }
}

void DolphinColumnViewContainer::assureVisibleActiveColumn()
{
    m_assureVisibleActiveColumnTimer->start();
}

void DolphinColumnViewContainer::layoutColumns()
{
    // Layout the position of the columns corresponding to their maximum width
    QRect emptyViewportRect;
    if (isRightToLeft()) {
        int columnWidth = m_columns[0]->maximumWidth();
        int x = viewport()->width() - columnWidth + m_contentX;
        foreach (DolphinColumnView* column, m_columns) {
            columnWidth = column->maximumWidth();
            column->setGeometry(QRect(x, 0, columnWidth, viewport()->height()));
            x -= columnWidth;
        }
        emptyViewportRect = QRect(0, 0, x + columnWidth, viewport()->height());
    } else {
        int x = m_contentX;
        foreach (DolphinColumnView* column, m_columns) {
            const int columnWidth = column->maximumWidth();
            column->setGeometry(QRect(x, 0, columnWidth, viewport()->height()));
            x += columnWidth;
        }
        emptyViewportRect = QRect(x, 0, viewport()->width() - x, viewport()->height());
    }

    // Show an empty viewport if the columns don't cover the whole viewport
    if (emptyViewportRect.isValid()) {
        m_emptyViewport->show();
        m_emptyViewport->setGeometry(emptyViewportRect);
    } else {
        m_emptyViewport->hide();
    }

    // Update the horizontal position indicator
    int contentWidth = 0;
    foreach (DolphinColumnView* column, m_columns) {
        contentWidth += column->maximumWidth();
    }

    const int scrollBarMax = contentWidth - viewport()->width();
    const bool updateScrollBar =    (horizontalScrollBar()->pageStep() != contentWidth)
                                 || (horizontalScrollBar()->maximum()  != scrollBarMax);
    if (updateScrollBar) {
        horizontalScrollBar()->setPageStep(contentWidth);
        horizontalScrollBar()->setRange(0, scrollBarMax);
    }
}

void DolphinColumnViewContainer::requestActivation(DolphinColumnView* column)
{
    if (m_dolphinViewController->itemView() != column) {
        m_dolphinViewController->setItemView(column);
    }
    if (focusProxy() != column) {
        setFocusProxy(column);
    }

    if (!column->isActive()) {
        // Deactivate the currently active column
        if (m_index >= 0) {
            m_columns[m_index]->setActive(false);
        }

        // Get the index of the column that should get activated
        int index = 0;
        foreach (DolphinColumnView* currColumn, m_columns) {
            if (currColumn == column) {
                break;
            }
            ++index;
        }

        Q_ASSERT(index != m_index);
        Q_ASSERT(index < m_columns.count());

        // Activate the requested column
        m_index = index;
        m_columns[m_index]->setActive(true);

        m_activeUrlTimer->start(); // calls slot updateActiveUrl()
    }

    assureVisibleActiveColumn();
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

void DolphinColumnViewContainer::deleteColumn(DolphinColumnView* column)
{
    if (column == 0) {
        return;
    }

    if (m_dolphinViewController->itemView() == column) {
        m_dolphinViewController->setItemView(0);
    }
    // deleteWhenNotDragSource(column) does not necessarily delete column,
    // and we want its preview generator destroyed immediately.
    column->hide();
    // Prevent automatic destruction of column when this DolphinColumnViewContainer
    // is destroyed.
    column->setParent(0);
    column->disconnect();

    if (DragAndDropHelper::instance().isDragSource(column)) {
        // The column is a drag source (the feature "Open folders
        // during drag operations" is used). Deleting the view
        // during an ongoing drag operation is not allowed, so
        // this will postponed.
        if (m_dragSource != 0) {
            // the old stored view is obviously not the drag source anymore
            m_dragSource->deleteLater();
            m_dragSource = 0;
        }
        m_dragSource = column;
    } else {
        delete column;
        column = 0;
    }
}

#include "dolphincolumnviewcontainer.moc"
