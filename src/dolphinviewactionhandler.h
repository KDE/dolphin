/***************************************************************************
 *   Copyright (C) 2008 by David Faure <faure@kde.org>                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/


#ifndef DOLPHINVIEWACTIONHANDLER_H
#define DOLPHINVIEWACTIONHANDLER_H

#include "dolphinview.h"
#include "libdolphin_export.h"
#include <QtCore/QObject>
class KToggleAction;
class QAction;
class QActionGroup;
class DolphinView;
class KActionCollection;

/**
 * @short Handles all actions for DolphinView
 *
 * The action handler owns all the actions and slots related to DolphinView,
 * but can the view that is acts upon can be switched to another one
 * (this is used in the case of split views).
 *
 * The purpose of this class is also to share this code between DolphinMainWindow
 * and DolphinPart.
 *
 * @see DolphinView
 * @see DolphinMainWindow
 * @see DolphinPart
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinViewActionHandler : public QObject
{
    Q_OBJECT

public:
    explicit DolphinViewActionHandler(KActionCollection* collection, QObject* parent);

    /**
     * Sets the view that this action handler should work on.
     */
    void setCurrentView(DolphinView* view);

    /**
     * Returns the view that this action handler should work on.
     */
    DolphinView* currentView();

    /**
     * Returns the name of the action for the current viewmode
     */
    QString currentViewModeActionName() const;

    /**
     * Returns m_actionCollection
     */
    KActionCollection* actionCollection();

public Q_SLOTS:
    /**
     * Update all actions in the 'View' menu, i.e. those that depend on the
     * settings in the current view.
     */
    void updateViewActions();

Q_SIGNALS:
    /**
     * Emitted by DolphinViewActionHandler when the user triggered an action.
     * This is only used for clearining the statusbar in DolphinMainWindow.
     */
    void actionBeingHandled();

    /**
     * Emitted if the user requested creating a new directory by the F10 key.
     * The receiver of the signal (DolphinMainWindow or DolphinPart) invokes
     * the method createDirectory of their KNewMenu instance.
     */
    void createDirectory();

private Q_SLOTS:
    /**
     * Emitted when the user requested a change of view mode
     */
    void slotViewModeActionTriggered(QAction*);

    /**
     * Let the user input a name for the selected item(s) and trigger
     * a renaming afterwards.
     */
    void slotRename();

    /**
     * Moves the selected items of the active view to the trash.
     * This methods adds "shift means del" handling.
     */
    void slotTrashActivated(Qt::MouseButtons, Qt::KeyboardModifiers);

    /**
     * Deletes the selected items of the active view.
     */
    void slotDeleteItems();

    /**
     * Switches between showing a preview of the file content and showing the icon.
     */
    void togglePreview(bool);

    /** Updates the state of the 'Show preview' menu action. */
    void slotShowPreviewChanged();

    /** Increases the size of the current set view mode. */
    void zoomIn();

    /** Decreases the size of the current set view mode. */
    void zoomOut();

    /** Switches between an ascending and descending sorting order. */
    void toggleSortOrder();

    /** Switches between a separate sorting and a mixed sorting of files and folders. */
    void toggleSortFoldersFirst();

    /**
     * Updates the state of the 'Sort Ascending/Descending' action.
     */
    void slotSortOrderChanged(Qt::SortOrder order);

    /**
     * Updates the state of the 'Sort Folders First' action.
     */
    void slotSortFoldersFirstChanged(bool foldersFirst);

    /**
     * Updates the state of the 'Sort by' actions.
     */
    void slotSortingChanged(DolphinView::Sorting sorting);

    /**
     * Updates the state of the 'Zoom In' and 'Zoom Out' actions.
     */
    void slotZoomLevelChanged(int level);

    /**
     * Switches on or off the displaying of additional information
     * as specified by \a action.
     */
    void toggleAdditionalInfo(QAction* action);

    /**
     * Changes the sorting of the current view.
     */
    void slotSortTriggered(QAction*);

    /**
     * Updates the state of the 'Additional Information' actions.
     */
    void slotAdditionalInfoChanged();

    /**
     * Switches between sorting by categories or not.
     */
    void toggleSortCategorization(bool);

    /**
     * Updates the state of the 'Categorized sorting' menu action.
     */
    void slotCategorizedSortingChanged();

    /**
     * Switches between showing and hiding of hidden marked files
     */
    void toggleShowHiddenFiles(bool);

    /**
     * Updates the state of the 'Show hidden files' menu action.
     */
    void slotShowHiddenFilesChanged();

    /**
     * Opens the view properties dialog, which allows to modify the properties
     * of the currently active view.
     */
    void slotAdjustViewProperties();

    /**
     * Opens the Find File dialog for the currently shown directory.
     */
    void slotFindFile();

    /**
     * Connected to the "properties" action.
     * Opens the properties dialog for the selected items of the
     * active view. The properties dialog shows information
     * like name, size and permissions.
     */
    void slotProperties();
	
    /**
     * Starts KHotNewStuff to download servicemenus.
     */
    void slotGetServiceMenu();

private:
    /**
     * Create all the actions.
     * This is called only once (by the constructor)
     */
    void createActions();
    /**
     * Creates an action group with all the "show additional information" actions in it.
     * Helper method for createActions();
     */
    QActionGroup* createAdditionalInformationActionGroup();

    /**
     * Creates an action group with all the "sort by" actions in it.
     * Helper method for createActions();
     */
    QActionGroup* createSortByActionGroup();

    /**
     * Returns the "switch to icons mode" action.
     * Helper method for createActions();
     */
    KToggleAction* iconsModeAction();

    /**
     * Returns the "switch to details mode" action.
     * Helper method for createActions();
     */
    KToggleAction* detailsModeAction();

    /**
     * Returns the "switch to columns mode" action.
     * Helper method for createActions();
     */
    KToggleAction* columnsModeAction();

    KActionCollection* m_actionCollection;
    DolphinView* m_currentView;
};

#endif /* DOLPHINVIEWACTIONHANDLER_H */
