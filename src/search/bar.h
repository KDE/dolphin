/*
    SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef SEARCHBAR_H
#define SEARCHBAR_H

#include "animatedheightwidget.h"
#include "dolphinquery.h"
#include "updatablestateinterface.h"

#include <KMessageWidget>

#include <QUrl>

class DolphinSearchBarTest;
class QHBoxLayout;
class QLineEdit;
class QToolButton;
class QVBoxLayout;

namespace Search
{
class BarSecondRowFlowLayout;
template<class Selector>
class Chip;
class DateSelector;
class FileTypeSelector;
class MinimumRatingSelector;
class Popup;
class TagsSelector;

/**
 * @brief User interface for searching files and folders.
 *
 * This Bar is both for configuring a new search as well as showing the search parameter of any search URL opened in Dolphin.
 * There are many search parameters whose availability can depend on various conditions. Those include:
 * - Where to search: Everywhere or below the current directory
 * - What to search: Filenames or content
 * - How to search: Which search tool to use
 * - etc.
 *
 * The class which defines the state of this Bar and its children is DolphinQuery.
 * @see DolphinQuery.
 */
class Bar : public AnimatedHeightWidget, public UpdatableStateInterface
{
    Q_OBJECT

public:
    /**
     * @brief Constructs a Search::Bar with an initial state matching @p dolphinQuery and with parent @p parent.
     */
    explicit Bar(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent = nullptr);

    /**
     * Returns the text that should be used as input
     * for searching.
     */
    QString text() const;

    /**
     * Sets the current path that is used as root for searching files.
     * If @url is the Home dir, "From Here" is selected instead.
     */
    void setSearchPath(const QUrl &url);

    /**
     * Selects the whole text of the search box.
     */
    void selectAll();

    /**
     * All showing and hiding of this bar is supposed to go through this method. When hiding this bar, it emits all the necessary signals to restore the view
     * container to a non-search URL.
     * This method also aims to make sure that visibilityChanged() will be emitted no matter from where setVisible() is called. This way the "Find" action can
     * be properly un/checked.
     * @see AnimatedHeightWidget::setVisible().
     */
    void setVisible(bool visible, Animated animated);

    /**
     * @returns false, when the search UI has not yet been changed to search for anything specific. For example when no search term has been entered yet.
     *          Otherwise returns true, for example when a search term has been entered or there is a search request for all files of a specific file type or
     *          with a specific modification date.
     */
    bool isSearchConfigured() const;

    /**
     * @returns the title for the search that is currently configured in this bar.
     * @see DolphinQuery::title().
     */
    QString queryTitle() const;

Q_SIGNALS:
    /**
     * This signals a request for the attached view container to switch to @p url.
     * A URL for searching is requested when the user actively engages with this UI to trigger a search.
     * A non-search URL is requested when this search UI is closed and no search results should be displayed anymore.
     */
    void urlChangeRequested(const QUrl &url);

    /**
     * Is emitted when the bar should receive focus. This is usually triggered by a user action that implies that this bar should no longer have focus.
     */
    void focusViewRequest();

    /**
     * Requests for @p message with the given @p messageType to be shown to the user in a non-modal way.
     */
    void showMessage(const QString &message, KMessageWidget::MessageType messageType);

    /**
     * Requests for a progress update to be shown to the user in a non-modal way.
     * @param currentlyRunningTaskTitle     The task that is currently progressing.
     * @param installationProgressPercent   The current percentage of completion.
     */
    void showInstallationProgress(const QString &currentlyRunningTaskTitle, int installationProgressPercent);

    /**
     * Is emitted when a change of the visibility of this bar is invoked in any way. This can happen from code calling from outside this class, for example
     * when the user triggered a keyboard shortcut to show this bar, or from inside, for example because the close button on this bar was pressed or an Escape
     * key press was received.
     */
    void visibilityChanged(bool visible);

protected:
    /** Handles Escape key presses to clear the search field or close this bar. */
    void keyPressEvent(QKeyEvent *event) override;
    /** Allows moving the focus to the view with the Down arrow key. */
    void keyReleaseEvent(QKeyEvent *event) override;

private Q_SLOTS:
    /**
     * Is called when any component within this Bar emits a configurationChanged() signal.
     * This method is then responsible to communicate the changed search configuration to every other interested party by calling
     * UpdatableStateInterface::updateStateToMatch() methods and commiting the new search configuration.
     * @see UpdatableStateInterface::updateStateToMatch().
     * @see commitCurrentConfiguration().
     */
    void slotConfigurationChanged(const DolphinQuery &searchConfiguration);

    /**
     * Changes the m_searchConfiguration in response to the user editing the search term. If no further changes to the search term happen within a time limit,
     * the new search configuration will eventually be commited.
     * @see commitCurrentConfiguration.
     */
    void slotSearchTermEdited(const QString &text);

    /**
     * Commits the current search configuration and then requests moving focus away from this bar and to the view.
     * @see commitCurrentConfiguration.
     */
    void slotReturnPressed();

    /**
     * Translates the current m_searchConfiguration into URLs which are then emitted through the urlChangeRequested() signal.
     * If the current m_searchConfiguration is a valid search, a searchUrl is emitted. If it is not a valid search, i.e. when isSearchConfigured() is false,
     * the search path is instead emitted so the view returns to showing a normal folder instead of search results.
     * @see urlChangeRequested().
     */
    void commitCurrentConfiguration();

    /** Adds the current search as a link/favorite to the Places panel. */
    void slotSaveSearch();

private:
    /**
     * This Search::Bar always represents a search configuration. This method takes a new @p dolphinQuery i.e. search configuration and updates itself and all
     * child widgets to match it. This way the user always knows which search parameters lead to the query results that appear in the view.
     * @see UpdatableStateInterface::updateStateToMatch().
     */
    void updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery) override;

    /** @see AnimatedHeightWidget::preferredHeight() */
    int preferredHeight() const override;

private:
    QVBoxLayout *m_topLayout = nullptr;

    // The widgets below are sorted by their tab order.

    QLineEdit *m_searchTermEditor = nullptr;
    QAction *m_saveSearchAction = nullptr;
    /// The main popup of this bar that allows configuring most search parameters.
    Popup *m_popup = nullptr;
    BarSecondRowFlowLayout *m_secondRowLayout = nullptr;
    QToolButton *m_fromHereButton = nullptr;
    QToolButton *m_everywhereButton = nullptr;
    Chip<FileTypeSelector> *m_fileTypeSelectorChip = nullptr;
    Chip<DateSelector> *m_modifiedSinceDateSelectorChip = nullptr;
    Chip<MinimumRatingSelector> *m_minimumRatingSelectorChip = nullptr;
    Chip<TagsSelector> *m_requiredTagsSelectorChip = nullptr;

    /// Starts a new search when the user has finished typing the search term.
    QTimer *m_startSearchTimer = nullptr;

    friend DolphinSearchBarTest;
};

}

#endif
