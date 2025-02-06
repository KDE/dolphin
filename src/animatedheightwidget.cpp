/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "animatedheightwidget.h"

#include <QGridLayout>
#include <QPropertyAnimation>
#include <QScrollArea>
#include <QStyle>

AnimatedHeightWidget::AnimatedHeightWidget(QWidget *parent)
    : QWidget{parent}
{
    // Showing of this widget is normally animated. We hide it for now and make it small.
    hide();
    setMaximumHeight(0);

    auto fillParentLayout = new QGridLayout(this);
    fillParentLayout->setContentsMargins(0, 0, 0, 0);

    // Put the contents into a QScrollArea. This prevents increasing the view width
    // in case there is not enough available width for the contents.
    m_contentsContainerParent = new QScrollArea(this);
    fillParentLayout->addWidget(m_contentsContainerParent);
    m_contentsContainerParent->setFrameShape(QFrame::NoFrame);
    m_contentsContainerParent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contentsContainerParent->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_contentsContainerParent->setWidgetResizable(true);

    setMinimumWidth(0);
}

QSize AnimatedHeightWidget::sizeHint() const
{
    return QSize{1, preferredHeight()};
    // 1 as width because this widget should never be the reason the DolphinViewContainer is made wider.
}

void AnimatedHeightWidget::setVisible(bool visible, Animated animated)
{
    setEnabled(visible);
    if (m_heightAnimation) {
        m_heightAnimation->stop(); // deletes because of QAbstractAnimation::DeleteWhenStopped.
    }

    if (animated == WithAnimation
        && (style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) < 1 || GlobalConfig::animationDurationFactor() <= 0.0)) {
        animated = WithoutAnimation;
    }

    if (animated == WithoutAnimation) {
        setMaximumHeight(visible ? preferredHeight() : 0);
        setVisible(visible);
        return;
    }

    m_heightAnimation = new QPropertyAnimation(this, "maximumHeight");
    m_heightAnimation->setDuration(2 * style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) * GlobalConfig::animationDurationFactor());

    m_heightAnimation->setStartValue(height());
    m_heightAnimation->setEasingCurve(QEasingCurve::OutCubic);
    if (visible) {
        show();
        m_heightAnimation->setEndValue(preferredHeight());
        connect(m_heightAnimation, &QAbstractAnimation::finished, this, [this]() {
            Q_EMIT visibilityChanged();
        });
    } else {
        m_heightAnimation->setEndValue(0);
        connect(m_heightAnimation, &QAbstractAnimation::finished, this, [this]() {
            hide();
            Q_EMIT visibilityChanged();
        });
    }

    m_heightAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

QWidget *AnimatedHeightWidget::prepareContentsContainer(QWidget *contentsContainer)
{
    Q_ASSERT_X(!m_contentsContainerParent->widget(),
               "AnimatedHeightWidget::prepareContentsContainer",
               "Another contentsContainer has already been prepared. There can only be one.");
    contentsContainer->setParent(m_contentsContainerParent);
    m_contentsContainerParent->setWidget(contentsContainer);
    m_contentsContainerParent->setFocusProxy(contentsContainer);
    return contentsContainer;
}

bool AnimatedHeightWidget::isAnimationRunning() const
{
    return m_heightAnimation && m_heightAnimation->state() == QAbstractAnimation::Running;
}
