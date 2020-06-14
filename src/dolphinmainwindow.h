/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Stefan Monov <logixoul@gmail.com>
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_MAINWINDOW_H
#define DOLPHIN_MAINWINDOW_H

#include "dolphintabwidget.h"
#include <config-baloo.h>
#include <kio/fileundomanager.h>
#include <KSortableList>
#include <kxmlguiwindow.h>

#include <QIcon>
#include <QList>
#include <QMenu>
#include <QPointer>
#include <QUrl>
#include <QVector>

typedef KIO::FileUndoManager::CommandType CommandType;

class DolphinBookmarkHandler;
class DolphinViewActionHandler;
class DolphinSettingsDialog;
class DolphinViewContainer;
class DolphinRemoteEncoding;
class DolphinTabWidget;
class KFileItem;
class KFileItemList;
class KJob;
class KNewFileMenu;
class KHelpMenu;
class KToolBarPopupAction;
class QToolButton;
class QIcon;
class PlacesPanel;
class TerminalPanel;

namespace KIO {
    class OpenUrlJob;
}

/**
 * @short Main window for Dolphin.
 *
 * Handles the menus, toolbars and Dolphin views.
 */
class DolphinMainWindow: public KXmlGuiWindow
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.dolphin.MainWindow")

public:
    DolphinMainWindow();
    ~DolphinMainWindow() override;

    /**
     * Returns the currently active view.
     * All menu actions are applied to this view. When
     * having a split view setup, the nonactive view
     * is usually shown in darker colors.
     */
    DolphinViewContainer* activeViewContainer() const;

    /**
     * Returns view containers for all tabs
     * @param includeInactive   When true all view containers available in
     *                          this window are returned. When false the
     *                          view containers of split views that are not
     *                          currently active are ignored.
     *                          Default is true.
     */
    QVector<DolphinViewContainer*> viewContainers(bool includeInactive = true) const;

    /**
     * Opens each directory in \p dirs in a separate tab. If \a splitView is set,
     * 2 directories are collected within one tab.
     * \pre \a dirs must contain at least one url.
     */
    void openDirectories(const QList<QUrl> &dirs, bool splitView);

    /**
     * Opens the directories which contain the files \p files and selects all files.
     * If \a splitView is set, 2 directories are collected within one tab.
     * \pre \a files must contain at least one url.
     */
    void openFiles(const QList<QUrl>& files, bool splitView);

    /**
     * Returns the 'Create New...' sub menu which also can be shared
     * with other menus (e. g. a context menu).
     */
    KNewFileMenu* newFileMenu() const;

    /**
     * Switch the window's view containers' locations to display the home path
     * for any which are currently displaying a location corresponding to or
     * within mountPath.
     *
     * This typically done after unmounting a disk at mountPath to ensure that
     * the window is not displaying an invalid location.
     */
    void setViewsToHomeIfMountPathOpen(const QString& mountPath);

    /**
     * Sets any of the window's view containers which are currently displaying
     * invalid locations to the home path
     */
    void setViewsWithInvalidPathsToHome();

    bool isFoldersPanelEnabled() const;
    bool isInformationPanelEnabled() const;

public slots:
    /**
     * Opens each directory in \p dirs in a separate tab. If \a splitView is set,
     * 2 directories are collected within one tab.
     * \pre \a dirs must contain at least one url.
     *
     * @note this function is overloaded so that it is callable via DBus.
     */
    void openDirectories(const QStringList &dirs, bool splitView);

    /**
     * Opens the directories which contain the files \p files and selects all files.
     * If \a splitView is set, 2 directories are collected within one tab.
     * \pre \a files must contain at least one url.
     *
     * @note this is overloaded so that this function is callable via DBus.
     */
    void openFiles(const QStringList &files, bool splitView);

    /**
     * Tries to raise/activate the Dolphin window.
     */
    void activateWindow();

    /**
     * Determines if a URL is open in any tab.
     * @note Use of QString instead of QUrl is required to be callable via DBus.
     *
     * @param url URL to look for
     * @returns true if url is currently open in a tab, false otherwise.
     */
    bool isUrlOpen(const QString &url);


    /**
     * Pastes the clipboard data into the currently selected folder
     * of the active view. If not exactly one folder is selected,
     * no pasting is done at all.
     */
    void pasteIntoFolder();

    /**
     * Implementation of the MainWindowAdaptor/QDBusAbstractAdaptor interface.
     * Inform all affected dolphin components (panels, views) of an URL
     * change.
     */
    void changeUrl(const QUrl& url);

    /**
     * The current directory of the Terminal Panel has changed, probably because
     * the user entered a 'cd' command. This slot calls changeUrl(url) and makes
     * sure that the panel keeps the keyboard focus.
     */
    void slotTerminalDirectoryChanged(const QUrl& url);

    /** Stores all settings and quits Dolphin. */
    void quit();

    /**
     * Opens a new tab and places it after the current tab
     */
    void openNewTabAfterCurrentTab(const QUrl& url);

    /**
     * Opens a new tab and places it as the last tab
     */
    void openNewTabAfterLastTab(const QUrl& url);

signals:
    /**
     * Is sent if the selection of the currently active view has
     * been changed.
     */
    void selectionChanged(const KFileItemList& selection);

    /**
     * Is sent if the url of the currently active view has
     * been changed.
     */
    void urlChanged(const QUrl& url);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the panel.
     * If item is null, no item information request is pending.
     */
    void requestItemInfo(const KFileItem& item);

    /**
     * Is emitted if the settings have been changed.
     */
    void settingsChanged();

protected:
    /** @see QWidget::showEvent() */
    void showEvent(QShowEvent* event) override;

    /** @see QMainWindow::closeEvent() */
    void closeEvent(QCloseEvent* event) override;

    /** @see KMainWindow::saveProperties() */
    void saveProperties(KConfigGroup& group) override;

    /** @see KMainWindow::readProperties() */
    void readProperties(const KConfigGroup& group) override;

    /** Handles QWhatsThisClickedEvent and passes all others on. */
    bool event(QEvent* event) override;
    /** Handles QWhatsThisClickedEvent and passes all others on. */
    bool eventFilter(QObject*, QEvent*) override;

private slots:
    /**
     * Refreshes the views of the main window by recreating them according to
     * the given Dolphin settings.
     */
    void refreshViews();

    void clearStatusBar();

    /** Updates the 'Create New...' sub menu. */
    void updateNewMenu();

    void createDirectory();

    /** Shows the error message in the status bar of the active view. */
    void showErrorMessage(const QString& message);

    /**
     * Updates the state of the 'Undo' menu action dependent
     * on the parameter \a available.
     */
    void slotUndoAvailable(bool available);

    /** Sets the text of the 'Undo' menu action to \a text. */
    void slotUndoTextChanged(const QString& text);

    /** Performs the current undo operation. */
    void undo();

    /**
     * Copies all selected items to the clipboard and marks
     * the items as cut.
     */
    void cut();

    /** Copies all selected items to the clipboard. */
    void copy();

    /** Pastes the clipboard data to the active view. */
    void paste();

    /** Replaces the URL navigator by a search box to find files. */
    void find();

    /** Updates the state of the search action according to the view container. */
    void updateSearchAction();

    /**
     * Updates the text of the paste action dependent on
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

    /**
     * Switches between one and two views:
     * If one view is visible, it will get split into two views.
     * If already two views are visible, the active view gets closed.
     */
    void toggleSplitView();

    /** Dedicated action to open the stash:/ ioslave in split view. */
    void toggleSplitStash();

    /** Reloads the currently active view. */
    void reloadView();

    /** Stops the loading process for the currently active view. */
    void stopLoading();

    void enableStopAction();
    void disableStopAction();

    void showFilterBar();

    /**
     * Toggle between either using an UrlNavigator in the toolbar or the
     * ones in the location bar for navigating.
     */
    void toggleLocationInToolbar();

    /**
     * Toggles between edit and browse mode of the navigation bar.
     */
    void toggleEditLocation();

    /**
     * Switches to the edit mode of the navigation bar and selects
     * the whole URL, so that it can be replaced by the user. If the edit mode is
     * already active, it is assured that the navigation bar get focused.
     */
    void replaceLocation();

    /**
     * Toggles the state of the panels between a locked and unlocked layout.
     */
    void togglePanelLockState();

    /**
     * Is invoked if the Terminal panel got visible/invisible and takes care
     * that the active view has the focus if the Terminal panel is invisible.
     */
    void slotTerminalPanelVisibilityChanged();

    /** Goes back one step of the URL history. */
    void goBack();

    /** Goes forward one step of the URL history. */
    void goForward();

    /** Goes up one hierarchy of the current URL. */
    void goUp();

    /** Changes the location to the home URL. */
    void goHome();

    /** Open the previous URL in the URL history in a new tab. */
    void goBackInNewTab();

    /** Open the next URL in the URL history in a new tab. */
    void goForwardInNewTab();

    /** Open the URL one hierarchy above the current URL in a new tab. */
    void goUpInNewTab();

    /** * Open the home URL in a new tab. */
    void goHomeInNewTab();

    /** Opens Kompare for 2 selected files. */
    void compareFiles();

    /**
     * Hides the menu bar if it is visible, makes the menu bar
     * visible if it is hidden.
     */
    void toggleShowMenuBar();

    /** Updates "Open Preferred Search Tool" action. */
    void updateOpenPreferredSearchToolAction();

    /** Opens preferred search tool for the current location. */
    void openPreferredSearchTool();

    /** Opens a terminal window for the current location. */
    void openTerminal();

    /** Focus a Terminal Panel. */
    void focusTerminalPanel();

    /** Opens the settings dialog for Dolphin. */
    void editSettings();

    /** Updates the state of the 'Show Full Location' action. */
    void slotEditableStateChanged(bool editable);

    /**
     * Updates the state of the 'Edit' menu actions and emits
     * the signal selectionChanged().
     */
    void slotSelectionChanged(const KFileItemList& selection);

    /**
     * Updates the state of the 'Back' and 'Forward' menu
     * actions corresponding to the current history.
     */
    void updateHistory();

    /** Updates the state of the 'Show filter bar' menu action. */
    void updateFilterBarAction(bool show);

    /** Open a new main window. */
    void openNewMainWindow();

    /**
     * Opens a new view with the current URL that is part of a tab and
     * activates it.
     */
    void openNewActivatedTab();

    /**
     * Adds the current URL as an entry to the Places panel
     */
    void addToPlaces();

    /**
     * Opens a new tab in the background showing the URL \a url.
     */
    void openNewTab(const QUrl& url, DolphinTabWidget::TabPlacement tabPlacement);

    /**
     * Opens the selected folder in a new tab.
     */
    void openInNewTab();

    /**
     * Opens the selected folder in a new window.
     */
    void openInNewWindow();

    /**
     * Show the target of the selected symlink
     */
    void showTarget();

    /**
     * Indicates in the statusbar that the execution of the command \a command
     * has been finished.
     */
    void showCommand(CommandType command);

    /**
     * If the URL can be listed, open it in the current view, otherwise
     * run it through KRun.
     */
    void handleUrl(const QUrl& url);

    /**
     * Is invoked when the write state of a folder has been changed and
     * enables/disables the "Create New..." menu entry.
     */
    void slotWriteStateChanged(bool isFolderWritable);

    /**
     * Opens the context menu on the current mouse position.
     * @pos           Position in screen coordinates.
     * @item          File item context. If item is null, the context menu
     *                should be applied to \a url.
     * @url           URL which contains \a item.
     * @customActions Actions that should be added to the context menu,
     *                if the file item is null.
     */
    void openContextMenu(const QPoint& pos,
                         const KFileItem& item,
                         const QUrl& url,
                         const QList<QAction*>& customActions);

    void updateControlMenu();
    void updateToolBar();
    void slotControlButtonDeleted();

    /**
     * Is called if the user clicked an item in the Places Panel.
     * Reloads the view if \a url is the current URL already, and changes the
     * current URL otherwise.
     */
    void slotPlaceActivated(const QUrl& url);

    /**
     * Is called if the another view has been activated by changing the current
     * tab or activating another view in split-view mode.
     *
     * Activates the given view, which means that all menu actions are applied
     * to this view. When having a split view setup, the nonactive view is
     * usually shown in darker colors.
     */
    void activeViewChanged(DolphinViewContainer* viewContainer);

    void closedTabsCountChanged(unsigned int count);

    /**
     * Is called if a new tab has been opened or a tab has been closed to
     * enable/disable the tab actions.
     */
    void tabCountChanged(int count);

    /**
     * Updates the Window Title with the caption from the active view container
     */
    void updateWindowTitle();

    /**
     * This slot is called when the user requested to unmount a removable media
     * from the places menu
     */
    void slotStorageTearDownFromPlacesRequested(const QString& mountPath);

    /**
     * This slot is called when the user requested to unmount a removable media
     * _not_ from the dolphin's places menu (from the notification area for e.g.)
     * This slot is basically connected to each removable device's
     * Solid::StorageAccess::teardownRequested(const QString & udi)
     * signal through the places panel.
     */
    void slotStorageTearDownExternallyRequested(const QString& mountPath);

    /**
     * Is called when the view has finished loading the directory.
     */
    void slotDirectoryLoadingCompleted();

    /**
     * Is called when the user middle clicks a toolbar button.
     *
     * Here middle clicking Back/Forward/Up/Home will open the resulting
     * folder in a new tab.
     */
    void slotToolBarActionMiddleClicked(QAction *action);

    /**
     * Is called before the Back popup menu is shown. This slot will populate
     * the menu with history data
     */
    void slotAboutToShowBackPopupMenu();

    /**
      * This slot is used by the Back Popup Menu to go back to a specific
      * history index. The QAction::data will carry an int with the index
      * to go to.
      */
    void slotGoBack(QAction* action);

    /**
     * Middle clicking Back/Forward will open the resulting folder in a new tab.
     */
    void slotBackForwardActionMiddleClicked(QAction *action);

    /**
     * Is called before the Forward popup menu is shown. This slot will populate
     * the menu with history data
     */
    void slotAboutToShowForwardPopupMenu();

    /**
      * This slot is used by the Forward Popup Menu to go forward to a specific
      * history index. The QAction::data will carry an int with the index
      * to go to.
      */
    void slotGoForward(QAction* action);
private:
    /**
     * Sets up the various menus and actions and connects them.
     */
    void setupActions();

    /**
     * Sets up the dock widgets and their panels.
     */
    void setupDockWidgets();

    void updateFileAndEditActions();
    void updateViewActions();
    void updateGoActions();

    void createControlButton();
    void deleteControlButton();

    /**
     * Adds the action \p action to the menu \p menu in
     * case if it has not added already to the toolbar.
     * @return True if the action has been added to the menu.
     */
    bool addActionToMenu(QAction* action, QMenu* menu);

    /**
     * Connects the signals from the created DolphinView with
     * the DolphinViewContainer \a container with the corresponding slots of
     * the DolphinMainWindow. This method must be invoked each
     * time a DolphinView has been created.
     */
    void connectViewSignals(DolphinViewContainer* container);

    /**
     * Updates the text of the split action:
     * If two views are shown, the text is set to "Split",
     * otherwise the text is set to "Join". The icon
     * is updated to match with the text and the currently active view.
     */
    void updateSplitAction();

    bool isKompareInstalled() const;

    /**
     * Creates an action for showing/hiding a panel, that is accessible
     * in "Configure toolbars..." and "Configure shortcuts...". This is necessary
     * as the action for toggling the dock visibility is done by Qt which
     * is no KAction instance.
     */
    void createPanelAction(const QIcon &icon,
                           const QKeySequence& shortcut,
                           QAction* dockAction,
                           const QString& actionName);

    /** Adds "What's This?" texts to many widgets and StandardActions. */
    void setupWhatsThis();

    /** Returns preferred search tool as configured in "More Search Tools" menu. */
    QPointer<QAction> preferredSearchTool();

private:
    /**
     * Implements a custom error handling for the undo manager. This
     * assures that all errors are shown in the status bar of Dolphin
     * instead as modal error dialog with an OK button.
     */
    class UndoUiInterface : public KIO::FileUndoManager::UiInterface
    {
    public:
        UndoUiInterface();
        ~UndoUiInterface() override;
        void jobError(KIO::Job* job) override;
    };

    KNewFileMenu* m_newFileMenu;
    KHelpMenu* m_helpMenu;
    DolphinTabWidget* m_tabWidget;
    DolphinViewContainer* m_activeViewContainer;

    DolphinViewActionHandler* m_actionHandler;
    DolphinRemoteEncoding* m_remoteEncoding;
    QPointer<DolphinSettingsDialog> m_settingsDialog;
    DolphinBookmarkHandler* m_bookmarkHandler;

    // Members for the toolbar menu that is shown when the menubar is hidden:
    QToolButton* m_controlButton;
    QTimer* m_updateToolBarTimer;

    KIO::OpenUrlJob *m_lastHandleUrlOpenJob;

    TerminalPanel* m_terminalPanel;
    PlacesPanel* m_placesPanel;
    bool m_tearDownFromPlacesRequested;

    KToolBarPopupAction* m_backAction;
    KToolBarPopupAction* m_forwardAction;

    QMenu m_searchTools;

};

inline DolphinViewContainer* DolphinMainWindow::activeViewContainer() const
{
    return m_activeViewContainer;
}

inline KNewFileMenu* DolphinMainWindow::newFileMenu() const
{
    return m_newFileMenu;
}

#endif // DOLPHIN_MAINWINDOW_H

