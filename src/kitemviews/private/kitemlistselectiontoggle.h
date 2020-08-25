/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef KITEMLISTSELECTIONTOGGLE_H
#define KITEMLISTSELECTIONTOGGLE_H

#include "dolphin_export.h"

#include <QGraphicsWidget>
#include <QPixmap>


/**
 * @brief Allows to toggle between the selected and unselected state of an item.
 */
class DOLPHIN_EXPORT KItemListSelectionToggle : public QGraphicsWidget
{
    Q_OBJECT

public:
    explicit KItemListSelectionToggle(QGraphicsItem* parent);
    ~KItemListSelectionToggle() override;

    void setChecked(bool checked);
    bool isChecked() const;

    void setHovered(bool hovered);

    void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = nullptr) override;

protected:
    void resizeEvent(QGraphicsSceneResizeEvent* event) override;

private:
    void updatePixmap();
    int iconSize() const;

private:
    bool m_checked;
    bool m_hovered;
    QPixmap m_pixmap;
};

#endif


