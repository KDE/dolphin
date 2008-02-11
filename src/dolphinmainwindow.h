/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Stefan Monov <logixoul@gmail.com>               *
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
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

#ifndef DOLPHIN_MAINWINDOW_H
#define DOLPHIN_MAINWINDOW_H

#include "dolphinview.h"
#include "sidebarpage.h"

#include <config-nepomuk.h>

#include <kfileitemdelegate.h>
#include <konq_fileundomanager.h>
#include <ksortablelist.h>
#include <kxmlguiwindow.h>

#include <QtCore/QList>

class DolphinViewActionHandler;
class DolphinApplication;
class DolphinViewContainer;
class KNewMenu;
class KUrl;
class QSplitter;

/**
 * @short Main window for Dolphin.
 *
 * Handles the menus, toolbars and Dolphin views.
 */
class DolphinMainWindow: public KXmlGuiWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dolphin.MainWindow")
    Q_PROPERTY(int id READ getId SCRIPTABLE true)
    friend class DolphinApplication;

public:
    virtual ~DolphinMainWindow();

    /**
     * Returns the currently active view.
     * All menu actions are applied to this view. When
     * having a split view setup, the nonactive view
     * is usually shown in darker colors.
     */
    DolphinViewContainer* activeViewContainer() const;

    /**
     * Returns true, if the main window contains two instances
     * of a view container. The active view constainer can be
     * accessed by DolphinMainWindow::activeViewContainer().
     */
    bool isSplit() const;

    /**
     * If the main window contains two instances of a view container
     * (DolphinMainWindow::isSplit() returns true), then the
     * two views get toggled (the right view is on the left, the left
     * view on the right).
     */
    void toggleViews();

    /** Renames the item represented by \a oldUrl to \a newUrl. */
    void rename(const KUrl& oldUrl, const KUrl& newUrl);

    /**
     * Refreshes the views of the main window by recreating them dependent from
     * the given Dolphin settings.
     */
    void refreshViews();

    /**
     * Returns the 'Create New...' sub menu which also can be shared
     * with other menus (e. g. a context menu).
     */
    KNewMenu* newMenu() const;

    /**
     * Returns the 'Show Menubar' action which can be shared with
     * other menus (e. g. a context menu).
     */
    KAction* showMenuBarAction() const;

public slots:
    /**
     * Handles the dropping of URLs to the given
     * destination. This is only called by the TreeViewSidebarPage.
     */
    void dropUrls(const KUrl::List& urls,
                  const KUrl& destination);

    /**
     * Returns the main window ID used through DBus.
     */
    int getId() const;

    /**
     * Inform all affected dolphin components (sidebars, views) of an URL
     * change.
     */
    void changeUrl(const KUrl& url);

    /**
     * Inform all affected dolphin components that a selection change is
     * requested.
     */
    void changeSelection(const KFileItemList& selection);

    /** Stores all settings and quits Dolphin. */
    void quit();

signals:
    /**
     * Is send if the active view has been changed in
     * the split view mode.
     */
    void activeViewChanged();

    /**
     * Is sent if the selection of the currently active view has
     * been changed.
     */
    void selectionChanged(const KFileItemList& selection);

    /**
     * Is sent if the url of the currently active view has
     * been changed.
     */
    void urlChanged(const KUrl& url);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the sidebar.
     * If item is null, no item information request is pending.
     */
    void requestItemInfo(const KFileItem& item);

protected:
    /** @see QMainWindow::closeEvent() */
    virtual void closeEvent(QCloseEvent* event);

    /** @see KMainWindow::saveProperties() */
    virtual void saveProperties(KConfigGroup& group);

    /** @see KMainWindow::readProperties() */
    virtual void readProperties(const KConfigGroup& group);

private slots:
    void clearStatusBar();

    /** Updates the 'Create New...' sub menu. */
    void updateNewMenu();

    /**
     * Opens the properties window for the selected items of the
     * active view. The properties windows shows information
     * like name, size and permissions.
     */
    void properties();

    /**
     * Shows the error information from the places model
     * in the status bar.
     */
    void slotHandlePlacesError(const QString &message);

    /**
     * Updates the state of the 'Undo' menu action dependent
     * from the parameter \a available.
     */
    void slotUndoAvailable(bool available);

    /** Sets the text of the 'Undo' menu action to \a text. */
    void slotUndoTextChanged(const QString& text);

    /** Performs the current undo operation. */
    void undo();

    /**
     * Copies all selected items to the clipboard and marks
     * the items as cutted.
     */
    void cut();

    /** Copies all selected items to the clipboard. */
    void copy();

    /** Pastes the clipboard data to the active view. */
    void paste();

    /**
     * Updates the text of the paste action dependent from
     * the number of items which are in the clipboard.
     */
    void updatePasteAction();

    /** Selects all items from the active view. */
    void selectAll();

    /**
     * Inverts the selection of all items of the active view:
     * Selected items get nonselected and nonselected items get
     * selected.
     */
    void invertSelection();

    /** The current active view is switched to a new view mode. */
    void setViewMode(QAction *);

    /** The sorting of the current view should be done by the name. */
    void sortByName();

    /** The sorting of the current view should be done by the size. */
    void sortBySize();

    /** The sorting of the current view should be done by the date. */
    void sortByDate();

    /** The sorting of the current view should be done by the permissions. */
    void sortByPermissions();

    /** The sorting of the current view should be done by the owner. */
    void sortByOwner();

    /** The sorting of the current view should be done by the group. */
    void sortByGroup();

    /** The sorting of the current view should be done by the type. */
    void sortByType();

    /** The sorting of the current view should be done by the rating. */
    void sortByRating();

    /** The sorting of the current view should be done by tags. */
    void sortByTags();

    /**
     * Switches between one and two views:
     * If one view is visible, it will get split into two views.
     * If already two views are visible, the nonactivated view gets closed.
     */
    void toggleSplitView();

    /** Reloads the current active view. */
    void reloadView();

    /** Stops the loading process for the current active view. */
    void stopLoading();

    /**
     * Toggles between showing and hiding of the filter bar
     */
    void toggleFilterBarVisibility(bool show);

    /**
     * Toggles between edit and browse mode of the navigation bar.
     */
    void toggleEditLocation();

    /**
     * Switches to the edit mode of the navigation bar. If the edit mode is
     * already active, it is assured that the navigation bar get focused.
     */
    void editLocation();

    /**
     * Opens the view properties dialog, which allows to modify the properties
     * of the currently active view.
     */
    void adjustViewProperties();

    /** Goes back on step of the URL history. */
    void goBack();

    /** Goes forward one step of the URL history. */
    void goForward();

    /** Goes up one hierarchy of the current URL. */
    void goUp();

    /** Goes to the home URL. */
    void goHome();

    /** Opens KFind for the current shown directory. */
    void findFile();

    /** Opens Kompare for 2 selected files. */
    void compareFiles();

    /**
     * Hides the menu bar if it is visible, makes the menu bar
     * visible if it is hidden.
     */
    void toggleShowMenuBar();

    /** Opens the settings dialog for Dolphin. */
    void editSettings();

    /** Updates the state of all 'View' menu actions. */
    void slotViewModeChanged();

    /** Updates the state of the 'Sort by' actions. */
    void slotSortingChanged(DolphinView::Sorting sorting);

    /** Updates the state of the 'Show Full Location' action. */
    void slotEditableStateChanged(bool editable);

    /**
     * Updates the state of the 'Edit' menu actions and emits
     * the signal selectionChanged().
     */
    void slotSelectionChanged(const KFileItemList& selection);

    /** Emits the signal requestItemInfo(). */
    void slotRequestItemInfo(const KFileItem&);

    /**
     * Updates the state of the 'Back' and 'Forward' menu
     * actions corresponding the the current history.
     */
    void slotHistoryChanged();

    /** Updates the state of the 'Show filter bar' menu action. */
    void updateFilterBarAction(bool show);

    /** Open a new main window. */
    void openNewMainWindow();

    /** Toggles the active view if two views are shown within the main window. */
    void toggleActiveView();

    /** Called when the view is doing a file operation, like renaming, copying, moving etc. */
    void slotDoingOperation(KonqFileUndoManager::CommandType type);

private:
    DolphinMainWindow(int id);
    void init();

    /**
     * Activates the given view, which means that
     * all menu actions are applied to this view. When
     * having a split view setup, the nonactive view
     * is usually shown in darker colors.
     */
    void setActiveViewContainer(DolphinViewContainer* view);

    void setupActions();
    void setupDockWidgets();
    void updateHistory();
    void updateEditActions();
    void updateViewActions();
    void updateGoActions();

    /**
     * Connects the signals from the created DolphinView with
     * the index \a viewIndex with the corresponding slots of
     * the DolphinMainWindow. This method must be invoked each
     * time a DolphinView has been created.
     */
    void connectViewSignals(int viewIndex);

    /**
     * Updates the text of the split action:
     * If two views are shown, the text is set to "Split",
     * otherwise the text is set to "Join". The icon
     * is updated to match with the text and the currently active view.
     */
    void updateSplitAction();

private:
    /**
     * DolphinMainWindow supports up to two views beside each other.
     */
    enum ViewIndex
    {
        PrimaryView = 0,   ///< View shown on the left side of the main window.
        SecondaryView = 1  ///< View shown on the left side of the main window.
    };

    /**
     * Implements a custom error handling for the undo manager. This
     * assures that all errors are shown in the status bar of Dolphin
     * instead as modal error dialog with an OK button.
     */
    class UndoUiInterface : public KonqFileUndoManager::UiInterface
    {
    public:
        UndoUiInterface(DolphinMainWindow* mainWin);
        virtual ~UndoUiInterface();
        virtual void jobError(KIO::Job* job);

    private:
        DolphinMainWindow* m_mainWin;
    };

    KNewMenu* m_newMenu;
    KAction* m_showMenuBar;
    QSplitter* m_splitter;
    DolphinViewContainer* m_activeViewContainer;
    int m_id;

    DolphinViewContainer* m_viewContainer[SecondaryView + 1];

    DolphinViewActionHandler* m_actionHandler;

    /// remember pending undo operations until they are finished
    QList<KonqFileUndoManager::CommandType> m_undoCommandTypes;
};

inline DolphinViewContainer* DolphinMainWindow::activeViewContainer() const
{
    return m_activeViewContainer;
}

inline bool DolphinMainWindow::isSplit() const
{
    return m_viewContainer[SecondaryView] != 0;
}

inline KNewMenu* DolphinMainWindow::newMenu() const
{
    return m_newMenu;
}

inline KAction* DolphinMainWindow::showMenuBarAction() const
{
    return m_showMenuBar;
}

inline int DolphinMainWindow::getId() const
{
    return m_id;
}

#endif // DOLPHIN_MAINWINDOW_H

