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

#ifndef KFILEITEMLISTWIDGET_H
#define KFILEITEMLISTWIDGET_H

#include <libdolphin_export.h>

#include <kitemviews/kitemlistwidget.h>

#include <QPixmap>
#include <QPointF>
#include <QStaticText>

class LIBDOLPHINPRIVATE_EXPORT KFileItemListWidget : public KItemListWidget
{
    Q_OBJECT

public:
    enum Layout
    {
        IconsLayout,
        CompactLayout,
        DetailsLayout
    };

    KFileItemListWidget(QGraphicsItem* parent);
    virtual ~KFileItemListWidget();

    void setLayout(Layout layout);
    Layout layout() const;

    void setSupportsItemExpanding(bool supportsItemExpanding);
    bool supportsItemExpanding() const;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    virtual QRectF iconRect() const;
    virtual QRectF textRect() const;
    virtual QRectF expansionToggleRect() const;
    virtual QRectF selectionToggleRect() const;

    /**
     * @return Shown string for the role \p role of the item with the values \p values.
     */
    // TODO: Move this method to a helper class shared by KFileItemListWidget and
    // KFileItemListView to share information that is required to calculate the size hints
    // in KFileItemListView and to represent the actual data in KFileItemListWidget.
    static QString roleText(const QByteArray& role, const QHash<QByteArray, QVariant>& values);

protected:
    /**
     * Invalidates the cache which results in calling KFileItemListWidget::refreshCache() as
     * soon as the item need to gets repainted.
     */
    void invalidateCache();

    /**
     * Is called if the cache got invalidated by KFileItemListWidget::invalidateCache().
     * The default implementation is empty.
     */
    virtual void refreshCache();

    void setTextColor(const QColor& color);
    QColor textColor() const;

    void setOverlay(const QPixmap& overlay);
    QPixmap overlay() const;

    virtual void dataChanged(const QHash<QByteArray, QVariant>& current, const QSet<QByteArray>& roles = QSet<QByteArray>());
    virtual void visibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous);
    virtual void columnWidthChanged(const QByteArray& role, qreal current, qreal previous);
    virtual void styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);
    virtual void hoveredChanged(bool hovered);
    virtual void selectedChanged(bool selected);
    virtual void siblingsInformationChanged(const QBitArray& current, const QBitArray& previous);
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void hideEvent(QHideEvent* event);

private slots:
    void slotCutItemsChanged();

private:
    enum TextId {
        Name,
        Size,
        Date,
        Permissions,
        Owner,
        Group,
        Type,
        Destination,
        Path,
        TextIdCount // Mandatory last entry
    };

    void triggerCacheRefreshing();
    void updateExpansionArea();
    void updatePixmapCache();

    void updateTextsCache();
    void updateIconsLayoutTextCache();
    void updateCompactLayoutTextCache();
    void updateDetailsLayoutTextCache();

    void updateAdditionalInfoTextColor();

    void drawPixmap(QPainter* painter, const QPixmap& pixmap);
    void drawSiblingsInformation(QPainter* painter);

    static QPixmap pixmapForIcon(const QString& name, int size);
    static TextId roleTextId(const QByteArray& role);
    static void applyCutEffect(QPixmap& pixmap);
    static void applyHiddenEffect(QPixmap& pixmap);

private:
    bool m_isCut;
    bool m_isHidden;
    bool m_isExpandable;
    bool m_supportsItemExpanding;

    bool m_dirtyLayout;
    bool m_dirtyContent;
    QSet<QByteArray> m_dirtyContentRoles;

    Layout m_layout;
    QPointF m_pixmapPos;
    QPixmap m_pixmap;
    QSize m_scaledPixmapSize;

    QRectF m_iconRect;          // Cache for KItemListWidget::iconRect()
    QPixmap m_hoverPixmap;      // Cache for modified m_pixmap when hovering the item

    QPointF m_textPos[TextIdCount];
    QStaticText m_text[TextIdCount];
    QRectF m_textRect;

    QList<QByteArray> m_sortedVisibleRoles;

    QRectF m_expansionArea;

    QColor m_customTextColor;
    QColor m_additionalInfoTextColor;

    QPixmap m_overlay;
};

#endif


