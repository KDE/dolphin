/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "bottombar.h"

#include "backgroundcolorhelper.h"
#include "bottombarcontentscontainer.h"

#include <QGridLayout>
#include <QResizeEvent>
#include <QStyle>
#include <QTimer>

using namespace SelectionMode;

BottomBar::BottomBar(KActionCollection *actionCollection, QWidget *parent)
    : AnimatedHeightWidget{parent}
{
    m_contentsContainer = new BottomBarContentsContainer(actionCollection, nullptr);
    prepareContentsContainer(m_contentsContainer);
    m_contentsContainer->installEventFilter(this); // Adjusts the height of this bar to the height of the contentsContainer
    connect(m_contentsContainer, &BottomBarContentsContainer::error, this, &BottomBar::error);
    connect(m_contentsContainer, &BottomBarContentsContainer::barVisibilityChangeRequested, this, [this](bool visible) {
        if (!m_allowedToBeVisible && visible) {
            return;
        }
        setVisibleInternal(visible, WithAnimation);
    });
    connect(m_contentsContainer, &BottomBarContentsContainer::selectionModeLeavingRequested, this, &BottomBar::selectionModeLeavingRequested);

    setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
    BackgroundColorHelper::instance()->controlBackgroundColor(this);
}

void BottomBar::setVisible(bool visible, Animated animated)
{
    m_allowedToBeVisible = visible;
    setVisibleInternal(visible, animated);
}

void BottomBar::setVisibleInternal(bool visible, Animated animated)
{
    if (!visible && contents() == PasteContents) {
        return; // The bar with PasteContents should not be hidden or users might not know how to paste what they just copied.
                // Set contents to anything else to circumvent this prevention mechanism.
    }
    if (visible && !m_contentsContainer->hasSomethingToShow()) {
        return; // There is nothing on the bar that we want to show. We keep it invisible and only show it when the selection or the contents change.
    }

    AnimatedHeightWidget::setVisible(visible, animated);
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
            if (isVisibleTo(parentWidget()) && isEnabled() && !isAnimationRunning()) {
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
        return AnimatedHeightWidget::resizeEvent(resizeEvent);
    }

    m_contentsContainer->adaptToNewBarWidth(width());

    return AnimatedHeightWidget::resizeEvent(resizeEvent);
}

int BottomBar::preferredHeight() const
{
    return m_contentsContainer->sizeHint().height();
}

#include "moc_bottombar.cpp"
