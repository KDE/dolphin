/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KSTANDARDITEMLISTWIDGET_H
#define KSTANDARDITEMLISTWIDGET_H

#include "dolphin_export.h"

#include <kitemviews/kitemlistwidget.h>

#include <QPixmap>
#include <QPointF>
#include <QStaticText>

class KItemListRoleEditor;
class KItemListStyleOption;
class KItemListView;

class DOLPHIN_EXPORT KStandardItemListWidgetInformant : public KItemListWidgetInformant
{
public:
    KStandardItemListWidgetInformant();
    virtual ~KStandardItemListWidgetInformant();

    virtual void calculateItemSizeHints(QVector<qreal>& logicalHeightHints, qreal& logicalWidthHint, const KItemListView* view) const Q_DECL_OVERRIDE;

    virtual qreal preferredRoleColumnWidth(const QByteArray& role,
                                           int index,
                                           const KItemListView* view) const Q_DECL_OVERRIDE;
protected:
    /**
     * @return The value of the "text" role. The default implementation returns
     *         view->model()->data(index)["text"]. If a derived class can
     *         prevent the (possibly expensive) construction of the
     *         QHash<QByteArray, QVariant> returned by KItemModelBase::data(int),
     *         it can reimplement this function.
     */
    virtual QString itemText(int index, const KItemListView* view) const;

    /**
     * @return The value of the "isLink" role. The default implementation returns false.
     *         The derived class should reimplement this function, when information about
     *         links is available and in usage.
     */
    virtual bool itemIsLink(int index, const KItemListView* view) const;

    /**
     * @return String representation of the role \a role. The representation of
     *         a role might depend on other roles, so the values of all roles
     *         are passed as parameter.
     */
    virtual QString roleText(const QByteArray& role,
                             const QHash<QByteArray, QVariant>& values) const;

    /**
    * @return A font based on baseFont which is customized for symlinks.
    */
    virtual QFont customizedFontForLinks(const QFont& baseFont) const;

    void calculateIconsLayoutItemSizeHints(QVector<qreal>& logicalHeightHints, qreal& logicalWidthHint, const KItemListView* view) const;
    void calculateCompactLayoutItemSizeHints(QVector<qreal>& logicalHeightHints, qreal& logicalWidthHint, const KItemListView* view) const;
    void calculateDetailsLayoutItemSizeHints(QVector<qreal>& logicalHeightHints, qreal& logicalWidthHint, const KItemListView* view) const;

    friend class KStandardItemListWidget; // Accesses roleText()
};

/**
 * @brief Itemlist widget implementation for KStandardItemView and KStandardItemModel.
 */
class DOLPHIN_EXPORT KStandardItemListWidget : public KItemListWidget
{
    Q_OBJECT

public:
    enum Layout
    {
        IconsLayout,
        CompactLayout,
        DetailsLayout
    };

    KStandardItemListWidget(KItemListWidgetInformant* informant, QGraphicsItem* parent);
    virtual ~KStandardItemListWidget();

    void setLayout(Layout layout);
    Layout layout() const;

    void setSupportsItemExpanding(bool supportsItemExpanding);
    bool supportsItemExpanding() const;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    virtual QRectF iconRect() const Q_DECL_OVERRIDE;
    virtual QRectF textRect() const Q_DECL_OVERRIDE;
    virtual QRectF textFocusRect() const Q_DECL_OVERRIDE;
    virtual QRectF selectionRect() const Q_DECL_OVERRIDE;
    virtual QRectF expansionToggleRect() const Q_DECL_OVERRIDE;
    virtual QRectF selectionToggleRect() const Q_DECL_OVERRIDE;
    virtual QPixmap createDragPixmap(const QStyleOptionGraphicsItem* option, QWidget* widget = 0) Q_DECL_OVERRIDE;

    static KItemListWidgetInformant* createInformant();

protected:
    /**
     * Invalidates the cache which results in calling KStandardItemListWidget::refreshCache() as
     * soon as the item need to gets repainted.
     */
    void invalidateCache();

    /**
     * Is called if the cache got invalidated by KStandardItemListWidget::invalidateCache().
     * The default implementation is empty.
     */
    virtual void refreshCache();

    /**
     * @return True if the give role should be right aligned when showing it inside a column.
     *         Per default false is returned.
     */
    virtual bool isRoleRightAligned(const QByteArray& role) const;

    /**
     * @return True if the item should be visually marked as hidden item. Per default
     *         false is returned.
     */
    virtual bool isHidden() const;

    /**
     * @return A font based on baseFont which is customized according to the data shown in the widget.
     */
    virtual QFont customizedFont(const QFont& baseFont) const;

    virtual QPalette::ColorRole normalTextColorRole() const;

    void setTextColor(const QColor& color);
    QColor textColor() const;

    void setOverlay(const QPixmap& overlay);
    QPixmap overlay() const;

    /**
     * @see KStandardItemListWidgetInformant::roleText().
     */
    QString roleText(const QByteArray& role, const QHash<QByteArray, QVariant>& values) const;

    /**
     * Fixes:
     * Select the text without MIME-type extension
     * This is file-item-specific and should be moved
     * into KFileItemListWidget.
     *
     * Inherited classes can define, if the MIME-type extension
     * should be selected or not.
     *
     * @return Selection length (with or without MIME-type extension)
     */
    virtual int selectionLength(const QString& text) const;

    virtual void dataChanged(const QHash<QByteArray, QVariant>& current, const QSet<QByteArray>& roles = QSet<QByteArray>()) Q_DECL_OVERRIDE;
    virtual void visibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous) Q_DECL_OVERRIDE;
    virtual void columnWidthChanged(const QByteArray& role, qreal current, qreal previous) Q_DECL_OVERRIDE;
    virtual void styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous) Q_DECL_OVERRIDE;
    virtual void hoveredChanged(bool hovered) Q_DECL_OVERRIDE;
    virtual void selectedChanged(bool selected) Q_DECL_OVERRIDE;
    virtual void siblingsInformationChanged(const QBitArray& current, const QBitArray& previous) Q_DECL_OVERRIDE;
    virtual void editedRoleChanged(const QByteArray& current, const QByteArray& previous) Q_DECL_OVERRIDE;
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event) Q_DECL_OVERRIDE;
    virtual void showEvent(QShowEvent* event) Q_DECL_OVERRIDE;
    virtual void hideEvent(QHideEvent* event) Q_DECL_OVERRIDE;

private slots:
    void slotCutItemsChanged();
    void slotRoleEditingCanceled(const QByteArray& role, const QVariant& value);
    void slotRoleEditingFinished(const QByteArray& role, const QVariant& value);

private:
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

    QRectF roleEditingRect(const QByteArray &role) const;

    /**
     * Closes the role editor and returns the focus back
     * to the KItemListContainer.
     */
    void closeRoleEditor();

    static QPixmap pixmapForIcon(const QString& name, const QStringList& overlays, int size);

    /**
     * @return Preferred size of the rating-image based on the given
     *         style-option. The height of the font is taken as
     *         reference.
     */
    static QSizeF preferredRatingSize(const KItemListStyleOption& option);

    /**
     * @return Horizontal padding in pixels that is added to the required width of
     *         a column to display the content.
     */
    static qreal columnPadding(const KItemListStyleOption& option);

private:
    bool m_isCut;
    bool m_isHidden;
    QFont m_customizedFont;
    QFontMetrics m_customizedFontMetrics;
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

    struct TextInfo
    {
        QPointF pos;
        QStaticText staticText;
    };
    QHash<QByteArray, TextInfo*> m_textInfo;

    QRectF m_textRect;

    QList<QByteArray> m_sortedVisibleRoles;

    QRectF m_expansionArea;

    QColor m_customTextColor;
    QColor m_additionalInfoTextColor;

    QPixmap m_overlay;
    QPixmap m_rating;

    KItemListRoleEditor* m_roleEditor;
    KItemListRoleEditor* m_oldRoleEditor;

    friend class KStandardItemListWidgetInformant; // Accesses private static methods to be able to
                                                   // share a common layout calculation
};

#endif
