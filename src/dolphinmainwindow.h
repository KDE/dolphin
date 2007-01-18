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

#ifndef _DOLPHIN_MAINWINDOW_H_
#define _DOLPHIN_MAINWINDOW_H_

#include "dolphinview.h"

#include <kmainwindow.h>
#include <ksortablelist.h>
#include <konq_operations.h>

#include <QList>

class KPrinter;
class KUrl;
class QLineEdit;
class KFileIconView;
class KHBox;
class Q3IconViewItem;
class QSplitter;
class KAction;
class UrlNavigator;
class DolphinApplication;

/**
 * @short Main window for Dolphin.
 *
 * Handles the menus, toolbars and Dolphin views.
 *
 * @author Peter Penz <peter.penz@gmx.at>
*/
class DolphinMainWindow: public KMainWindow
{
    Q_OBJECT
    friend class DolphinApplication;
public:
    virtual ~DolphinMainWindow();

	/**
     * Activates the given view, which means that
     * all menu actions are applied to this view. When
     * having a split view setup the nonactive view
     * is usually shown in darker colors.
     */
    void setActiveView(DolphinView* view);

    /**
     * Returns the currently active view. See
     * DolphinMainWindow::setActiveView() for more details.
     */
    DolphinView* activeView() const { return m_activeView; }

    /**
     * Handles the dropping of Urls to the given
     * destination. A context menu with the options
     * 'Move Here', 'Copy Here', 'Link Here' and
     * 'Cancel' is offered to the user.
     * @param urls        List of Urls which have been
     *                    dropped.
     * @param destination Destination Url, where the
     *                    list or Urls should be moved,
     *                    copied or linked to.
     */
    void dropUrls(const KUrl::List& urls,
                  const KUrl& destination);

    /**
     * Returns the list of actions which are part of the file group
     * of the 'Create New...' sub menu. Usually the list contains actions
     * for creating folders, text files, HTML files etc.
     */
    QLinkedList<QAction*> fileGroupActions() const { return m_fileGroupActions; }

    /**
     * Refreshs the views of the main window by recreating them dependent from
     * the given Dolphin settings.
     */
    void refreshViews();

signals:
    /**
     * Is send if the active view has been changed in
     * the split view mode.
     */
    void activeViewChanged();

    /**
     * Is send if the selection of the currently active view has
     * been changed.
     */
    void selectionChanged();

protected:
    /** @see QMainWindow::closeEvent */
    virtual void closeEvent(QCloseEvent* event);

    /**
     * This method is called when it is time for the app to save its
     * properties for session management purposes.
     */
    void saveProperties(KConfig*);

    /**
     * This method is called when this app is restored.  The KConfig
     * object points to the session management config file that was saved
     * with @ref saveProperties
     */
    void readProperties(KConfig*);

private slots:
    /** Opens an input dialog for creating a new folder. */
    void createFolder();

    /** Creates a file with the MIME type given by the sender. */
    void createFile();

    /** Renames the selected item of the active view. */
    void rename();

    /** Moves the selected items of the active view to the trash. */
    void moveToTrash();

    /** Deletes the selected items of the active view. */
    void deleteItems();

    /**
     * Opens the properties window for the selected items of the
     * active view. The properties windows shows informations
     * like name, size and permissions.
     */
    void properties();

    /** Stores all settings and quits Dolphin. */
    void quit();

    /**
     * Shows the error information of the job \a job
     * in the status bar.
     */
    void slotHandleJobError(KJob* job);

    /**
     * Indicates in the status bar that the delete operation
     * of the job \a job has been finished.
     */
    void slotDeleteFileFinished(KJob* job);

    /**
     * Updates the state of the 'Undo' menu action dependent
     * from the parameter \a available.
     */
    void slotUndoAvailable(bool available);

    /** Sets the text of the 'Undo' menu action to \a text. */
    void slotUndoTextChanged(const QString& text);

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

    /** The current active view is switched to the icons mode. */
    void setIconsView();

    /** The current active view is switched to the details mode. */
    void setDetailsView();

    /** The sorting of the current view should be done by the name. */
    void sortByName();

    /** The sorting of the current view should be done by the size. */
    void sortBySize();

    /** The sorting of the current view should be done by the date. */
    void sortByDate();

    /** Switches between an ascending and descending sorting order. */
    void toggleSortOrder();

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

    /** Switches between showing a preview of the file content and showing the icon. */
    void togglePreview();

    /**
     * Switches between showing and hiding of hidden marked files dependent
     * from the current state of the 'Show Hidden Files' menu toggle action.
     */
    void toggleShowHiddenFiles();

    /**
     * Switches between showing and hiding of the filter bar dependent
     * from the current state of the 'Show Filter Bar' menu toggle action.
     */
    void showFilterBar();

    /** Increases the size of the current set view mode. */
    void zoomIn();

    /** Decreases the size of the current set view mode. */
    void zoomOut();

    /**
     * Toggles between edit and brose mode of the navigation bar.
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

    /** Goes back on step of the Url history. */
    void goBack();

    /** Goes forward one step of the Url history. */
    void goForward();

    /** Goes up one hierarchy of the current Url. */
    void goUp();

    /** Goes to the home Url. */
    void goHome();

    /** Opens a terminal for the current shown directory. */
    void openTerminal();

    /** Opens KFind for the current shown directory. */
    void findFile();

    /** Opens Kompare for 2 selected files. */
    void compareFiles();

    /** Opens the settings dialog for Dolphin. */
    void editSettings();

    /** Updates the state of all 'View' menu actions. */
    void slotViewModeChanged();

    /** Updates the state of the 'Show hidden files' menu action. */
    void slotShowHiddenFilesChanged();

    /** Updates the state of the 'Sort by' actions. */
    void slotSortingChanged(DolphinView::Sorting sorting);

    /** Updates the state of the 'Sort Ascending/Descending' action. */
    void slotSortOrderChanged(Qt::SortOrder order);

    /** Updates the state of the 'Edit' menu actions. */
    void slotSelectionChanged();

    /**
     * Updates the state of the 'Back' and 'Forward' menu
     * actions corresponding the the current history.
     */
    void slotHistoryChanged();

    /**
     * Updates the caption of the main window and the state
     * of all menu actions which depend from a changed Url.
     */
    void slotUrlChanged(const KUrl& url);

    /** Updates the state of the 'Show filter bar' menu action. */
    void updateFilterBarAction(bool show);

    /** Open a new main window. */
    void openNewMainWindow();

    /**
     * Moves the items indicated by m_droppedUrls to the URL
     * m_destination.
     */
    void moveDroppedItems();

    /**
     * Copies the items indicated by m_droppedUrls to the URL
     * m_destination.
     */
    void copyDroppedItems();

    /**
     * Links the items indicated by m_droppedUrls to the URL
     * m_destination.
     */
    void linkDroppedItems();

private:
    DolphinMainWindow();
    void init();
    void loadSettings();

    void setupAccel();
    void setupActions();
    void setupDockWidgets();
    void setupCreateNewMenuActions();
    void updateHistory();
    void updateEditActions();
    void updateViewActions();
    void updateGoActions();
    void copyUrls(const KUrl::List& source, const KUrl& dest);
    void moveUrls(const KUrl::List& source, const KUrl& dest);
    void clearStatusBar();

    /**
     * Connects the signals from the created DolphinView with
     * the index \a viewIndex with the corresponding slots of
     * the DolphinMainWindow. This method must be invoked each
     * time a DolphinView has been created.
     */
    void connectViewSignals(int viewIndex);

private:
    QSplitter* m_splitter;
    DolphinView* m_activeView;

    KUrl m_dropDestination;
    KUrl::List m_droppedUrls;

    /**
     * DolphinMainWindowsupports only one or two views, which
     * are handled internally as primary and secondary view.
     */
    enum ViewIndex
    {
        PrimaryIdx = 0,
        SecondaryIdx = 1
    };
    DolphinView* m_view[SecondaryIdx + 1];

    /// remember pending undo operations until they are finished
    QList<KonqOperations::Operation> m_undoOperations;

    /** Contains meta information for creating files. */
    struct CreateFileEntry
    {
        QString name;
        QString filePath;
        QString templatePath;
        QString icon;
        QString comment;
    };

    QLinkedList<QAction*> m_fileGroupActions;
    KSortableList<CreateFileEntry,QString> m_createFileTemplates;
};

#endif // _DOLPHIN_H_

