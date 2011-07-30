/*******************************************************************************
 *   Copyright (C) 2008 by Konstantin Heil <konst.heil@stud.uni-heidelberg.de> *
 *                                                                             *
 *   This program is free software; you can redistribute it and/or modify      *
 *   it under the terms of the GNU General Public License as published by      *
 *   the Free Software Foundation; either version 2 of the License, or         *
 *   (at your option) any later version.                                       *
 *                                                                             *
 *   This program is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the             *
 *   GNU General Public License for more details.                              *
 *                                                                             *
 *   You should have received a copy of the GNU General Public License         *
 *   along with this program; if not, write to the                             *
 *   Free Software Foundation, Inc.,                                           *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA                *
 *******************************************************************************/

#include "tooltipmanager.h"

#include "filemetadatatooltip.h"
#include <KIcon>
#include <KIO/PreviewJob>
#include <KSharedConfig>

#include <QApplication>
#include <QDesktopWidget>
#include <QLayout>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>

ToolTipManager::ToolTipManager(QWidget* parent) :
    QObject(parent),
    m_view(parent),
    m_showToolTipTimer(0),
    m_contentRetrievalTimer(0),
    m_fileMetaDataToolTip(0),
    m_toolTipRequested(false),
    m_metaDataRequested(false),
    m_appliedWaitCursor(false),
    m_item(),
    m_itemRect()
{
    //m_dolphinModel = static_cast<DolphinModel*>(m_proxyModel->sourceModel());
    //connect(parent, SIGNAL(entered(const QModelIndex&)),
    //        this, SLOT(requestToolTip(const QModelIndex&)));
    //connect(parent, SIGNAL(viewportEntered()),
    //        this, SLOT(hideToolTip()));

    // Initialize timers
    m_showToolTipTimer = new QTimer(this);
    m_showToolTipTimer->setSingleShot(true);
    m_showToolTipTimer->setInterval(500);
    connect(m_showToolTipTimer, SIGNAL(timeout()), this, SLOT(showToolTip()));

    m_contentRetrievalTimer = new QTimer(this);
    m_contentRetrievalTimer->setSingleShot(true);
    m_contentRetrievalTimer->setInterval(200);
    connect(m_contentRetrievalTimer, SIGNAL(timeout()), this, SLOT(startContentRetrieval()));

    Q_ASSERT(m_contentRetrievalTimer->interval() < m_showToolTipTimer->interval());

    // When the mousewheel is used, the items don't get a hovered indication
    // (Qt-issue #200665). To assure that the tooltip still gets hidden,
    // the scrollbars are observed.
    /*connect(parent->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideToolTip()));
    connect(parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideToolTip()));*/

    Q_ASSERT(m_view);
    //m_view->viewport()->installEventFilter(this);
    //m_view->installEventFilter(this);
}

ToolTipManager::~ToolTipManager()
{
}

void ToolTipManager::hideToolTip()
{
    if (m_appliedWaitCursor) {
        QApplication::restoreOverrideCursor();
        m_appliedWaitCursor = false;
    }

    m_toolTipRequested = false;
    m_metaDataRequested = false;
    m_showToolTipTimer->stop();
    m_contentRetrievalTimer->stop();

    delete m_fileMetaDataToolTip;
    m_fileMetaDataToolTip = 0;
}


bool ToolTipManager::eventFilter(QObject* watched, QEvent* event)
{
    /*if (watched == m_view->viewport()) {
        switch (event->type()) {
        case QEvent::Leave:
        case QEvent::MouseButtonPress:
            hideToolTip();
            break;
        default:
            break;
        }
    } else if ((watched == m_view) && (event->type() == QEvent::KeyPress)) {
        hideToolTip();
    }*/

    return QObject::eventFilter(watched, event);
}

void ToolTipManager::requestToolTip(const QModelIndex& index)
{
    Q_UNUSED(index);
    hideToolTip();

    // Only request a tooltip for the name column and when no selection or
    // drag & drop operation is done (indicated by the left mouse button)
    if (!(QApplication::mouseButtons() & Qt::LeftButton)) {
        m_itemRect = QRect(); //m_view->visualRect(index);
        const QPoint pos; // = m_view->viewport()->mapToGlobal(m_itemRect.topLeft());
        m_itemRect.moveTo(pos);

        //const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        //m_item = m_dolphinModel->itemForIndex(dirIndex);

        // Only start the retrieving of the content, when the mouse has been over this
        // item for 200 milliseconds. This prevents a lot of useless preview jobs and
        // meta data retrieval, when passing rapidly over a lot of items.
        Q_ASSERT(!m_fileMetaDataToolTip);
        m_fileMetaDataToolTip = new FileMetaDataToolTip(m_view);
        connect(m_fileMetaDataToolTip, SIGNAL(metaDataRequestFinished(KFileItemList)),
                this, SLOT(slotMetaDataRequestFinished()));

        m_contentRetrievalTimer->start();
        m_showToolTipTimer->start();
        m_toolTipRequested = true;
        Q_ASSERT(!m_metaDataRequested);
    }
}

void ToolTipManager::startContentRetrieval()
{
    if (!m_toolTipRequested) {
        return;
    }

    m_fileMetaDataToolTip->setName(m_item.text());

    // Request the retrieval of meta-data. The slot
    // slotMetaDataRequestFinished() is invoked after the
    // meta-data have been received.
    m_metaDataRequested = true;
    m_fileMetaDataToolTip->setItems(KFileItemList() << m_item);
    m_fileMetaDataToolTip->adjustSize();

    // Request a preview of the item
    m_fileMetaDataToolTip->setPreview(QPixmap());

    KIO::PreviewJob* job = KIO::filePreview(KFileItemList() << m_item, QSize(256, 256));

    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            this, SLOT(setPreviewPix(const KFileItem&, const QPixmap&)));
    connect(job, SIGNAL(failed(const KFileItem&)),
            this, SLOT(previewFailed()));
}


void ToolTipManager::setPreviewPix(const KFileItem& item,
                                   const QPixmap& pixmap)
{
    if (!m_toolTipRequested || (m_item.url() != item.url())) {
        // No tooltip is requested anymore or an old preview has been received
        return;
    }

    if (pixmap.isNull()) {
        previewFailed();
    } else {
        m_fileMetaDataToolTip->setPreview(pixmap);
        if (!m_showToolTipTimer->isActive()) {
            showToolTip();
        }
    }
}

void ToolTipManager::previewFailed()
{
    if (!m_toolTipRequested) {
        return;
    }

    const QPixmap pixmap = KIcon(m_item.iconName()).pixmap(128, 128);
    m_fileMetaDataToolTip->setPreview(pixmap);
    if (!m_showToolTipTimer->isActive()) {
        showToolTip();
    }
}

void ToolTipManager::slotMetaDataRequestFinished()
{
    if (!m_toolTipRequested) {
        return;
    }

    m_metaDataRequested = false;

    if (!m_showToolTipTimer->isActive()) {
        showToolTip();
    }
}

void ToolTipManager::showToolTip()
{
    Q_ASSERT(m_toolTipRequested);
    if (m_appliedWaitCursor) {
        QApplication::restoreOverrideCursor();
        m_appliedWaitCursor = false;
    }

    if (QApplication::mouseButtons() & Qt::LeftButton) {
        return;
    }

    if (m_fileMetaDataToolTip->preview().isNull() || m_metaDataRequested) {
        Q_ASSERT(!m_appliedWaitCursor);
        QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
        m_appliedWaitCursor = true;
        return;
    }

    const QRect screen = QApplication::desktop()->screenGeometry(QCursor::pos());

    // Restrict tooltip size to current screen size when needed.
    // Because layout controlling widget doesn't respect widget's maximumSize property
    // (correct me if I'm wrong), we need to let layout do its work, then manually change
    // geometry if resulting widget doesn't fit the screen.

    // Step #1 - make sizeHint return calculated tooltip size
    m_fileMetaDataToolTip->layout()->setSizeConstraint(QLayout::SetFixedSize);
    m_fileMetaDataToolTip->adjustSize();
    QSize size = m_fileMetaDataToolTip->sizeHint();

    // Step #2 - correct tooltip size when needed
    if (size.width() > screen.width()) {
        size.setWidth(screen.width());
    }
    if (size.height() > screen.height()) {
        size.setHeight(screen.height());
    }

    // m_itemRect defines the area of the item, where the tooltip should be
    // shown. Per default the tooltip is shown centered at the bottom.
    // It must be assured that:
    // - the content is fully visible
    // - the content is not drawn inside m_itemRect
    const int margin = 3;
    const bool hasRoomToLeft  = (m_itemRect.left()   - size.width()  - margin >= screen.left());
    const bool hasRoomToRight = (m_itemRect.right()  + size.width()  + margin <= screen.right());
    const bool hasRoomAbove   = (m_itemRect.top()    - size.height() - margin >= screen.top());
    const bool hasRoomBelow   = (m_itemRect.bottom() + size.height() + margin <= screen.bottom());
    if (!hasRoomAbove && !hasRoomBelow && !hasRoomToLeft && !hasRoomToRight) {
        return;
    }

    int x, y;
    if (hasRoomBelow || hasRoomAbove) {
        x = qMax(screen.left(), m_itemRect.center().x() - size.width() / 2);
        if (x + size.width() >= screen.right()) {
            x = screen.right() - size.width() + 1;
        }
        if (hasRoomBelow) {
            y = m_itemRect.bottom() + margin;
        } else {
            y = m_itemRect.top() - size.height() - margin;
        }
    } else {
        Q_ASSERT(hasRoomToLeft || hasRoomToRight);
        if (hasRoomToRight) {
            x = m_itemRect.right() + margin;
        } else {
            x = m_itemRect.left() - size.width() - margin;
        }
        // Put the tooltip at the bottom of the screen. The x-coordinate has already
        // been adjusted, so that no overlapping with m_itemRect occurs.
        y = screen.bottom() - size.height() + 1;
    }

    // Step #3 - Alter tooltip geometry
    m_fileMetaDataToolTip->setFixedSize(size);
    m_fileMetaDataToolTip->layout()->setSizeConstraint(QLayout::SetNoConstraint);
    m_fileMetaDataToolTip->move(QPoint(x, y));
    m_fileMetaDataToolTip->show();

    m_toolTipRequested = false;
}

#include "tooltipmanager.moc"
