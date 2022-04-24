/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef BOTTOMBARCONTENTSCONTAINER_H
#define BOTTOMBARCONTENTSCONTAINER_H

#include "actionwithwidget.h"
#include "bottombar.h"

#include <QPointer>
#include <QToolButton>
#include <QWidget>

class DolphinContextMenu;
class KActionCollection;
class KFileItemActions;
class KFileItemList;
class QHBoxLayout;
class QLabel;
class QUrl;

namespace SelectionMode
{

/**
 * @brief An internal widget of BottomBar that controls the visible contents/widgets on it.
 *
 * This class should only be interacted with from the BottomBar class.
 * @see BottomBar
 */
class BottomBarContentsContainer : public QWidget
{
    Q_OBJECT

public:
    /**
     * @param actionCollection the collection where the actions for the contents are retrieved from
     * @param parent           the parent widget. Typically a ScrollView within the BottomBar
     */
    explicit BottomBarContentsContainer(KActionCollection *actionCollection, QWidget *parent);

    void resetContents(BottomBar::Contents contents);
    inline BottomBar::Contents contents() const
    {
        return m_contents;
    };

    inline bool hasSomethingToShow() {
        return contents() != BottomBar::GeneralContents || m_internalContextMenu;
    }

    void updateForNewWidth();

public Q_SLOTS:
    void slotSelectionChanged(const KFileItemList &selection, const QUrl &baseUrl);

Q_SIGNALS:
    /**
     * Forwards the errors from the KFileItemAction::error() used for contextual actions.
     */
    void error(const QString &errorMessage);

    /**
     * Sometimes the contents see no reason to be visible and request the bar to be hidden instead which emits this signal.
     * This can later change e.g. because the user selected items. Then this signal is used to request showing of the bar.
     */
    void barVisibilityChangeRequested(bool visible);

    void leaveSelectionModeRequested();

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
    BottomBar::Contents m_contents;
    /** The layout all the buttons and labels are added to.
     * Do not confuse this with layout() because we do have a QScrollView in between this widget and m_layout. */
    QHBoxLayout *m_layout;

    /// The info label used for some of the BarContents. Is hidden for narrow widths.
    QPointer<QLabel> m_explanatoryLabel;
};

}

#endif // BOTTOMBARCONTENTSCONTAINER_H
