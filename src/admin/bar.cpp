/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "bar.h"

#include "workerintegration.h"

#include <KColorScheme>
#include <KContextualHelpButton>
#include <KLocalizedString>

#include <QEvent>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>
#include <QWidgetAction>

using namespace Admin;

Bar::Bar(QWidget *parent)
    : AnimatedHeightWidget{parent}
{
    setAutoFillBackground(true);
    updateColors();

    QWidget *contenntsContainer = prepareContentsContainer();

    m_fullLabelString = i18nc("@info label above the view explaining the state", "Acting as an Administrator – Be careful!");
    m_shortLabelString = i18nc("@info label above the view explaining the state, keep short", "Acting as Admin");
    m_label = new QLabel(contenntsContainer);
    m_label->setMinimumWidth(0);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard | Qt::LinksAccessibleByKeyboard); // for keyboard accessibility

    m_warningButton = new KContextualHelpButton(warningMessage(), nullptr, contenntsContainer);
    m_warningButton->setIcon(QIcon::fromTheme(QStringLiteral("emblem-warning")));

    m_closeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("window-close-symbolic")), "", contenntsContainer);
    m_closeButton->setToolTip(i18nc("@action:button", "Stop Acting as an Administrator"));
    m_closeButton->setFlat(true);
    connect(m_closeButton, &QAbstractButton::clicked, this, &Bar::activated); // Make sure the view connected to this bar is active before exiting admin mode.
    connect(m_closeButton, &QAbstractButton::clicked, this, &WorkerIntegration::exitAdminMode);

    QHBoxLayout *layout = new QHBoxLayout(contenntsContainer);
    auto contentsMargins = layout->contentsMargins();
    m_preferredHeight = contentsMargins.top() + m_label->sizeHint().height() + contentsMargins.bottom();
    m_warningButton->setFixedHeight(m_preferredHeight);
    m_closeButton->setFixedHeight(m_preferredHeight);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addStretch();
    layout->addWidget(m_label);
    layout->addWidget(m_warningButton);
    layout->addStretch();
    layout->addWidget(m_closeButton);
}

bool Bar::event(QEvent *event)
{
    if (event->type() == QEvent::PaletteChange) {
        updateColors();
    }
    return AnimatedHeightWidget::event(event);
}

void Bar::resizeEvent(QResizeEvent *resizeEvent)
{
    updateLabelString();
    return QWidget::resizeEvent(resizeEvent);
}

void Bar::updateColors()
{
    QPalette palette = parentWidget()->palette();
    KColorScheme colorScheme{QPalette::Normal, KColorScheme::ColorSet::Window};
    colorScheme.adjustBackground(palette, KColorScheme::NegativeBackground, QPalette::Window, KColorScheme::ColorSet::Window);
    colorScheme.adjustForeground(palette, KColorScheme::NegativeText, QPalette::WindowText, KColorScheme::ColorSet::Window);
    setPalette(palette);
}

void Bar::updateLabelString()
{
    QFontMetrics fontMetrics = m_label->fontMetrics();
    if (fontMetrics.horizontalAdvance(m_fullLabelString) + m_warningButton->sizeHint().width() + m_closeButton->sizeHint().width()
            + style()->pixelMetric(QStyle::PM_LayoutLeftMargin) * 2 + style()->pixelMetric(QStyle::PM_LayoutRightMargin) * 2
        < width()) {
        m_label->setText(m_fullLabelString);
    } else {
        m_label->setText(m_shortLabelString);
    }
}

#include "moc_bar.cpp"
