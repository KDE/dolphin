/***************************************************************************
 * Copyright (C) 2013 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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

struct ClosedTab;
class DolphinViewContainer;
class DolphinTabPage;
class KConfigGroup;
class KUrl;

class DolphinTabWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit DolphinTabWidget(QWidget* parent);

    DolphinViewContainer* activeViewContainer() const;

    DolphinTabPage* currentTabPage() const;
    DolphinTabPage* tabPageAt(const int index) const;

    void saveProperties(KConfigGroup& group) const;
    void readProperties(const KConfigGroup& group);

signals:
    void errorMessage(const QString& error);
    void activeViewChanged();
    void tabsCountChanged(int count);
    void rememberClosedTab(const ClosedTab& tab);

public slots:
    /** Creates a new tab with the url of the active view. */
    void openNewTab();

    /** Creates a new tab with the given \url. */
    void openNewTab(const KUrl& url);

    /**
     * Creates a new tab with the url of the active view and sets this tab
     * as active tab.
     */
    void openNewActivatedTab();

    /**
     * Creates a new tab with the given \url and sets this tab as active tab.
     */
    void openNewActivatedTab(const KUrl& url);

    /** Closes the currently active tab. */
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
     * Switches between one and two views:
     * If one view is visible, it will get split into two views.
     * If already two views are visible, the active view gets closed.
     */
    void toggleSplitView();

    void restoreClosedTab(const ClosedTab& tab);

private slots:
    void slotDetachTab(int index);

    void openNewActivatedTab(int index);

    void slotTabDropEvent(int index, QDropEvent* event);

    /**
     * If the current tab has changed, and the tab has a valid DolphinTabPage,
     * which is initialized, emit the signal activeViewChanged().
     */
    void slotCurrentChanged();

    /**
     * The tab properties (text, icon) have been changed for \a tabPage, so update the text
     * and the icon of the corresponding tab.
     */
    void slotTabPropertiesChanged(DolphinTabPage* tabPage);

protected:
    virtual void tabInserted(int index);
    virtual void tabRemoved(int index);
};

#endif