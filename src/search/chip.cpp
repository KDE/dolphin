/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "chip.h"

#include <KColorUtils>
#include <KLocalizedString>
#include <QPaintEvent>
#include <QStylePainter>

using namespace Search;

ChipBase::ChipBase(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent)
    : QWidget{parent}
    , UpdatableStateInterface{dolphinQuery}
{
    m_removeButton = new QToolButton{this};
    m_removeButton->setText(i18nc("@action:button", "Remove Filter"));
    m_removeButton->setIcon(QIcon::fromTheme("list-remove"));
    m_removeButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    m_removeButton->setAutoRaise(true);

    auto layout = new QHBoxLayout{this};
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
}

void ChipBase::paintEvent(QPaintEvent *event)
{
    QStylePainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    QColor penColor = KColorUtils::mix(palette().base().color(), palette().text().color(), 0.3);
    // QPainter is bad at drawing lines that are exactly 1px.
    // Using QPen::setCosmetic(true) with a 1px pen width
    // doesn't look quite as good as just using 1.001px.
    qreal penWidth = 1.001;
    qreal penMargin = penWidth / 2;
    QPen pen(penColor, penWidth);
    pen.setCosmetic(true);
    QRectF rect = event->rect();
    rect.adjust(penMargin, penMargin, -penMargin, -penMargin);
    painter.setBrush(palette().base());
    painter.setPen(pen);
    painter.drawRoundedRect(rect, 5, 5); // 5 is the current default Breeze corner radius
    QWidget::paintEvent(event);
}

#include "moc_chip.cpp"
