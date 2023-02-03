/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTWIDGET_H
#define KITEMLISTWIDGET_H

#include "dolphin_export.h"
#include "kitemviews/kitemliststyleoption.h"

#include <QBitArray>
#include <QGraphicsWidget>
#include <QStyle>
#include <QTimer>

class KItemListSelectionToggle;
class KItemListView;
class QPropertyAnimation;

/**
 * @brief Provides information for creating an instance of KItemListWidget.
 *
 * KItemListView only creates KItemListWidget instances for the visible
 * area. For calculating the required size of all items the expected
 * size for the invisible items must be accessible. KItemListWidgetInformant
 * provides this information.
 */
class DOLPHIN_EXPORT KItemListWidgetInformant
{
public:
    KItemListWidgetInformant();
    virtual ~KItemListWidgetInformant();

    virtual void calculateItemSizeHints(QVector<std::pair<qreal, bool>> &logicalHeightHints, qreal &logicalWidthHint, const KItemListView *view) const = 0;

    virtual qreal preferredRoleColumnWidth(const QByteArray &role, int index, const KItemListView *view) const = 0;
};

/**
 * @brief Widget that shows a visible item from the model.
 *
 * For showing an item from a custom model it is required to at least overwrite KItemListWidget::paint().
 * All properties are set by KItemListView, for each property there is a corresponding
 * virtual protected method that allows to react on property changes.
 */
class DOLPHIN_EXPORT KItemListWidget : public QGraphicsWidget
{
    Q_OBJECT

    Q_PROPERTY(int iconSize READ iconSize WRITE setIconSize)

public:
    KItemListWidget(KItemListWidgetInformant *informant, QGraphicsItem *parent);
    ~KItemListWidget() override;

    void setIndex(int index);
    int index() const;

    void setData(const QHash<QByteArray, QVariant> &data, const QSet<QByteArray> &roles = QSet<QByteArray>());
    QHash<QByteArray, QVariant> data() const;

    /**
     * Draws the hover-rectangle if the item is hovered. Overwrite this method
     * to show the data of the custom model provided by KItemListWidget::data().
     * @reimp
     */
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

    void setVisibleRoles(const QList<QByteArray> &roles);
    QList<QByteArray> visibleRoles() const;

    /**
     * Sets the width of a role that should be used if the alignment of the content
     * should be done in columns.
     */
    void setColumnWidth(const QByteArray &role, qreal width);
    qreal columnWidth(const QByteArray &role) const;

    void setSidePadding(qreal width);
    qreal sidePadding() const;

    void setStyleOption(const KItemListStyleOption &option);
    const KItemListStyleOption &styleOption() const;

    // TODO: Hides QGraphicsItem::setSelected()/isSelected(). Replace
    // this by using the default mechanism.
    void setSelected(bool selected);
    bool isSelected() const;

    void setCurrent(bool current);
    bool isCurrent() const;

    void setHovered(bool hovered);
    bool isHovered() const;

    void setExpansionAreaHovered(bool hover);
    bool expansionAreaHovered() const;

    void setHoverPosition(const QPointF &pos);

    void setAlternateBackground(bool enable);
    bool alternateBackground() const;

    void setEnabledSelectionToggle(bool enabled);
    bool enabledSelectionToggle() const;

    /**
     * Sets the sibling information for the item and all of its parents.
     * The sibling information of the upper most parent is represented by
     * the first bit, the sibling information of the item by the last bit.
     * The sibling information is useful for drawing the branches in
     * tree views.
     */
    void setSiblingsInformation(const QBitArray &siblings);
    QBitArray siblingsInformation() const;

    /**
     * Allows the user to edit the role \a role. The signals
     * roleEditingCanceled() or roleEditingFinished() will be
     * emitted after editing. An ongoing editing gets canceled if
     * the role is empty. Derived classes must implement
     * editedRoleChanged().
     */
    void setEditedRole(const QByteArray &role);
    QByteArray editedRole() const;

    /**
     * Contains the actual icon size used to draw the icon.
     * Also used during icon resizing animation.
     */
    void setIconSize(int iconSize);
    int iconSize() const;

    /**
     * @return True if \a point is inside KItemListWidget::hoverRect(),
     *         KItemListWidget::textRect(), KItemListWidget::selectionToggleRect()
     *         or KItemListWidget::expansionToggleRect().
     * @reimp
     */
    bool contains(const QPointF &point) const override;

    /**
     * @return Rectangle for the area that shows the icon.
     */
    virtual QRectF iconRect() const = 0;

    /**
     * @return Rectangle for the area that contains the text-properties.
     */
    virtual QRectF textRect() const = 0;

    /**
     * @return Focus rectangle for indicating the current item. Per default
     *         textRect() will be returned. Overwrite this method if textRect()
     *         provides a larger rectangle than the actual text (e.g. to
     *         be aligned with the iconRect()). The textFocusRect() may not be
     *         outside the boundaries of textRect().
     */
    virtual QRectF textFocusRect() const;

    /**
     * @return Rectangle around which a selection box should be drawn if the item is selected.
     */
    virtual QRectF selectionRect() const = 0;

    /**
     * @return Rectangle for the selection-toggle that is used to select or deselect an item.
     *         Per default an empty rectangle is returned which means that no selection-toggle
     *         is available.
     */
    virtual QRectF selectionToggleRect() const;

    /**
     * @return Rectangle for the expansion-toggle that is used to open a sub-tree of the model.
     *         Per default an empty rectangle is returned which means that no opening of sub-trees
     *         is supported.
     */
    virtual QRectF expansionToggleRect() const;

    /**
     * @return Pixmap that is used when dragging an item. Per default the current state of the
     *         widget is returned as pixmap.
     */
    virtual QPixmap createDragPixmap(const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr);

Q_SIGNALS:
    void roleEditingCanceled(int index, const QByteArray &role, const QVariant &value);
    void roleEditingFinished(int index, const QByteArray &role, const QVariant &value);

protected:
    virtual void dataChanged(const QHash<QByteArray, QVariant> &current, const QSet<QByteArray> &roles = QSet<QByteArray>());
    virtual void visibleRolesChanged(const QList<QByteArray> &current, const QList<QByteArray> &previous);
    virtual void columnWidthChanged(const QByteArray &role, qreal current, qreal previous);
    virtual void sidePaddingChanged(qreal width);
    virtual void styleOptionChanged(const KItemListStyleOption &current, const KItemListStyleOption &previous);
    virtual void currentChanged(bool current);
    virtual void selectedChanged(bool selected);
    virtual void hoveredChanged(bool hovered);
    virtual void alternateBackgroundChanged(bool enabled);
    virtual void siblingsInformationChanged(const QBitArray &current, const QBitArray &previous);
    virtual void editedRoleChanged(const QByteArray &current, const QByteArray &previous);
    virtual void iconSizeChanged(int current, int previous);
    void resizeEvent(QGraphicsSceneResizeEvent *event) override;
    void clearHoverCache();

    /**
     * Called when the user starts hovering this item.
     */
    virtual void hoverSequenceStarted();

    /**
     * Called in regular intervals while the user is hovering this item.
     *
     * @param sequenceIndex An index that increases over time while the user hovers.
     */
    virtual void hoverSequenceIndexChanged(int sequenceIndex);

    /**
     * Called when the user stops hovering this item.
     */
    virtual void hoverSequenceEnded();

    /**
     * @return The current opacity of the hover-animation. When implementing a custom painting-code for a hover-state
     *         this opacity value should be respected.
     */
    qreal hoverOpacity() const;

    int hoverSequenceIndex() const;

    const KItemListWidgetInformant *informant() const;

private Q_SLOTS:
    void slotHoverAnimationFinished();
    void slotHoverSequenceTimerTimeout();

private:
    void initializeSelectionToggle();
    void setHoverOpacity(qreal opacity);
    void drawItemStyleOption(QPainter *painter, QWidget *widget, QStyle::State styleState);

private:
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)

    KItemListWidgetInformant *m_informant;
    int m_index;
    bool m_selected;
    bool m_current;
    bool m_hovered;
    bool m_expansionAreaHovered;
    bool m_alternateBackground;
    bool m_enabledSelectionToggle;
    QHash<QByteArray, QVariant> m_data;
    QList<QByteArray> m_visibleRoles;
    QHash<QByteArray, qreal> m_columnWidths;
    qreal m_sidePadding;
    KItemListStyleOption m_styleOption;
    QBitArray m_siblingsInfo;

    qreal m_hoverOpacity;
    mutable QPixmap *m_hoverCache;
    QPropertyAnimation *m_hoverAnimation;

    int m_hoverSequenceIndex;
    QTimer m_hoverSequenceTimer;

    KItemListSelectionToggle *m_selectionToggle;

    QByteArray m_editedRole;
    int m_iconSize;
};

inline const KItemListWidgetInformant *KItemListWidget::informant() const
{
    return m_informant;
}

#endif
