/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placesitemlistwidget.h"

#include <QGraphicsView>
#include <QStyleOption>

#include <KDiskFreeSpaceInfo>
#include <KMountPoint>

#define CAPACITYBAR_HEIGHT 2
#define CAPACITYBAR_MARGIN 2


PlacesItemListWidget::PlacesItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent) :
    KStandardItemListWidget(informant, parent)
{
}

PlacesItemListWidget::~PlacesItemListWidget()
{
}

bool PlacesItemListWidget::isHidden() const
{
    return data().value("isHidden").toBool() ||
           data().value("isGroupHidden").toBool();
}

QPalette::ColorRole PlacesItemListWidget::normalTextColorRole() const
{
    return QPalette::WindowText;
}

void PlacesItemListWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    KStandardItemListWidget::paint(painter, option, widget);

    bool drawCapacityBar = false;
    const QUrl url = data().value("url").toUrl();
    if (url.isLocalFile()) {
        const QString mountPointPath = url.toLocalFile();
        KMountPoint::Ptr mp = KMountPoint::currentMountPoints().findByPath(mountPointPath);
        bool isMountPoint = (mp && mp->mountPoint() == mountPointPath);

        if (isMountPoint) {
            const KDiskFreeSpaceInfo info = KDiskFreeSpaceInfo::freeSpaceInfo(mountPointPath);
            drawCapacityBar = info.size() != 0;
            if (drawCapacityBar) {
                const TextInfo* textInfo = m_textInfo.value("text");
                if (textInfo) { // See KStandarItemListWidget::paint() for info on why we check textInfo.
                    painter->save();

                    QRect capacityRect(
                        textInfo->pos.x(),
                        option->rect.top() + option->rect.height() - CAPACITYBAR_HEIGHT - CAPACITYBAR_MARGIN,
                        qMin((qreal)option->rect.width(), selectionRect().width()) - (textInfo->pos.x() - option->rect.left()),
                        CAPACITYBAR_HEIGHT
                    );

                    const qreal ratio = (qreal)info.used() / (qreal)info.size();
                    // qDebug() << "ratio:" << ratio << "(" << info.used() << "/" << info.size() << ")";

                    const QPalette pal = palette();
                    const QPalette::ColorGroup group = isActiveWindow() ? QPalette::Active : QPalette::Inactive;
                    // QColor bgColor = QColor::fromRgb(230, 230, 230);
                    // QColor outlineColor = QColor::fromRgb(208, 208, 208);
                    // QColor bgColor = QColor::fromRgb(0, 230, 0);
                    // QColor outlineColor = QColor::fromRgb(208, 0, 0, 127);
                    // QColor normalUsedColor = QColor::fromRgb(38, 160, 218);
                    // QColor dangerUsedColor = QColor::fromRgb(218, 38, 38);
                    // QColor bgColor = pal.base().color().darker(130);
                    // QColor outlineColor = pal.base().color().darker(150);

                    QPalette::ColorRole role;
                    // role = isSelected() ? QPalette::Highlight : QPalette::Window;
                    // QColor bgColor = styleOption().palette.color(group, role).darker(150);
                    // QColor outlineColor = styleOption().palette.color(group, role).darker(170);
                    QColor bgColor = isSelected()
                        ? styleOption().palette.color(group, QPalette::Highlight).darker(180)
                        : styleOption().palette.color(group, QPalette::Window).darker(120);

                    role = isSelected() ? QPalette::HighlightedText : QPalette::Highlight;
                    QColor normalUsedColor = styleOption().palette.color(group, role);

                    QColor dangerUsedColor = QColor::fromRgb(218, 38, 38);

                    // Background
                    painter->fillRect(capacityRect, bgColor);

                    // Outline
                    // const QRect outlineRect(capacityRect.x(), capacityRect.y(), capacityRect.width() - 1, capacityRect.height() - 1);
                    // painter->setPen(outlineColor);
                    // painter->drawRect(outlineRect);

                    // Fill
                    const QRect fillRect(capacityRect.x(), capacityRect.y(), capacityRect.width() * ratio, capacityRect.height());
                    if (ratio < 0.95) { // Fill
                        painter->fillRect(fillRect, normalUsedColor);
                    } else {
                        painter->fillRect(fillRect, dangerUsedColor);
                    }

                    painter->restore();
                }
            }
        }
    }
}
