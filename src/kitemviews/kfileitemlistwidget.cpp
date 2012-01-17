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
#include "kfileitemmodel.h"
#include "kitemlistview.h"
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
    m_dirtyLayout(true),
    m_dirtyContent(true),
    m_dirtyContentRoles(),
    m_layout(IconsLayout),
    m_pixmapPos(),
    m_pixmap(),
    m_scaledPixmapSize(),
    m_originalPixmapSize(),
    m_iconRect(),
    m_hoverPixmap(),
    m_textPos(),
    m_text(),
    m_textRect(),
    m_sortedVisibleRoles(),
    m_expansionArea(),
    m_customTextColor(),
    m_additionalInfoTextColor(),
    m_overlay()
{
    for (int i = 0; i < TextIdCount; ++i) {
        m_text[i].setTextFormat(Qt::PlainText);
        m_text[i].setPerformanceHint(QStaticText::AggressiveCaching);
    }
}

KFileItemListWidget::~KFileItemListWidget()
{
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

void KFileItemListWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();

    KItemListWidget::paint(painter, option, widget);

    // Draw expansion toggle '>' or 'V'
    if (m_isExpandable && !m_expansionArea.isEmpty()) {
        QStyleOption arrowOption;
        arrowOption.rect = m_expansionArea.toRect();
        const QStyle::PrimitiveElement arrow = data()["isExpanded"].toBool()
                                               ? QStyle::PE_IndicatorArrowDown : QStyle::PE_IndicatorArrowRight;
        style()->drawPrimitive(arrow, &arrowOption, painter);
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
    painter->drawStaticText(m_textPos[Name], m_text[Name]);

    bool clipAdditionalInfoBounds = false;
    if (m_layout == DetailsLayout) {
        // Prevent a possible overlapping of the additional-information texts
        // with the icon. This can happen if the user has minimized the width
        // of the name-column to a very small value.
        const qreal minX = m_pixmapPos.x() + m_pixmap.width() + 4 * itemListStyleOption.margin;
        if (m_textPos[Name + 1].x() < minX) {
            clipAdditionalInfoBounds = true;
            painter->save();
            painter->setClipRect(minX, 0, size().width() - minX, size().height(), Qt::IntersectClip);
        }
    }

    painter->setPen(m_additionalInfoTextColor);
    painter->setFont(itemListStyleOption.font);
    for (int i = Name + 1; i < TextIdCount; ++i) {
        painter->drawStaticText(m_textPos[i], m_text[i]);
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

QRectF KFileItemListWidget::expansionToggleRect() const
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();
    return m_isExpandable ? m_expansionArea : QRectF();
}

QRectF KFileItemListWidget::selectionToggleRect() const
{
    const_cast<KFileItemListWidget*>(this)->triggerCacheRefreshing();

    const int iconHeight = m_pixmap.height();

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
        toggleSize = widgetHeight;
        pos.setY(0);
    }
    if (toggleSize + minMargin * 2 >= widgetWidth) {
        toggleSize = widgetWidth;
        pos.setX(0);
    }

    return QRectF(pos, QSizeF(toggleSize, toggleSize));
}

QString KFileItemListWidget::roleText(const QByteArray& role, const QHash<QByteArray, QVariant>& values)
{
    QString text;
    const QVariant roleValue = values.value(role);

    switch (roleTextId(role)) {
    case Name:
    case Permissions:
    case Owner:
    case Group:
    case Type:
    case Destination:
    case Path:
        text = roleValue.toString();
        break;

    case Size: {
        if (values.value("isDir").toBool()) {
            // The item represents a directory. Show the number of sub directories
            // instead of the file size of the directory.
            if (roleValue.isNull()) {
                text = i18nc("@item:intable", "Unknown");
            } else {
                const KIO::filesize_t size = roleValue.value<KIO::filesize_t>();
                text = i18ncp("@item:intable", "%1 item", "%1 items", size);
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

    default:
        Q_ASSERT(false);
        break;
    }

    return text;
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
    KItemListWidget::dataChanged(current, roles);
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
    KItemListWidget::visibleRolesChanged(current, previous);
    m_sortedVisibleRoles = current;
    m_dirtyLayout = true;
}

void KFileItemListWidget::visibleRolesSizesChanged(const QHash<QByteArray, QSizeF>& current,
                                                   const QHash<QByteArray, QSizeF>& previous)
{
    KItemListWidget::visibleRolesSizesChanged(current, previous);
    m_dirtyLayout = true;
}

void KFileItemListWidget::styleOptionChanged(const KItemListStyleOption& current,
                                             const KItemListStyleOption& previous)
{
    KItemListWidget::styleOptionChanged(current, previous);
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
    m_isExpandable = values["isExpandable"].toBool();
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
    if (m_layout == DetailsLayout) {
        const QHash<QByteArray, QVariant> values = data();
        Q_ASSERT(values.contains("expansionLevel"));
        const KItemListStyleOption& option = styleOption();
        const int expansionLevel = values.value("expansionLevel", 0).toInt();
        if (expansionLevel >= 0) {
            const qreal widgetHeight = size().height();
            const qreal expansionLevelSize = KIconLoader::SizeSmall;
            const qreal x = option.margin + expansionLevel * widgetHeight;
            const qreal y = (widgetHeight - expansionLevelSize) / 2;
            m_expansionArea = QRectF(x, y, expansionLevelSize, expansionLevelSize);
            return;
        }
    }

    m_expansionArea = QRectF();
}

void KFileItemListWidget::updatePixmapCache()
{
    // Precondition: Requires already updated m_textPos values to calculate
    // the remaining height when the alignment is vertical.

    const bool iconOnTop = (m_layout == IconsLayout);
    const KItemListStyleOption& option = styleOption();
    const int iconHeight = option.iconSize;

    const QHash<QByteArray, QVariant> values = data();
    const QSizeF widgetSize = size();

    int scaledIconHeight = 0;
    if (iconOnTop) {
        scaledIconHeight = static_cast<int>(m_textPos[Name].y() - 3 * option.margin);
    } else {
        const int textRowsCount = (m_layout == CompactLayout) ? visibleRoles().count() : 1;
        const qreal requiredTextHeight = textRowsCount * option.fontMetrics.height();
        scaledIconHeight = (requiredTextHeight < iconHeight) ? widgetSize.height() - 2 * option.margin : iconHeight;
    }

    bool updatePixmap = (iconHeight != m_pixmap.height());
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
            m_pixmap = pixmapForIcon(iconName, iconHeight);
            m_originalPixmapSize = m_pixmap.size();
        } else if (m_pixmap.size() != QSize(iconHeight, iconHeight)) {
            // A custom pixmap has been applied. Assure that the pixmap
            // is scaled to the available size.
            const bool scale = m_pixmap.width() > iconHeight || m_pixmap.height() > iconHeight ||
                               (m_pixmap.width() < iconHeight && m_pixmap.height() < iconHeight);
            if (scale) {
                KPixmapModifier::scale(m_pixmap, QSize(iconHeight, iconHeight));
            }
            m_originalPixmapSize = m_pixmap.size();

            // To simplify the handling of scaling the original pixmap
            // will be embedded into a square pixmap.
            QPixmap squarePixmap(iconHeight, iconHeight);
            squarePixmap.fill(Qt::transparent);

            QPainter painter(&squarePixmap);
            const int x = (iconHeight - m_pixmap.width()) / 2;  // Center horizontally
            int y = iconHeight - m_pixmap.height();             // Move to bottom
            if (!iconOnTop) {
                y /= 2.0;                                       // Center vertically
            }
            painter.drawPixmap(x, y, m_pixmap);

            m_pixmap = squarePixmap;
        } else {
            m_originalPixmapSize = m_pixmap.size();
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

        Q_ASSERT(m_pixmap.height() == iconHeight);
    }
    if (!m_overlay.isNull()) {
        QPainter painter(&m_pixmap);
        painter.drawPixmap(0, m_pixmap.height() - m_overlay.height(), m_overlay);
    }

    m_scaledPixmapSize = QSize(scaledIconHeight, scaledIconHeight);

    if (iconOnTop) {
        m_pixmapPos.setX((widgetSize.width() - m_scaledPixmapSize.width()) / 2);
    } else {
        m_pixmapPos.setX(m_textPos[Name].x() - 2 * option.margin - scaledIconHeight);
    }
    m_pixmapPos.setY(option.margin);

    // Center the hover rectangle horizontally and align it on bottom
    qreal hoverWidth = m_originalPixmapSize.width();
    qreal hoverHeight = m_originalPixmapSize.height();
    if (scaledIconHeight != m_pixmap.height()) {
        const qreal scaleFactor = qreal(scaledIconHeight) / qreal(m_pixmap.height());
        hoverWidth  *= scaleFactor;
        hoverHeight *= scaleFactor;
    }
    const qreal hoverX = m_pixmapPos.x() + (m_scaledPixmapSize.width() - hoverWidth) / 2.0;
    qreal hoverY = m_scaledPixmapSize.height() - hoverHeight;
    if (!iconOnTop) {
        hoverY /= 2.0;
    }
    hoverY += m_pixmapPos.y();

    m_iconRect = QRectF(hoverX, hoverY, hoverWidth, hoverHeight);
    const qreal margin = option.margin;
    m_iconRect.adjust(-margin, -margin, margin, margin);
    
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

    for (int i = 0; i < TextIdCount; ++i) {
        m_text[i].setText(QString());
        m_text[i].setTextOption(textOption);
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
    const qreal maxWidth = size().width() - 2 * option.margin;
    const qreal widgetHeight = size().height();
    const qreal fontHeight = option.fontMetrics.height();

    // Initialize properties for the "name" role. It will be used as anchor
    // for initializing the position of the other roles.
    m_text[Name].setText(KStringHandler::preProcessWrap(values["name"].toString()));

    // Calculate the number of lines required for the name and the required width
    int textLinesCountForName = 0;
    qreal requiredWidthForName = 0;
    QTextLine line;

    QTextLayout layout(m_text[Name].text(), option.font);
    layout.setTextOption(m_text[Name].textOption());
    layout.beginLayout();
    while ((line = layout.createLine()).isValid()) {
        line.setLineWidth(maxWidth);
        requiredWidthForName = qMax(requiredWidthForName, line.naturalTextWidth());
        ++textLinesCountForName;
    }
    layout.endLayout();

    // Use one line for each additional information
    int textLinesCount = textLinesCountForName;
    const int additionalRolesCount = qMax(visibleRoles().count() - 1, 0);
    textLinesCount += additionalRolesCount;

    m_text[Name].setTextWidth(maxWidth);
    m_textPos[Name] = QPointF(option.margin, widgetHeight - textLinesCount * fontHeight - option.margin);
    m_textRect = QRectF(option.margin + (maxWidth - requiredWidthForName) / 2,
                        m_textPos[Name].y(),
                        requiredWidthForName,
                        textLinesCountForName * fontHeight);

    // Calculate the position for each additional information
    qreal y = m_textPos[Name].y() + textLinesCountForName * fontHeight;
    foreach (const QByteArray& role, m_sortedVisibleRoles) {
        const TextId textId = roleTextId(role);
        if (textId == Name) {
            continue;
        }

        const QString text = roleText(role, values);
        m_text[textId].setText(text);

        qreal requiredWidth = 0;

        QTextLayout layout(text, option.font);
        layout.setTextOption(m_text[textId].textOption());
        layout.beginLayout();
        QTextLine textLine = layout.createLine();
        if (textLine.isValid()) {
            textLine.setLineWidth(maxWidth);
            requiredWidth = textLine.naturalTextWidth();
            if (textLine.textLength() < text.length()) {
                // TODO: QFontMetrics::elidedText() works different regarding the given width
                // in comparison to QTextLine::setLineWidth(). It might happen that the text does
                // not get elided although it does not fit into the given width. As workaround
                // the margin is substracted.
                const QString elidedText = option.fontMetrics.elidedText(text, Qt::ElideRight, maxWidth - option.margin);
                m_text[textId].setText(elidedText);
            }
        }
        layout.endLayout();

        m_textPos[textId] = QPointF(option.margin, y);
        m_text[textId].setTextWidth(maxWidth);

        const QRectF textRect(option.margin + (maxWidth - requiredWidth) / 2, y, requiredWidth, fontHeight);
        m_textRect |= textRect;

        y += fontHeight;
    }

    // Add a margin to the text rectangle
    const qreal margin = option.margin;
    m_textRect.adjust(-margin, -margin, margin, margin);
}

void KFileItemListWidget::updateCompactLayoutTextCache()
{
    // +------+  Name role
    // | Icon |  Additional role 1
    // +------+  Additional role 2

    const QHash<QByteArray, QVariant> values = data();

    const KItemListStyleOption& option = styleOption();
    const qreal widgetHeight = size().height();
    const qreal fontHeight = option.fontMetrics.height();
    const qreal textLinesHeight = qMax(visibleRoles().count(), 1) * fontHeight;
    const int scaledIconSize = (textLinesHeight < option.iconSize) ? widgetHeight - 2 * option.margin : option.iconSize;

    qreal maximumRequiredTextWidth = 0;
    const qreal x = option.margin * 3 + scaledIconSize;
    qreal y = (widgetHeight - textLinesHeight) / 2;
    const qreal maxWidth = size().width() - x - option.margin;
    foreach (const QByteArray& role, m_sortedVisibleRoles) {
        const TextId textId = roleTextId(role);

        const QString text = roleText(role, values);
        m_text[textId].setText(text);

        qreal requiredWidth = option.fontMetrics.width(text);
        if (requiredWidth > maxWidth) {
            requiredWidth = maxWidth;
            const QString elidedText = option.fontMetrics.elidedText(text, Qt::ElideRight, maxWidth);
            m_text[textId].setText(elidedText);
        }

        m_textPos[textId] = QPointF(x, y);
        m_text[textId].setTextWidth(maxWidth);

        maximumRequiredTextWidth = qMax(maximumRequiredTextWidth, requiredWidth);

        y += fontHeight;
    }

    m_textRect = QRectF(x - option.margin, 0, maximumRequiredTextWidth + 2 * option.margin, widgetHeight);
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
    const int scaledIconSize = widgetHeight - 2 * option.margin;
    const int fontHeight = option.fontMetrics.height();

    const qreal columnMargin = option.margin * 3;
    const qreal firstColumnInc = m_expansionArea.right() + option.margin * 2 + scaledIconSize;
    qreal x = firstColumnInc;
    const qreal y = qMax(qreal(option.margin), (widgetHeight - fontHeight) / 2);

    foreach (const QByteArray& role, m_sortedVisibleRoles) {
        const TextId textId = roleTextId(role);

        QString text = roleText(role, values);

        // Elide the text in case it does not fit into the available column-width
        qreal requiredWidth = option.fontMetrics.width(text);
        const qreal columnWidth = visibleRolesSizes().value(role, QSizeF(0, 0)).width();
        qreal availableTextWidth = columnWidth - 2 * columnMargin;
        if (textId == Name) {
            availableTextWidth -= firstColumnInc;
        }

        if (requiredWidth > availableTextWidth) {
            text = option.fontMetrics.elidedText(text, Qt::ElideRight, availableTextWidth);
            requiredWidth = option.fontMetrics.width(text);
        }

        m_text[textId].setText(text);
        m_textPos[textId] = QPointF(x + columnMargin, y);
        x += columnWidth;

        switch (textId) {
        case Name: {
            m_textRect = QRectF(m_textPos[textId].x() - option.margin, 0,
                                        requiredWidth + 2 * option.margin, size().height());

            // The column after the name should always be aligned on the same x-position independent
            // from the expansion-level shown in the name column
            x -= firstColumnInc;
            break;
        }
        case Size:
            // The values for the size should be right aligned
            m_textPos[textId].rx() += columnWidth - requiredWidth - 2 * columnMargin;
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
        painter->drawRect(QRectF(m_pixmapPos, QSizeF(scaledPixmap.size())));
#endif
    } else {
        painter->drawPixmap(m_pixmapPos, pixmap);
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

KFileItemListWidget::TextId KFileItemListWidget::roleTextId(const QByteArray& role)
{
    static QHash<QByteArray, TextId> rolesHash;
    if (rolesHash.isEmpty()) {
        rolesHash.insert("name", Name);
        rolesHash.insert("size", Size);
        rolesHash.insert("date", Date);
        rolesHash.insert("permissions", Permissions);
        rolesHash.insert("owner", Owner);
        rolesHash.insert("group", Group);
        rolesHash.insert("type", Type);
        rolesHash.insert("destination", Destination);
        rolesHash.insert("path", Path);
    }

    return rolesHash.value(role);
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

#include "kfileitemlistwidget.moc"
