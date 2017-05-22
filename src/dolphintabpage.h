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

#ifndef DOLPHIN_TAB_PAGE_H
#define DOLPHIN_TAB_PAGE_H

#include <QWidget>
#include <QPointer>
#include <QUrl>

class QSplitter;
class DolphinViewContainer;
class KFileItemList;

class DolphinTabPage : public QWidget
{
    Q_OBJECT

public:
    explicit DolphinTabPage(const QUrl& primaryUrl, const QUrl& secondaryUrl = QUrl(), QWidget* parent = 0);

    /**
     * @return True if primary view is the active view in this tab.
     */
    bool primaryViewActive() const;

    /**
     * @return True if split view is enabled.
     */
    bool splitViewEnabled() const;

    /**
     * Enables or disables the split view mode.
     *
     * If \a enabled is true, it creates a secondary view with the url of the primary view.
     */
    void setSplitViewEnabled(bool enabled, const QUrl &secondaryUrl = QUrl());

    /**
     * @return The primary view containter.
     */
    DolphinViewContainer* primaryViewContainer() const;

    /**
     * @return The secondary view containter, can be 0 if split view is disabled.
     */
    DolphinViewContainer* secondaryViewContainer() const;

    /**
     * @return DolphinViewContainer of the active view
     */
    DolphinViewContainer* activeViewContainer() const;

    /**
     * Returns the selected items. The list is empty if no item has been
     * selected.
     */
    KFileItemList selectedItems() const;

    /**
     * Returns the number of selected items (this is faster than
     * invoking selectedItems().count()).
     */
    int selectedItemsCount() const;

    /**
     * Marks the items indicated by \p urls to get selected after the
     * directory DolphinView::url() has been loaded. Note that nothing
     * gets selected if no loading of a directory has been triggered
     * by DolphinView::setUrl() or DolphinView::reload().
     */
    void markUrlsAsSelected(const QList<QUrl> &urls);

    /**
     * Marks the item indicated by \p url to be scrolled to and as the
     * current item after directory DolphinView::url() has been loaded.
     */
    void markUrlAsCurrent(const QUrl& url);

    /**
     * Sets the places selector visible, if \a visible is true.
     * The places selector allows to select the places provided
     * by the places model passed in the constructor. Per default
     * the places selector is visible.
     */
    void setPlacesSelectorVisible(bool visible);

    /**
     * Refreshes the views of the main window by recreating them according to
     * the given Dolphin settings.
     */
    void refreshViews();

    /**
     * Saves all tab related properties (urls, splitter layout, ...).
     *
     * @return A byte-array which contains all properties.
     */
    QByteArray saveState() const;

    /**
     * Restores all tab related properties (urls, splitter layout, ...) from
     * the given \a state.
     */
    void restoreState(const QByteArray& state);

    /**
     * Restores all tab related properties (urls, splitter layout, ...) from
     * the given \a state.
     *
     * @deprecated The first tab state version has no version number, we keep
     *             this method to restore old states (<= Dolphin 4.14.x).
     */
    void restoreStateV1(const QByteArray& state);

    /**
     * Set whether the tab page is active
     *
     */
    void setActive(bool active);

signals:
    void activeViewChanged(DolphinViewContainer* viewContainer);
    void activeViewUrlChanged(const QUrl& url);

private slots:
    /**
     * Handles the view activated event.
     *
     * It sets the previous active view to inactive, updates the current
     * active view type and triggers the activeViewChanged event.
     */
    void slotViewActivated();

    /**
     * Handles the view url redirection event.
     *
     * It emits the activeViewUrlChanged signal with the url \a newUrl.
     */
    void slotViewUrlRedirection(const QUrl& oldUrl, const QUrl& newUrl);

    void switchActiveView();

private:
    /**
     * Creates a new view container and does the default initialization.
     */
    DolphinViewContainer* createViewContainer(const QUrl& url) const;

private:
    QSplitter* m_splitter;

    QPointer<DolphinViewContainer> m_primaryViewContainer;
    QPointer<DolphinViewContainer> m_secondaryViewContainer;

    bool m_primaryViewActive;
    bool m_splitViewEnabled;
    bool m_active;
};

#endif // DOLPHIN_TAB_PAGE_H
