/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "barsecondrowflowlayout.h"

#include <QWidget>

#include <vector>

using namespace Search;

namespace
{
constexpr int searchLocationButtonsCount = 2;
}

BarSecondRowFlowLayout::BarSecondRowFlowLayout(QWidget *parent)
    : QLayout{parent}
{
    setContentsMargins(0, 0, 0, 0);
}

BarSecondRowFlowLayout::~BarSecondRowFlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}

void BarSecondRowFlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}

Qt::Orientations BarSecondRowFlowLayout::expandingDirections() const
{
    return {};
}

bool BarSecondRowFlowLayout::hasHeightForWidth() const
{
    return false;
}

int BarSecondRowFlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *BarSecondRowFlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QSize BarSecondRowFlowLayout::sizeHint() const
{
    const QRect rect = geometry();
    QSize size;
    for (const QLayoutItem *item : std::as_const(itemList)) {
        size = size.expandedTo(QSize{item->geometry().right() - rect.x(), item->geometry().bottom() - rect.y()});
    }
    return size;
}

void BarSecondRowFlowLayout::setGeometry(const QRect &rect)
{
    const int oldHeightHint = sizeHint().height();
    QLayout::setGeometry(rect);
    const bool isLeftToRight = itemAt(0)->widget()->layoutDirection() == Qt::LeftToRight;
    int x = rect.left();
    int y = rect.top();

    /// The search location buttons are treated differently. They are meant to be in the same row and aligned the other way.
    int totalLocationButtonWidth = 0;
    for (int i = 0; i < searchLocationButtonsCount; i++) {
        totalLocationButtonWidth += itemAt(i)->widget()->sizeHint().width();
    }
    if (totalLocationButtonWidth > rect.width()) {
        /// There is not enough space so we will smush all the location buttons into the first row.
        for (int i = 0; i < searchLocationButtonsCount; i++) {
            QWidget *widget = itemAt(i)->widget();
            const int targetWidth = qMin(widget->sizeHint().width(), rect.width() / searchLocationButtonsCount);
            widget->setGeometry(isLeftToRight ? x : rect.right() - x - targetWidth, y, targetWidth, widget->sizeHint().height());
            x += widget->width();
        }
    } else {
        for (int i = 0; i < searchLocationButtonsCount; i++) {
            QWidget *widget = itemAt(i)->widget();
            QSize preferredSize = widget->sizeHint();
            widget->setGeometry(isLeftToRight ? x : rect.right() - x - preferredSize.width(), y, preferredSize.width(), preferredSize.height());
            x += widget->width() + spacing();
        }
    }

    // We want to align all further widgets the other way. We do this by first filling up the row like usual and then moving all widgets of the current row by
    // the remaining space.
    std::vector<QWidget *> currentRowWidgets;
    for (int i = searchLocationButtonsCount; i < count(); i++) {
        QWidget *widget = itemAt(i)->widget();
        const int remainingSpace = rect.right() - x + spacing();
        if (widget->sizeHint().width() < remainingSpace) {
            QSize preferredSize = widget->sizeHint();
            widget->setGeometry(isLeftToRight ? x : rect.right() - x - preferredSize.width(), y, preferredSize.width(), preferredSize.height());
            x += widget->width() + spacing();
            currentRowWidgets.push_back(widget);
            continue;
        }

        // There is not enough space for the next widget. We need to open up a new row.
        // Right align all the widgets of the previous row.
        for (QWidget *widget : std::as_const(currentRowWidgets)) {
            widget->setGeometry(widget->geometry().translated(isLeftToRight ? remainingSpace : -remainingSpace, 0));
        }
        currentRowWidgets.clear();

        x = 0;
        y += itemAt(i - 1)->widget()->height() + spacing();

        QSize preferredSize = widget->sizeHint();
        const int targetWidth = qMin(preferredSize.width(), rect.width());
        widget->setGeometry(isLeftToRight ? x : rect.right() - x - targetWidth, y, targetWidth, preferredSize.height());
        x += widget->width() + spacing();
        currentRowWidgets.push_back(widget);
    }

    // Right align all the widgets of the previous row.
    int remainingSpace = rect.right() - x + spacing();
    for (QWidget *widget : std::as_const(currentRowWidgets)) {
        widget->setGeometry(widget->geometry().translated(isLeftToRight ? remainingSpace : -remainingSpace, 0));
    }

    if (sizeHint().height() != oldHeightHint) {
        Q_EMIT heightHintChanged();
    }
}

QSize BarSecondRowFlowLayout::minimumSize() const
{
    return QSize{0, sizeHint().height()};
}

QLayoutItem *BarSecondRowFlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    return nullptr;
}
