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

    /**
     * @param contents The kind of contents that should be contained instead.
     */
    void resetContents(BottomBar::Contents contents);
    inline BottomBar::Contents contents() const
    {
        return m_contents;
    };

    inline bool hasSomethingToShow() {
        return contents() != BottomBar::GeneralContents || m_internalContextMenu;
    }

    /**
     * Is called when the BottomBar resizes to let this ContentsContainer know that it should adapt its contents to the new width.
     * Adapting is done by showing or hiding labels or buttons.
     */
    void adaptToNewBarWidth(int newBarWidth);

public Q_SLOTS:
    /** Adapts the contents based on the selection in the related view. */
    void slotSelectionChanged(const KFileItemList &selection, const QUrl &baseUrl);

Q_SIGNALS:
    /**
     * Forwards the errors from the KFileItemAction::error() used for contextual actions.
     */
    void error(const QString &errorMessage);

    /**
     * When it does not make sense to show any specific contents, this signal is emitted and the receiver hides the bar.
     * Later it might sense to show it again e.g. because the user selected items. Then this signal is used to request showing of the bar.
     */
    void barVisibilityChangeRequested(bool visible);

    void selectionModeLeavingRequested();

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
     * Deletes every child layout and child widget of this container.
     */
    void emptyBarContents();

    /**
     * @returns A vector containing contextual actions for the given \a selectedItems in the \a baseUrl.
     * Cut, Copy, Rename and MoveToTrash are always added. Any further contextual actions depend on
     * \a selectedItems and \a baseUrl.
     * If there are no \a selectedItems, an empty vector is returned and m_internalContextMenu is deleted.
     * @param selectedItems The selected items for which contextual actions should be displayed.
     * @param baseUrl       Base URL of the viewport the contextual actions apply to.
     */
    std::vector<QAction *> contextActionsFor(const KFileItemList &selectedItems, const QUrl &baseUrl);

    /**
     * @returns the amount of pixels that can be spared to add more widgets. A negative value might
     * be returned which signifies that some widgets should be hidden or removed from this bar to
     * make sure that this BottomBarContentsContainer can fully fit on the BottomBar.
     */
    int unusedSpace() const;

    /**
     * The label isn't that important. This method hides it if there isn't enough room on the bar or
     * shows it if there is.
     */
    void updateExplanatoryLabelVisibility();

    /**
     * Changes the text and enabled state of the main action button based on the amount of currently
     * selected items and the state of the current m_mainAction.
     * The current main action depends on the current barContents.
     * @param selectedItems the currently selected fileItems.
     */
    void updateMainActionButton(const KFileItemList &selectedItems);

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
    /// The main layout of this ContentsContainer that all the buttons and labels are added to.
    QHBoxLayout *m_layout;

    /// Caches the totalBarWidth as set in adaptToNewWidth(newBarWidth). */
    int m_barWidth = 0;
    /// The info label used for some of the Contents. Is hidden for narrow widths.
    QPointer<QLabel> m_explanatoryLabel;
};

}

#endif // BOTTOMBARCONTENTSCONTAINER_H
