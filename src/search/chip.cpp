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
    , UpdatableStateInterface{std::move(dolphinQuery)}
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
    QStyleOptionFrame option;
    option.initFrom(this);
    option.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &option, this);
    option.state = QStyle::State_Enabled | QStyle::State_Sunken;
    painter.drawPrimitive(QStyle::PE_FrameLineEdit, option);
    QWidget::paintEvent(event);
}

#include "moc_chip.cpp"
