/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "kfileitemlistwidget.h"

#include "kfileitemclipboard_p.h"
#include "kfileitemlistview.h"
#include "kfileitemmodel.h"
#include "kpixmapmodifier_p.h"

#include <KIcon>
#include <KIconEffect>
#include <KIconLoader>
#include <KLocale>
#include <KStringHandler>
#include <KDebug>

#include <QFontMetricsF>
#include <QGraphicsSceneResizeEvent>
#include <QPainter>
#include <QStyleOption>
#include <QTextLayout>
#include <QTextLine>

// #define KFILEITEMLISTWIDGET_DEBUG

KFileItemListWidget::KFileItemListWidget(QGraphicsItem* parent) :
    KItemListWidget(parent),
    m_isCut(false),
    m_isHidden(false),
    m_isExpandable(false),
    m_supportsItemExpanding(false),
    m_dirtyLayout(true),
    m_dirtyContent(true),
    m_dirtyContentRoles(),
    m_layout(IconsLayout),
    m_pixmapPos(),
    m_pixmap(),
    m_scaledPixmapSize(),
    m_iconRect(),
    m_hoverPixmap(),
    m_textInfo(),
    m_textRect(),
    m_sortedVisibleRoles(),
    m_expansionArea(),
    m_customTextColor(),
    m_additionalInfoTextColor(),
    m_overlay()
{
}

KFileItemListWidget::~KFileItemListWidget()
{
    qDeleteAll(m_textInfo);
    m_textInfo.clear();
}

void KFileItemListWidget::setLayout(Layout layout)
{
    if (m_layout != layout) {
        m_layout = layout;
        m_dirtyLayout = true;
        updateAdditionalInfoTextColor();
        update();
    }
}

KFileItemListWidget::Layout KFileItemListWidget::layout() const
{
    return m_layout;
}

void KFileItemListWidget::setSupportsItemExpanding(bool supportsItemExpanding)
{
    if (m_supportsItemExpanding != supportsItemExpanding) {
        m_supportsItemExpanding = supportsItemExpanding;
        m_dirtyLayout = true;
        update();
    }
}

bool KFileItemListWidget::supportsItemExpanding() const
{
    return m_supportsItemExpanding;
}

void KFileItemListWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();

    KItemListWidget::paint(painter, option, widget);

    if (!m_expansionArea.isEmpty()) {
        drawSiblingsInformation(painter);
    }

    const KItemListStyleOption& itemListStyleOption = styleOption();
    if (isHovered()) {
        // Blend the unhovered and hovered pixmap if the hovering
        // animation is ongoing
        if (hoverOpacity() < 1.0) {
            drawPixmap(painter, m_pixmap);
        }

        const qreal opacity = painter->opacity();
        painter->setOpacity(hoverOpacity() * opacity);
        drawPixmap(painter, m_hoverPixmap);
        painter->setOpacity(opacity);
    } else {
        drawPixmap(painter, m_pixmap);
    }

    painter->setFont(itemListStyleOption.font);
    painter->setPen(textColor());
    const TextInfo* textInfo = m_textInfo.value("name");
    painter->drawStaticText(textInfo->pos, textInfo->staticText);

    bool clipAdditionalInfoBounds = false;
    if (m_supportsItemExpanding) {
        // Prevent a possible overlapping of the additional-information texts
        // with the icon. This can happen if the user has minimized the width
        // of the name-column to a very small value.
        const qreal minX = m_pixmapPos.x() + m_pixmap.width() + 4 * itemListStyleOption.padding;
        if (textInfo->pos.x() + columnWidth("name") > minX) {
            clipAdditionalInfoBounds = true;
            painter->save();
            painter->setClipRect(minX, 0, size().width() - minX, size().height(), Qt::IntersectClip);
        }
    }

    painter->setPen(m_additionalInfoTextColor);
    painter->setFont(itemListStyleOption.font);

    for (int i = 1; i < m_sortedVisibleRoles.count(); ++i) {
        const TextInfo* textInfo = m_textInfo.value(m_sortedVisibleRoles[i]);
        painter->drawStaticText(textInfo->pos, textInfo->staticText);
    }

    if (clipAdditionalInfoBounds) {
        painter->restore();
    }

#ifdef KFILEITEMLISTWIDGET_DEBUG
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::green);
    painter->drawRect(m_iconRect);

    painter->setPen(Qt::red);
    painter->drawText(QPointF(0, itemListStyleOption.fontMetrics.height()), QString::number(index()));
    painter->drawRect(rect());
#endif
}

QRectF KFileItemListWidget::iconRect() const
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();
    return m_iconRect;
}

QRectF KFileItemListWidget::textRect() const
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();
    return m_textRect;
}

QRectF KFileItemListWidget::textFocusRect() const
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();
    if (m_layout == CompactLayout) {
        // In the compact layout a larger textRect() is returned to be aligned
        // with the iconRect(). This is useful to have a larger selection/hover-area
        // when having a quite large icon size but only one line of text. Still the
        // focus rectangle should be shown as narrow as possible around the text.
        QRectF rect = m_textRect;
        const TextInfo* topText    = m_textInfo.value(m_sortedVisibleRoles.first());
        const TextInfo* bottomText = m_textInfo.value(m_sortedVisibleRoles.last());
        rect.setTop(topText->pos.y());
        rect.setBottom(bottomText->pos.y() + bottomText->staticText.size().height());
        return rect;
    }
    return m_textRect;
}

QRectF KFileItemListWidget::expansionToggleRect() const
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();
    return m_isExpandable ? m_expansionArea : QRectF();
}

QRectF KFileItemListWidget::selectionToggleRect() const
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();

    const int iconHeight = styleOption().iconSize;

    int toggleSize = KIconLoader::SizeSmall;
    if (iconHeight >= KIconLoader::SizeEnormous) {
        toggleSize = KIconLoader::SizeMedium;
    } else if (iconHeight >= KIconLoader::SizeLarge) {
        toggleSize = KIconLoader::SizeSmallMedium;
    }

    QPointF pos = iconRect().topLeft();

    // If the selection toggle has a very small distance to the
    // widget borders, the size of the selection toggle will get
    // increased to prevent an accidental clicking of the item
    // when trying to hit the toggle.
    const int widgetHeight = size().height();
    const int widgetWidth = size().width();
    const int minMargin = 2;

    if (toggleSize + minMargin * 2 >= widgetHeight) {
        pos.rx() -= (widgetHeight - toggleSize) / 2;
        toggleSize = widgetHeight;
        pos.setY(0);
    }
    if (toggleSize + minMargin * 2 >= widgetWidth) {
        pos.ry() -= (widgetWidth - toggleSize) / 2;
        toggleSize = widgetWidth;
        pos.setX(0);
    }

    return QRectF(pos, QSizeF(toggleSize, toggleSize));
}

QSizeF KFileItemListWidget::itemSizeHint(int index, const KItemListView* view)
{
    const QHash<QByteArray, QVariant> values = view->model()->data(index);
    const KItemListStyleOption& option = view->styleOption();
    const int additionalRolesCount = qMax(view->visibleRoles().count() - 1, 0);

    switch (static_cast<const KFileItemListView*>(view)->itemLayout()) {
    case IconsLayout: {
        const QString text = KStringHandler::preProcessWrap(values["name"].toString());

        const qreal maxWidth = view->itemSize().width() - 2 * option.padding;
        QTextLine line;

        // Calculate the number of lines required for wrapping the name
        QTextOption textOption(Qt::AlignHCenter);
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

        qreal textHeight = 0;
        QTextLayout layout(text, option.font);
        layout.setTextOption(textOption);
        layout.beginLayout();
        while ((line = layout.createLine()).isValid()) {
            line.setLineWidth(maxWidth);
            line.naturalTextWidth();
            textHeight += line.height();
        }
        layout.endLayout();

        // Add one line for each additional information
        const qreal height = textHeight +
                             additionalRolesCount * option.fontMetrics.lineSpacing() +
                             option.iconSize +
                             option.padding * 3;
        return QSizeF(view->itemSize().width(), height);
    }

    case CompactLayout: {
        // For each row exactly one role is shown. Calculate the maximum required width that is necessary
        // to show all roles without horizontal clipping.
        qreal maximumRequiredWidth = 0.0;

        foreach (const QByteArray& role, view->visibleRoles()) {
            const QString text = KFileItemListWidget::roleText(role, values);
            const qreal requiredWidth = option.fontMetrics.width(text);
            maximumRequiredWidth = qMax(maximumRequiredWidth, requiredWidth);
        }

        const qreal width = option.padding * 4 + option.iconSize + maximumRequiredWidth;
        const qreal height = option.padding * 2 + qMax(option.iconSize, (1 + additionalRolesCount) * option.fontMetrics.lineSpacing());
        return QSizeF(width, height);
    }

    case DetailsLayout: {
        const qreal height = option.padding * 2 + qMax(option.iconSize, option.fontMetrics.height());
        return QSizeF(-1, height);
    }

    default:
        Q_ASSERT(false);
        break;
    }

    return QSize();
}

qreal KFileItemListWidget::preferredRoleColumnWidth(const QByteArray& role,
                                                    int index,
                                                    const KItemListView* view)
{
    qreal width = 0;

    const QHash<QByteArray, QVariant> values = view->model()->data(index);
    const KItemListStyleOption& option = view->styleOption();

    const QString text = KFileItemListWidget::roleText(role, values);
    if (!text.isEmpty()) {
        const qreal columnPadding = option.padding * 3;
        width = qMax(width, qreal(2 * columnPadding + option.fontMetrics.width(text)));
    }

    if (role == "name") {
        // Increase the width by the expansion-toggle and the current expansion level
        const int expandedParentsCount = values.value("expandedParentsCount", 0).toInt();
        width += option.padding + (expandedParentsCount + 1) * view->itemSize().height() + KIconLoader::SizeSmall;

        // Increase the width by the required space for the icon
        width += option.padding * 2 + option.iconSize;
    }

    return width;
}

void KFileItemListWidget::invalidateCache()
{
    m_dirtyLayout = true;
    m_dirtyContent = true;
}

void KFileItemListWidget::refreshCache()
{
}

void KFileItemListWidget::setTextColor(const QColor& color)
{
    if (color != m_customTextColor) {
        m_customTextColor = color;
        updateAdditionalInfoTextColor();
        update();
    }
}

QColor KFileItemListWidget::textColor() const
{
    if (m_customTextColor.isValid() && !isSelected()) {
        return m_customTextColor;
    }

    const QPalette::ColorGroup group = isActiveWindow() ? QPalette::Active : QPalette::Inactive;
    const QPalette::ColorRole role = isSelected() ? QPalette::HighlightedText : QPalette::Text;
    return styleOption().palette.brush(group, role).color();
}

void KFileItemListWidget::setOverlay(const QPixmap& overlay)
{
    m_overlay = overlay;
    m_dirtyContent = true;
    update();
}

QPixmap KFileItemListWidget::overlay() const
{
    return m_overlay;
}

void KFileItemListWidget::dataChanged(const QHash<QByteArray, QVariant>& current,
                                      const QSet<QByteArray>& roles)
{
    Q_UNUSED(current);

    m_dirtyContent = true;

    QSet<QByteArray> dirtyRoles;
    if (roles.isEmpty()) {
        dirtyRoles = visibleRoles().toSet();
        dirtyRoles.insert("iconPixmap");
        dirtyRoles.insert("iconName");
    } else {
        dirtyRoles = roles;
    }

    QSetIterator<QByteArray> it(dirtyRoles);
    while (it.hasNext()) {
        const QByteArray& role = it.next();
        m_dirtyContentRoles.insert(role);
    }
}

void KFileItemListWidget::visibleRolesChanged(const QList<QByteArray>& current,
                                              const QList<QByteArray>& previous)
{
    Q_UNUSED(previous);
    m_sortedVisibleRoles = current;
    m_dirtyLayout = true;
}

void KFileItemListWidget::columnWidthChanged(const QByteArray& role,
                                             qreal current,
                                             qreal previous)
{
    Q_UNUSED(role);
    Q_UNUSED(current);
    Q_UNUSED(previous);
    m_dirtyLayout = true;
}

void KFileItemListWidget::styleOptionChanged(const KItemListStyleOption& current,
                                             const KItemListStyleOption& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    updateAdditionalInfoTextColor();
    m_dirtyLayout = true;
}

void KFileItemListWidget::hoveredChanged(bool hovered)
{
    Q_UNUSED(hovered);
    m_dirtyLayout = true;
}

void KFileItemListWidget::selectedChanged(bool selected)
{
    Q_UNUSED(selected);
    updateAdditionalInfoTextColor();
}

void KFileItemListWidget::siblingsInformationChanged(const QBitArray& current, const QBitArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    m_dirtyLayout = true;
}


void KFileItemListWidget::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    KItemListWidget::resizeEvent(event);
    m_dirtyLayout = true;
}

void KFileItemListWidget::showEvent(QShowEvent* event)
{
    KItemListWidget::showEvent(event);

    // Listen to changes of the clipboard to mark the item as cut/uncut
    KFileItemClipboard* clipboard = KFileItemClipboard::instance();

    const KUrl itemUrl = data().value("url").value<KUrl>();
    m_isCut = clipboard->isCut(itemUrl);

    connect(clipboard, SIGNAL(cutItemsChanged()),
            this, SLOT(slotCutItemsChanged()));
}

void KFileItemListWidget::hideEvent(QHideEvent* event)
{
    disconnect(KFileItemClipboard::instance(), SIGNAL(cutItemsChanged()),
               this, SLOT(slotCutItemsChanged()));

    KItemListWidget::hideEvent(event);
}

void KFileItemListWidget::slotCutItemsChanged()
{
    const KUrl itemUrl = data().value("url").value<KUrl>();
    const bool isCut = KFileItemClipboard::instance()->isCut(itemUrl);
    if (m_isCut != isCut) {
        m_isCut = isCut;
        m_pixmap = QPixmap();
        m_dirtyContent = true;
        update();
    }
}

void KFileItemListWidget::triggerCacheRefreshing()
{
    if ((!m_dirtyContent && !m_dirtyLayout) || index() < 0) {
        return;
    }

    refreshCache();

    const QHash<QByteArray, QVariant> values = data();
    m_isExpandable = m_supportsItemExpanding && values["isExpandable"].toBool();
    m_isHidden = values["name"].toString().startsWith(QLatin1Char('.'));

    updateExpansionArea();
    updateTextsCache();
    updatePixmapCache();

    m_dirtyLayout = false;
    m_dirtyContent = false;
    m_dirtyContentRoles.clear();
}

void KFileItemListWidget::updateExpansionArea()
{
    if (m_supportsItemExpanding) {
        const QHash<QByteArray, QVariant> values = data();
        Q_ASSERT(values.contains("expandedParentsCount"));
        const int expandedParentsCount = values.value("expandedParentsCount", 0).toInt();
        if (expandedParentsCount >= 0) {
            const qreal widgetHeight = size().height();
            const qreal inc = (widgetHeight - KIconLoader::SizeSmall) / 2;
            const qreal x = expandedParentsCount * widgetHeight + inc;
            const qreal y = inc;
            m_expansionArea = QRectF(x, y, KIconLoader::SizeSmall, KIconLoader::SizeSmall);
            return;
        }
    }

    m_expansionArea = QRectF();
}

void KFileItemListWidget::updatePixmapCache()
{
    // Precondition: Requires already updated m_textPos values to calculate
    // the remaining height when the alignment is vertical.

    const QSizeF widgetSize = size();
    const bool iconOnTop = (m_layout == IconsLayout);
    const KItemListStyleOption& option = styleOption();
    const qreal padding = option.padding;

    const int maxIconWidth = iconOnTop ? widgetSize.width() - 2 * padding : option.iconSize;
    const int maxIconHeight = option.iconSize;

    const QHash<QByteArray, QVariant> values = data();

    bool updatePixmap = (m_pixmap.width() != maxIconWidth || m_pixmap.height() != maxIconHeight);
    if (!updatePixmap && m_dirtyContent) {
        updatePixmap = m_dirtyContentRoles.isEmpty()
                       || m_dirtyContentRoles.contains("iconPixmap")
                       || m_dirtyContentRoles.contains("iconName")
                       || m_dirtyContentRoles.contains("iconOverlays");
    }

    if (updatePixmap) {
        m_pixmap = values["iconPixmap"].value<QPixmap>();
        if (m_pixmap.isNull()) {
            // Use the icon that fits to the MIME-type
            QString iconName = values["iconName"].toString();
            if (iconName.isEmpty()) {
                // The icon-name has not been not resolved by KFileItemModelRolesUpdater,
                // use a generic icon as fallback
                iconName = QLatin1String("unknown");
            }
            m_pixmap = pixmapForIcon(iconName, maxIconHeight);
        } else if (m_pixmap.width() != maxIconWidth || m_pixmap.height() != maxIconHeight) {
            // A custom pixmap has been applied. Assure that the pixmap
            // is scaled to the maximum available size.
            KPixmapModifier::scale(m_pixmap, QSize(maxIconWidth, maxIconHeight));
        }

        const QStringList overlays = values["iconOverlays"].toStringList();

        // Strangely KFileItem::overlays() returns empty string-values, so
        // we need to check first whether an overlay must be drawn at all.
        // It is more efficient to do it here, as KIconLoader::drawOverlays()
        // assumes that an overlay will be drawn and has some additional
        // setup time.
        foreach (const QString& overlay, overlays) {
            if (!overlay.isEmpty()) {
                // There is at least one overlay, draw all overlays above m_pixmap
                // and cancel the check
                KIconLoader::global()->drawOverlays(overlays, m_pixmap, KIconLoader::Desktop);
                break;
            }
        }

        if (m_isCut) {
            applyCutEffect(m_pixmap);
        }

        if (m_isHidden) {
            applyHiddenEffect(m_pixmap);
        }
    }

    if (!m_overlay.isNull()) {
        QPainter painter(&m_pixmap);
        painter.drawPixmap(0, m_pixmap.height() - m_overlay.height(), m_overlay);
    }

    int scaledIconSize = 0;
    if (iconOnTop) {
        const TextInfo* textInfo = m_textInfo.value("name");
        scaledIconSize = static_cast<int>(textInfo->pos.y() - 2 * padding);
    } else {
        const int textRowsCount = (m_layout == CompactLayout) ? visibleRoles().count() : 1;
        const qreal requiredTextHeight = textRowsCount * option.fontMetrics.height();
        scaledIconSize = (requiredTextHeight < maxIconHeight) ?
                           widgetSize.height() - 2 * padding : maxIconHeight;
    }

    const int maxScaledIconWidth = iconOnTop ? widgetSize.width() - 2 * padding : scaledIconSize;
    const int maxScaledIconHeight = scaledIconSize;

    m_scaledPixmapSize = m_pixmap.size();
    m_scaledPixmapSize.scale(maxScaledIconWidth, maxScaledIconHeight, Qt::KeepAspectRatio);

    if (iconOnTop) {
        // Center horizontally and align on bottom within the icon-area
        m_pixmapPos.setX((widgetSize.width() - m_scaledPixmapSize.width()) / 2);
        m_pixmapPos.setY(padding + scaledIconSize - m_scaledPixmapSize.height());
    } else {
        // Center horizontally and vertically within the icon-area
        const TextInfo* textInfo = m_textInfo.value("name");
        m_pixmapPos.setX(textInfo->pos.x() - 2 * padding
                         - (scaledIconSize + m_scaledPixmapSize.width()) / 2);
        m_pixmapPos.setY(padding
                         + (scaledIconSize - m_scaledPixmapSize.height()) / 2);
    }

    m_iconRect = QRectF(m_pixmapPos, QSizeF(m_scaledPixmapSize));

    // Prepare the pixmap that is used when the item gets hovered
    if (isHovered()) {
        m_hoverPixmap = m_pixmap;
        KIconEffect* effect = KIconLoader::global()->iconEffect();
        // In the KIconLoader terminology, active = hover.
        if (effect->hasEffect(KIconLoader::Desktop, KIconLoader::ActiveState)) {
            m_hoverPixmap = effect->apply(m_pixmap, KIconLoader::Desktop, KIconLoader::ActiveState);
        } else {
            m_hoverPixmap = m_pixmap;
        }
    } else if (hoverOpacity() <= 0.0) {
        // No hover animation is ongoing. Clear m_hoverPixmap to save memory.
        m_hoverPixmap = QPixmap();
    }
}

void KFileItemListWidget::updateTextsCache()
{
    QTextOption textOption;
    switch (m_layout) {
    case IconsLayout:
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        textOption.setAlignment(Qt::AlignHCenter);
        break;
    case CompactLayout:
    case DetailsLayout:
        textOption.setAlignment(Qt::AlignLeft);
        textOption.setWrapMode(QTextOption::NoWrap);
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    qDeleteAll(m_textInfo);
    m_textInfo.clear();
    for (int i = 0; i < m_sortedVisibleRoles.count(); ++i) {
        TextInfo* textInfo = new TextInfo();
        textInfo->staticText.setTextFormat(Qt::PlainText);
        textInfo->staticText.setPerformanceHint(QStaticText::AggressiveCaching);
        textInfo->staticText.setTextOption(textOption);
        m_textInfo.insert(m_sortedVisibleRoles[i], textInfo);
    }

    switch (m_layout) {
    case IconsLayout:   updateIconsLayoutTextCache(); break;
    case CompactLayout: updateCompactLayoutTextCache(); break;
    case DetailsLayout: updateDetailsLayoutTextCache(); break;
    default: Q_ASSERT(false); break;
    }
}

void KFileItemListWidget::updateIconsLayoutTextCache()
{
    //      +------+
    //      | Icon |
    //      +------+
    //
    //    Name role that
    // might get wrapped above
    //    several lines.
    //  Additional role 1
    //  Additional role 2

    const QHash<QByteArray, QVariant> values = data();

    const KItemListStyleOption& option = styleOption();
    const qreal padding = option.padding;
    const qreal maxWidth = size().width() - 2 * padding;
    const qreal widgetHeight = size().height();
    const qreal lineSpacing = option.fontMetrics.lineSpacing();

    // Initialize properties for the "name" role. It will be used as anchor
    // for initializing the position of the other roles.
    TextInfo* nameTextInfo = m_textInfo.value("name");
    nameTextInfo->staticText.setText(KStringHandler::preProcessWrap(values["name"].toString()));

    // Calculate the number of lines required for the name and the required width
    qreal nameWidth = 0;
    qreal nameHeight = 0;
    QTextLine line;

    QTextLayout layout(nameTextInfo->staticText.text(), option.font);
    layout.setTextOption(nameTextInfo->staticText.textOption());
    layout.beginLayout();
    while ((line = layout.createLine()).isValid()) {
        line.setLineWidth(maxWidth);
        nameWidth = qMax(nameWidth, line.naturalTextWidth());
        nameHeight += line.height();
    }
    layout.endLayout();

    // Use one line for each additional information
    const int additionalRolesCount = qMax(visibleRoles().count() - 1, 0);
    nameTextInfo->staticText.setTextWidth(maxWidth);
    nameTextInfo->pos = QPointF(padding, widgetHeight -
                                         nameHeight -
                                         additionalRolesCount * lineSpacing -
                                         padding);
    m_textRect = QRectF(padding + (maxWidth - nameWidth) / 2,
                        nameTextInfo->pos.y(),
                        nameWidth,
                        nameHeight);

    // Calculate the position for each additional information
    qreal y = nameTextInfo->pos.y() + nameHeight;
    foreach (const QByteArray& role, m_sortedVisibleRoles) {
        if (role == "name") {
            continue;
        }

        const QString text = roleText(role, values);
        TextInfo* textInfo = m_textInfo.value(role);
        textInfo->staticText.setText(text);

        qreal requiredWidth = 0;

        QTextLayout layout(text, option.font);
        layout.setTextOption(textInfo->staticText.textOption());
        layout.beginLayout();
        QTextLine textLine = layout.createLine();
        if (textLine.isValid()) {
            textLine.setLineWidth(maxWidth);
            requiredWidth = textLine.naturalTextWidth();
            if (textLine.textLength() < text.length()) {
                // TODO: QFontMetrics::elidedText() works different regarding the given width
                // in comparison to QTextLine::setLineWidth(). It might happen that the text does
                // not get elided although it does not fit into the given width. As workaround
                // the padding is substracted.
                const QString elidedText = option.fontMetrics.elidedText(text, Qt::ElideRight, maxWidth - padding);
                textInfo->staticText.setText(elidedText);
            }
        }
        layout.endLayout();

        textInfo->pos = QPointF(padding, y);
        textInfo->staticText.setTextWidth(maxWidth);

        const QRectF textRect(padding + (maxWidth - requiredWidth) / 2, y, requiredWidth, lineSpacing);
        m_textRect |= textRect;

        y += lineSpacing;
    }

    // Add a padding to the text rectangle
    m_textRect.adjust(-padding, -padding, padding, padding);
}

void KFileItemListWidget::updateCompactLayoutTextCache()
{
    // +------+  Name role
    // | Icon |  Additional role 1
    // +------+  Additional role 2

    const QHash<QByteArray, QVariant> values = data();

    const KItemListStyleOption& option = styleOption();
    const qreal widgetHeight = size().height();
    const qreal lineSpacing = option.fontMetrics.lineSpacing();
    const qreal textLinesHeight = qMax(visibleRoles().count(), 1) * lineSpacing;
    const int scaledIconSize = (textLinesHeight < option.iconSize) ? widgetHeight - 2 * option.padding : option.iconSize;

    qreal maximumRequiredTextWidth = 0;
    const qreal x = option.padding * 3 + scaledIconSize;
    qreal y = qRound((widgetHeight - textLinesHeight) / 2);
    const qreal maxWidth = size().width() - x - option.padding;
    foreach (const QByteArray& role, m_sortedVisibleRoles) {
        const QString text = roleText(role, values);
        TextInfo* textInfo = m_textInfo.value(role);
        textInfo->staticText.setText(text);

        qreal requiredWidth = option.fontMetrics.width(text);
        if (requiredWidth > maxWidth) {
            requiredWidth = maxWidth;
            const QString elidedText = option.fontMetrics.elidedText(text, Qt::ElideRight, maxWidth);
            textInfo->staticText.setText(elidedText);
        }

        textInfo->pos = QPointF(x, y);
        textInfo->staticText.setTextWidth(maxWidth);

        maximumRequiredTextWidth = qMax(maximumRequiredTextWidth, requiredWidth);

        y += lineSpacing;
    }

    m_textRect = QRectF(x - option.padding, 0, maximumRequiredTextWidth + 2 * option.padding, widgetHeight);
}

void KFileItemListWidget::updateDetailsLayoutTextCache()
{
    // Precondition: Requires already updated m_expansionArea
    // to determine the left position.

    // +------+
    // | Icon |  Name role   Additional role 1   Additional role 2
    // +------+
    m_textRect = QRectF();

    const KItemListStyleOption& option = styleOption();
    const QHash<QByteArray, QVariant> values = data();

    const qreal widgetHeight = size().height();
    const int scaledIconSize = widgetHeight - 2 * option.padding;
    const int fontHeight = option.fontMetrics.height();

    const qreal columnPadding = option.padding * 3;
    qreal firstColumnInc = scaledIconSize;
    if (m_supportsItemExpanding) {
        firstColumnInc += (m_expansionArea.left() + m_expansionArea.right() + widgetHeight) / 2;
    } else {
        firstColumnInc += option.padding;
    }

    qreal x = firstColumnInc;
    const qreal y = qMax(qreal(option.padding), (widgetHeight - fontHeight) / 2);

    foreach (const QByteArray& role, m_sortedVisibleRoles) {
        const RoleType type = roleType(role);

        QString text = roleText(role, values);

        // Elide the text in case it does not fit into the available column-width
        qreal requiredWidth = option.fontMetrics.width(text);
        const qreal roleWidth = columnWidth(role);
        qreal availableTextWidth = roleWidth - 2 * columnPadding;
        if (type == Name) {
            availableTextWidth -= firstColumnInc;
        }

        if (requiredWidth > availableTextWidth) {
            text = option.fontMetrics.elidedText(text, Qt::ElideRight, availableTextWidth);
            requiredWidth = option.fontMetrics.width(text);
        }

        TextInfo* textInfo = m_textInfo.value(role);
        textInfo->staticText.setText(text);
        textInfo->pos = QPointF(x + columnPadding, y);
        x += roleWidth;

        switch (type) {
        case Name: {
            const qreal textWidth = option.extendedSelectionRegion
                                    ? size().width() - textInfo->pos.x()
                                    : requiredWidth + 2 * option.padding;
            m_textRect = QRectF(textInfo->pos.x() - option.padding, 0,
                                textWidth, size().height());

            // The column after the name should always be aligned on the same x-position independent
            // from the expansion-level shown in the name column
            x -= firstColumnInc;
            break;
        }
        case Size:
            // The values for the size should be right aligned
            textInfo->pos.rx() += roleWidth - requiredWidth - 2 * columnPadding;
            break;

        default:
            break;
        }
    }
}

void KFileItemListWidget::updateAdditionalInfoTextColor()
{
    QColor c1;
    if (m_customTextColor.isValid()) {
        c1 = m_customTextColor;
    } else if (isSelected() && m_layout != DetailsLayout) {
        c1 = styleOption().palette.highlightedText().color();
    } else {
        c1 = styleOption().palette.text().color();
    }

    // For the color of the additional info the inactive text color
    // is not used as this might lead to unreadable text for some color schemes. Instead
    // the text color c1 is slightly mixed with the background color.
    const QColor c2 = styleOption().palette.base().color();
    const int p1 = 70;
    const int p2 = 100 - p1;
    m_additionalInfoTextColor = QColor((c1.red()   * p1 + c2.red()   * p2) / 100,
                                       (c1.green() * p1 + c2.green() * p2) / 100,
                                       (c1.blue()  * p1 + c2.blue()  * p2) / 100);
}

void KFileItemListWidget::drawPixmap(QPainter* painter, const QPixmap& pixmap)
{
    if (m_scaledPixmapSize != pixmap.size()) {
        QPixmap scaledPixmap = pixmap;
        KPixmapModifier::scale(scaledPixmap, m_scaledPixmapSize);
        painter->drawPixmap(m_pixmapPos, scaledPixmap);

#ifdef KFILEITEMLISTWIDGET_DEBUG
        painter->setPen(Qt::blue);
        painter->drawRect(QRectF(m_pixmapPos, QSizeF(m_scaledPixmapSize)));
#endif
    } else {
        painter->drawPixmap(m_pixmapPos, pixmap);
    }
}

void KFileItemListWidget::drawSiblingsInformation(QPainter* painter)
{
    const int siblingSize = size().height();
    const int x = (m_expansionArea.left() + m_expansionArea.right() - siblingSize) / 2;
    QRect siblingRect(x, 0, siblingSize, siblingSize);

    QStyleOption option;
    bool isItemSibling = true;

    const QBitArray siblings = siblingsInformation();
    for (int i = siblings.count() - 1; i >= 0; --i) {
        option.rect = siblingRect;
        option.state = siblings.at(i) ? QStyle::State_Sibling : QStyle::State_None;

        if (isItemSibling) {
            option.state |= QStyle::State_Item;
            if (m_isExpandable) {
                option.state |= QStyle::State_Children;
            }
            if (data()["isExpanded"].toBool()) {
                option.state |= QStyle::State_Open;
            }
            isItemSibling = false;
        }

        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter);

        siblingRect.translate(-siblingRect.width(), 0);
    }
}

QPixmap KFileItemListWidget::pixmapForIcon(const QString& name, int size)
{
    const KIcon icon(name);

    int requestedSize;
    if (size <= KIconLoader::SizeSmall) {
        requestedSize = KIconLoader::SizeSmall;
    } else if (size <= KIconLoader::SizeSmallMedium) {
        requestedSize = KIconLoader::SizeSmallMedium;
    } else if (size <= KIconLoader::SizeMedium) {
        requestedSize = KIconLoader::SizeMedium;
    } else if (size <= KIconLoader::SizeLarge) {
        requestedSize = KIconLoader::SizeLarge;
    } else if (size <= KIconLoader::SizeHuge) {
        requestedSize = KIconLoader::SizeHuge;
    } else if (size <= KIconLoader::SizeEnormous) {
        requestedSize = KIconLoader::SizeEnormous;
    } else if (size <= KIconLoader::SizeEnormous * 2) {
        requestedSize = KIconLoader::SizeEnormous * 2;
    } else {
        requestedSize = size;
    }

    QPixmap pixmap = icon.pixmap(requestedSize, requestedSize);
    if (requestedSize != size) {
        KPixmapModifier::scale(pixmap, QSize(size, size));
    }

    return pixmap;
}

void KFileItemListWidget::applyCutEffect(QPixmap& pixmap)
{
    KIconEffect* effect = KIconLoader::global()->iconEffect();
    pixmap = effect->apply(pixmap, KIconLoader::Desktop, KIconLoader::DisabledState);
}

void KFileItemListWidget::applyHiddenEffect(QPixmap& pixmap)
{
    KIconEffect::semiTransparent(pixmap);
}

KFileItemListWidget::RoleType KFileItemListWidget::roleType(const QByteArray& role)
{
    static QHash<QByteArray, RoleType> rolesHash;
    if (rolesHash.isEmpty()) {
        rolesHash.insert("name", Name);
        rolesHash.insert("size", Size);
        rolesHash.insert("date", Date);
    }

    return rolesHash.value(role, Generic);
}

QString KFileItemListWidget::roleText(const QByteArray& role, const QHash<QByteArray, QVariant>& values)
{
    QString text;
    const QVariant roleValue = values.value(role);

    switch (roleType(role)) {
    case Size: {
        if (values.value("isDir").toBool()) {
            // The item represents a directory. Show the number of sub directories
            // instead of the file size of the directory.
            if (!roleValue.isNull()) {
                const int count = roleValue.toInt();
                if (count < 0) {
                    text = i18nc("@item:intable", "Unknown");
                } else {
                    text = i18ncp("@item:intable", "%1 item", "%1 items", count);
                }
            }
        } else {
            // Show the size in kilobytes (always round up)
            const KLocale* locale = KGlobal::locale();
            const int roundInc = (locale->binaryUnitDialect() == KLocale::MetricBinaryDialect) ? 499 : 511;
            const KIO::filesize_t size = roleValue.value<KIO::filesize_t>() + roundInc;
            text = locale->formatByteSize(size, 0, KLocale::DefaultBinaryDialect, KLocale::UnitKiloByte);
        }
        break;
    }

    case Date: {
        const QDateTime dateTime = roleValue.toDateTime();
        text = KGlobal::locale()->formatDateTime(dateTime);
        break;
    }

    case Name:
    case Generic:
        text = roleValue.toString();
        break;

    default:
        Q_ASSERT(false);
        break;
    }

    return text;
}
#include "kfileitemlistwidget.moc"
