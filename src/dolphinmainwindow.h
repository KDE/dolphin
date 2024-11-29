/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2006 Stefan Monov <logixoul@gmail.com>
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_MAINWINDOW_H
#define DOLPHIN_MAINWINDOW_H

#include "config-dolphin.h"
#include "disabledactionnotifier.h"
#include "dolphintabwidget.h"
#include "selectionmode/bottombar.h"
#include <KActionMenu>
#include <KFileItemActions>
#include <kio/fileundomanager.h>
#include <kxmlguiwindow.h>

#if HAVE_BALOO
#include "panels/information/informationpanel.h"
#endif

#include <QFutureWatcher>
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
class KRecentFilesAction;
class KToolBarPopupAction;
class QToolButton;
class PlacesPanel;
class TerminalPanel;

/** Used to identify that a custom command should be triggered on a view background double-click.*/
constexpr QLatin1String customCommand{"CUSTOM_COMMAND"};

namespace KIO
{
class OpenUrlJob;
class CommandLauncherJob;
}
namespace SelectionMode
{
class ActionTextHelper;
}

/**
 * @short Main window for Dolphin.
 *
 * Handles the menus, toolbars and Dolphin views.
 */
class DolphinMainWindow : public KXmlGuiWindow
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
    DolphinViewContainer *activeViewContainer() const;

    /**
     * Returns view container for all tabs
     */
    QVector<DolphinViewContainer *> viewContainers() const;

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
    void openFiles(const QList<QUrl> &files, bool splitView);

    /**
     * Returns the 'Create New...' sub menu which also can be shared
     * with other menus (e. g. a context menu).
     */
    KNewFileMenu *newFileMenu() const;

    /**
     * Augments Qt's build-in QMainWindow context menu to add
     * Dolphin-specific actions, such as "unlock panels".
     */
    QMenu *createPopupMenu() override;

    /**
     * Switch the window's view containers' locations to display the home path
     * for any which are currently displaying a location corresponding to or
     * within mountPath.
     *
     * This typically done after unmounting a disk at mountPath to ensure that
     * the window is not displaying an invalid location.
     */
    void setViewsToHomeIfMountPathOpen(const QString &mountPath);

    /**
     * Enables or disables the session autosaving feature.
     *
     * @param enable If true, saves the session automatically after a fixed
     *               time interval from the last state change.
     */
    void setSessionAutoSaveEnabled(bool enable);

    bool isFoldersPanelEnabled() const;
    bool isInformationPanelEnabled() const;
    bool isSplitViewEnabledInCurrentTab() const;

    /**
     * Activates a user set action when double clicking the view's background.
     */
    void slotDoubleClickViewBackground(Qt::MouseButton button);

public Q_SLOTS:
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
    void activateWindow(const QString &activationToken);

    bool isActiveWindow();

    /**
     * Determines if a URL is open in any tab.
     * @note Use of QString instead of QUrl is required to be callable via DBus.
     *
     * @param url URL to look for
     * @returns true if url is currently open in a tab, false otherwise.
     */
    bool isUrlOpen(const QString &url);

    /**
     * @return Whether the item with @p url can be found in any view only by switching
     * between already open tabs and scrolling in their primary or secondary view.
     * @note Use of QString instead of QUrl is required to be callable via DBus.
     */
    bool isItemVisibleInAnyView(const QString &urlOfItem);

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
    void changeUrl(const QUrl &url);

    /**
     * The current directory of the Terminal Panel has changed, probably because
     * the user entered a 'cd' command. This slot calls changeUrl(url) and makes
     * sure that the panel keeps the keyboard focus.
     */
    void slotTerminalDirectoryChanged(const QUrl &url);

    /** Stores all settings and quits Dolphin. */
    void quit();

    /**
     * Opens a new tab in the background showing the URL \a url.
     * @return A pointer to the opened DolphinTabPage.
     */
    DolphinTabPage *openNewTab(const QUrl &url);

    /**
     * Opens a new tab  showing the URL \a url and activate it.
     */
    void openNewTabAndActivate(const QUrl &url);

    /**
     * Opens a new window showing the URL \a url.
     */
    void openNewWindow(const QUrl &url);

    /** @see GeneralSettings::splitViewChanged() */
    void slotSplitViewChanged();

Q_SIGNALS:
    /**
     * Is sent if the selection of the currently active view has
     * been changed.
     */
    void selectionChanged(const KFileItemList &selection);

    /**
     * Is sent if the url of the currently active view has
     * been changed.
     */
    void urlChanged(const QUrl &url);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the panel.
     * If item is null, no item information request is pending.
     */
    void requestItemInfo(const KFileItem &item);

    /**
     * It is emitted when in the current view, files are changed,
     * or dirs have files/removed from them.
     */
    void fileItemsChanged(const KFileItemList &changedFileItems);

    /**
     * Is emitted if the settings have been changed.
     */
    void settingsChanged();

protected:
    /** @see QObject::event() */
    bool event(QEvent *event) override;

    /** @see QWidget::showEvent() */
    void showEvent(QShowEvent *event) override;

    /** @see QMainWindow::closeEvent() */
    void closeEvent(QCloseEvent *event) override;

    /** @see KMainWindow::saveProperties() */
    void saveProperties(KConfigGroup &group) override;

    /** @see KMainWindow::readProperties() */
    void readProperties(const KConfigGroup &group) override;

    /** Sets a sane initial window size **/
    QSize sizeHint() const override;

protected Q_SLOTS:
    /**
     * Calls the base method KXmlGuiWindow::saveNewToolbarConfig().
     * Is also used to set toolbar constraints and UrlNavigator position
     * based on the newly changed toolbar configuration.
     */
    void saveNewToolbarConfig() override;

private Q_SLOTS:
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
    void showErrorMessage(const QString &message);

    /**
     * Updates the state of the 'Undo' menu action dependent
     * on the parameter \a available.
     */
    void slotUndoAvailable(bool available);

    /** Sets the text of the 'Undo' menu action to \a text. */
    void slotUndoTextChanged(const QString &text);

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

    /** Calls DolphinViewContainer::setSelectionMode() for m_activeViewContainer. */
    void slotSetSelectionMode(bool enabled, SelectionMode::BottomBar::Contents bottomBarContents);

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

    /**
     * Pops out a split view.
     * The active view will be popped out, unless the view is not split,
     * in which case nothing will happen.
     */
    void popoutSplitView();

    /** Dedicated action to open the stash:/ ioslave in split view. */
    void toggleSplitStash();

    /** Copies all selected items to the inactive view. */
    void copyToInactiveSplitView();

    /** Moves all selected items to the inactive view. */
    void moveToInactiveSplitView();

    /** Reloads the currently active view. */
    void reloadView();

    /** Stops the loading process for the currently active view. */
    void stopLoading();

    void enableStopAction();
    void disableStopAction();

    void toggleSelectionMode();

    void showFilterBar();
    void toggleFilterBar();

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
     * Is invoked whenever the Terminal panel visibility is changed by the user and then moves the focus
     * to the active view if the panel was hidden.
     * @note The opposite action (putting focus to the Terminal) is not handled
     *       here but in TerminalPanel::showEvent().
     * @param visible the new visibility state of the terminal panel
     */
    void slotTerminalPanelVisibilityChanged(bool visible);

    /**
     * Is invoked whenever the Places panel visibility is changed by the user and then either moves the focus
     * - to the Places panel if it was made visible, or
     * - to the active view if the panel was hidden.
     * @param visible the new visibility state of the Places panel
     */
    void slotPlacesPanelVisibilityChanged(bool visible);

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

    /** Opens terminal windows for the selected items' locations. */
    void openTerminalHere();

    /** Opens a terminal window for the URL. */
    void openTerminalJob(const QUrl &url);

    /** Toggles focus to/from a Terminal Panel. */
    void toggleTerminalPanelFocus();

    /** Toggles focus to/from the Places Panel. */
    void togglePlacesPanelFocus();

    /** Opens the settings dialog for Dolphin. */
    void editSettings();

    /** Updates the state of the 'Show Full Location' action. */
    void slotEditableStateChanged(bool editable);

    /**
     * Updates the state of the 'Edit' menu actions and emits
     * the signal selectionChanged().
     */
    void slotSelectionChanged(const KFileItemList &selection);

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
     * Opens the selected folder in a new tab.
     */
    void openInNewTab();

    /**
     * Opens the selected folder in a new window.
     */
    void openInNewWindow();

    /**
     * Opens the selected folder in the other inactive split view, enables split view if necessary.
     */
    void openInSplitView(const QUrl &url);

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
    void handleUrl(const QUrl &url);

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
     * @selectedItems The selected items for which the context menu
     *                is opened. This list generally includes \a item.
     * @url           URL which contains \a item.
     */
    void openContextMenu(const QPoint &pos, const KFileItem &item, const KFileItemList &selectedItems, const QUrl &url);

    /**
     * Updates the menu that is by default at the right end of the toolbar.
     *
     * In true "simple by default" fashion, the menu only contains the most important
     * and necessary actions to be able to use Dolphin. This is supposed to hold true even
     * if the user does not know how to open a context menu. More advanced actions can be
     * discovered through a sub-menu (@see KConfigWidgets::KHamburgerMenu::setMenuBarAdvertised()).
     */
    void updateHamburgerMenu();

    /**
     * Is called if the user clicked an item in the Places Panel.
     * Reloads the view if \a url is the current URL already, and changes the
     * current URL otherwise.
     */
    void slotPlaceActivated(const QUrl &url);

    /**
     * Is called if the another view has been activated by changing the current
     * tab or activating another view in split-view mode.
     *
     * Activates the given view, which means that all menu actions are applied
     * to this view. When having a split view setup, the nonactive view is
     * usually shown in darker colors.
     */
    void activeViewChanged(DolphinViewContainer *viewContainer);

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
    void slotStorageTearDownFromPlacesRequested(const QString &mountPath);

    /**
     * This slot is called when the user requested to unmount a removable media
     * _not_ from the dolphin's places menu (from the notification area for e.g.)
     * This slot is basically connected to each removable device's
     * Solid::StorageAccess::teardownRequested(const QString & udi)
     * signal through the places panel.
     */
    void slotStorageTearDownExternallyRequested(const QString &mountPath);

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
    void slotGoBack(QAction *action);

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
    void slotGoForward(QAction *action);

    /**
     * Is called when configuring the keyboard shortcuts
     */
    void slotKeyBindings();

    /**
     * Saves the session.
     */
    void slotSaveSession();

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

    /**
     * Connects the signals from the created DolphinView with
     * the DolphinViewContainer \a container with the corresponding slots of
     * the DolphinMainWindow. This method must be invoked each
     * time a DolphinView has been created.
     */
    void connectViewSignals(DolphinViewContainer *container);

    /**
     * Updates the text of the split action:
     * If two views are shown, the text is set to "Split",
     * otherwise the text is set to "Join". The icon
     * is updated to match with the text and the currently active view.
     */
    void updateSplitActions();

    /**
     * Sets the window sides the toolbar may be moved to based on toolbar contents.
     */
    void updateAllowedToolbarAreas();

    bool isKompareInstalled() const;

    /**
     * Creates an action for showing/hiding a panel, that is accessible
     * in "Configure toolbars..." and "Configure shortcuts...".
     */
    void createPanelAction(const QIcon &icon, const QKeySequence &shortcut, QDockWidget *dockAction, const QString &actionName);

    /** Adds "What's This?" texts to many widgets and StandardActions. */
    void setupWhatsThis();

    /** Returns preferred search tool as configured in "More Search Tools" menu. */
    QPointer<QAction> preferredSearchTool();

    /**
     * Adds this action to the mainWindow's toolbar and saves the change
     * in the users ui configuration file.
     * This method is only needed for migration and should be removed once we can expect
     * that pretty much all users have been migrated. Remove in 2026 because that's when
     * even the most risk-averse distros will already have been forced to upgrade.
     * @return true if successful. Otherwise false.
     */
    bool addHamburgerMenuToToolbar();

    /** Creates an action representing an item in the URL navigator history */
    static QAction *urlNavigatorHistoryAction(const KUrlNavigator *urlNavigator, int historyIndex, QObject *parent = nullptr);

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
        void jobError(KIO::Job *job) override;
    };

    KNewFileMenu *m_newFileMenu;
    DolphinTabWidget *m_tabWidget;
    DolphinViewContainer *m_activeViewContainer;

    DolphinViewActionHandler *m_actionHandler;
    DolphinRemoteEncoding *m_remoteEncoding;
    QPointer<DolphinSettingsDialog> m_settingsDialog;
    DolphinBookmarkHandler *m_bookmarkHandler;
    SelectionMode::ActionTextHelper *m_actionTextHelper;
    DisabledActionNotifier *m_disabledActionNotifier;

    KIO::OpenUrlJob *m_lastHandleUrlOpenJob;

    TerminalPanel *m_terminalPanel;
    PlacesPanel *m_placesPanel;
    bool m_tearDownFromPlacesRequested;

    KToolBarPopupAction *m_backAction;
    KToolBarPopupAction *m_forwardAction;
    KActionMenu *m_splitViewAction;
    QAction *m_splitViewMenuAction;

    QMenu m_searchTools;
    KFileItemActions m_fileItemActions;

    QTimer *m_sessionSaveTimer;
    QFutureWatcher<void> *m_sessionSaveWatcher;
    bool m_sessionSaveScheduled;

    KIO::CommandLauncherJob *m_job;

    KRecentFilesAction *m_recentFiles = nullptr;

    friend class DolphinMainWindowTest;
};

inline DolphinViewContainer *DolphinMainWindow::activeViewContainer() const
{
    return m_activeViewContainer;
}

inline KNewFileMenu *DolphinMainWindow::newFileMenu() const
{
    return m_newFileMenu;
}

#endif // DOLPHIN_MAINWINDOW_H
