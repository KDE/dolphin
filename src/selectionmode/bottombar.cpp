/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "bottombar.h"

#include "bottombarcontentscontainer.h"
#include "backgroundcolorhelper.h"

#include <QGridLayout>
#include <QResizeEvent>
#include <QScrollArea>
#include <QStyle>
#include <QTimer>

using namespace SelectionMode;

BottomBar::BottomBar(KActionCollection *actionCollection, QWidget *parent) :
    QWidget{parent}
{
    // Showing of this widget is normally animated. We hide it for now and make it small.
    hide();
    setMaximumHeight(0);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    setMinimumWidth(0);

    auto fillParentLayout = new QGridLayout(this);
    fillParentLayout->setContentsMargins(0, 0, 0, 0);

    // Put the contents into a QScrollArea. This prevents increasing the view width
    // in case that not enough width for the contents is available. (this trick is also used in dolphinsearchbox.cpp.)
    m_scrollArea = new QScrollArea(this);
    fillParentLayout->addWidget(m_scrollArea);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setWidgetResizable(true);

    m_contentsContainer = new BottomBarContentsContainer(actionCollection, m_scrollArea);
    m_scrollArea->setWidget(m_contentsContainer);
    m_contentsContainer->installEventFilter(this); // Adjusts the height of this bar to the height of the contentsContainer
    connect(m_contentsContainer, &BottomBarContentsContainer::error, this, &BottomBar::error);
    connect(m_contentsContainer, &BottomBarContentsContainer::barVisibilityChangeRequested, this, [this](bool visible){
        if (!m_allowedToBeVisible && visible) {
            return;
        }
        setVisibleInternal(visible, WithAnimation);
    });
    connect(m_contentsContainer, &BottomBarContentsContainer::selectionModeLeavingRequested, this, &BottomBar::selectionModeLeavingRequested);

    BackgroundColorHelper::instance()->controlBackgroundColor(this);
}

void BottomBar::setVisible(bool visible, Animated animated)
{
    m_allowedToBeVisible = visible;
    setVisibleInternal(visible, animated);
}

void BottomBar::setVisibleInternal(bool visible, Animated animated)
{
    Q_ASSERT_X(animated == WithAnimation, "SelectionModeBottomBar::setVisible", "This wasn't implemented.");
    if (!visible && contents() == PasteContents) {
        return; // The bar with PasteContents should not be hidden or users might not know how to paste what they just copied.
                // Set contents to anything else to circumvent this prevention mechanism.
    }
    if (visible && !m_contentsContainer->hasSomethingToShow()) {
        return; // There is nothing on the bar that we want to show. We keep it invisible and only show it when the selection or the contents change.
    }

    setEnabled(visible);
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
        m_heightAnimation->setEndValue(sizeHint().height());
        connect(m_heightAnimation, &QAbstractAnimation::finished,
                this, [this](){ setMaximumHeight(sizeHint().height()); });
    } else {
        m_heightAnimation->setEndValue(0);
        connect(m_heightAnimation, &QAbstractAnimation::finished,
                this, &QWidget::hide);
    }

    m_heightAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

QSize BottomBar::sizeHint() const
{
    return QSize{1, m_contentsContainer->sizeHint().height()};
    // 1 as width because this widget should never be the reason the DolphinViewContainer is made wider.
}

void BottomBar::slotSelectionChanged(const KFileItemList &selection, const QUrl &baseUrl)
{
    m_contentsContainer->slotSelectionChanged(selection, baseUrl);
}

void BottomBar::slotSplitTabDisabled()
{
    switch (contents()) {
    case CopyToOtherViewContents:
    case MoveToOtherViewContents:
        Q_EMIT selectionModeLeavingRequested();
    default:
        return;
    }
}

void BottomBar::resetContents(BottomBar::Contents contents)
{
    m_contentsContainer->resetContents(contents);

    if (m_allowedToBeVisible) {
        setVisibleInternal(true, WithAnimation);
    }
}

BottomBar::Contents BottomBar::contents() const
{
    return m_contentsContainer->contents();
}

bool BottomBar::eventFilter(QObject *watched, QEvent *event)
{
    Q_ASSERT(qobject_cast<QWidget *>(watched)); // This evenfFilter is only implemented for QWidgets.

    switch (event->type()) {
    case QEvent::ChildAdded:
    case QEvent::ChildRemoved:
        QTimer::singleShot(0, this, [this]() {
            // The necessary height might have changed because of the added/removed child so we change the height manually.
            if (isVisibleTo(parentWidget()) && isEnabled() && (!m_heightAnimation || m_heightAnimation->state() != QAbstractAnimation::Running)) {
                setMaximumHeight(sizeHint().height());
            }
        });
        // Fall through.
    default:
        return false;
    }
}

void BottomBar::resizeEvent(QResizeEvent *resizeEvent)
{
    if (resizeEvent->oldSize().width() == resizeEvent->size().width()) {
        // The width() didn't change so our custom override isn't needed.
        return QWidget::resizeEvent(resizeEvent);
    }

    m_contentsContainer->adaptToNewBarWidth(width());

    return QWidget::resizeEvent(resizeEvent);
}
