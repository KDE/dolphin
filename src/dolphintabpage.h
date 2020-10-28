/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_TAB_PAGE_H
#define DOLPHIN_TAB_PAGE_H

#include <QPointer>
#include <QUrl>
#include <QWidget>

class DolphinNavigatorsWidgetAction;
class DolphinViewContainer;
class QSplitter;
class KFileItemList;

class DolphinTabPage : public QWidget
{
    Q_OBJECT

public:
    explicit DolphinTabPage(const QUrl& primaryUrl, const QUrl& secondaryUrl = QUrl(), QWidget* parent = nullptr);

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
     * @return The primary view container.
     */
    DolphinViewContainer* primaryViewContainer() const;

    /**
     * @return The secondary view container, can be 0 if split view is disabled.
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
     * Connects a navigatorsWidget to this. It will be connected to the DolphinViewContainers
     * managed by this tab. For alignment purposes this will from now on notify the
     * navigatorsWidget when this tab or its viewContainers are resized.
     */
    void connectNavigators(DolphinNavigatorsWidgetAction *navigatorsWidget);

    /**
     * Makes it so this tab and its DolphinViewContainers aren't controlled by any
     * UrlNavigators anymore.
     */
    void disconnectNavigators();

    /**
     * Calls resizeNavigators() when a watched object is resized.
     */
    bool eventFilter(QObject */* watched */, QEvent *event) override;

    /**
     * Notify the connected DolphinNavigatorsWidgetAction of geometry changes which it
     * needs for visual alignment.
     */
    void resizeNavigators() const;

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
    Q_DECL_DEPRECATED void restoreStateV1(const QByteArray& state);

    /**
     * Set whether the tab page is active
     *
     */
    void setActive(bool active);

signals:
    void activeViewChanged(DolphinViewContainer* viewContainer);
    void activeViewUrlChanged(const QUrl& url);
    void splitterMoved(int pos, int index);

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

    QPointer<DolphinNavigatorsWidgetAction> m_navigatorsWidget;
    QPointer<DolphinViewContainer> m_primaryViewContainer;
    QPointer<DolphinViewContainer> m_secondaryViewContainer;

    bool m_primaryViewActive;
    bool m_splitViewEnabled;
    bool m_active;
};

#endif // DOLPHIN_TAB_PAGE_H
