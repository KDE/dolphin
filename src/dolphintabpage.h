/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 * SPDX-FileCopyrightText: 2020 Felix Ernst <fe.a.ernst@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHIN_TAB_PAGE_H
#define DOLPHIN_TAB_PAGE_H

#include <QPointer>
#include <QUrl>
#include <QWidget>
#include <QSplitter>

class DolphinNavigatorsWidgetAction;
class DolphinViewContainer;
class QVariantAnimation;
class KFileItemList;
class DolphinTabPageSplitter;

enum Animated {
    WithAnimation,
    WithoutAnimation
};

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
     * @param enabled      If true, creates a secondary viewContainer in this tab.
     *                     Otherwise deletes it.
     * @param animated     Decides wether the effects of this method call should
     *                     happen instantly or be transitioned to smoothly.
     * @param secondaryUrl If \p enabled is true, the new viewContainer will be opened at this
     *                     parameter. The default value will set the Url of the new viewContainer
     *                     to be the same as the existing one.
     */
    void setSplitViewEnabled(bool enabled, Animated animated, const QUrl &secondaryUrl = QUrl());

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

    void insertNavigatorsWidget(DolphinNavigatorsWidgetAction *navigatorsWidget);

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
     * Set whether the tab page is active
     *
     */
    void setActive(bool active);

Q_SIGNALS:
    void activeViewChanged(DolphinViewContainer* viewContainer);
    void activeViewUrlChanged(const QUrl& url);
    void splitterMoved(int pos, int index);

private Q_SLOTS:
    /**
     * Deletes all zombie viewContainers that were used for the animation
     * and resets the minimum size of the others to a sane value.
     */
    void slotAnimationFinished();

    /**
     * This method is called for every frame of the m_expandViewAnimation.
     */
    void slotAnimationValueChanged(const QVariant &value);

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

    /**
     * Starts an animation that transitions between split view mode states.
     *
     * One of the viewContainers is always being expanded when toggling so
     * this method can animate both opening and closing of viewContainers.
     * @param expandingContainer The container that will increase in size
     *                           over the course of the animation.
     */
    void startExpandViewAnimation(DolphinViewContainer *expandingContainer);

private:
    DolphinTabPageSplitter *m_splitter;

    QPointer<DolphinNavigatorsWidgetAction> m_navigatorsWidget;
    QPointer<DolphinViewContainer> m_primaryViewContainer;
    QPointer<DolphinViewContainer> m_secondaryViewContainer;

    DolphinViewContainer *m_expandingContainer;
    QPointer<QVariantAnimation> m_expandViewAnimation;

    bool m_primaryViewActive;
    bool m_splitViewEnabled;
    bool m_active;
};

class DolphinTabPageSplitterHandle : public QSplitterHandle
{
    Q_OBJECT

public:
    explicit DolphinTabPageSplitterHandle(Qt::Orientation orientation, QSplitter *parent);

protected:
    bool event(QEvent *event) override;

private:
    void resetSplitterSizes();

    // Sometimes QSplitterHandle doesn't receive MouseButtonDblClick event.
    // We can detect that MouseButtonDblClick event should have been
    // received if we receive two MouseButtonRelease events in a row.
    bool m_mouseReleaseWasReceived;
};

class DolphinTabPageSplitter : public QSplitter
{
    Q_OBJECT

public:
    explicit DolphinTabPageSplitter(Qt::Orientation orientation, QWidget *parent);

protected:
    QSplitterHandle* createHandle() override;
};

#endif // DOLPHIN_TAB_PAGE_H
