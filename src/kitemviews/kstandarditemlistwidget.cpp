/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kstandarditemlistwidget.h"

#include "kfileitemlistview.h"
#include "kfileitemmodel.h"
#include "private/kfileitemclipboard.h"
#include "private/kitemlistroleeditor.h"
#include "private/kitemviewsutils.h"
#include "private/kpixmapmodifier.h"

#include <KIconEffect>
#include <KIconLoader>
#include <KIconUtils>
#include <KRatingPainter>
#include <KStringHandler>

#include <QApplication>
#include <QGraphicsScene>
#include <QGraphicsSceneResizeEvent>
#include <QGraphicsView>
#include <QPixmapCache>
#include <QStyleOption>
#include <QTextBoundaryFinder>
#include <QVariantAnimation>

// #define KSTANDARDITEMLISTWIDGET_DEBUG

KStandardItemListWidgetInformant::KStandardItemListWidgetInformant()
    : KItemListWidgetInformant()
{
}

KStandardItemListWidgetInformant::~KStandardItemListWidgetInformant()
{
}

void KStandardItemListWidgetInformant::calculateItemSizeHints(QVector<std::pair<qreal, bool>> &logicalHeightHints,
                                                              qreal &logicalWidthHint,
                                                              const KItemListView *view) const
{
    switch (static_cast<const KStandardItemListView *>(view)->itemLayout()) {
    case KStandardItemListView::IconsLayout:
        calculateIconsLayoutItemSizeHints(logicalHeightHints, logicalWidthHint, view);
        break;

    case KStandardItemListView::CompactLayout:
        calculateCompactLayoutItemSizeHints(logicalHeightHints, logicalWidthHint, view);
        break;

    case KStandardItemListView::DetailsLayout:
        calculateDetailsLayoutItemSizeHints(logicalHeightHints, logicalWidthHint, view);
        break;

    default:
        Q_ASSERT(false);
        break;
    }
}

qreal KStandardItemListWidgetInformant::preferredRoleColumnWidth(const QByteArray &role, int index, const KItemListView *view) const
{
    const QHash<QByteArray, QVariant> values = view->model()->data(index);
    const KItemListStyleOption &option = view->styleOption();

    const QString text = roleText(role, values);
    qreal width = KStandardItemListWidget::columnPadding(option);

    const QFontMetrics &normalFontMetrics = option.fontMetrics;
    const QFontMetrics linkFontMetrics(customizedFontForLinks(option.font));

    if (role == "rating") {
        width += KStandardItemListWidget::preferredRatingSize(option).width();
    } else {
        // If current item is a link, we use the customized link font metrics instead of the normal font metrics.
        const QFontMetrics &fontMetrics = itemIsLink(index, view) ? linkFontMetrics : normalFontMetrics;

        width += fontMetrics.horizontalAdvance(text);

        if (role == "text") {
            if (view->supportsItemExpanding()) {
                // Increase the width by the expansion-toggle and the current expansion level
                const int expandedParentsCount = values.value("expandedParentsCount", 0).toInt();
                const qreal height = option.padding * 2 + qMax(option.iconSize, fontMetrics.height());
                width += (expandedParentsCount + 1) * height;
            }

            // Increase the width by the required space for the icon
            width += option.padding * 2 + option.iconSize;
        }
    }

    return width;
}

QString KStandardItemListWidgetInformant::itemText(int index, const KItemListView *view) const
{
    return view->model()->data(index).value("text").toString();
}

bool KStandardItemListWidgetInformant::itemIsLink(int index, const KItemListView *view) const
{
    Q_UNUSED(index)
    Q_UNUSED(view)
    return false;
}

QString KStandardItemListWidgetInformant::roleText(const QByteArray &role, const QHash<QByteArray, QVariant> &values, ForUsageAs forUsageAs) const
{
    if (role == "rating") {
        if (forUsageAs == ForUsageAs::DisplayedText) {
            // Always use an empty text, as the rating is shown by the image m_rating.
            return QString();
        } else {
            const int rating{values.value(role).toInt()};
            // Check if there are half stars
            if (rating % 2) {
                return i18ncp("@accessible rating", "%1 and a half stars", "%1 and a half stars", rating / 2);
            }
            return i18ncp("@accessible rating", "%1 star", "%1 stars", rating / 2);
        }
    }
    return values.value(role).toString();
}

QFont KStandardItemListWidgetInformant::customizedFontForLinks(const QFont &baseFont) const
{
    return baseFont;
}

void KStandardItemListWidgetInformant::calculateIconsLayoutItemSizeHints(QVector<std::pair<qreal, bool>> &logicalHeightHints,
                                                                         qreal &logicalWidthHint,
                                                                         const KItemListView *view) const
{
    const KItemListStyleOption &option = view->styleOption();
    const QFont &normalFont = option.font;
    const int additionalRolesCount = qMax(view->visibleRoles().count() - 1, 0);

    const qreal itemWidth = view->itemSize().width();
    const qreal maxWidth = itemWidth - 2 * option.padding;
    const qreal additionalRolesSpacing = additionalRolesCount * option.fontMetrics.lineSpacing();
    const qreal spacingAndIconHeight = option.iconSize + option.padding * 3;

    const QFont linkFont = customizedFontForLinks(normalFont);

    QTextOption textOption(Qt::AlignHCenter);
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    for (int index = 0; index < logicalHeightHints.count(); ++index) {
        if (logicalHeightHints.at(index).first > 0.0) {
            continue;
        }

        // If the current item is a link, we use the customized link font instead of the normal font.
        const QFont &font = itemIsLink(index, view) ? linkFont : normalFont;

        const QString &text = KStringHandler::preProcessWrap(itemText(index, view));

        // Calculate the number of lines required for wrapping the name
        qreal textHeight = 0;
        QTextLayout layout(text, font);
        layout.setTextOption(textOption);
        layout.beginLayout();
        QTextLine line;
        int lineCount = 0;
        bool isElided = false;
        while ((line = layout.createLine()).isValid()) {
            line.setLineWidth(maxWidth);
            line.naturalTextWidth();
            textHeight += line.height();

            ++lineCount;
            if (lineCount == option.maxTextLines) {
                isElided = layout.createLine().isValid();
                break;
            }
        }
        layout.endLayout();

        // Add one line for each additional information
        textHeight += additionalRolesSpacing;

        logicalHeightHints[index].first = textHeight + spacingAndIconHeight;
        logicalHeightHints[index].second = isElided;
    }

    logicalWidthHint = itemWidth;
}

void KStandardItemListWidgetInformant::calculateCompactLayoutItemSizeHints(QVector<std::pair<qreal, bool>> &logicalHeightHints,
                                                                           qreal &logicalWidthHint,
                                                                           const KItemListView *view) const
{
    const KItemListStyleOption &option = view->styleOption();
    const QFontMetrics &normalFontMetrics = option.fontMetrics;
    const int additionalRolesCount = qMax(view->visibleRoles().count() - 1, 0);

    const QList<QByteArray> &visibleRoles = view->visibleRoles();
    const bool showOnlyTextRole = (visibleRoles.count() == 1) && (visibleRoles.first() == "text");
    const qreal maxWidth = option.maxTextWidth;
    const qreal paddingAndIconWidth = option.padding * 4 + option.iconSize;
    const qreal height = option.padding * 2 + qMax(option.iconSize, (1 + additionalRolesCount) * normalFontMetrics.lineSpacing());

    const QFontMetrics linkFontMetrics(customizedFontForLinks(option.font));

    for (int index = 0; index < logicalHeightHints.count(); ++index) {
        if (logicalHeightHints.at(index).first > 0.0) {
            continue;
        }

        // If the current item is a link, we use the customized link font metrics instead of the normal font metrics.
        const QFontMetrics &fontMetrics = itemIsLink(index, view) ? linkFontMetrics : normalFontMetrics;

        // For each row exactly one role is shown. Calculate the maximum required width that is necessary
        // to show all roles without horizontal clipping.
        qreal maximumRequiredWidth = 0.0;

        if (showOnlyTextRole) {
            maximumRequiredWidth = fontMetrics.horizontalAdvance(itemText(index, view));
        } else {
            const QHash<QByteArray, QVariant> &values = view->model()->data(index);
            for (const QByteArray &role : visibleRoles) {
                const QString &text = roleText(role, values);
                const qreal requiredWidth = fontMetrics.horizontalAdvance(text);
                maximumRequiredWidth = qMax(maximumRequiredWidth, requiredWidth);
            }
        }

        qreal width = paddingAndIconWidth + maximumRequiredWidth;
        if (maxWidth > 0 && width > maxWidth) {
            width = maxWidth;
        }

        logicalHeightHints[index].first = width;
    }

    logicalWidthHint = height;
}

void KStandardItemListWidgetInformant::calculateDetailsLayoutItemSizeHints(QVector<std::pair<qreal, bool>> &logicalHeightHints,
                                                                           qreal &logicalWidthHint,
                                                                           const KItemListView *view) const
{
    const KItemListStyleOption &option = view->styleOption();
    const qreal height = option.padding * 2 + qMax(option.iconSize, option.fontMetrics.height());
    logicalHeightHints.fill(std::make_pair(height, false));
    logicalWidthHint = -1.0;
}

KStandardItemListWidget::KStandardItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent)
    : KItemListWidget(informant, parent)
    , m_textInfo()
    , m_isCut(false)
    , m_isHidden(false)
    , m_customizedFont()
    , m_customizedFontMetrics(m_customizedFont)
    , m_isExpandable(false)
    , m_highlightEntireRow(false)
    , m_supportsItemExpanding(false)
    , m_dirtyLayout(true)
    , m_dirtyContent(true)
    , m_dirtyContentRoles()
    , m_layout(IconsLayout)
    , m_pixmapPos()
    , m_pixmap()
    , m_scaledPixmapSize()
    , m_columnWidthSum()
    , m_iconRect()
    , m_hoverPixmap()
    , m_textRect()
    , m_sortedVisibleRoles()
    , m_expansionArea()
    , m_customTextColor()
    , m_additionalInfoTextColor()
    , m_overlays()
    , m_rating()
    , m_roleEditor(nullptr)
    , m_oldRoleEditor(nullptr)
{
}

KStandardItemListWidget::~KStandardItemListWidget()
{
    qDeleteAll(m_textInfo);
    m_textInfo.clear();

    if (m_roleEditor) {
        m_roleEditor->deleteLater();
    }

    if (m_oldRoleEditor) {
        m_oldRoleEditor->deleteLater();
    }
}

void KStandardItemListWidget::setLayout(Layout layout)
{
    if (m_layout != layout) {
        m_layout = layout;
        m_dirtyLayout = true;
        updateAdditionalInfoTextColor();
        update();
    }
}

void KStandardItemListWidget::setHighlightEntireRow(bool highlightEntireRow)
{
    if (m_highlightEntireRow != highlightEntireRow) {
        m_highlightEntireRow = highlightEntireRow;
        m_dirtyLayout = true;
        update();
    }
}

bool KStandardItemListWidget::highlightEntireRow() const
{
    return m_highlightEntireRow;
}

void KStandardItemListWidget::setSupportsItemExpanding(bool supportsItemExpanding)
{
    if (m_supportsItemExpanding != supportsItemExpanding) {
        m_supportsItemExpanding = supportsItemExpanding;
        m_dirtyLayout = true;
        update();
    }
}

bool KStandardItemListWidget::supportsItemExpanding() const
{
    return m_supportsItemExpanding;
}

void KStandardItemListWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    const_cast<KStandardItemListWidget *>(this)->triggerCacheRefreshing();

    KItemListWidget::paint(painter, option, widget);

    if (!m_expansionArea.isEmpty()) {
        drawSiblingsInformation(painter);
    }

    const KItemListStyleOption &itemListStyleOption = styleOption();
    if (isHovered() && !m_pixmap.isNull()) {
        if (hoverOpacity() < 1.0) {
            /*
             * Linear interpolation between m_pixmap and m_hoverPixmap.
             *
             * Note that this cannot be achieved by painting m_hoverPixmap over
             * m_pixmap, even if the opacities are adjusted. For details see
             * https://git.reviewboard.kde.org/r/109614/
             */
            // Paint pixmap1 so that pixmap1 = m_pixmap * (1.0 - hoverOpacity())
            QPixmap pixmap1(m_pixmap.size());
            pixmap1.setDevicePixelRatio(m_pixmap.devicePixelRatio());
            pixmap1.fill(Qt::transparent);
            {
                QPainter p(&pixmap1);
                p.setOpacity(1.0 - hoverOpacity());
                p.drawPixmap(0, 0, m_pixmap);
            }

            // Paint pixmap2 so that pixmap2 = m_hoverPixmap * hoverOpacity()
            QPixmap pixmap2(pixmap1.size());
            pixmap2.setDevicePixelRatio(pixmap1.devicePixelRatio());
            pixmap2.fill(Qt::transparent);
            {
                QPainter p(&pixmap2);
                p.setOpacity(hoverOpacity());
                p.drawPixmap(0, 0, m_hoverPixmap);
            }

            // Paint pixmap2 on pixmap1 using CompositionMode_Plus
            // Now pixmap1 = pixmap2 + m_pixmap * (1.0 - hoverOpacity())
            //             = m_hoverPixmap * hoverOpacity() + m_pixmap * (1.0 - hoverOpacity())
            {
                QPainter p(&pixmap1);
                p.setCompositionMode(QPainter::CompositionMode_Plus);
                p.drawPixmap(0, 0, pixmap2);
            }

            // Finally paint pixmap1 on the widget
            drawPixmap(painter, pixmap1);
        } else {
            drawPixmap(painter, m_hoverPixmap);
        }
    } else if (!m_pixmap.isNull()) {
        drawPixmap(painter, m_pixmap);
    }

    painter->setFont(m_customizedFont);
    painter->setPen(textColor(*widget));
    const TextInfo *textInfo = m_textInfo.value("text");

    if (!textInfo) {
        // It seems that we can end up here even if m_textInfo does not contain
        // the key "text", see bug 306167. According to triggerCacheRefreshing(),
        // this can only happen if the index is negative. This can happen when
        // the item is about to be removed, see KItemListView::slotItemsRemoved().
        // TODO: try to reproduce the crash and find a better fix.
        return;
    }

    painter->drawStaticText(textInfo->pos, textInfo->staticText);

    bool clipAdditionalInfoBounds = false;
    if (m_supportsItemExpanding && m_sortedVisibleRoles.count() > 1) {
        // Prevent a possible overlapping of the additional-information-texts with the icon.
        // This will happen if the user has resized the width of the name-column to be very narrow while having folders expanded.
        // We only want to draw additional info text outside the area of the icon or expansion area, so we set a clip rect that does not contain the icon area.
        // This needs to work both for left-to-right as well as right-to-left layout directions.
        const TextInfo *potentiallyOverlappingRoleText = m_textInfo.value(m_sortedVisibleRoles[1]); // Only the first column after the name column can overlap.
        if (layoutDirection() == Qt::LeftToRight) { // In left-to-right languages the left end of text would overlap. This is mirrored for right-to-left.
            const qreal minX = m_iconRect.right() + 2 * itemListStyleOption.padding;
            if (potentiallyOverlappingRoleText->pos.x() < minX) {
                clipAdditionalInfoBounds = true;
                painter->save();
                painter->setClipRect(minX, 0, size().width() - minX, size().height(), Qt::IntersectClip);
            }
        } else {
            const qreal maxX = m_iconRect.left() - 2 * itemListStyleOption.padding;
            if (potentiallyOverlappingRoleText->pos.x() + m_customizedFontMetrics.horizontalAdvance(potentiallyOverlappingRoleText->staticText.text()) > maxX) {
                clipAdditionalInfoBounds = true;
                painter->save();
                painter->setClipRect(0, 0, maxX, size().height(), Qt::IntersectClip);
            }
        }
    }

    painter->setPen(m_additionalInfoTextColor);
    painter->setFont(m_customizedFont);

    for (int i = 1; i < m_sortedVisibleRoles.count(); ++i) {
        const TextInfo *textInfo = m_textInfo.value(m_sortedVisibleRoles[i]);
        painter->drawStaticText(textInfo->pos, textInfo->staticText);
    }

    if (!m_rating.isNull()) {
        const TextInfo *ratingTextInfo = m_textInfo.value("rating");
        QPointF pos = ratingTextInfo->pos;
        const Qt::Alignment align = ratingTextInfo->staticText.textOption().alignment();
        if (align & Qt::AlignHCenter) {
            pos.rx() += (size().width() - m_rating.width() / m_rating.devicePixelRatioF()) / 2 - 2;
        }
        painter->drawPixmap(pos, m_rating);
    }

    if (clipAdditionalInfoBounds) {
        painter->restore();
    }

#ifdef KSTANDARDITEMLISTWIDGET_DEBUG
    painter->setBrush(Qt::NoBrush);
    painter->setPen(Qt::green);
    painter->drawRect(m_iconRect);

    painter->setPen(Qt::blue);
    painter->drawRect(m_textRect);

    painter->setPen(Qt::red);
    painter->drawText(QPointF(0, m_customizedFontMetrics.height()), QString::number(index()));
    painter->drawRect(rect());
#endif
}

QRectF KStandardItemListWidget::iconRect() const
{
    const_cast<KStandardItemListWidget *>(this)->triggerCacheRefreshing();
    return m_iconRect;
}

QRectF KStandardItemListWidget::textRect() const
{
    const_cast<KStandardItemListWidget *>(this)->triggerCacheRefreshing();
    return m_textRect;
}

QRectF KStandardItemListWidget::textFocusRect() const
{
    // In the compact- and details-layout a larger textRect() is returned to be aligned
    // with the iconRect(). This is useful to have a larger selection/hover-area
    // when having a quite large icon size but only one line of text. Still the
    // focus rectangle should be shown as narrow as possible around the text.

    const_cast<KStandardItemListWidget *>(this)->triggerCacheRefreshing();

    switch (m_layout) {
    case CompactLayout: {
        QRectF rect = m_textRect;
        const TextInfo *topText = m_textInfo.value(m_sortedVisibleRoles.first());
        const TextInfo *bottomText = m_textInfo.value(m_sortedVisibleRoles.last());
        rect.setTop(topText->pos.y());
        rect.setBottom(bottomText->pos.y() + bottomText->staticText.size().height());
        return rect;
    }

    case DetailsLayout: {
        QRectF rect = m_textRect;
        const TextInfo *textInfo = m_textInfo.value(m_sortedVisibleRoles.first());
        rect.setTop(textInfo->pos.y());
        rect.setBottom(textInfo->pos.y() + textInfo->staticText.size().height());

        const KItemListStyleOption &option = styleOption();
        if (option.extendedSelectionRegion) {
            const QString text = textInfo->staticText.text();
            rect.setWidth(m_customizedFontMetrics.horizontalAdvance(text) + 2 * option.padding);
        }

        return rect;
    }

    default:
        break;
    }

    return m_textRect;
}

QRectF KStandardItemListWidget::selectionRect() const
{
    const_cast<KStandardItemListWidget *>(this)->triggerCacheRefreshing();

    switch (m_layout) {
    case IconsLayout:
        return m_textRect;

    case CompactLayout:
    case DetailsLayout: {
        const int padding = styleOption().padding;
        QRectF adjustedIconRect = iconRect().adjusted(-padding, -padding, padding, padding);
        QRectF result = adjustedIconRect | m_textRect;
        if (m_highlightEntireRow) {
            if (layoutDirection() == Qt::LeftToRight) {
                result.setRight(leftPadding() + m_columnWidthSum);
            } else {
                result.setLeft(size().width() - m_columnWidthSum - rightPadding());
            }
        }
        return result;
    }

    default:
        Q_ASSERT(false);
        break;
    }

    return m_textRect;
}

QRectF KStandardItemListWidget::expansionToggleRect() const
{
    const_cast<KStandardItemListWidget *>(this)->triggerCacheRefreshing();
    return m_isExpandable ? m_expansionArea : QRectF();
}

QRectF KStandardItemListWidget::selectionToggleRect() const
{
    const_cast<KStandardItemListWidget *>(this)->triggerCacheRefreshing();

    const QRectF widgetIconRect = iconRect();
    const int widgetIconSize = iconSize();
    int toggleSize = KIconLoader::SizeSmall;
    if (widgetIconSize >= KIconLoader::SizeEnormous) {
        toggleSize = KIconLoader::SizeMedium;
    } else if (widgetIconSize >= KIconLoader::SizeLarge) {
        toggleSize = KIconLoader::SizeSmallMedium;
    }

    QPointF pos = widgetIconRect.topLeft();

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

    if (QApplication::isRightToLeft()) {
        pos.setX(widgetIconRect.right() - (pos.x() + toggleSize - widgetIconRect.left()));
    }

    return QRectF(pos, QSizeF(toggleSize, toggleSize));
}

QPixmap KStandardItemListWidget::createDragPixmap(const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    QPixmap pixmap = KItemListWidget::createDragPixmap(option, widget);
    if (m_layout != DetailsLayout) {
        return pixmap;
    }

    // Only return the content of the text-column as pixmap
    const int leftClip = m_pixmapPos.x();

    const TextInfo *textInfo = m_textInfo.value("text");
    const int rightClip = textInfo->pos.x() + textInfo->staticText.size().width() + 2 * styleOption().padding;

    QPixmap clippedPixmap(rightClip - leftClip + 1, pixmap.height());
    clippedPixmap.fill(Qt::transparent);

    QPainter painter(&clippedPixmap);
    painter.drawPixmap(-leftClip, 0, pixmap);

    return clippedPixmap;
}

void KStandardItemListWidget::startActivateSoonAnimation(int timeUntilActivation)
{
    if (m_activateSoonAnimation) {
        m_activateSoonAnimation->stop(); // automatically DeleteWhenStopped
    }

    m_activateSoonAnimation = new QVariantAnimation{this};
    m_activateSoonAnimation->setStartValue(0.0);
    m_activateSoonAnimation->setEndValue(1.0);
    m_activateSoonAnimation->setDuration(timeUntilActivation);

    const QVariant originalIconName{data()["iconName"]};
    connect(m_activateSoonAnimation, &QVariantAnimation::valueChanged, this, [originalIconName, this](const QVariant &value) {
        auto progress = value.toFloat();

        QVariant wantedIconName;
        if (progress < 0.333) {
            wantedIconName = "folder-open";
        } else if (progress < 0.666) {
            wantedIconName = originalIconName;
        } else {
            wantedIconName = "folder-open";
        }

        QHash<QByteArray, QVariant> itemData{data()};
        if (itemData["iconName"] != wantedIconName) {
            itemData.insert("iconName", wantedIconName);
            setData(itemData);
            invalidateIconCache();
        }
    });

    connect(m_activateSoonAnimation, &QObject::destroyed, this, [originalIconName, this]() {
        QHash<QByteArray, QVariant> itemData{data()};
        if (itemData["iconName"] == "folder-open") {
            itemData.insert("iconName", originalIconName);
            setData(itemData);
            invalidateIconCache();
        }
    });

    m_activateSoonAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

bool KStandardItemListWidget::isIconControlledByActivateSoonAnimation() const
{
    return m_activateSoonAnimation && data()["iconName"] == "folder-open";
}

KItemListWidgetInformant *KStandardItemListWidget::createInformant()
{
    return new KStandardItemListWidgetInformant();
}

void KStandardItemListWidget::invalidateCache()
{
    m_dirtyLayout = true;
    m_dirtyContent = true;
}

void KStandardItemListWidget::invalidateIconCache()
{
    m_dirtyContent = true;
    m_dirtyContentRoles.insert("iconPixmap");
    m_dirtyContentRoles.insert("iconOverlays");
}

void KStandardItemListWidget::refreshCache()
{
}

bool KStandardItemListWidget::isRoleRightAligned(const QByteArray &role) const
{
    Q_UNUSED(role)
    return false;
}

bool KStandardItemListWidget::isHidden() const
{
    return false;
}

QFont KStandardItemListWidget::customizedFont(const QFont &baseFont) const
{
    return baseFont;
}

QPalette::ColorRole KStandardItemListWidget::normalTextColorRole() const
{
    return QPalette::Text;
}

void KStandardItemListWidget::setTextColor(const QColor &color)
{
    if (color != m_customTextColor) {
        m_customTextColor = color;
        updateAdditionalInfoTextColor();
        update();
    }
}

QColor KStandardItemListWidget::textColor(const QWidget &widget) const
{
    if (!isSelected()) {
        if (m_isHidden) {
            return m_additionalInfoTextColor;
        } else if (m_customTextColor.isValid()) {
            return m_customTextColor;
        }
    }

    const QPalette::ColorGroup group = isActiveWindow() && widget.hasFocus() ? QPalette::Active : QPalette::Inactive;
    const QPalette::ColorRole role = isSelected() ? QPalette::HighlightedText : normalTextColorRole();
    return styleOption().palette.color(group, role);
}

void KStandardItemListWidget::setOverlays(QHash<Qt::Corner, QString> &overlays)
{
    if (overlays == m_overlays) {
        return;
    }

    m_overlays = overlays;
    m_dirtyContent = true;
    m_dirtyContentRoles.insert("iconOverlays");
    update();
}

QHash<Qt::Corner, QString> KStandardItemListWidget::overlays() const
{
    return m_overlays;
}

QString KStandardItemListWidget::roleText(const QByteArray &role, const QHash<QByteArray, QVariant> &values) const
{
    return static_cast<const KStandardItemListWidgetInformant *>(informant())->roleText(role, values);
}

void KStandardItemListWidget::dataChanged(const QHash<QByteArray, QVariant> &current, const QSet<QByteArray> &roles)
{
    Q_UNUSED(current)

    m_dirtyContent = true;

    QSet<QByteArray> dirtyRoles;
    if (roles.isEmpty()) {
        const auto visibleRoles = this->visibleRoles();
        dirtyRoles = QSet<QByteArray>(visibleRoles.constBegin(), visibleRoles.constEnd());
    } else {
        dirtyRoles = roles;
    }

    // The URL might have changed (i.e., if the sort order of the items has
    // been changed). Therefore, the "is cut" state must be updated.
    KFileItemClipboard *clipboard = KFileItemClipboard::instance();
    const QUrl itemUrl = data().value("url").toUrl();
    m_isCut = clipboard->isCut(itemUrl);

    // The icon-state might depend from other roles and hence is
    // marked as dirty whenever a role has been changed
    dirtyRoles.insert("iconPixmap");
    dirtyRoles.insert("iconName");

    QSetIterator<QByteArray> it(dirtyRoles);
    while (it.hasNext()) {
        const QByteArray &role = it.next();
        m_dirtyContentRoles.insert(role);
    }
}

void KStandardItemListWidget::visibleRolesChanged(const QList<QByteArray> &current, const QList<QByteArray> &previous)
{
    Q_UNUSED(previous)
    m_sortedVisibleRoles = current;
    m_dirtyLayout = true;
}

void KStandardItemListWidget::columnWidthChanged(const QByteArray &role, qreal current, qreal previous)
{
    Q_UNUSED(role)
    Q_UNUSED(current)
    Q_UNUSED(previous)
    m_dirtyLayout = true;
}

void KStandardItemListWidget::sidePaddingChanged(qreal leftPaddingWidth, qreal rightPaddingWidth)
{
    Q_UNUSED(leftPaddingWidth)
    Q_UNUSED(rightPaddingWidth)
    m_dirtyLayout = true;
}

void KStandardItemListWidget::styleOptionChanged(const KItemListStyleOption &current, const KItemListStyleOption &previous)
{
    KItemListWidget::styleOptionChanged(current, previous);

    updateAdditionalInfoTextColor();
    m_dirtyLayout = true;
}

void KStandardItemListWidget::hoveredChanged(bool hovered)
{
    if (!hovered && m_activateSoonAnimation) {
        m_activateSoonAnimation->stop(); // automatically DeleteWhenStopped
    }
    m_dirtyLayout = true;
}

void KStandardItemListWidget::selectedChanged(bool selected)
{
    Q_UNUSED(selected)
    updateAdditionalInfoTextColor();
    m_dirtyContent = true;
}

void KStandardItemListWidget::siblingsInformationChanged(const QBitArray &current, const QBitArray &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    m_dirtyLayout = true;
}

int KStandardItemListWidget::numberOfUnicodeCharactersIn(const QString &text)
{
    int count = 0;
    QTextBoundaryFinder boundaryFinder(QTextBoundaryFinder::Grapheme, text);
    while (boundaryFinder.toNextBoundary() != -1) {
        ++count;
    }
    return count;
}

int KStandardItemListWidget::selectionLength(const QString &text) const
{
    return numberOfUnicodeCharactersIn(text);
}

void KStandardItemListWidget::editedRoleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(previous)

    QGraphicsView *parent = scene()->views()[0];
    if (current.isEmpty() || !parent || current != "text") {
        if (m_roleEditor) {
            Q_EMIT roleEditingCanceled(index(), current, data().value(current));
            closeRoleEditor();
        }
        return;
    }

    Q_ASSERT(!m_roleEditor);

    const TextInfo *textInfo = m_textInfo.value("text");

    m_roleEditor = new KItemListRoleEditor(parent);
    m_roleEditor->setRole(current);
    m_roleEditor->setAllowUpDownKeyChainEdit(m_layout != IconsLayout);
    m_roleEditor->setFont(styleOption().font);

    const QString text = data().value(current).toString();
    m_roleEditor->setPlainText(text);

    QTextOption textOption = textInfo->staticText.textOption();
    m_roleEditor->document()->setDefaultTextOption(textOption);

    const int textSelectionLength = selectionLength(text);

    if (textSelectionLength > 0) {
        QTextCursor cursor = m_roleEditor->textCursor();
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, textSelectionLength);
        m_roleEditor->setTextCursor(cursor);
    }

    connect(m_roleEditor, &KItemListRoleEditor::roleEditingCanceled, this, &KStandardItemListWidget::slotRoleEditingCanceled);
    connect(m_roleEditor, &KItemListRoleEditor::roleEditingFinished, this, &KStandardItemListWidget::slotRoleEditingFinished);

    // Adjust the geometry of the editor
    QRectF rect = roleEditingRect(current);
    const int frameWidth = m_roleEditor->frameWidth();
    rect.adjust(-frameWidth, -frameWidth, frameWidth, frameWidth);
    rect.translate(pos());
    if (rect.right() > parent->width()) {
        rect.setWidth(parent->width() - rect.left());
    }
    m_roleEditor->setGeometry(rect.toRect());
    m_roleEditor->autoAdjustSize();
    m_roleEditor->show();
    m_roleEditor->setFocus();
}

void KStandardItemListWidget::iconSizeChanged(int current, int previous)
{
    KItemListWidget::iconSizeChanged(current, previous);

    invalidateIconCache();
    triggerCacheRefreshing();
    update();
}

void KStandardItemListWidget::resizeEvent(QGraphicsSceneResizeEvent *event)
{
    if (m_roleEditor) {
        setEditedRole(QByteArray());
        Q_ASSERT(!m_roleEditor);
    }

    KItemListWidget::resizeEvent(event);

    m_dirtyLayout = true;
}

void KStandardItemListWidget::showEvent(QShowEvent *event)
{
    KItemListWidget::showEvent(event);

    // Listen to changes of the clipboard to mark the item as cut/uncut
    KFileItemClipboard *clipboard = KFileItemClipboard::instance();

    const QUrl itemUrl = data().value("url").toUrl();
    m_isCut = clipboard->isCut(itemUrl);

    connect(clipboard, &KFileItemClipboard::cutItemsChanged, this, &KStandardItemListWidget::slotCutItemsChanged);
}

void KStandardItemListWidget::hideEvent(QHideEvent *event)
{
    disconnect(KFileItemClipboard::instance(), &KFileItemClipboard::cutItemsChanged, this, &KStandardItemListWidget::slotCutItemsChanged);

    KItemListWidget::hideEvent(event);
}

bool KStandardItemListWidget::event(QEvent *event)
{
    if (event->type() == QEvent::WindowDeactivate || event->type() == QEvent::WindowActivate || event->type() == QEvent::PaletteChange) {
        m_dirtyContent = true;
    }

    return KItemListWidget::event(event);
}

void KStandardItemListWidget::finishRoleEditing()
{
    if (!editedRole().isEmpty() && m_roleEditor) {
        slotRoleEditingFinished(editedRole(), KIO::encodeFileName(m_roleEditor->toPlainText()));
    }
}

void KStandardItemListWidget::slotCutItemsChanged()
{
    const QUrl itemUrl = data().value("url").toUrl();
    const bool isCut = KFileItemClipboard::instance()->isCut(itemUrl);
    if (m_isCut != isCut) {
        m_isCut = isCut;
        m_pixmap = QPixmap();
        m_dirtyContent = true;
        update();
    }
}

void KStandardItemListWidget::slotRoleEditingCanceled(const QByteArray &role, const QVariant &value)
{
    closeRoleEditor();
    Q_EMIT roleEditingCanceled(index(), role, value);
    setEditedRole(QByteArray());
}

void KStandardItemListWidget::slotRoleEditingFinished(const QByteArray &role, const QVariant &value)
{
    closeRoleEditor();
    Q_EMIT roleEditingFinished(index(), role, value);
    setEditedRole(QByteArray());
}

void KStandardItemListWidget::triggerCacheRefreshing()
{
    if ((!m_dirtyContent && !m_dirtyLayout) || index() < 0) {
        return;
    }

    refreshCache();

    const QHash<QByteArray, QVariant> values = data();
    m_isExpandable = m_supportsItemExpanding && values["isExpandable"].toBool();
    m_isHidden = isHidden();
    m_customizedFont = customizedFont(styleOption().font);
    m_customizedFontMetrics = QFontMetrics(m_customizedFont);
    m_columnWidthSum = std::accumulate(m_sortedVisibleRoles.begin(), m_sortedVisibleRoles.end(), qreal(), [this](qreal sum, const auto &role) {
        return sum + columnWidth(role);
    });

    updateExpansionArea();
    updateTextsCache();
    updatePixmapCache();
    clearHoverCache();

    m_dirtyLayout = false;
    m_dirtyContent = false;
    m_dirtyContentRoles.clear();
}

void KStandardItemListWidget::updateExpansionArea()
{
    if (m_supportsItemExpanding) {
        const QHash<QByteArray, QVariant> values = data();
        const int expandedParentsCount = values.value("expandedParentsCount", 0).toInt();
        if (expandedParentsCount >= 0) {
            const int widgetIconSize = iconSize();
            const qreal widgetHeight = size().height();
            const qreal inc = (widgetHeight - widgetIconSize) / 2;
            const qreal x = expandedParentsCount * widgetHeight + inc;
            const qreal y = inc;
            if (layoutDirection() == Qt::LeftToRight) {
                const qreal leftPaddingWidth = m_highlightEntireRow ? leftPadding() : 0;
                m_expansionArea = QRectF(leftPaddingWidth + x, y, widgetIconSize, widgetIconSize);
                return;
            }
            const qreal rightPaddingWidth = m_highlightEntireRow ? rightPadding() : 0;
            m_expansionArea = QRectF(size().width() - rightPaddingWidth - x - widgetIconSize, y, widgetIconSize, widgetIconSize);
            return;
        }
    }

    m_expansionArea = QRectF();
}

void KStandardItemListWidget::updatePixmapCache()
{
    // Precondition: Requires already updated m_textPos values to calculate
    // the remaining height when the alignment is vertical.

    const QSizeF widgetSize = size();
    const bool iconOnTop = (m_layout == IconsLayout);
    const KItemListStyleOption &option = styleOption();
    const qreal padding = option.padding;
    const qreal dpr = KItemViewsUtils::devicePixelRatio(this);

    const int widgetIconSize = iconSize();
    const int maxIconWidth = iconOnTop ? widgetSize.width() - 2 * padding : widgetIconSize;
    const int maxIconHeight = widgetIconSize;

    const QHash<QByteArray, QVariant> values = data();

    bool updatePixmap = (m_pixmap.width() != maxIconWidth || m_pixmap.height() != maxIconHeight);
    if (!updatePixmap && m_dirtyContent) {
        updatePixmap = m_dirtyContentRoles.isEmpty() || m_dirtyContentRoles.contains("iconPixmap") || m_dirtyContentRoles.contains("iconName")
            || m_dirtyContentRoles.contains("iconOverlays");
    }

    if (updatePixmap) {
        m_pixmap = QPixmap();

        int sequenceIndex = hoverSequenceIndex();

        if (values.contains("hoverSequencePixmaps") && !isIconControlledByActivateSoonAnimation()) {
            // Use one of the hover sequence pixmaps instead of the default
            // icon pixmap.

            const QVector<QPixmap> pixmaps = values["hoverSequencePixmaps"].value<QVector<QPixmap>>();

            if (values.contains("hoverSequenceWraparoundPoint")) {
                const float wap = values["hoverSequenceWraparoundPoint"].toFloat();
                if (wap >= 1.0f) {
                    sequenceIndex %= static_cast<int>(wap);
                }
            }

            const int loadedIndex = qMax(qMin(sequenceIndex, pixmaps.size() - 1), 0);

            if (loadedIndex != 0) {
                m_pixmap = pixmaps[loadedIndex];
            }
        }

        if (m_pixmap.isNull() && !isIconControlledByActivateSoonAnimation()) {
            m_pixmap = values["iconPixmap"].value<QPixmap>();
        }

        if (m_pixmap.isNull()) {
            // Use the icon that fits to the MIME-type
            QString iconName = values["iconName"].toString();
            if (iconName.isEmpty()) {
                // The icon-name has not been not resolved by KFileItemModelRolesUpdater,
                // use a generic icon as fallback
                iconName = QStringLiteral("unknown");
            }
            const QStringList overlays = values["iconOverlays"].toStringList();
            const bool hasFocus = scene()->views()[0]->parentWidget()->hasFocus();
            m_pixmap = pixmapForIcon(iconName,
                                     m_overlays,
                                     QSize(maxIconWidth, maxIconHeight),
                                     m_layout != IconsLayout && isActiveWindow() && isSelected() && hasFocus ? QIcon::Selected : QIcon::Normal);

        } else {
            if (!m_overlays.isEmpty()) {
                m_pixmap = addOverlays(m_pixmap, m_overlays, QSize(maxIconWidth, maxIconHeight), dpr);
            } else if (m_pixmap.width() / m_pixmap.devicePixelRatio() != maxIconWidth || m_pixmap.height() / m_pixmap.devicePixelRatio() != maxIconHeight) {
                // A custom pixmap has been applied. Assure that the pixmap
                // is scaled to the maximum available size.
                KPixmapModifier::scale(m_pixmap, QSize(maxIconWidth, maxIconHeight) * dpr);
            }
        }

        if (m_pixmap.isNull()) {
            m_hoverPixmap = QPixmap();
            return;
        }

        if (m_isCut) {
            KIconEffect::toDisabled(m_pixmap);
        }

        if (m_isHidden) {
            KIconEffect::semiTransparent(m_pixmap);
        }

        if (m_layout == IconsLayout && isSelected()) {
            const QColor color = palette().brush(QPalette::Normal, QPalette::Highlight).color();
            QImage image = m_pixmap.toImage();
            if (image.isNull()) {
                m_hoverPixmap = QPixmap();
                return;
            }
            KIconEffect::colorize(image, color, 0.8f);
            m_pixmap = QPixmap::fromImage(image);
        }
    }

    int scaledIconSize = 0;
    if (iconOnTop) {
        const TextInfo *textInfo = m_textInfo.value("text");
        scaledIconSize = static_cast<int>(textInfo->pos.y() - 2 * padding);
    } else {
        const int textRowsCount = (m_layout == CompactLayout) ? visibleRoles().count() : 1;
        const qreal requiredTextHeight = textRowsCount * m_customizedFontMetrics.height();
        scaledIconSize = (requiredTextHeight < maxIconHeight) ? widgetSize.height() - 2 * padding : maxIconHeight;
    }

    const int maxScaledIconWidth = iconOnTop ? widgetSize.width() - 2 * padding : scaledIconSize;
    const int maxScaledIconHeight = scaledIconSize;

    m_scaledPixmapSize = m_pixmap.size();
    m_scaledPixmapSize.scale(maxScaledIconWidth * dpr, maxScaledIconHeight * dpr, Qt::KeepAspectRatio);
    m_scaledPixmapSize = m_scaledPixmapSize / dpr;

    if (iconOnTop) {
        // Center horizontally and align on bottom within the icon-area
        m_pixmapPos.setX((widgetSize.width() - m_scaledPixmapSize.width()) / 2.0);
        m_pixmapPos.setY(padding + scaledIconSize - m_scaledPixmapSize.height());
    } else {
        // Center horizontally and vertically within the icon-area
        const TextInfo *textInfo = m_textInfo.value("text");
        if (QApplication::isRightToLeft()) {
            m_pixmapPos.setX(m_textRect.right() + 2.0 * padding);
        } else {
            m_pixmapPos.setX(textInfo->pos.x() - 2.0 * padding - (scaledIconSize + m_scaledPixmapSize.width()) / 2.0);
        }

        // Derive icon's vertical center from the center of the text frame, including
        // any necessary adjustment if the font's midline is offset from the frame center
        const qreal midlineShift = m_customizedFontMetrics.height() / 2.0 - m_customizedFontMetrics.descent() - m_customizedFontMetrics.capHeight() / 2.0;
        m_pixmapPos.setY(m_textRect.center().y() + midlineShift - m_scaledPixmapSize.height() / 2.0);
    }

    if (m_layout == IconsLayout) {
        m_iconRect = QRectF(m_pixmapPos, QSizeF(m_scaledPixmapSize));
    } else {
        const qreal widthOffset = widgetIconSize - m_scaledPixmapSize.width();
        const qreal heightOffset = widgetIconSize - m_scaledPixmapSize.height();
        const QPointF squareIconPos(m_pixmapPos.x() - 0.5 * widthOffset, m_pixmapPos.y() - 0.5 * heightOffset);
        const QSizeF squareIconSize(widgetIconSize, widgetIconSize);
        m_iconRect = QRectF(squareIconPos, squareIconSize);
    }

    // Prepare the pixmap that is used when the item gets hovered
    if (isHovered()) {
        m_hoverPixmap = m_pixmap;
        KIconEffect::toActive(m_hoverPixmap);
    } else if (hoverOpacity() <= 0.0) {
        // No hover animation is ongoing. Clear m_hoverPixmap to save memory.
        m_hoverPixmap = QPixmap();
    }
}

void KStandardItemListWidget::updateTextsCache()
{
    QTextOption textOption;
    switch (m_layout) {
    case IconsLayout:
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        textOption.setAlignment(Qt::AlignHCenter);
        break;
    case CompactLayout:
        textOption.setAlignment(QApplication::isRightToLeft() ? Qt::AlignRight : Qt::AlignLeft);
        textOption.setWrapMode(QTextOption::NoWrap);
        break;
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
        TextInfo *textInfo = new TextInfo();
        textInfo->staticText.setTextFormat(Qt::PlainText);
        textInfo->staticText.setPerformanceHint(QStaticText::AggressiveCaching);
        textInfo->staticText.setTextOption(textOption);
        m_textInfo.insert(m_sortedVisibleRoles[i], textInfo);
    }

    switch (m_layout) {
    case IconsLayout:
        updateIconsLayoutTextCache();
        break;
    case CompactLayout:
        updateCompactLayoutTextCache();
        break;
    case DetailsLayout:
        updateDetailsLayoutTextCache();
        break;
    default:
        Q_ASSERT(false);
        break;
    }

    const TextInfo *ratingTextInfo = m_textInfo.value("rating");
    if (ratingTextInfo) {
        // The text of the rating-role has been set to empty to get
        // replaced by a rating-image showing the rating as stars.
        const KItemListStyleOption &option = styleOption();
        QSizeF ratingSize = preferredRatingSize(option);

        const qreal availableWidth = (m_layout == DetailsLayout) ? columnWidth("rating") - columnPadding(option) : size().width();
        if (ratingSize.width() > availableWidth) {
            ratingSize.rwidth() = availableWidth;
        }
        const qreal dpr = KItemViewsUtils::devicePixelRatio(this);
        m_rating = QPixmap(ratingSize.toSize() * dpr);
        m_rating.setDevicePixelRatio(dpr);
        m_rating.fill(Qt::transparent);

        QPainter painter(&m_rating);
        const QRect rect(QPoint(0, 0), ratingSize.toSize());
        const int rating = data().value("rating").toInt();
        KRatingPainter::paintRating(&painter, rect, Qt::AlignJustify | Qt::AlignVCenter, rating);
    } else if (!m_rating.isNull()) {
        m_rating = QPixmap();
    }
}

QString KStandardItemListWidget::escapeString(const QString &text) const
{
    QString escaped(text);

    const QChar returnSymbol(0x21b5);
    escaped.replace('\n', returnSymbol);

    return escaped;
}

void KStandardItemListWidget::updateIconsLayoutTextCache()
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

    const KItemListStyleOption &option = styleOption();
    const qreal padding = option.padding;
    const qreal maxWidth = size().width() - 2 * padding;
    const qreal lineSpacing = m_customizedFontMetrics.lineSpacing();

    // Initialize properties for the "text" role. It will be used as anchor
    // for initializing the position of the other roles.
    TextInfo *nameTextInfo = m_textInfo.value("text");
    const QString nameText = KStringHandler::preProcessWrap(escapeString(values["text"].toString()));
    nameTextInfo->staticText.setText(nameText);

    // Calculate the number of lines required for the name and the required width
    qreal nameWidth = 0;
    qreal nameHeight = 0;
    QTextLine line;

    QTextLayout layout(nameTextInfo->staticText.text(), m_customizedFont);
    layout.setTextOption(nameTextInfo->staticText.textOption());
    layout.beginLayout();
    int nameLineIndex = 0;
    while ((line = layout.createLine()).isValid()) {
        line.setLineWidth(maxWidth);
        nameWidth = qMax(nameWidth, line.naturalTextWidth());
        nameHeight += line.height();

        ++nameLineIndex;
        if (nameLineIndex == option.maxTextLines) {
            // The maximum number of textlines has been reached. If this is
            // the case provide an elided text if necessary.
            const int textLength = line.textStart() + line.textLength();
            if (textLength < nameText.length()) {
                // Elide the last line of the text
                qreal elidingWidth = maxWidth;
                qreal lastLineWidth;
                do {
                    QString lastTextLine = nameText.mid(line.textStart());
                    lastTextLine = m_customizedFontMetrics.elidedText(lastTextLine, Qt::ElideMiddle, elidingWidth);
                    const QString elidedText = nameText.left(line.textStart()) + lastTextLine;
                    nameTextInfo->staticText.setText(elidedText);

                    lastLineWidth = m_customizedFontMetrics.horizontalAdvance(lastTextLine);

                    // We do the text eliding in a loop with decreasing width (1 px / iteration)
                    // to avoid problems related to different width calculation code paths
                    // within Qt. (see bug 337104)
                    elidingWidth -= 1.0;
                } while (lastLineWidth > maxWidth);

                nameWidth = qMax(nameWidth, lastLineWidth);
            }
            break;
        }
    }
    layout.endLayout();

    // Use one line for each additional information
    nameTextInfo->staticText.setTextWidth(maxWidth);
    nameTextInfo->pos = QPointF(padding, iconSize() + 2 * padding);
    m_textRect = QRectF(padding + (maxWidth - nameWidth) / 2, nameTextInfo->pos.y(), nameWidth, nameHeight);

    // Calculate the position for each additional information
    qreal y = nameTextInfo->pos.y() + nameHeight;
    for (const QByteArray &role : std::as_const(m_sortedVisibleRoles)) {
        if (role == "text") {
            continue;
        }

        const QString text = roleText(role, values);
        TextInfo *textInfo = m_textInfo.value(role);
        textInfo->staticText.setText(text);

        qreal requiredWidth = 0;

        QTextLayout layout(text, m_customizedFont);
        QTextOption textOption;
        textOption.setWrapMode(QTextOption::NoWrap);
        layout.setTextOption(textOption);

        layout.beginLayout();
        QTextLine textLine = layout.createLine();
        if (textLine.isValid()) {
            textLine.setLineWidth(maxWidth);
            requiredWidth = textLine.naturalTextWidth();
            if (requiredWidth > maxWidth) {
                const QString elidedText = m_customizedFontMetrics.elidedText(text, Qt::ElideMiddle, maxWidth);
                textInfo->staticText.setText(elidedText);
                requiredWidth = m_customizedFontMetrics.horizontalAdvance(elidedText);
            } else if (role == "rating") {
                // Use the width of the rating pixmap, because the rating text is empty.
                requiredWidth = m_rating.width() / m_rating.devicePixelRatioF();
            }
        }
        layout.endLayout();

        textInfo->pos = QPointF(padding, y);
        textInfo->staticText.setTextWidth(maxWidth);

        const QRectF textRect(padding + (maxWidth - requiredWidth) / 2, y, requiredWidth, lineSpacing);

        // Ignore empty roles. Avoids a text rect taller than the area that actually contains text.
        if (!textRect.isEmpty()) {
            m_textRect |= textRect;
        }

        y += lineSpacing;
    }

    // Add a padding to the text rectangle
    m_textRect.adjust(-padding, -padding, padding, padding);
}

void KStandardItemListWidget::updateCompactLayoutTextCache()
{
    // +------+  Name role
    // | Icon |  Additional role 1
    // +------+  Additional role 2

    const QHash<QByteArray, QVariant> values = data();

    const KItemListStyleOption &option = styleOption();
    const qreal widgetHeight = size().height();
    const qreal lineSpacing = m_customizedFontMetrics.lineSpacing();
    const qreal textLinesHeight = qMax(visibleRoles().count(), 1) * lineSpacing;

    qreal maximumRequiredTextWidth = 0;
    const qreal x = QApplication::isRightToLeft() ? option.padding : option.padding * 3 + iconSize();
    qreal y = qRound((widgetHeight - textLinesHeight) / 2);
    const qreal maxWidth = size().width() - iconSize() - 4 * option.padding;
    for (const QByteArray &role : std::as_const(m_sortedVisibleRoles)) {
        const QString text = escapeString(roleText(role, values));
        TextInfo *textInfo = m_textInfo.value(role);
        textInfo->staticText.setText(text);

        qreal requiredWidth = m_customizedFontMetrics.horizontalAdvance(text);
        if (requiredWidth > maxWidth) {
            requiredWidth = maxWidth;
            const QString elidedText = m_customizedFontMetrics.elidedText(text, Qt::ElideMiddle, maxWidth);
            textInfo->staticText.setText(elidedText);
        }

        textInfo->pos = QPointF(x, y);
        textInfo->staticText.setTextWidth(maxWidth);

        maximumRequiredTextWidth = qMax(maximumRequiredTextWidth, requiredWidth);

        y += lineSpacing;
    }

    m_textRect = QRectF(x - option.padding, 0, maximumRequiredTextWidth + 2 * option.padding, widgetHeight);
}

void KStandardItemListWidget::updateDetailsLayoutTextCache()
{
    // Precondition: Requires already updated m_expansionArea
    // to determine the left position.

    // +------+
    // | Icon |  Name role   Additional role 1   Additional role 2
    // +------+
    // Mirror the above for right-to-left languages.
    const bool isLeftToRight = QApplication::layoutDirection() == Qt::LeftToRight;
    m_textRect = QRectF();

    const KItemListStyleOption &option = styleOption();
    const QHash<QByteArray, QVariant> values = data();

    const qreal widgetHeight = size().height();
    const int fontHeight = m_customizedFontMetrics.height();

    const qreal columnWidthInc = columnPadding(option);
    qreal firstColumnInc = iconSize();
    if (m_supportsItemExpanding) {
        firstColumnInc += isLeftToRight ? (m_expansionArea.left() + m_expansionArea.right() + widgetHeight) / 2
                                        : ((size().width() - m_expansionArea.left()) + (size().width() - m_expansionArea.right()) + widgetHeight) / 2;
    } else {
        firstColumnInc += option.padding + (isLeftToRight ? leftPadding() : rightPadding());
    }

    qreal x = firstColumnInc;
    const qreal y = qMax(qreal(option.padding), (widgetHeight - fontHeight) / 2);

    for (const QByteArray &role : std::as_const(m_sortedVisibleRoles)) {
        QString text = roleText(role, values);

        // Elide the text in case it does not fit into the available column-width
        qreal requiredWidth = m_customizedFontMetrics.horizontalAdvance(text);
        const qreal roleWidth = columnWidth(role);
        qreal availableTextWidth = roleWidth - columnWidthInc;

        const bool isTextRole = (role == "text");
        if (isTextRole) {
            text = escapeString(text);
            availableTextWidth -= firstColumnInc - (isLeftToRight ? leftPadding() : rightPadding());
        }

        if (requiredWidth > availableTextWidth) {
            text = m_customizedFontMetrics.elidedText(text, Qt::ElideMiddle, availableTextWidth);
            requiredWidth = m_customizedFontMetrics.horizontalAdvance(text);
        }

        TextInfo *textInfo = m_textInfo.value(role);
        textInfo->staticText.setText(text);
        textInfo->pos = QPointF(isLeftToRight ? (x + columnWidthInc / 2) : (size().width() - (x + columnWidthInc / 2) - requiredWidth), y);
        x += roleWidth;

        if (isTextRole) {
            m_textRect = QRectF(textInfo->pos.x() - option.padding, 0, requiredWidth + 2 * option.padding, size().height());

            // The column after the name should always be aligned on the same x-position independent
            // from the expansion-level shown in the name column
            x -= firstColumnInc - (isLeftToRight ? leftPadding() : rightPadding());
        } else if (isRoleRightAligned(role) && isLeftToRight) {
            textInfo->pos.rx() += roleWidth - requiredWidth - columnWidthInc;
        }
    }
}

void KStandardItemListWidget::updateAdditionalInfoTextColor()
{
    QColor c1;
    const bool hasFocus = scene()->views()[0]->parentWidget()->hasFocus();
    if (m_customTextColor.isValid()) {
        c1 = m_customTextColor;
    } else if (isSelected() && hasFocus && (m_layout != DetailsLayout || m_highlightEntireRow)) {
        // The detail text color needs to match the main text (HighlightedText) for the same level
        // of readability. We short circuit early here to avoid interpolating with another color.
        m_additionalInfoTextColor = styleOption().palette.color(QPalette::HighlightedText);
        return;
    } else {
        c1 = styleOption().palette.text().color();
    }

    // For the color of the additional info the inactive text color
    // is not used as this might lead to unreadable text for some color schemes. Instead
    // the text color c1 is slightly mixed with the background color.
    const QColor c2 = styleOption().palette.base().color();
    const int p1 = 70;
    const int p2 = 100 - p1;
    m_additionalInfoTextColor =
        QColor((c1.red() * p1 + c2.red() * p2) / 100, (c1.green() * p1 + c2.green() * p2) / 100, (c1.blue() * p1 + c2.blue() * p2) / 100);
}

QPixmap KStandardItemListWidget::addOverlays(const QPixmap &pixmap,
                                             const QHash<Qt::Corner, QString> &overlays,
                                             const QSize &size,
                                             qreal devicePixelRatioF,
                                             QIcon::Mode mode) const
{
    if (overlays.isEmpty()) {
        return pixmap;
    }

    QHash<Qt::Corner, QIcon> overlayIcons;

    for (const auto &[corner, overlay] : overlays.asKeyValueRange()) {
        overlayIcons.insert(corner, QIcon::fromTheme(overlay));
    }

    return KIconUtils::addOverlays(pixmap, overlayIcons).pixmap(size, devicePixelRatioF, mode);
}

void KStandardItemListWidget::drawPixmap(QPainter *painter, const QPixmap &pixmap)
{
    if (m_scaledPixmapSize != pixmap.size() / pixmap.devicePixelRatio()) {
        const qreal dpr = KItemViewsUtils::devicePixelRatio(this);
        QPixmap scaledPixmap = pixmap;
        KPixmapModifier::scale(scaledPixmap, m_scaledPixmapSize * dpr);
        scaledPixmap.setDevicePixelRatio(dpr);
        painter->drawPixmap(m_pixmapPos, scaledPixmap);

#ifdef KSTANDARDITEMLISTWIDGET_DEBUG
        painter->setPen(Qt::blue);
        painter->drawRect(QRectF(m_pixmapPos, QSizeF(m_scaledPixmapSize)));
#endif
    } else {
        painter->drawPixmap(m_pixmapPos, pixmap);
    }
}

void KStandardItemListWidget::drawSiblingsInformation(QPainter *painter)
{
    const int siblingSize = size().height();
    const int x = (m_expansionArea.left() + m_expansionArea.right() - siblingSize) / 2;
    QRect siblingRect(x, 0, siblingSize, siblingSize);

    bool isItemSibling = true;

    const QBitArray siblings = siblingsInformation();
    QStyleOption option;
    const auto normalColor = option.palette.color(normalTextColorRole());
    const auto highlightColor = option.palette.color(expansionAreaHovered() ? QPalette::Highlight : normalTextColorRole());
    for (int i = siblings.count() - 1; i >= 0; --i) {
        option.rect = siblingRect;
        option.state = siblings.at(i) ? QStyle::State_Sibling : QStyle::State_None;
        if (isItemSibling) {
            option.state |= QStyle::State_Item;
            if (m_isExpandable) {
                option.state |= QStyle::State_Children;
            }
            if (data().value("isExpanded").toBool()) {
                option.state |= QStyle::State_Open;
            }
            option.palette.setColor(QPalette::Text, highlightColor);
            isItemSibling = false;
        } else {
            option.palette.setColor(QPalette::Text, normalColor);
        }

        style()->drawPrimitive(QStyle::PE_IndicatorBranch, &option, painter);

        siblingRect.translate(layoutDirection() == Qt::LeftToRight ? -siblingRect.width() : siblingRect.width(), 0);
    }
}

QRectF KStandardItemListWidget::roleEditingRect(const QByteArray &role) const
{
    const TextInfo *textInfo = m_textInfo.value(role);
    if (!textInfo) {
        return QRectF();
    }

    QRectF rect(textInfo->pos, textInfo->staticText.size());
    if (m_layout == DetailsLayout) {
        rect.setWidth(columnWidth(role) - rect.x());
    }

    return rect;
}

void KStandardItemListWidget::closeRoleEditor()
{
    disconnect(m_roleEditor, &KItemListRoleEditor::roleEditingCanceled, this, &KStandardItemListWidget::slotRoleEditingCanceled);
    disconnect(m_roleEditor, &KItemListRoleEditor::roleEditingFinished, this, &KStandardItemListWidget::slotRoleEditingFinished);

    if (m_roleEditor->hasFocus()) {
        // If the editing was not ended by a FocusOut event, we have
        // to transfer the keyboard focus back to the KItemListContainer.
        scene()->views()[0]->parentWidget()->setFocus();
    }

    if (m_oldRoleEditor) {
        m_oldRoleEditor->deleteLater();
    }
    m_oldRoleEditor = m_roleEditor;
    m_roleEditor->hide();
    m_roleEditor = nullptr;
}

QPixmap KStandardItemListWidget::pixmapForIcon(const QString &name, const QHash<Qt::Corner, QString> &overlays, const QSize &size, QIcon::Mode mode) const
{
    static const QIcon fallbackIcon = QIcon::fromTheme(QStringLiteral("unknown"));
    const qreal dpr = KItemViewsUtils::devicePixelRatio(this);

    int iconHeight = size.height();
    QSize iconSize = QSize(iconHeight, iconHeight);

    const QString key = "KStandardItemListWidget:" % name % ":" % overlays.values().join(QLatin1Char(':')) % ":" % QString::number(iconHeight) % "@"
        % QString::number(dpr) % ":" % QString::number(mode);
    QPixmap pixmap;

    if (!QPixmapCache::find(key, &pixmap)) {
        QIcon icon = QIcon::fromTheme(name);
        if (icon.isNull()) {
            icon = QIcon(name);
        }
        if (!icon.isNull()) {
            pixmap = icon.pixmap(iconSize, dpr, mode);
        }
        if (pixmap.isNull()) {
            icon = fallbackIcon;
            pixmap = icon.pixmap(iconSize, dpr, mode);
        }
        if (pixmap.width() != iconHeight * dpr || pixmap.height() != iconHeight * dpr) {
            KPixmapModifier::scale(pixmap, iconSize * dpr);
        }

        pixmap = addOverlays(pixmap, overlays, size, dpr, mode);

        QPixmapCache::insert(key, pixmap);
    }
    pixmap.setDevicePixelRatio(dpr);

    return pixmap;
}

QSizeF KStandardItemListWidget::preferredRatingSize(const KItemListStyleOption &option)
{
    const qreal height = option.fontMetrics.ascent();
    return QSizeF(height * 5, height);
}

qreal KStandardItemListWidget::columnPadding(const KItemListStyleOption &option)
{
    return option.padding * 6;
}

#include "moc_kstandarditemlistwidget.cpp"
