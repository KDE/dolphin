/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef ANIMATEDHEIGHTWIDGET_H
#define ANIMATEDHEIGHTWIDGET_H

#include "global.h"

#include <QPointer>
#include <QWidget>

class QPropertyAnimation;
class QScrollArea;

/**
 * @brief An abstract base class which facilitates animated showing and hiding of sub-classes
 */
class AnimatedHeightWidget : public QWidget
{
    Q_OBJECT
public:
    AnimatedHeightWidget(QWidget *parent);

    /**
     * Plays a show or hide animation while changing visibility.
     * Therefore, if this method is used to hide this widget, the actual hiding will be delayed until the animation finished.
     *
     * @param visible   Whether this bar is supposed to be visible long-term
     * @param animated  Whether this should be animated. The animation is skipped if the users' settings are configured that way.
     *
     * @see QWidget::setVisible()
     */
    void setVisible(bool visible, Animated animated);

    /**
     * @returns a QSize with a width of 1 to make sure that this bar never causes side panels to shrink.
     *          The returned height equals preferredHeight().
     */
    QSize sizeHint() const override;

Q_SIGNALS:
    void visibilityChanged();

protected:
    /**
     * AnimatedHeightWidget always requires a singular main child which we call the "contentsContainer".
     * Use this method to register such an object.
     *
     * @returns a "contentsContainer" which is a QWidget that consists of/contains all visible contents of this AnimatedHeightWidget.
     *          It will be the only grandchild of this AnimatedHeightWidget.
     * @param contentsContainer The object that should be used as the "contentsContainer".
     */
    QWidget *prepareContentsContainer(QWidget *contentsContainer = new QWidget);

    /** @returns whether this object is currently animating a visibility change. */
    bool isAnimationRunning() const;

private:
    using QWidget::hide; // Use QAbstractAnimation::setVisible() instead.
    using QWidget::setVisible; // Makes sure that the setVisible() declaration above doesn't fully hide the one from QWidget so we can still use it privately.
    using QWidget::show; // Use QAbstractAnimation::setVisible() instead.

    /** @returns the full preferred height this widget should have when it is done animating and visible. */
    virtual int preferredHeight() const = 0;

private:
    /** @see contentsContainerParent() */
    QScrollArea *m_contentsContainerParent = nullptr;

    /** @see AnimatedHeightWidget::setVisible() */
    QPointer<QPropertyAnimation> m_heightAnimation;
};

#endif // ANIMATEDHEIGHTWIDGET_H
