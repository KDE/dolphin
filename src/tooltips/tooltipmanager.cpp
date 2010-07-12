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

#include "dolphinmodel.h"
#include "dolphinsortfilterproxymodel.h"

#include "filemetadatatooltip.h"
#include <kicon.h>
#include <kio/previewjob.h>

#include <QApplication>
#include <QDesktopWidget>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>

ToolTipManager::ToolTipManager(QAbstractItemView* parent,
                               DolphinSortFilterProxyModel* model) :
    QObject(parent),
    m_view(parent),
    m_dolphinModel(0),
    m_proxyModel(model),
    m_prepareToolTipTimer(0),
    m_startPreviewJobTimer(0),
    m_waitOnPreviewTimer(0),
    m_showToolTipDelayedTimer(0),
    m_fileMetaDataToolTip(0),
    m_item(),
    m_itemRect(),
    m_generatingPreview(false),
    m_hasDefaultIcon(false),
    m_previewPixmap()
{
    static FileMetaDataToolTip* sharedToolTip = 0;
    if (sharedToolTip == 0) {
        sharedToolTip = new FileMetaDataToolTip();
        // TODO: Using K_GLOBAL_STATIC would be preferable to maintain the
        // instance, but the cleanup of KFileMetaDataWidget at this stage does
        // not work.
    }
    m_fileMetaDataToolTip = sharedToolTip;
    
    m_dolphinModel = static_cast<DolphinModel*>(m_proxyModel->sourceModel());
    connect(parent, SIGNAL(entered(const QModelIndex&)),
            this, SLOT(requestToolTip(const QModelIndex&)));
    connect(parent, SIGNAL(viewportEntered()),
            this, SLOT(hideToolTip()));

    // Initialize timers
    m_prepareToolTipTimer = new QTimer(this);
    m_prepareToolTipTimer->setSingleShot(true);
    m_prepareToolTipTimer->setInterval(500);
    connect(m_prepareToolTipTimer, SIGNAL(timeout()), this, SLOT(prepareToolTip()));   
    
    m_startPreviewJobTimer = new QTimer(this);
    m_startPreviewJobTimer->setSingleShot(true);
    m_startPreviewJobTimer->setInterval(200);
    connect(m_startPreviewJobTimer, SIGNAL(timeout()), this, SLOT(startPreviewJob()));
    
    m_waitOnPreviewTimer = new QTimer(this);
    m_waitOnPreviewTimer->setSingleShot(true);
    m_waitOnPreviewTimer->setInterval(250);     
    connect(m_waitOnPreviewTimer, SIGNAL(timeout()), this, SLOT(prepareToolTip()));

    m_showToolTipDelayedTimer = new QTimer(this);
    m_showToolTipDelayedTimer->setSingleShot(true);
    m_showToolTipDelayedTimer->setInterval(100);     
    connect(m_showToolTipDelayedTimer, SIGNAL(timeout()), this, SLOT(showToolTip()));
    
    // When the mousewheel is used, the items don't get a hovered indication
    // (Qt-issue #200665). To assure that the tooltip still gets hidden,
    // the scrollbars are observed.
    connect(parent->horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideTip()));
    connect(parent->verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SLOT(hideTip()));

    m_view->viewport()->installEventFilter(this);
    m_view->installEventFilter(this);
}

ToolTipManager::~ToolTipManager()
{
}

void ToolTipManager::hideTip()
{
    hideToolTip();
}

bool ToolTipManager::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_view->viewport()) {
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
    }

    return QObject::eventFilter(watched, event);
}

void ToolTipManager::requestToolTip(const QModelIndex& index)
{
    hideToolTip();
    
    // Only request a tooltip for the name column and when no selection or
    // drag & drop operation is done (indicated by the left mouse button)
    if ((index.column() == DolphinModel::Name) && !(QApplication::mouseButtons() & Qt::LeftButton)) {
        m_itemRect = m_view->visualRect(index);
        const QPoint pos = m_view->viewport()->mapToGlobal(m_itemRect.topLeft());
        m_itemRect.moveTo(pos);

        const QModelIndex dirIndex = m_proxyModel->mapToSource(index);
        m_item = m_dolphinModel->itemForIndex(dirIndex);

        // Only start the previewJob when the mouse has been over this item for 200 milliseconds.
        // This prevents a lot of useless preview jobs when passing rapidly over a lot of items.
        m_startPreviewJobTimer->start();
        m_previewPixmap = QPixmap();
        m_hasDefaultIcon = false;

        m_prepareToolTipTimer->start();
    }
}

void ToolTipManager::hideToolTip()
{
    m_prepareToolTipTimer->stop();
    m_startPreviewJobTimer->stop();
    m_waitOnPreviewTimer->stop();
    m_showToolTipDelayedTimer->stop();

    m_fileMetaDataToolTip->hide();
}

void ToolTipManager::prepareToolTip()
{
    if (m_generatingPreview) {
        m_waitOnPreviewTimer->start();
    }

    if (!m_previewPixmap.isNull()) {
        showToolTipDelayed(m_previewPixmap);
    } else if (!m_hasDefaultIcon) {
        const QPixmap image(KIcon(m_item.iconName()).pixmap(128, 128));
        showToolTipDelayed(image);
        m_hasDefaultIcon = true;
    }
}

void ToolTipManager::startPreviewJob()
{
    m_generatingPreview = true;
    KIO::PreviewJob* job = KIO::filePreview(KFileItemList() << m_item, 256, 256);

    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            this, SLOT(setPreviewPix(const KFileItem&, const QPixmap&)));
    connect(job, SIGNAL(failed(const KFileItem&)),
            this, SLOT(previewFailed()));
}


void ToolTipManager::setPreviewPix(const KFileItem& item,
                                   const QPixmap& pixmap)
{
    if ((m_item.url() != item.url()) || pixmap.isNull()) {
        // An old preview or an invalid preview has been received
        previewFailed();
    } else {
        m_previewPixmap = pixmap;
        m_generatingPreview = false;
    }
}

void ToolTipManager::previewFailed()
{
    m_generatingPreview = false;
}

void ToolTipManager::showToolTip()
{
    const QRect desktop = QApplication::desktop()->screenGeometry(m_itemRect.bottomRight());
    const QSize size = m_fileMetaDataToolTip->sizeHint();
    
    // m_itemRect defines the area of the item, where the tooltip should be
    // shown. Per default the tooltip is shown in the bottom right corner.
    // If the tooltip content exceeds the desktop borders, it must be assured that:
    // - the content is fully visible
    // - the content is not drawn inside m_itemRect
    const bool hasRoomToLeft  = (m_itemRect.left()   - size.width()  >= desktop.left());
    const bool hasRoomToRight = (m_itemRect.right()  + size.width()  <= desktop.right());
    const bool hasRoomAbove   = (m_itemRect.top()    - size.height() >= desktop.top());
    const bool hasRoomBelow   = (m_itemRect.bottom() + size.height() <= desktop.bottom());
    if (!hasRoomAbove && !hasRoomBelow && !hasRoomToLeft && !hasRoomToRight) {
        return;
    }
    
    int x = 0;
    int y = 0;
    if (hasRoomBelow || hasRoomAbove) {
        x = QCursor::pos().x() + 16; // TODO: use mouse pointer width instead of the magic value of 16
        if (x + size.width() >= desktop.right()) {
            x = desktop.right() - size.width();
        }
        if (hasRoomBelow) {
            y = m_itemRect.bottom();
        } else {
            y = m_itemRect.top() - size.height();
        }
    } else {
        Q_ASSERT(hasRoomToLeft || hasRoomToRight);
        if (hasRoomToRight) {
            x = m_itemRect.right();
        } else {
            x = m_itemRect.left() - size.width();
        }

        // Put the tooltip at the bottom of the screen. The x-coordinate has already
        // been adjusted, so that no overlapping with m_itemRect occurs.
        y = desktop.bottom() - size.height();
    }
    
    m_fileMetaDataToolTip->move(qMax(x, 0), qMax(y, 0));
    m_fileMetaDataToolTip->show();
}

void ToolTipManager::showToolTipDelayed(const QPixmap& pixmap)
{
    if (QApplication::mouseButtons() & Qt::LeftButton) {
        return;
    }
    
    m_fileMetaDataToolTip->setPreview(pixmap);
    m_fileMetaDataToolTip->setName(m_item.text());
    m_fileMetaDataToolTip->setItems(KFileItemList() << m_item);

    // Because one tooltip instance is reused, the size hint always depends on the previous
    // content (QWidgets don't update their layout geometry if they are invisible). To
    // assure having a consistent size without relayout flickering, the tooltip is opened
    // on an invisible position first. This gives the layout system some time to asynchronously
    // update the content.
    const QRect desktop = QApplication::desktop()->screenGeometry(m_itemRect.bottomRight());
    m_fileMetaDataToolTip->move(desktop.bottomRight());
    m_fileMetaDataToolTip->show();
    
    m_showToolTipDelayedTimer->start(); // Calls ToolTipManager::showToolTip()
}

#include "tooltipmanager.moc"
