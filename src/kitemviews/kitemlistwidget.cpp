/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistwidget.h"

#include "kitemlistview.h"
#include "private/kitemlistselectiontoggle.h"

#include <KConfigGroup>
#include <KSharedConfig>

#include <QApplication>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOption>

KItemListWidgetInformant::KItemListWidgetInformant()
{
}

KItemListWidgetInformant::~KItemListWidgetInformant()
{
}

KItemListWidget::KItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent)
    : QGraphicsWidget(parent)
    , m_informant(informant)
    , m_index(-1)
    , m_selected(false)
    , m_current(false)
    , m_hovered(false)
    , m_expansionAreaHovered(false)
    , m_alternateBackground(false)
    , m_enabledSelectionToggle(false)
    , m_data()
    , m_visibleRoles()
    , m_columnWidths()
    , m_sidePadding(0)
    , m_styleOption()
    , m_siblingsInfo()
    , m_hoverOpacity(0)
    , m_hoverCache(nullptr)
    , m_hoverSequenceIndex(0)
    , m_selectionToggle(nullptr)
    , m_editedRole()
    , m_iconSize(-1)
{
    connect(&m_hoverSequenceTimer, &QTimer::timeout, this, &KItemListWidget::slotHoverSequenceTimerTimeout);
}

KItemListWidget::~KItemListWidget()
{
    clearHoverCache();
}

void KItemListWidget::setIndex(int index)
{
    if (m_index != index) {
        delete m_selectionToggle;
        m_selectionToggle = nullptr;

        m_hoverOpacity = 0;

        clearHoverCache();

        m_index = index;
    }
}

int KItemListWidget::index() const
{
    return m_index;
}

void KItemListWidget::setData(const QHash<QByteArray, QVariant> &data, const QSet<QByteArray> &roles)
{
    clearHoverCache();
    if (roles.isEmpty()) {
        m_data = data;
        dataChanged(m_data);
    } else {
        for (const QByteArray &role : roles) {
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

void KItemListWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)

    if (m_alternateBackground) {
        QColor backgroundColor = m_styleOption.palette.color(QPalette::AlternateBase);
        if (!widget->hasFocus()) {
            QColor baseColor = m_styleOption.palette.color(QPalette::Base);
            if (baseColor.lightnessF() > 0.5) {
                // theme seems light
                backgroundColor = backgroundColor.lighter(101);
            } else {
                // theme seems dark
                backgroundColor = backgroundColor.darker(101);
            }
        }

        const QRectF backgroundRect(0, 0, size().width(), size().height());
        painter->fillRect(backgroundRect, backgroundColor);
    }

    if ((m_selected || m_current) && m_editedRole.isEmpty()) {
        const QStyle::State activeState(isActiveWindow() && widget->hasFocus() ? QStyle::State_Active : 0);
        drawItemStyleOption(painter, widget, activeState | QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_Item);
    }

    /*if (m_current && m_editedRole.isEmpty()) {
        QStyleOptionFocusRect focusRectOption;
        initStyleOption(&focusRectOption);
        focusRectOption.rect = textFocusRect().toRect();
        focusRectOption.state = QStyle::State_Enabled | QStyle::State_Item | QStyle::State_KeyboardFocusChange;
        if (m_selected && widget->hasFocus()) {
            focusRectOption.state |= QStyle::State_Selected;
        }

        style()->drawPrimitive(QStyle::PE_FrameFocusRect, &focusRectOption, painter, widget);
    }*/

    if (m_hoverOpacity > 0.0) {
        if (!m_hoverCache) {
            // Initialize the m_hoverCache pixmap to improve the drawing performance
            // when fading the hover background
            m_hoverCache = new QPixmap(size().toSize());
            m_hoverCache->fill(Qt::transparent);

            QPainter pixmapPainter(m_hoverCache);
            const QStyle::State activeState(isActiveWindow() && widget->hasFocus() ? QStyle::State_Active | QStyle::State_Enabled : 0);
            drawItemStyleOption(&pixmapPainter, widget, activeState | QStyle::State_MouseOver | QStyle::State_Item);
        }

        const qreal opacity = painter->opacity();
        painter->setOpacity(m_hoverOpacity * opacity);
        painter->drawPixmap(0, 0, *m_hoverCache);
        painter->setOpacity(opacity);
    }
}

void KItemListWidget::setVisibleRoles(const QList<QByteArray> &roles)
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

void KItemListWidget::setColumnWidth(const QByteArray &role, qreal width)
{
    const qreal previousWidth = m_columnWidths.value(role);
    if (previousWidth != width) {
        m_columnWidths.insert(role, width);
        columnWidthChanged(role, width, previousWidth);
        update();
    }
}

qreal KItemListWidget::columnWidth(const QByteArray &role) const
{
    return m_columnWidths.value(role);
}

qreal KItemListWidget::sidePadding() const
{
    return m_sidePadding;
}

void KItemListWidget::setSidePadding(qreal width)
{
    if (m_sidePadding != width) {
        m_sidePadding = width;
        sidePaddingChanged(width);
        update();
    }
}

void KItemListWidget::setStyleOption(const KItemListStyleOption &option)
{
    if (m_styleOption == option) {
        return;
    }

    const KItemListStyleOption previous = m_styleOption;
    clearHoverCache();
    m_styleOption = option;
    styleOptionChanged(option, previous);
    update();
}

const KItemListStyleOption &KItemListWidget::styleOption() const
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

    m_hoverSequenceIndex = 0;

    if (hovered) {
        setHoverOpacity(1.0);

        if (m_enabledSelectionToggle && !(QApplication::mouseButtons() & Qt::LeftButton)) {
            initializeSelectionToggle();
        }

        hoverSequenceStarted();

        const KConfigGroup globalConfig(KSharedConfig::openConfig(), QStringLiteral("PreviewSettings"));
        const int interval = globalConfig.readEntry("HoverSequenceInterval", 700);

        m_hoverSequenceTimer.start(interval);
    } else {
        setHoverOpacity(0.0);

        if (m_selectionToggle) {
            m_selectionToggle->deleteLater();
            m_selectionToggle = nullptr;
        }

        hoverSequenceEnded();
        m_hoverSequenceTimer.stop();
    }

    hoveredChanged(hovered);
    update();
}

bool KItemListWidget::isHovered() const
{
    return m_hovered;
}

void KItemListWidget::setExpansionAreaHovered(bool hovered)
{
    if (hovered == m_expansionAreaHovered) {
        return;
    }
    m_expansionAreaHovered = hovered;
    update();
}

bool KItemListWidget::expansionAreaHovered() const
{
    return m_expansionAreaHovered;
}

void KItemListWidget::setHoverPosition(const QPointF &pos)
{
    if (m_selectionToggle) {
        m_selectionToggle->setHovered(selectionToggleRect().contains(pos));
    }
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

        // We want the change to take effect immediately.
        if (m_enabledSelectionToggle) {
            if (m_hovered) {
                initializeSelectionToggle();
            }
        } else if (m_selectionToggle) {
            m_selectionToggle->deleteLater();
            m_selectionToggle = nullptr;
        }

        update();
    }
}

bool KItemListWidget::enabledSelectionToggle() const
{
    return m_enabledSelectionToggle;
}

void KItemListWidget::setSiblingsInformation(const QBitArray &siblings)
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

void KItemListWidget::setEditedRole(const QByteArray &role)
{
    if (m_editedRole != role) {
        const QByteArray previous = m_editedRole;
        m_editedRole = role;
        editedRoleChanged(role, previous);
    }
}

QByteArray KItemListWidget::editedRole() const
{
    return m_editedRole;
}

void KItemListWidget::setIconSize(int iconSize)
{
    if (m_iconSize != iconSize) {
        const int previousIconSize = m_iconSize;
        m_iconSize = iconSize;
        iconSizeChanged(iconSize, previousIconSize);
    }
}

int KItemListWidget::iconSize() const
{
    return m_iconSize;
}

bool KItemListWidget::contains(const QPointF &point) const
{
    if (!QGraphicsWidget::contains(point)) {
        return false;
    }

    return iconRect().contains(point) || textRect().contains(point) || expansionToggleRect().contains(point) || selectionToggleRect().contains(point);
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

QPixmap KItemListWidget::createDragPixmap(const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPixmap pixmap(size().toSize() * widget->devicePixelRatio());
    pixmap.setDevicePixelRatio(widget->devicePixelRatio());
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);

    const bool oldAlternateBackground = m_alternateBackground;
    const bool wasSelected = m_selected;
    const bool wasHovered = m_hovered;

    setAlternateBackground(false);
    setHovered(false);

    paint(&painter, option, widget);

    setAlternateBackground(oldAlternateBackground);
    setSelected(wasSelected);
    setHovered(wasHovered);

    return pixmap;
}

void KItemListWidget::startActivateSoonAnimation(int timeUntilActivation)
{
    Q_UNUSED(timeUntilActivation)
}

void KItemListWidget::dataChanged(const QHash<QByteArray, QVariant> &current, const QSet<QByteArray> &roles)
{
    Q_UNUSED(current)
    Q_UNUSED(roles)
}

void KItemListWidget::visibleRolesChanged(const QList<QByteArray> &current, const QList<QByteArray> &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListWidget::columnWidthChanged(const QByteArray &role, qreal current, qreal previous)
{
    Q_UNUSED(role)
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListWidget::sidePaddingChanged(qreal width)
{
    Q_UNUSED(width)
}

void KItemListWidget::styleOptionChanged(const KItemListStyleOption &current, const KItemListStyleOption &previous)
{
    Q_UNUSED(previous)

    // set the initial value of m_iconSize if not set
    if (m_iconSize == -1) {
        m_iconSize = current.iconSize;
    }
}

void KItemListWidget::currentChanged(bool current)
{
    Q_UNUSED(current)
}

void KItemListWidget::selectedChanged(bool selected)
{
    Q_UNUSED(selected)
}

void KItemListWidget::hoveredChanged(bool hovered)
{
    Q_UNUSED(hovered)
}

void KItemListWidget::alternateBackgroundChanged(bool enabled)
{
    Q_UNUSED(enabled)
}

void KItemListWidget::siblingsInformationChanged(const QBitArray &current, const QBitArray &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListWidget::editedRoleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListWidget::iconSizeChanged(int current, int previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KItemListWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    QGraphicsWidget::resizeEvent(event);
    clearHoverCache();

    if (m_selectionToggle) {
        const QRectF &toggleRect = selectionToggleRect();
        m_selectionToggle->setPos(toggleRect.topLeft());
        m_selectionToggle->resize(toggleRect.size());
    }
}

void KItemListWidget::hoverSequenceStarted()
{
}

void KItemListWidget::hoverSequenceIndexChanged(int sequenceIndex)
{
    Q_UNUSED(sequenceIndex);
}

void KItemListWidget::hoverSequenceEnded()
{
}

qreal KItemListWidget::hoverOpacity() const
{
    return m_hoverOpacity;
}

int KItemListWidget::hoverSequenceIndex() const
{
    return m_hoverSequenceIndex;
}

void KItemListWidget::slotHoverSequenceTimerTimeout()
{
    m_hoverSequenceIndex++;
    hoverSequenceIndexChanged(m_hoverSequenceIndex);
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
        m_hoverCache = nullptr;
    }

    update();
}

void KItemListWidget::clearHoverCache()
{
    delete m_hoverCache;
    m_hoverCache = nullptr;
}

void KItemListWidget::drawItemStyleOption(QPainter *painter, QWidget *widget, QStyle::State styleState)
{
    QStyleOptionViewItem viewItemOption;
    initStyleOption(&viewItemOption);
    viewItemOption.state = styleState;
    viewItemOption.viewItemPosition = QStyleOptionViewItem::OnlyOne;
    viewItemOption.showDecorationSelected = true;
    viewItemOption.rect = selectionRect().toRect().adjusted(2, 2, -2, -2);
    QPainterPath path;
    path.addRoundedRect(viewItemOption.rect, 5, 5);
    QColor accentColor{widget->palette().color(QPalette::Accent)};
    painter->setRenderHint(QPainter::Antialiasing);

    bool current = m_current && styleState & QStyle::State_Active;
    accentColor.setAlphaF(0.0);
    if (m_selected && m_hovered) {
        accentColor.setAlphaF(0.5);
    } else if (m_selected) {
        accentColor.setAlphaF(0.4);
    } else if (m_hovered && current) {
        accentColor.setAlphaF(0.3);
    } else if (m_hovered) {
        accentColor.setAlphaF(0.1);
    }
    painter->fillPath(path, accentColor);
    if (current && m_hovered) {
        accentColor.setAlphaF(1.0);
    } else if (current || m_hovered) {
        accentColor.setAlphaF(0.9);
    } else if (m_current) {
        accentColor.setAlphaF(0.3);
    }
    if (m_current || m_hovered) {
        const QPen pen{accentColor, 2};
        painter->setPen(pen);
        painter->drawPath(path);
    }
    // style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &viewItemOption, painter, widget);
}

#include "moc_kitemlistwidget.cpp"
