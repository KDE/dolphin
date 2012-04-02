/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#include "kitemlistwidget.h"

#include "kitemlistselectiontoggle_p.h"
#include "kitemlistview.h"
#include "kitemmodelbase.h"

#include <KDebug>

#include <KGlobalSettings>
#include <QApplication>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOption>

KItemListWidget::KItemListWidget(QGraphicsItem* parent) :
    QGraphicsWidget(parent, 0),
    m_index(-1),
    m_selected(false),
    m_current(false),
    m_hovered(false),
    m_alternateBackground(false),
    m_enabledSelectionToggle(false),
    m_data(),
    m_visibleRoles(),
    m_columnWidths(),
    m_styleOption(),
    m_siblingsInfo(),
    m_hoverOpacity(0),
    m_hoverCache(0),
    m_hoverAnimation(0),
    m_selectionToggle(0)
{
}

KItemListWidget::~KItemListWidget()
{
    clearHoverCache();
}

void KItemListWidget::setIndex(int index)
{
    if (m_index != index) {
        delete m_selectionToggle;
        m_selectionToggle = 0;

        if (m_hoverAnimation) {
            m_hoverAnimation->stop();
            m_hoverOpacity = 0;
        }
        clearHoverCache();

        m_index = index;
    }
}

int KItemListWidget::index() const
{
    return m_index;
}

void KItemListWidget::setData(const QHash<QByteArray, QVariant>& data,
                              const QSet<QByteArray>& roles)
{
    clearHoverCache();
    if (roles.isEmpty()) {
        m_data = data;
        dataChanged(m_data);
    } else {
        foreach (const QByteArray& role, roles) {
            m_data[role] = data[role];
        }
        dataChanged(m_data, roles);
    }
    update();
}

QHash<QByteArray, QVariant> KItemListWidget::data() const
{
    return m_data;
}

void KItemListWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);

    if (m_alternateBackground) {
        const QColor backgroundColor = m_styleOption.palette.color(QPalette::AlternateBase);
        const QRectF backgroundRect(0, 0, size().width(), size().height());
        painter->fillRect(backgroundRect, backgroundColor);
    }

    if (m_selected) {
        const QStyle::State activeState(isActiveWindow() ? QStyle::State_Active : 0);
        drawItemStyleOption(painter, widget, activeState |
                                             QStyle::State_Enabled |
                                             QStyle::State_Selected |
                                             QStyle::State_Item);
    }

    if (isCurrent()) {
        QStyleOptionFocusRect focusRectOption;
        focusRectOption.initFrom(widget);
        focusRectOption.rect = textFocusRect().toRect();
        focusRectOption.state = QStyle::State_Enabled | QStyle::State_Item | QStyle::State_KeyboardFocusChange;
        if (m_selected) {
            focusRectOption.state |= QStyle::State_Selected;
        }

        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusRectOption, painter, widget);
    }

    if (m_hoverOpacity > 0.0) {
        if (!m_hoverCache) {
            // Initialize the m_hoverCache pixmap to improve the drawing performance
            // when fading the hover background
            m_hoverCache = new QPixmap(size().toSize());
            m_hoverCache->fill(Qt::transparent);

            QPainter pixmapPainter(m_hoverCache);
            const QStyle::State activeState(isActiveWindow() ? QStyle::State_Active : 0);
            drawItemStyleOption(&pixmapPainter, widget, activeState |
                                                        QStyle::State_Enabled |
                                                        QStyle::State_MouseOver |
                                                        QStyle::State_Item);
        }

        const qreal opacity = painter->opacity();
        painter->setOpacity(m_hoverOpacity * opacity);
        painter->drawPixmap(0, 0, *m_hoverCache);
        painter->setOpacity(opacity);
    }
}

void KItemListWidget::setVisibleRoles(const QList<QByteArray>& roles)
{
    const QList<QByteArray> previousRoles = m_visibleRoles;
    m_visibleRoles = roles;

    visibleRolesChanged(roles, previousRoles);
    update();
}

QList<QByteArray> KItemListWidget::visibleRoles() const
{
    return m_visibleRoles;
}


void KItemListWidget::setColumnWidth(const QByteArray& role, qreal width)
{
    if (m_columnWidths.value(role) != width) {
        const qreal previousWidth = width;
        m_columnWidths.insert(role, width);
        columnWidthChanged(role, width, previousWidth);
        update();
    }
}

qreal KItemListWidget::columnWidth(const QByteArray& role) const
{
    return m_columnWidths.value(role);
}

void KItemListWidget::setStyleOption(const KItemListStyleOption& option)
{
    const KItemListStyleOption previous = m_styleOption;
    clearHoverCache();
    m_styleOption = option;

    styleOptionChanged(option, previous);
    update();
}

const KItemListStyleOption& KItemListWidget::styleOption() const
{
    return m_styleOption;
}

void KItemListWidget::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        if (m_selectionToggle) {
            m_selectionToggle->setChecked(selected);
        }
        selectedChanged(selected);
        update();
    }
}

bool KItemListWidget::isSelected() const
{
    return m_selected;
}

void KItemListWidget::setCurrent(bool current)
{
    if (m_current != current) {
        m_current = current;
        currentChanged(current);
        update();
    }
}

bool KItemListWidget::isCurrent() const
{
    return m_current;
}

void KItemListWidget::setHovered(bool hovered)
{
    if (hovered == m_hovered) {
        return;
    }

    m_hovered = hovered;

    if (!m_hoverAnimation) {
        m_hoverAnimation = new QPropertyAnimation(this, "hoverOpacity", this);
        const int duration = (KGlobalSettings::graphicEffectsLevel() == KGlobalSettings::NoEffects) ? 1 : 200;
        m_hoverAnimation->setDuration(duration);
        connect(m_hoverAnimation, SIGNAL(finished()), this, SLOT(slotHoverAnimationFinished()));
    }
    m_hoverAnimation->stop();

    if (hovered) {
        const qreal startValue = qMax(hoverOpacity(), qreal(0.1));
        m_hoverAnimation->setStartValue(startValue);
        m_hoverAnimation->setEndValue(1.0);
        if (m_enabledSelectionToggle && !(QApplication::mouseButtons() & Qt::LeftButton)) {
            initializeSelectionToggle();
        }
    } else {
        m_hoverAnimation->setStartValue(hoverOpacity());
        m_hoverAnimation->setEndValue(0.0);
    }

    m_hoverAnimation->start();

    hoveredChanged(hovered);
    update();
}

bool KItemListWidget::isHovered() const
{
    return m_hovered;
}

void KItemListWidget::setAlternateBackground(bool enable)
{
    if (m_alternateBackground != enable) {
        m_alternateBackground = enable;
        alternateBackgroundChanged(enable);
        update();
    }
}

bool KItemListWidget::alternateBackground() const
{
    return m_alternateBackground;
}

void KItemListWidget::setEnabledSelectionToggle(bool enable)
{
    if (m_enabledSelectionToggle != enable) {
        m_enabledSelectionToggle = enable;
        update();
    }
}

bool KItemListWidget::enabledSelectionToggle() const
{
    return m_enabledSelectionToggle;
}

void KItemListWidget::setSiblingsInformation(const QBitArray& siblings)
{
    const QBitArray previous = m_siblingsInfo;
    m_siblingsInfo = siblings;
    siblingsInformationChanged(m_siblingsInfo, previous);
    update();
}

QBitArray KItemListWidget::siblingsInformation() const
{
    return m_siblingsInfo;
}

bool KItemListWidget::contains(const QPointF& point) const
{
    if (!QGraphicsWidget::contains(point)) {
        return false;
    }

    return iconRect().contains(point) ||
           textRect().contains(point) ||
           expansionToggleRect().contains(point) ||
           selectionToggleRect().contains(point);
}

QRectF KItemListWidget::textFocusRect() const
{
    return textRect();
}

QRectF KItemListWidget::selectionToggleRect() const
{
    return QRectF();
}

QRectF KItemListWidget::expansionToggleRect() const
{
    return QRectF();
}

void KItemListWidget::dataChanged(const QHash<QByteArray, QVariant>& current,
                                  const QSet<QByteArray>& roles)
{
    Q_UNUSED(current);
    Q_UNUSED(roles);
}

void KItemListWidget::visibleRolesChanged(const QList<QByteArray>& current,
                                          const QList<QByteArray>& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListWidget::columnWidthChanged(const QByteArray& role,
                                         qreal current,
                                         qreal previous)
{
    Q_UNUSED(role);
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListWidget::styleOptionChanged(const KItemListStyleOption& current,
                                         const KItemListStyleOption& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListWidget::currentChanged(bool current)
{
    Q_UNUSED(current);
}

void KItemListWidget::selectedChanged(bool selected)
{
    Q_UNUSED(selected);
}

void KItemListWidget::hoveredChanged(bool hovered)
{
    Q_UNUSED(hovered);
}

void KItemListWidget::alternateBackgroundChanged(bool enabled)
{
    Q_UNUSED(enabled);
}

void KItemListWidget::siblingsInformationChanged(const QBitArray& current, const QBitArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListWidget::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    clearHoverCache();
}

qreal KItemListWidget::hoverOpacity() const
{
    return m_hoverOpacity;
}

void KItemListWidget::slotHoverAnimationFinished()
{
    if (!m_hovered) {
        delete m_selectionToggle;
        m_selectionToggle = 0;
    }
}

void KItemListWidget::initializeSelectionToggle()
{
    Q_ASSERT(m_enabledSelectionToggle);

    if (!m_selectionToggle) {
        m_selectionToggle = new KItemListSelectionToggle(this);
    }

    const QRectF toggleRect = selectionToggleRect();
    m_selectionToggle->setPos(toggleRect.topLeft());
    m_selectionToggle->resize(toggleRect.size());

    m_selectionToggle->setChecked(isSelected());
}

void KItemListWidget::setHoverOpacity(qreal opacity)
{
    m_hoverOpacity = opacity;
    if (m_selectionToggle) {
        m_selectionToggle->setOpacity(opacity);
    }

    if (m_hoverOpacity <= 0.0) {
        delete m_hoverCache;
        m_hoverCache = 0;
    }

    update();
}

void KItemListWidget::clearHoverCache()
{
    delete m_hoverCache;
    m_hoverCache = 0;
}

void KItemListWidget::drawItemStyleOption(QPainter* painter, QWidget* widget, QStyle::State styleState)
{
    const QRect textBounds = textRect().toRect();

    QStyleOptionViewItemV4 viewItemOption;
    viewItemOption.initFrom(widget);
    viewItemOption.state = styleState;
    viewItemOption.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
    viewItemOption.showDecorationSelected = true;
    viewItemOption.rect = textBounds;
    widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &viewItemOption, painter, widget);
}

#include "kitemlistwidget.moc"
