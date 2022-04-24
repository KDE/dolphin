/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef BOTTOMBAR_H
#define BOTTOMBAR_H

#include "global.h"

#include <QAction>
#include <QPointer>
#include <QPropertyAnimation>
#include <QWidget>

#include <memory>

class KActionCollection;
class KFileItemList;
class QAction;
class QPushButton;
class QResizeEvent;
class QScrollArea;
class QUrl;

namespace SelectionMode
{
    class BottomBarContentsContainer;

/**
 * A bar used in selection mode that serves various purposes depending on what the user is currently trying to do.
 *
 * The Contents enum below gives a rough idea about the different states this bar might have.
 * The bar is notified of various changes that make changing or updating the content worthwhile.
 *
 * The visible contents of the bar are managed in BottomBarContentsContainer. This class serves as a wrapper around it.
 */
class BottomBar : public QWidget
{
    Q_OBJECT

public:
    /** The different contents this bar can have. */
    enum Contents{
        CopyContents,
        CopyLocationContents,
        CopyToOtherViewContents,
        CutContents,
        DeleteContents,
        DuplicateContents,
        GeneralContents,
        MoveToOtherViewContents,
        MoveToTrashContents,
        PasteContents,
        RenameContents
    };

    /**
     * @param actionCollection the collection this bar retrieves its actions from
     * @param parent           the parent widget. Typically a DolphinViewContainer
     */
    explicit BottomBar(KActionCollection *actionCollection, QWidget *parent);

    /**
     * Plays a show or hide animation while changing visibility.
     * Therefore, if this method is used to hide this widget, the actual hiding will be postponed until the animation finished.
     *
     * This bar might also not show itself when setVisible(true), when context menu actions are supposed to be shown
     * for the selected items but no items have been selected yet. In that case it will only show itself once items were selected.
     * @see QWidget::setVisible()
     */
    void setVisible(bool visible, Animated animated);
    using QWidget::setVisible; // Makes sure that the setVisible() declaration above doesn't hide the one from QWidget.

    void resetContents(Contents contents);
    Contents contents() const;

    /** @returns a width of 1 to make sure that this bar never causes side panels to shrink. */
    QSize sizeHint() const override;

public Q_SLOTS:
    void slotSelectionChanged(const KFileItemList &selection, const QUrl &baseUrl);

    /** Used to notify the m_selectionModeBottomBar that there is no other ViewContainer in the tab. */
    void slotSplitTabDisabled();

Q_SIGNALS:
    /**
     * Forwards the errors from the KFileItemAction::error() used for contextual actions.
     */
    void error(const QString &errorMessage);

    void leaveSelectionModeRequested();

protected:
    /** Is installed on an internal widget to make sure that the height of the bar is adjusted to its contents. */
    bool eventFilter(QObject *watched, QEvent *event) override;

    /** Adapts the way the contents of this bar are displayed based on the available width. */
    void resizeEvent(QResizeEvent *resizeEvent) override;

private:
    /**
     * Identical to SelectionModeBottomBar::setVisible() but doesn't change m_allowedToBeVisible.
     * @see SelectionModeBottomBar::setVisible()
     * @see m_allowedToBeVisible
     */
    void setVisibleInternal(bool visible, Animated animated);

private:
    /** The only direct child widget of this bar. */
    QScrollArea *m_scrollArea;
    /** The only direct grandchild of this bar. */
    BottomBarContentsContainer *m_contentsContainer;

    /** Remembers if this bar was setVisible(true) or setVisible(false) the last time.
     * This is necessary because this bar might have been setVisible(true) but there is no reason to show the bar currently so it was kept hidden.
     * @see SelectionModeBottomBar::setVisible() */
    bool m_allowedToBeVisible = false;
    /// @see SelectionModeBottomBar::setVisible()
    QPointer<QPropertyAnimation> m_heightAnimation;
};

}

#endif // BOTTOMBAR_H
