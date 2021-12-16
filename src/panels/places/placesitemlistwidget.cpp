/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "placesitemlistwidget.h"

#include <QStyleOption>

#include <KColorScheme>

#include <Solid/Device>
#include <Solid/NetworkShare>

#define CAPACITYBAR_HEIGHT 2
#define CAPACITYBAR_MARGIN 2
#define CAPACITYBAR_CACHE_TTL 60000


PlacesItemListWidget::PlacesItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent) :
    KStandardItemListWidget(informant, parent)
    , m_drawCapacityBar(false)
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

void PlacesItemListWidget::updateCapacityBar()
{
    if (!data().value("isCapacityBarRecommended").toBool()) {
        resetCapacityBar();
        return;
    }

    const QUrl url = data().value("url").toUrl();

    if (url.isEmpty() || m_freeSpaceInfo.job || !m_freeSpaceInfo.lastUpdated.hasExpired()) {
        // No url, job running or cache is still valid.
        return;
    }

    m_freeSpaceInfo.job = KIO::fileSystemFreeSpace(url);
    connect(
        m_freeSpaceInfo.job,
        &KIO::FileSystemFreeSpaceJob::result,
        this,
        [this](KIO::Job *job, KIO::filesize_t size, KIO::filesize_t available) {
            // even if we receive an error we want to refresh lastUpdated to avoid repeatedly querying in this case
            m_freeSpaceInfo.lastUpdated.setRemainingTime(CAPACITYBAR_CACHE_TTL);

            if (job->error()) {
                return;
            }

            m_freeSpaceInfo.size = size;
            m_freeSpaceInfo.used = size - available;
            m_freeSpaceInfo.usedRatio = (qreal)m_freeSpaceInfo.used / (qreal)m_freeSpaceInfo.size;
            m_drawCapacityBar = size > 0;

            update();
        }
    );
}

void PlacesItemListWidget::resetCapacityBar()
{
    m_drawCapacityBar = false;
    delete m_freeSpaceInfo.job;
    m_freeSpaceInfo.lastUpdated.setRemainingTime(0);
    m_freeSpaceInfo.size = 0;
    m_freeSpaceInfo.used = 0;
    m_freeSpaceInfo.usedRatio = 0;
}

void PlacesItemListWidget::polishEvent()
{
    updateCapacityBar();

    QGraphicsWidget::polishEvent();
}

void PlacesItemListWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    KStandardItemListWidget::paint(painter, option, widget);

    // We check if option=nullptr since it is null when the place is dragged (Bug #430441)
    if (m_drawCapacityBar && option) {
        const TextInfo* textInfo = m_textInfo.value("text");
        if (textInfo) { // See KStandarItemListWidget::paint() for info on why we check textInfo.
            painter->save();

            const QRect capacityRect(
                textInfo->pos.x(),
                option->rect.top() + option->rect.height() - CAPACITYBAR_HEIGHT - CAPACITYBAR_MARGIN,
                qMin((qreal)option->rect.width(), selectionRect().width()) - (textInfo->pos.x() - option->rect.left()),
                CAPACITYBAR_HEIGHT
            );

            const QPalette pal = palette();
            const QPalette::ColorGroup group = isActiveWindow() ? QPalette::Active : QPalette::Inactive;

            // Background
            const QColor bgColor = isSelected()
                ? pal.color(group, QPalette::Highlight).darker(180)
                : pal.color(group, QPalette::Window).darker(120);

            painter->fillRect(capacityRect, bgColor);

            // Fill
            const QRect fillRect(capacityRect.x(), capacityRect.y(), capacityRect.width() * m_freeSpaceInfo.usedRatio, capacityRect.height());
            if (m_freeSpaceInfo.usedRatio >= 0.95) { // More than 95% full!
                const QColor dangerUsedColor = KColorScheme(group, KColorScheme::View).foreground(KColorScheme::NegativeText).color();
                painter->fillRect(fillRect, dangerUsedColor);
            } else {
                const QPalette::ColorRole role = isSelected() ? QPalette::HighlightedText : QPalette::Highlight;
                const QColor normalUsedColor = styleOption().palette.color(group, role);
                painter->fillRect(fillRect, normalUsedColor);
            }

            painter->restore();
        }
    }

    updateCapacityBar();
}
