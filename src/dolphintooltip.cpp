/***************************************************************************
 *   Copyright (C) 2008 by Simon St James <kdedevel@etotheipiplusone.com>  *
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
 
#include "dolphintooltip.h"

#include <kicon.h>
#include <kio/previewjob.h>
#include <kfileitem.h>

#include <QtGui/QPixmap>

const int PREVIEW_WIDTH = 256;
const int PREVIEW_HEIGHT = 256;

DolphinToolTipItem::DolphinToolTipItem(const KFileItem & fileItem) :
        KToolTipItem(KIcon(fileItem.iconName()), fileItem.getToolTipText(), UserType)
{

    //if (icon().actualSize(QSize(256,256)).width() != PREVIEW_WIDTH)
    /*{
        QPixmap paddedImage(QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT));
        paddedImage.fill(Qt::transparent);
        QPainter painter(&paddedImage);
        KIcon kicon(fileItem.iconName());
        painter.drawPixmap((PREVIEW_WIDTH - 128) / 2, (PREVIEW_HEIGHT - 128) / 2, kicon.pixmap(QSize(128,128))); 
        setData(Qt::DecorationRole, QIcon(paddedImage));
    }*/

    // Initiate the preview job.
    KFileItemList fileItems;
    fileItems << fileItem;
    KIO::PreviewJob* job = KIO::filePreview(fileItems, PREVIEW_WIDTH, PREVIEW_HEIGHT );
    connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
            this, SLOT(setPreview(const KFileItem&, const QPixmap&)));

}
DolphinToolTipItem::~DolphinToolTipItem()
{
}
void DolphinToolTipItem::setPreview(const KFileItem& fileItem, const QPixmap& preview)
{
    Q_UNUSED(fileItem);
   /* QPixmap paddedImage(QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT));
    paddedImage.fill(Qt::transparent);
    QPainter painter(&paddedImage);
    KIcon kicon(fileItem.iconName());
    painter.drawPixmap((PREVIEW_WIDTH - preview.width()) / 2, (PREVIEW_HEIGHT - preview.height()) / 2, preview); 
    setData(Qt::DecorationRole, QIcon(paddedImage));*/
    setData(Qt::DecorationRole, QIcon(preview));
}
// Delegate everything to the base class, after re-setting the decorationSize
// to the preview size.
QSize DolphinBalloonTooltipDelegate::sizeHint(const KStyleOptionToolTip *option, const KToolTipItem *item) const
{
    KStyleOptionToolTip updatedStyleOption = *option;
    updatedStyleOption.decorationSize = QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
    return KFormattedBalloonTipDelegate::sizeHint(&updatedStyleOption, item);
}
void DolphinBalloonTooltipDelegate::paint(QPainter *painter, const KStyleOptionToolTip *option, const KToolTipItem *item) const
{    
    KStyleOptionToolTip updatedStyleOption = *option;
    updatedStyleOption.decorationSize = QSize(PREVIEW_WIDTH, PREVIEW_HEIGHT);
    return KFormattedBalloonTipDelegate::paint(painter, &updatedStyleOption, item);
}
