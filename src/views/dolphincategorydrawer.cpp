/*
  * This file is part of the KDE project
  * Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
  *
  * This library is free software; you can redistribute it and/or
  * modify it under the terms of the GNU Library General Public
  * License as published by the Free Software Foundation; either
  * version 2 of the License, or (at your option) any later version.
  *
  * This library is distributed in the hope that it will be useful,
  * but WITHOUT ANY WARRANTY; without even the implied warranty of
  * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  * Library General Public License for more details.
  *
  * You should have received a copy of the GNU Library General Public License
  * along with this library; see the file COPYING.LIB.  If not, write to
  * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  * Boston, MA 02110-1301, USA.
  */

#include "dolphincategorydrawer.h"

#include <config-nepomuk.h>

#include <QPainter>
#include <QFile>
#include <QDir>
#include <QApplication>
#include <QStyleOption>

#ifdef HAVE_NEPOMUK
#include <nepomuk/kratingpainter.h>
#endif

#include <kiconloader.h>
#include <kiconeffect.h>
#include <kcategorizedsortfilterproxymodel.h>
#include <kuser.h>
#include <kcategorizedview.h>

#include "dolphinview.h"
#include "dolphinmodel.h"

#define HORIZONTAL_HINT 3

DolphinCategoryDrawer::DolphinCategoryDrawer(KCategorizedView *view)
        : KCategoryDrawerV3(view)
        , hotSpotPressed(NoneHotSpot)
        , selectAll(KIconLoader::global()->loadIcon("list-add", KIconLoader::Desktop, 16))
        , selectAllHovered(KIconLoader::global()->iconEffect()->apply(selectAll, KIconLoader::Desktop, KIconLoader::ActiveState))
        , selectAllDisabled(KIconLoader::global()->iconEffect()->apply(selectAll, KIconLoader::Desktop, KIconLoader::DisabledState))
        , unselectAll(KIconLoader::global()->loadIcon("list-remove", KIconLoader::Desktop, 16))
        , unselectAllHovered(KIconLoader::global()->iconEffect()->apply(unselectAll, KIconLoader::Desktop, KIconLoader::ActiveState))
        , unselectAllDisabled(KIconLoader::global()->iconEffect()->apply(unselectAll, KIconLoader::Desktop, KIconLoader::DisabledState))
{
}

DolphinCategoryDrawer::~DolphinCategoryDrawer()
{
}

bool DolphinCategoryDrawer::allCategorySelected(const QString &category) const
{
    const QModelIndexList list = view()->block(category);
    foreach (const QModelIndex &index, list) {
        if (!view()->selectionModel()->isSelected(index)) {
            return false;
        }
    }
    return true;
}

bool DolphinCategoryDrawer::someCategorySelected(const QString &category) const
{
    const QModelIndexList list = view()->block(category);
    foreach (const QModelIndex &index, list) {
        if (view()->selectionModel()->isSelected(index)) {
            return true;
        }
    }
    return false;
}

void DolphinCategoryDrawer::drawCategory(const QModelIndex &index, int sortRole,
                                         const QStyleOption &option, QPainter *painter) const
{
    Q_UNUSED(sortRole);
    painter->setRenderHint(QPainter::Antialiasing);

    if (!index.isValid()) {
        return;
    }

    const QString category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
    const QRect optRect = option.rect;
    QFont font(QApplication::font());
    font.setBold(true);
    const QFontMetrics fontMetrics = QFontMetrics(font);

    QColor outlineColor = option.palette.text().color();
    outlineColor.setAlphaF(0.35);

    //BEGIN: top left corner
    {
        painter->save();
        painter->setPen(outlineColor);
        const QPointF topLeft(optRect.topLeft());
        QRectF arc(topLeft, QSizeF(4, 4));
        arc.translate(0.5, 0.5);
        painter->drawArc(arc, 1440, 1440);
        painter->restore();
    }
    //END: top left corner

    //BEGIN: left vertical line
    {
        QPoint start(optRect.topLeft());
        start.ry() += 3;
        QPoint verticalGradBottom(optRect.topLeft());
        verticalGradBottom.ry() += fontMetrics.height() + 5;
        QLinearGradient gradient(start, verticalGradBottom);
        gradient.setColorAt(0, outlineColor);
        gradient.setColorAt(1, Qt::transparent);
        painter->fillRect(QRect(start, QSize(1, fontMetrics.height() + 5)), gradient);
    }
    //END: left vertical line

    //BEGIN: horizontal line
    {
        QPoint start(optRect.topLeft());
        start.rx() += 3;
        QPoint horizontalGradTop(optRect.topLeft());
        horizontalGradTop.rx() += optRect.width() - 6;
        painter->fillRect(QRect(start, QSize(optRect.width() - 6, 1)), outlineColor);
    }
    //END: horizontal line

    //BEGIN: top right corner
    {
        painter->save();
        painter->setPen(outlineColor);
        QPointF topRight(optRect.topRight());
        topRight.rx() -= 4;
        QRectF arc(topRight, QSizeF(4, 4));
        arc.translate(0.5, 0.5);
        painter->drawArc(arc, 0, 1440);
        painter->restore();
    }
    //END: top right corner

    //BEGIN: right vertical line
    {
        QPoint start(optRect.topRight());
        start.ry() += 3;
        QPoint verticalGradBottom(optRect.topRight());
        verticalGradBottom.ry() += fontMetrics.height() + 5;
        QLinearGradient gradient(start, verticalGradBottom);
        gradient.setColorAt(0, outlineColor);
        gradient.setColorAt(1, Qt::transparent);
        painter->fillRect(QRect(start, QSize(1, fontMetrics.height() + 5)), gradient);
    }
    //END: right vertical line

    const int iconSize = KIconLoader::global()->currentSize(KIconLoader::Small);

    //BEGIN: select/unselect all
    {
        if (this->category == category) {
            QRect iconAllRect(option.rect);
            iconAllRect.setTop(iconAllRect.top() + 4);
            iconAllRect.setLeft(iconAllRect.right() - 16 - 7);
            iconAllRect.setSize(QSize(iconSize, iconSize));
            if (!allCategorySelected(category)) {
                if (iconAllRect.contains(pos)) {
                    painter->drawPixmap(iconAllRect, selectAllHovered);
                } else {
                    painter->drawPixmap(iconAllRect, selectAll);
                }
            } else {
                painter->drawPixmap(iconAllRect, selectAllDisabled);
            }
            QRect iconNoneRect(option.rect);
            iconNoneRect.setTop(iconNoneRect.top() + 4);
            iconNoneRect.setLeft(iconNoneRect.right() - 16 * 2 - 7 * 2);
            iconNoneRect.setSize(QSize(iconSize, iconSize));
            if (someCategorySelected(category)) {
                if (iconNoneRect.contains(pos)) {
                    painter->drawPixmap(iconNoneRect, unselectAllHovered);
                } else {
                    painter->drawPixmap(iconNoneRect, unselectAll);
                }
            } else {
                painter->drawPixmap(iconNoneRect, unselectAllDisabled);
            }
        }
    }
    //END: select/unselect all

    //BEGIN: category information
    {
        bool paintIcon;
        QPixmap icon;
        switch (index.column()) {
             case KDirModel::Owner: {
                     paintIcon = true;
                     KUser user(category);
                     const QString faceIconPath = user.faceIconPath();
                     if (faceIconPath.isEmpty()) {
                         icon = KIconLoader::global()->loadIcon("user-identity", KIconLoader::NoGroup, iconSize);
                     } else {
                         icon = QPixmap::fromImage(QImage(faceIconPath).scaledToHeight(iconSize, Qt::SmoothTransformation));
                     }
                 }
                 break;
             case KDirModel::Type: {
                     paintIcon = true;
                     const KCategorizedSortFilterProxyModel *proxyModel = static_cast<const KCategorizedSortFilterProxyModel*>(index.model());
                     const DolphinModel *model = static_cast<const DolphinModel*>(proxyModel->sourceModel());
                     KFileItem item = model->itemForIndex(proxyModel->mapToSource(index));
                     // This is the only way of getting the icon right. Others will fail on corner
                     // cases like the item representing this group has been set a different icon,
                     // so the group icon drawn is that one particularly. This way assures the drawn
                     // icon is the one of the mimetype of the group itself. (ereslibre)
                     icon = KIconLoader::global()->loadMimeTypeIcon(item.mimeTypePtr()->iconName(), KIconLoader::NoGroup, iconSize);
                 }
                 break;
             default:
                 paintIcon = false;
             }

        if (paintIcon) {
            QRect iconRect(option.rect);
            iconRect.setTop(iconRect.top() + 4);
            iconRect.setLeft(iconRect.left() + 7);
            iconRect.setSize(QSize(iconSize, iconSize));

            painter->drawPixmap(iconRect, icon);
        }

        //BEGIN: text
        {
            QRect textRect(option.rect);
            textRect.setTop(textRect.top() + 7);
            textRect.setLeft(textRect.left() + 7 + (paintIcon ? (iconSize + 6) : 0));
            textRect.setHeight(qMax(fontMetrics.height(), iconSize));
            textRect.setRight(textRect.right() - 7);
            textRect.setBottom(textRect.bottom() - 5); // only one pixel separation here (no gradient)

            painter->save();
            painter->setFont(font);
            QColor penColor(option.palette.text().color());
            penColor.setAlphaF(0.6);
            painter->setPen(penColor);
            painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, category);
            painter->restore();
        }
        //END: text
    }
    //BEGIN: category information
}

int DolphinCategoryDrawer::categoryHeight(const QModelIndex &index, const QStyleOption &) const
{
    int iconSize = KIconLoader::global()->currentSize(KIconLoader::Small);
    QFont font(QApplication::font());
    font.setBold(true);
    const QFontMetrics fontMetrics = QFontMetrics(font);
    int heightWithoutIcon = fontMetrics.height() + (iconSize / 4) * 2 + 1; /* 1 pixel-width gradient */
    bool paintIcon;

    switch (index.column()) {
    case KDirModel::Owner:
    case KDirModel::Type:
        paintIcon = true;
        break;
    default:
        paintIcon = false;
    }

    if (paintIcon) {
        return qMax(heightWithoutIcon + 5, iconSize + 1 /* 1 pixel-width gradient */
                    + 5 /* top and bottom separation */);
    }

    return heightWithoutIcon + 5;
}

void DolphinCategoryDrawer::mouseButtonPressed(const QModelIndex &index, const QRect &blockRect, QMouseEvent *event)
{
    if (!index.isValid()) {
        event->ignore();
        return;
    }
    const QString category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
    int iconSize = KIconLoader::global()->currentSize(KIconLoader::Small);
    if (this->category == category) {
        QRect iconAllRect(blockRect);
        iconAllRect.setTop(iconAllRect.top() + 4);
        iconAllRect.setLeft(iconAllRect.right() - 16 - 7);
        iconAllRect.setSize(QSize(iconSize, iconSize));
        if (iconAllRect.contains(pos)) {
            event->accept();
            hotSpotPressed = SelectAllHotSpot;
            categoryPressed = index;
            return;
        }
        QRect iconNoneRect(blockRect);
        iconNoneRect.setTop(iconNoneRect.top() + 4);
        iconNoneRect.setLeft(iconNoneRect.right() - 16 * 2 - 7 * 2);
        iconNoneRect.setSize(QSize(iconSize, iconSize));
        if (iconNoneRect.contains(pos)) {
            event->accept();
            hotSpotPressed = UnselectAllHotSpot;
            categoryPressed = index;
            return;
        }
    }
    event->ignore();
}

void DolphinCategoryDrawer::mouseButtonReleased(const QModelIndex &index, const QRect &blockRect, QMouseEvent *event)
{
    if (!index.isValid() || hotSpotPressed == NoneHotSpot || categoryPressed != index) {
        event->ignore();
        return;
    }
    categoryPressed = QModelIndex();
    const QString category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
    int iconSize = KIconLoader::global()->currentSize(KIconLoader::Small);
    if (this->category == category) {
        QRect iconAllRect(blockRect);
        iconAllRect.setTop(iconAllRect.top() + 4);
        iconAllRect.setLeft(iconAllRect.right() - 16 - 7);
        iconAllRect.setSize(QSize(iconSize, iconSize));
        if (iconAllRect.contains(pos)) {
            if (hotSpotPressed == SelectAllHotSpot) {
                event->accept();
                emit actionRequested(SelectAll, index);
            } else {
                event->ignore();
                hotSpotPressed = NoneHotSpot;
            }
            return;
        }
        QRect iconNoneRect(blockRect);
        iconNoneRect.setTop(iconNoneRect.top() + 4);
        iconNoneRect.setLeft(iconNoneRect.right() - 16 * 2 - 7 * 2);
        iconNoneRect.setSize(QSize(iconSize, iconSize));
        if (iconNoneRect.contains(pos)) {
            if (hotSpotPressed == UnselectAllHotSpot) {
                event->accept();
                emit actionRequested(UnselectAll, index);
            } else {
                event->ignore();
                hotSpotPressed = NoneHotSpot;
            }
            return;
        }
    }
    event->ignore();
}

void DolphinCategoryDrawer::mouseMoved(const QModelIndex &index, const QRect &, QMouseEvent *event)
{
    event->ignore();
    if (!index.isValid()) {
        return;
    }
    pos = event->pos();
    category = index.model()->data(index, KCategorizedSortFilterProxyModel::CategoryDisplayRole).toString();
}

void DolphinCategoryDrawer::mouseLeft(const QModelIndex &, const QRect &)
{
    pos = QPoint();
    category.clear();
}
