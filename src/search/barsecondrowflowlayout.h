/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef BARSECONDROWFLOWLAYOUT_H
#define BARSECONDROWFLOWLAYOUT_H

#include <QLayout>

namespace Search
{

/**
 * @brief The layout for all Search::Bar contents which are not in the first row.
 *
 * For left-to-right languages the search location buttons are kept left-aligned while the chips are right-aligned. When there is not enough space for all the
 * widgts in the current row, a new row is started and the Search::Bar is notified that it needs to resize itself.
 */
class BarSecondRowFlowLayout : public QLayout
{
    Q_OBJECT

public:
    explicit BarSecondRowFlowLayout(QWidget *parent);
    ~BarSecondRowFlowLayout();

    void addItem(QLayoutItem *item) override;
    Qt::Orientations expandingDirections() const override;
    bool hasHeightForWidth() const override;
    int count() const override;
    QLayoutItem *itemAt(int index) const override;
    QSize minimumSize() const override;
    void setGeometry(const QRect &rect) override;
    QSize sizeHint() const override;
    QLayoutItem *takeAt(int index) override;

Q_SIGNALS:
    void heightHintChanged();

private:
    QList<QLayoutItem *> itemList;
};

}

#endif // BARSECONDROWFLOWLAYOUT_H
