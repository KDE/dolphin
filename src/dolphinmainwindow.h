/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz19@gmail.com>             *
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

#include <config-baloo.h>

#include <KFileItemDelegate>
#include <kio/fileundomanager.h>
#include <ksortablelist.h>
#include <kxmlguiwindow.h>
#include <KIcon>

#include <QList>
#include <QWeakPointer>

typedef KIO::FileUndoManager::CommandType CommandType;

class DolphinViewActionHandler;
class DolphinApplication;
class DolphinSettingsDialog;
class DolphinTabBar;
class DolphinViewContainer;
class DolphinRemoteEncoding;
class DolphinTabPage;
class KAction;
class KFileItem;
class KFileItemList;
class KJob;
class KNewFileMenu;
class KUrl;
class QSplitter;
class QToolButton;
class QVBoxLayout;

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
    DolphinMainWindow();
    virtual ~DolphinMainWindow();

    /**
     * Returns the currently active view.
     * All menu actions are applied to this view. When
     * having a split view setup, the nonactive view
     * is usually shown in darker colors.
     */
    DolphinViewContainer* activeViewContainer() const;

    /**
     * Opens each directory in \p dirs in a separate tab. If the "split view"
     * option is enabled, 2 directories are collected within one tab.
     */
    void openDirectories(const QList<KUrl>& dirs);

    /**
     * Opens the directory which contains the files \p files
     * and selects all files (implements the --select option
     * of Dolphin).
     */
    void openFiles(const QList<KUrl>& files);

    /**
     * Returns the 'Create New...' sub menu which also can be shared
     * with other menus (e. g. a context menu).
     */
    KNewFileMenu* newFileMenu() const;

public slots:
    /**
     * Pastes the clipboard data into the currently selected folder
     * of the active view. If not exactly one folder is selected,
     * no pasting is done at all.
     */
    void pasteIntoFolder();

    /**
     * Returns the main window ID used through DBus.
     */
    int getId() const;

    /**
     * Implementation of the MainWindowAdaptor/QDBusAbstractAdaptor interface.
     * Inform all affected dolphin components (panels, views) of an URL
     * change.
     */
    void changeUrl(const KUrl& url);

    /**
     * The current directory of the Terminal Panel has changed, probably because
     * the user entered a 'cd' command. This slot calls changeUrl(url) and makes
     * sure that the panel keeps the keyboard focus.
     */
    void slotTerminalDirectoryChanged(const KUrl& url);

    /** Stores all settings and quits Dolphin. */
    void quit();

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
    void urlChanged(const KUrl& url);

    /**
     * Is emitted if information of an item is requested to be shown e. g. in the panel.
     * If item is null, no item information request is pending.
     */
    void requestItemInfo(const KFileItem& item);

    /**
     * Is emitted if the settings have been changed.
     */
    void settingsChanged();

    /**
     * Is emitted when a tab has been closed.
     */
    void rememberClosedTab(const KUrl& primaryUrl, const KUrl& secondaryUrl);

protected:
    /** @see QWidget::showEvent() */
    virtual void showEvent(QShowEvent* event);

    /** @see QMainWindow::closeEvent() */
    virtual void closeEvent(QCloseEvent* event);

    /** @see KMainWindow::saveProperties() */
    virtual void saveProperties(KConfigGroup& group);

    /** @see KMainWindow::readProperties() */
    virtual void readProperties(const KConfigGroup& group);

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

    /** Reloads the currently active view. */
    void reloadView();

    /** Stops the loading process for the currently active view. */
    void stopLoading();

    void enableStopAction();
    void disableStopAction();

    void showFilterBar();

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
     * Is invoked if the Places panel got visible/invisible and takes care
     * that the places-selector of all views is only shown if the Places panel
     * is invisible.
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

    /**
     * Open the previous URL in the URL history in a new tab
     * if the middle mouse button is clicked.
     */
    void goBack(Qt::MouseButtons buttons);

    /**
     * Open the next URL in the URL history in a new tab
     * if the middle mouse button is clicked.
     */
    void goForward(Qt::MouseButtons buttons);

    /**
     * Open the URL one hierarchy above the current URL in a new tab
     * if the middle mouse button is clicked.
     */
    void goUp(Qt::MouseButtons buttons);

    /**
     * Open the home URL in a new tab
     */
    void goHome(Qt::MouseButtons buttons);

    /** Opens Kompare for 2 selected files. */
    void compareFiles();

    /**
     * Hides the menu bar if it is visible, makes the menu bar
     * visible if it is hidden.
     */
    void toggleShowMenuBar();

    /** Opens a terminal window for the current location. */
    void openTerminal();

    /** Opens the settings dialog for Dolphin. */
    void editSettings();

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
     * actions corresponding to the current history.
     */
    void updateHistory();

    /** Updates the state of the 'Show filter bar' menu action. */
    void updateFilterBarAction(bool show);

    /** Open a new main window. */
    void openNewMainWindow();

    /** Opens a new view with the current URL that is part of a tab. */
    void openNewTab();

    /**
     * Opens a new tab in the background showing the URL \a primaryUrl and the
     * optional URL \a secondaryUrl.
     */
    void openNewTab(const KUrl& primaryUrl, const KUrl& secondaryUrl = KUrl());

    /**
     * Opens a new tab showing the  URL \a primaryUrl and the optional URL
     * \a secondaryUrl and activates the tab.
     */
    void openNewActivatedTab(const KUrl& primaryUrl, const KUrl& secondaryUrl = KUrl());

    /**
     * Opens a new tab showing the url from tab at the given \a index and
     * activates the tab.
     */
    void openNewActivatedTab(int index);

    void activateNextTab();

    void activatePrevTab();

    /**
     * Opens the selected folder in a new tab.
     */
    void openInNewTab();

    /**
     * Opens the selected folder in a new window.
     */
    void openInNewWindow();

    /**
     * Indicates in the statusbar that the execution of the command \a command
     * has been finished.
     */
    void showCommand(CommandType command);

    /**
     * Activates the tab with the index \a index, which means that the current view
     * is replaced by the view of the given tab.
     */
    void setActiveTab(int index);

    /** Closes the currently active tab. */
    void closeTab();

    /**
     * Closes the tab with the index \a index and activates the tab with index - 1.
     */
    void closeTab(int index);

    /**
     * Opens the tab with the index \a index in a new Dolphin instance and closes
     * this tab.
     */
    void detachTab(int index);

    /**
     * Is connected to the QTabBar signal tabMoved(int from, int to).
     * Reorders the list of tabs after a tab was moved in the tab bar
     * and sets m_tabIndex to the new index of the current tab.
     */
    void slotTabMoved(int from, int to);

    /**
     * If the URL can be listed, open it in the current view, otherwise
     * run it through KRun.
     */
    void handleUrl(const KUrl& url);

    /**
     * handleUrl() can trigger a stat job to see if the url can actually
     * be listed.
     */
    void slotHandleUrlStatFinished(KJob* job);

    /**
     * Is connected to the KTabBar signal receivedDropEvent.
     * Allows dragging and dropping files onto tabs.
     */
    void tabDropEvent(int tab, QDropEvent* event);

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
                         const KUrl& url,
                         const QList<QAction*>& customActions);

    void updateControlMenu();
    void updateToolBar();
    void slotControlButtonDeleted();

    /**
     * Is called if a panel emits an error-message and shows
     * the error-message in the active view-container.
     */
    void slotPanelErrorMessage(const QString& error);

    /**
     * Is called if the user clicked an item in the Places Panel.
     * Reloads the view if \a url is the current URL already, and changes the
     * current URL otherwise.
     */
    void slotPlaceActivated(const KUrl& url);

    void activeViewChanged();

    void closedTabsCountChanged(unsigned int count);

private:
    /**
     * Activates the given view, which means that
     * all menu actions are applied to this view. When
     * having a split view setup, the nonactive view
     * is usually shown in darker colors.
     */
    void setActiveViewContainer(DolphinViewContainer* view);

    void setupActions();
    void setupDockWidgets();
    void updateEditActions();
    void updateViewActions();
    void updateGoActions();

    void createControlButton();
    void deleteControlButton();

    /**
     * Adds the action \p action to the menu \p menu in
     * case if it has not added already to the toolbar.
     * @return True if the action has been added to the menu.
     */
    bool addActionToMenu(QAction* action, KMenu* menu);

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

    /** Returns the name of the tab for the URL \a url. */
    QString tabName(const KUrl& url) const;


    bool isKompareInstalled() const;

    /**
     * Sets the window caption to url.fileName() if this is non-empty,
     * "/" if the URL is "file:///", and url.protocol() otherwise.
     */
    void setUrlAsCaption(const KUrl& url);

    QString squeezedText(const QString& text) const;

    /**
     * Creates an action for showing/hiding a panel, that is accessible
     * in "Configure toolbars..." and "Configure shortcuts...". This is necessary
     * as the action for toggling the dock visibility is done by Qt which
     * is no KAction instance.
     */
    void createPanelAction(const KIcon& icon,
                           const QKeySequence& shortcut,
                           QAction* dockAction,
                           const QString& actionName);

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
        virtual ~UndoUiInterface();
        virtual void jobError(KIO::Job* job);
    };

    KNewFileMenu* m_newFileMenu;
    DolphinTabBar* m_tabBar;
    DolphinViewContainer* m_activeViewContainer;
    QVBoxLayout* m_centralWidgetLayout;
    int m_id;

    int m_tabIndex;
    QList<DolphinTabPage*> m_viewTab;

    DolphinViewActionHandler* m_actionHandler;
    DolphinRemoteEncoding* m_remoteEncoding;
    QWeakPointer<DolphinSettingsDialog> m_settingsDialog;

    // Members for the toolbar menu that is shown when the menubar is hidden:
    QToolButton* m_controlButton;
    QTimer* m_updateToolBarTimer;

    KIO::Job* m_lastHandleUrlStatJob;
};

inline DolphinViewContainer* DolphinMainWindow::activeViewContainer() const
{
    return m_activeViewContainer;
}

inline KNewFileMenu* DolphinMainWindow::newFileMenu() const
{
    return m_newFileMenu;
}

inline int DolphinMainWindow::getId() const
{
    return m_id;
}

#endif // DOLPHIN_MAINWINDOW_H

