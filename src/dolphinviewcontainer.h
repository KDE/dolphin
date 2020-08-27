/*
 * SPDX-FileCopyrightText: 2007 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINVIEWCONTAINER_H
#define DOLPHINVIEWCONTAINER_H

#include "config-kactivities.h"
#include "dolphinurlnavigator.h"
#include "views/dolphinview.h"

#include <KFileItem>
#include <KIO/Job>
#include <KUrlNavigator>

#include <QElapsedTimer>
#include <QPushButton>
#include <QWidget>

#ifdef HAVE_KACTIVITIES
namespace KActivities {
    class ResourceInstance;
}
#endif

class FilterBar;
class KMessageWidget;
class QUrl;
class KUrlNavigator;
class DolphinSearchBox;
class DolphinStatusBar;

/**
 * @short Represents a view for the directory content
 *        including the navigation bar, filter bar and status bar.
 *
 * View modes for icons, compact and details are supported. Currently
 * Dolphin allows to have up to two views inside the main window.
 *
 * @see DolphinView
 * @see FilterBar
 * @see KUrlNavigator
 * @see DolphinStatusBar
 */
class DolphinViewContainer : public QWidget
{
    Q_OBJECT

public:
    enum MessageType
    {
        Information,
        Warning,
        Error
    };

    DolphinViewContainer(const QUrl& url, QWidget* parent);
    ~DolphinViewContainer() override;

    /**
     * Returns the current active URL, where all actions are applied.
     * The URL navigator is synchronized with this URL.
     */
    QUrl url() const;

    /**
     * If \a active is true, the view container will marked as active. The active
     * view container is defined as view where all actions are applied to.
     */
    void setActive(bool active);
    bool isActive() const;

    /**
     * If \a grab is set to true, the container automatically grabs the focus
     * as soon as the URL has been changed. Per default the grabbing
     * of the focus is enabled.
     */
    void setAutoGrabFocus(bool grab);
    bool autoGrabFocus() const;

    QString currentSearchText() const;

    const DolphinStatusBar* statusBar() const;
    DolphinStatusBar* statusBar();

    /**
     * @return  An UrlNavigator that is controlling this view
     *          or nullptr if there is none.
     * @see connectUrlNavigator()
     * @see disconnectUrlNavigator()
     * 
     * Use urlNavigatorInternalWithHistory() if you want to access the history.
     * @see urlNavigatorInternalWithHistory()
     */
    const DolphinUrlNavigator *urlNavigator() const;
    /**
     * @return  An UrlNavigator that is controlling this view
     *          or nullptr if there is none.
     * @see connectUrlNavigator()
     * @see disconnectUrlNavigator()
     * 
     * Use urlNavigatorInternalWithHistory() if you want to access the history.
     * @see urlNavigatorInternalWithHistory()
     */
    DolphinUrlNavigator *urlNavigator();

    /**
     * @return An UrlNavigator that contains this view's history.
     * Use urlNavigator() instead when not accessing the history.
     */
    const DolphinUrlNavigator *urlNavigatorInternalWithHistory() const;
    /**
     * @return An UrlNavigator that contains this view's history.
     * Use urlNavigator() instead when not accessing the history.
     */
    DolphinUrlNavigator *urlNavigatorInternalWithHistory();

    const DolphinView* view() const;
    DolphinView* view();

    /**
     * @param urlNavigator  The UrlNavigator that is supposed to control
     *                      this view.
     */
    void connectUrlNavigator(DolphinUrlNavigator *urlNavigator);

    inline void connectToInternalUrlNavigator()
    {
        connectUrlNavigator(m_urlNavigator);
    }

    /**
     * Disconnects the navigator that is currently controling the view.
     * This method completely reverses connectUrlNavigator().
     */
    void disconnectUrlNavigator();

    /**
     * Shows the message \msg with the given type non-modal above
     * the view-content.
     * @return the KMessageWidget used to show the message
     */
    KMessageWidget *showMessage(const QString& msg, MessageType type);

    /**
     * Refreshes the view container to get synchronized with the (updated) Dolphin settings.
     */
    void readSettings();

    /** Returns true, if the filter bar is visible. */
    bool isFilterBarVisible() const;


    /** Returns true if the search mode is enabled. */
    bool isSearchModeEnabled() const;

    /**
     * @return Text that should be used for the current URL when creating
     *         a new place.
     */
    QString placesText() const;

    /**
     * Reload the view of this container. This will also hide messages in a messagewidget.
     */
    void reload();

    /**
     * @return Returns a Caption suitable for display in the window title.
     * It is calculated depending on GeneralSettings::showFullPathInTitlebar().
     * If it's false, it calls caption().
     */
    QString captionWindowTitle() const;

    /**
     * @return Returns a Caption suitable for display to the user. It is
     * calculated depending on settings, if a search is active and other
     * factors.
     */
    QString caption() const;

public slots:
    /**
     * Sets the current active URL, where all actions are applied. The
     * URL navigator is synchronized with this URL. The signals
     * KUrlNavigator::urlChanged() and KUrlNavigator::historyChanged()
     * are emitted.
     * @see DolphinViewContainer::urlNavigator()
     */
    void setUrl(const QUrl& url);

    /**
     * Popups the filter bar above the status bar if \a visible is true.
     * It \a visible is true, it is assured that the filter bar gains
     * the keyboard focus.
     */
    void setFilterBarVisible(bool visible);

    /**
     * Enables the search mode, if \p enabled is true. In the search mode the URL navigator
     * will be hidden and replaced by a line editor that allows to enter a search term.
     */
    void setSearchModeEnabled(bool enabled);

signals:
    /**
     * Is emitted whenever the filter bar has changed its visibility state.
     */
    void showFilterBarChanged(bool shown);
    /**
     * Is emitted whenever the search mode has changed its state.
     */
    void searchModeEnabledChanged(bool enabled);

    /**
     * Is emitted when the write state of the folder has been changed. The application
     * should disable all actions like "Create New..." that depend on the write
     * state.
     */
    void writeStateChanged(bool isFolderWritable);

private slots:
    /**
     * Updates the number of items (= number of files + number of
     * directories) in the statusbar. If files are selected, the number
     * of selected files and the sum of the filesize is shown. The update
     * is done asynchronously, as getting the sum of the
     * filesizes can be an expensive operation.
     * Unless a previous OperationCompletedMessage was set very shortly before
     * calling this method, it will be overwritten (see DolphinStatusBar::setMessage).
     * Previous ErrorMessages however are always preserved.
     */
    void delayedStatusBarUpdate();

    /**
     * Is invoked by DolphinViewContainer::delayedStatusBarUpdate() and
     * updates the status bar synchronously.
     */
    void updateStatusBar();

    void updateDirectoryLoadingProgress(int percent);

    void updateDirectorySortingProgress(int percent);

    void updateNavigatorWidgetVisibility();

    /**
     * Updates the statusbar to show an undetermined progress with the correct
     * context information whether a searching or a directory loading is done.
     */
    void slotDirectoryLoadingStarted();

    /**
     * Assures that the viewport position is restored and updates the
     * statusbar to reflect the current content.
     */
    void slotDirectoryLoadingCompleted();

    /**
     * Updates the statusbar to show, that the directory loading has
     * been canceled.
     */
    void slotDirectoryLoadingCanceled();

    /**
     * Is called if the URL set by DolphinView::setUrl() represents
     * a file and not a directory. Takes care to activate the file.
     */
    void slotUrlIsFileError(const QUrl& url);

    /**
     * Handles clicking on an item. If the item is a directory, the
     * directory is opened in the view. If the item is a file, the file
     * gets started by the corresponding application.
     */
    void slotItemActivated(const KFileItem& item);

    /**
     * Handles activation of multiple files. The files get started by
     * the corresponding applications.
     */
    void slotItemsActivated(const KFileItemList& items);

    /**
     * Shows the information for the item \a item inside the statusbar. If the
     * item is null, the default statusbar information is shown.
     */
    void showItemInfo(const KFileItem& item);

    void closeFilterBar();

    /**
     * Filters the currently shown items by \a nameFilter. All items
     * which contain the given filter string will be shown.
     */
    void setNameFilter(const QString& nameFilter);

    /**
     * Marks the view container as active
     * (see DolphinViewContainer::setActive()).
     */
    void activate();

    /**
     * Is invoked if the signal urlAboutToBeChanged() from the URL navigator
     * is emitted. Tries to save the view-state.
     */
    void slotUrlNavigatorLocationAboutToBeChanged(const QUrl& url);

    /**
     * Restores the current view to show \a url and assures
     * that the root URL of the view is respected.
     */
    void slotUrlNavigatorLocationChanged(const QUrl& url);

    /**
     * @see KUrlNavigator::urlSelectionRequested
     */
    void slotUrlSelectionRequested(const QUrl& url);

    /**
     * Is invoked when a redirection is done and changes the
     * URL of the URL navigator to \a newUrl without triggering
     * a reloading of the directory.
     */
    void redirect(const QUrl& oldUrl, const QUrl& newUrl);

    /** Requests the focus for the view \a m_view. */
    void requestFocus();

    /**
     * Gets the search URL from the searchbox and starts searching.
     */
    void startSearching();
    void closeSearchBox();

    /**
     * Stops the loading of a directory. Is connected with the "stopPressed" signal
     * from the statusbar.
     */
    void stopDirectoryLoading();

    void slotStatusBarZoomLevelChanged(int zoomLevel);

    /**
     * Slot that calls showMessage(msg, Error).
     */
    void showErrorMessage(const QString& msg);

private:
    /**
     * @return True if the URL protocol is a search URL (e. g. baloosearch:// or filenamesearch://).
     */
    bool isSearchUrl(const QUrl& url) const;

    /**
     * Saves the state of the current view: contents position,
     * root URL, ...
     */
    void saveViewState();

    /**
     * Restores the state of the current view iff the URL navigator contains a
     * non-empty location state.
     */
    void tryRestoreViewState();

private:
    QVBoxLayout* m_topLayout;
    QWidget* m_navigatorWidget;

    /**
     * The UrlNavigator within the m_navigatorWidget. m_urlNavigator is
     * used even when another UrlNavigator is controlling the view to keep
     * track of this view containers history.
     */
    DolphinUrlNavigator *m_urlNavigator;

    /**
     * The UrlNavigator that is currently connected to the view. This could
     * either be m_urlNavigator, the urlNavigator in the toolbar or nullptr.
     */
    QPointer<DolphinUrlNavigator> m_urlNavigatorConnected;
    QPushButton* m_emptyTrashButton;
    DolphinSearchBox* m_searchBox;
    bool m_searchModeEnabled;
    KMessageWidget* m_messageWidget;

    DolphinView* m_view;

    FilterBar* m_filterBar;

    DolphinStatusBar* m_statusBar;
    QTimer* m_statusBarTimer;            // Triggers a delayed update
    QElapsedTimer m_statusBarTimestamp;  // Time in ms since last update
    bool m_autoGrabFocus;

#ifdef HAVE_KACTIVITIES
private:
    KActivities::ResourceInstance * m_activityResourceInstance;
#endif
};

#endif // DOLPHINVIEWCONTAINER_H
