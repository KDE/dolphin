/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTGROUPHEADER_H
#define KITEMLISTGROUPHEADER_H

#include "dolphin_export.h"
#include "kitemviews/kitemliststyleoption.h"

#include <QByteArray>
#include <QGraphicsWidget>
#include <QVariant>

class KItemListView;

/**
 * @brief Base class for group headers.
 *
 * Draws a default header background. Derived classes must reimplement
 * the method paint() and draw the role within the given roleBounds() with
 * the color roleColor().
 */
class DOLPHIN_EXPORT KItemListGroupHeader : public QGraphicsWidget
{
    Q_OBJECT

public:
    explicit KItemListGroupHeader(QGraphicsWidget* parent = nullptr);
    ~KItemListGroupHeader() override;

    void setRole(const QByteArray& role);
    QByteArray role() const;

    void setData(const QVariant& data);
    QVariant data() const;

    void setStyleOption(const KItemListStyleOption& option);
    const KItemListStyleOption& styleOption() const;

    /**
     * Sets the scroll orientation that is used by the KItemListView.
     * This allows the group header to use a modified look dependent
     * on the orientation.
     */
    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setItemIndex(int index);
    int itemIndex() const;

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    virtual void paintRole(QPainter* painter, const QRectF& roleBounds, const QColor& color) = 0;
    virtual void paintSeparator(QPainter* painter, const QColor& color) = 0;

    /**
     * Is called after the role has been changed and allows the derived class
     * to react on this change.
     */
    virtual void roleChanged(const QByteArray& current, const QByteArray& previous);

    /**
     * Is called after the role has been changed and allows the derived class
     * to react on this change.
     */
    virtual void dataChanged(const QVariant& current, const QVariant& previous);

    /**
     * Is called after the style option has been changed and allows the derived class
     * to react on this change.
     */
    virtual void styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);

    /**
     * Is called after the scroll orientation has been changed and allows the derived class
     * to react on this change.
     */
    virtual void scrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);

    /**
     * Is called after the item index has been changed and allows the derived class to react on
     * this change.
     */
    virtual void itemIndexChanged(int current, int previous);

    void resizeEvent(QGraphicsSceneResizeEvent* event) override;

    virtual QPalette::ColorRole normalTextColorRole() const;
    virtual QPalette::ColorRole normalBaseColorRole() const;

private:
    void updateCache();
    void updateSize();

    static QColor mixedColor(const QColor& c1, const QColor& c2, int c1Percent = 50);

    QColor textColor() const;
    QColor baseColor() const;

private:
    bool m_dirtyCache;
    QByteArray m_role;
    QVariant m_data;
    KItemListStyleOption m_styleOption;
    Qt::Orientation m_scrollOrientation;
    int m_itemIndex;

    QColor m_separatorColor;
    QColor m_roleColor;
    QRectF m_roleBounds;
};
#endif


