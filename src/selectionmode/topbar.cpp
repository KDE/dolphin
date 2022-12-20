/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "topbar.h"

#include "backgroundcolorhelper.h"

#include <KColorScheme>
#include <KLocalizedString>
#include <KToolTipHelper>

#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QStyle>

using namespace SelectionMode;

TopBar::TopBar(QWidget *parent) :
    QWidget{parent}
{
    // Showing of this widget is normally animated. We hide it for now and make it small.
    hide();
    setMaximumHeight(0);

    setToolTip(KToolTipHelper::whatsThisHintOnly());
    setWhatsThis(xi18nc("@info:whatsthis", "<title>Selection Mode</title><para>Select files or folders to manage or manipulate them."
        "<list><item>Press on a file or folder to select it.</item><item>Press on an already selected file or folder to deselect it.</item>"
        "<item>Pressing an empty area does <emphasis>not</emphasis> clear the selection.</item>"
        "<item>Selection rectangles (created by dragging from an empty area) invert the selection status of items within.</item></list></para>"
        "<para>The available action buttons at the bottom change depending on the current selection.</para>"));

    auto fillParentLayout = new QGridLayout(this);
    fillParentLayout->setContentsMargins(0, 0, 0, 0);

    // Put the contents into a QScrollArea. This prevents increasing the view width
    // in case that not enough width for the contents is available. (this trick is also used in bottombar.cpp.)
    auto scrollArea = new QScrollArea(this);
    fillParentLayout->addWidget(scrollArea);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scrollArea->setWidgetResizable(true);

    auto contentsContainer = new QWidget(scrollArea);
    scrollArea->setWidget(contentsContainer);

    BackgroundColorHelper::instance()->controlBackgroundColor(this);

    setMinimumWidth(0);

    m_fullLabelString = i18nc("@info label above the view explaining the state",
                                     "Selection Mode: Click on files or folders to select or deselect them.");
    m_shortLabelString = i18nc("@info label above the view explaining the state",
                                     "Selection Mode");
    m_label = new QLabel(contentsContainer);
    m_label->setMinimumWidth(0);
    BackgroundColorHelper::instance()->controlBackgroundColor(m_label);

    m_closeButton = new QPushButton(QIcon::fromTheme(QStringLiteral("window-close-symbolic")), "", contentsContainer);
    m_closeButton->setText(i18nc("@action:button", "Exit Selection Mode"));
    m_closeButton->setFlat(true);
    connect(m_closeButton, &QAbstractButton::clicked,
            this, &TopBar::selectionModeLeavingRequested);

    QHBoxLayout *layout = new QHBoxLayout(contentsContainer);
    auto contentsMargins = layout->contentsMargins();
    m_preferredHeight = contentsMargins.top() + m_label->sizeHint().height() + contentsMargins.bottom();
    scrollArea->setMaximumHeight(m_preferredHeight);
    m_closeButton->setFixedHeight(m_preferredHeight);
    layout->setContentsMargins(0, 0, 0, 0);

    layout->addStretch();
    layout->addWidget(m_label);
    layout->addStretch();
    layout->addWidget(m_closeButton);
}

void TopBar::setVisible(bool visible, Animated animated)
{
    Q_ASSERT_X(animated == WithAnimation, "SelectionModeTopBar::setVisible", "This wasn't implemented.");

    if (m_heightAnimation) {
        m_heightAnimation->stop(); // deletes because of QAbstractAnimation::DeleteWhenStopped.
    }
    m_heightAnimation = new QPropertyAnimation(this, "maximumHeight");
    m_heightAnimation->setDuration(2 *
            style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) *
            GlobalConfig::animationDurationFactor());

    m_heightAnimation->setStartValue(height());
    m_heightAnimation->setEasingCurve(QEasingCurve::OutCubic);
    if (visible) {
        show();
        m_heightAnimation->setEndValue(m_preferredHeight);
    } else {
        m_heightAnimation->setEndValue(0);
        connect(m_heightAnimation, &QAbstractAnimation::finished,
                this, &QWidget::hide);
    }

    m_heightAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

void TopBar::resizeEvent(QResizeEvent *resizeEvent)
{
    updateLabelString();
    return QWidget::resizeEvent(resizeEvent);
}

void TopBar::updateLabelString()
{
    QFontMetrics fontMetrics = m_label->fontMetrics();
    if (fontMetrics.horizontalAdvance(m_fullLabelString) + m_closeButton->sizeHint().width() + style()->pixelMetric(QStyle::PM_LayoutLeftMargin) * 2 + style()->pixelMetric(QStyle::PM_LayoutRightMargin) * 2 < width()) {
        m_label->setText(m_fullLabelString);
    } else {
        m_label->setText(m_shortLabelString);
    }
}
