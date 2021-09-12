/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef SELECTIONMODEBOTTOMBAR_H
#define SELECTIONMODEBOTTOMBAR_H

#include "actionwithwidget.h"
#include "global.h"

#include <QAction>
#include <QPointer>
#include <QPropertyAnimation>
#include <QWidget>

#include <memory>

class DolphinContextMenu;
class KActionCollection;
class KFileItemActions;
class KFileItemList;
class QAbstractButton;
class QAction;
class QFontMetrics;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QResizeEvent;
class QToolButton;
class QUrl;

/**
 * A bar mainly used in selection mode that serves various purposes depending on what the user is currently trying to do.
 *
 * The Contents enum below gives a rough idea about the different states this bar might have.
 * The bar is notified of various changes that make changing or updating the content worthwhile.
 */
class SelectionModeBottomBar : public QWidget
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
     * Default constructor
     */
    explicit SelectionModeBottomBar(KActionCollection *actionCollection, QWidget *parent);

    /**
     * Plays a show or hide animation while changing visibility.
     * Therefore, if this method is used to hide this widget, the actual hiding will be postponed until the animation finished.
     * @see QWidget::setVisible()
     */
    void setVisible(bool visible, Animated animated);
    using QWidget::setVisible; // Makes sure that the setVisible() declaration above doesn't hide the one from QWidget.

    void resetContents(Contents contents);
    inline Contents contents() const
    {
        return m_contents;
    };

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
    void resizeEvent(QResizeEvent *resizeEvent) override;

private:
    void addCopyContents();
    void addCopyLocationContents();
    void addCopyToOtherViewContents();
    void addCutContents();
    void addDeleteContents();
    void addDuplicateContents();
    /**
     * Adds the actions of m_generalBarActions as buttons to the bar. An overflow menu button is
     * created to make sure any amount of actions can be accessed.
     */
    void addGeneralContents();
    void addMoveToOtherViewContents();
    void addMoveToTrashContents();
    void addPasteContents();
    void addRenameContents();

    /**
     * Deletes all visible widgets and layouts from the bar.
     */
    void emptyBarContents();

    /**
     * @returns A vector containing contextual actions for the given \a selection in the \a baseUrl.
     * Cut, Copy, Rename and MoveToTrash are always added. Any further contextual actions depend on
     * \a selection and \a baseUrl. \a selection and \a baseUrl can be empty/default constructed if
     * no item- or view-specific actions should be added aside from Cut, Copy, Rename, MoveToTrash.
     * @param selectedItems The selected items for which contextual actions should be displayed.
     * @param baseUrl       Base URL of the viewport the contextual actions apply to.
     */
    std::vector<QAction *> contextActionsFor(const KFileItemList &selectedItems, const QUrl &baseUrl);

    /**
     * @returns the amount of pixels that can be spared to add more widgets. A negative value might
     * be returned which signifies that some widgets should be hidden or removed from this bar to
     * make sure that this SelectionModeBottomBar won't stretch the width of its parent.
     */
    int unusedSpace() const;

    /**
     * The label isn't that important. This method hides it if there isn't enough room on the bar or
     * shows it if there is.
     */
    void updateExplanatoryLabelVisibility();

    /**
     * Changes the text and enabled state of the main action button
     * based on the amount of currently selected items and the state of the current m_mainAction.
     * The current main action depends on the current barContents.
     * @param selection the currently selected fileItems.
     */
    void updateMainActionButton(const KFileItemList &selection);

private:
    /// All the actions that should be available from this bar when in general mode.
    std::vector<ActionWithWidget> m_generalBarActions;
    /// The context menu used to retrieve all the actions that are relevant for the current selection.
    std::unique_ptr<DolphinContextMenu> m_internalContextMenu;
    /// An object that is necessary to keep around for m_internalContextMenu.
    KFileItemActions *m_fileItemActions = nullptr;

    /// @see updateMainActionButtonText
    ActionWithWidget m_mainAction = ActionWithWidget(nullptr);
    /// The button containing all the actions that don't currently fit into the bar.
    QPointer<QToolButton> m_overflowButton;
    /// The actionCollection from which the actions for this bar are retrieved.
    KActionCollection *m_actionCollection;
    /// Describes the current contents of the bar.
    Contents m_contents;
    /** The layout all the buttons and labels are added to.
     * Do not confuse this with layout() because we do have a QScrollView in between this widget and m_layout. */
    QHBoxLayout *m_layout;

    /// @see SelectionModeBottomBar::setVisible()
    QPointer<QPropertyAnimation> m_heightAnimation;

    /// The info label used for some of the BarContents. Is hidden for narrow widths.
    QPointer<QLabel> m_explanatoryLabel;
};

#endif // SELECTIONMODEBOTTOMBAR_H
