/***************************************************************************
 * Copyright (C) 2014 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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

#ifndef DOLPHIN_TAB_WIDGET_H
#define DOLPHIN_TAB_WIDGET_H

#include <QTabWidget>
#include <QUrl>

class DolphinViewContainer;
class DolphinTabPage;
class KConfigGroup;

class DolphinTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    /**
     * @brief Controls where tabs are placed
     */
    enum TabPlacement {
        /**
          * The new tab is placed after the current tab
          */
        AfterCurrentTab,
        /**
          * The new tab is placed after the last tab
          */
        AfterLastTab
    };
    explicit DolphinTabWidget(QWidget* parent);

    /**
     * @return Tab page at the current index (can be 0 if tabs count is smaller than 1)
     */
    DolphinTabPage* currentTabPage() const;

    /**
     * @return the next tab page. If the current active tab is the last tab,
     * it returns the first tab. If there is only one tab, returns nullptr
     */
    DolphinTabPage* nextTabPage() const;

    /**
     * @return the previous tab page. If the current active tab is the first tab,
     * it returns the last tab. If there is only one tab, returns nullptr
     */
    DolphinTabPage* prevTabPage() const;

    /**
     * @return Tab page at the given \a index (can be 0 if the index is out-of-range)
     */
    DolphinTabPage* tabPageAt(const int index) const;

    void saveProperties(KConfigGroup& group) const;
    void readProperties(const KConfigGroup& group);

    /**
     * Refreshes the views of the main window by recreating them according to
     * the given Dolphin settings.
     */
    void refreshViews();

signals:
    /**
     * Is emitted when the active view has been changed, by changing the current
     * tab or by activating another view when split view is enabled in the current
     * tab.
     */
    void activeViewChanged(DolphinViewContainer* viewContainer);

    /**
     * Is emitted when the number of open tabs has changed (e.g. by opening or
     * closing a tab)
     */
    void tabCountChanged(int count);

    /**
     * Is emitted when a tab has been closed.
     */
    void rememberClosedTab(const QUrl& url, const QByteArray& state);

    /**
     * Is emitted when the url of the current tab has been changed. This signal
     * is also emitted when the active view has been changed.
     */
    void currentUrlChanged(const QUrl& url);

public slots:
    /**
     * Opens a new view with the current URL that is part of a tab and activates
     * the tab.
     */
    void openNewActivatedTab();

    /**
     * Opens a new tab showing the  URL \a primaryUrl and the optional URL
     * \a secondaryUrl and activates the tab.
     */
    void openNewActivatedTab(const QUrl& primaryUrl, const QUrl& secondaryUrl = QUrl());

    /**
     * Opens a new tab in the background showing the URL \a primaryUrl and the
     * optional URL \a secondaryUrl. \a tabPlacement controls where the new tab
     * is placed.
     */
    void openNewTab(const QUrl &primaryUrl, const QUrl &secondaryUrl = QUrl(),
                    TabPlacement tabPlacement = AfterLastTab);

    /**
     * Opens each directory in \p dirs in a separate tab. If \a splitView is set,
     * 2 directories are collected within one tab.
     * \pre \a dirs must contain at least one url.
     */
    void openDirectories(const QList<QUrl>& dirs, bool splitView);

    /**
     * Opens the directories which contain the files \p files and selects all files.
     * If \a splitView is set, 2 directories are collected within one tab.
     * \pre \a files must contain at least one url.
     */
    void openFiles(const QList<QUrl> &files, bool splitView);

    /**
     * Closes the currently active tab.
     */
    void closeTab();

    /**
     * Closes the tab with the index \a index and activates the tab with index - 1.
     */
    void closeTab(const int index);

    /**
     * Activates the next tab in the tab bar.
     * If the current active tab is the last tab, it activates the first tab.
     */
    void activateNextTab();

    /**
     * Activates the previous tab in the tab bar.
     * If the current active tab is the first tab, it activates the last tab.
     */
    void activatePrevTab();

    /**
     * Is invoked if the Places panel got visible/invisible and takes care
     * that the places-selector of all views is only shown if the Places panel
     * is invisible.
     */
    void slotPlacesPanelVisibilityChanged(bool visible);

    /**
     * Is called when the user wants to reopen a previously closed tab from
     * the recent tabs menu.
     */
    void restoreClosedTab(const QByteArray& state);

private slots:
    /**
     * Opens the tab with the index \a index in a new Dolphin instance and closes
     * this tab.
     */
    void detachTab(int index);

    /**
     * Opens a new tab showing the url from tab at the given \a index and
     * activates the tab.
     */
    void openNewActivatedTab(int index);

    /**
     * Is connected to the KTabBar signal receivedDropEvent.
     * Allows dragging and dropping files onto tabs.
     */
    void tabDropEvent(int tab, QDropEvent* event);

    /**
     * The active view url of a tab has been changed so update the text and the
     * icon of the corresponding tab.
     */
    void tabUrlChanged(const QUrl& url);

    void currentTabChanged(int index);

protected:
    void tabInserted(int index) override;
    void tabRemoved(int index) override;

private:
    /**
     * @param tabPage The tab page to get the name of
     * @return The name of the tab page
     */
    QString tabName(DolphinTabPage* tabPage) const;

private:
    /** Caches the (negated) places panel visibility */
    bool m_placesSelectorVisible;

    int m_lastViewedTab;
};

#endif
