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

#include "dolphintabwidget.h"

#include "dolphintabbar.h"
#include "dolphintabpage.h"
#include "dolphinrecenttabsmenu.h"
#include "dolphinviewcontainer.h"
#include "dolphin_generalsettings.h"
#include "views/draganddrophelper.h"

#include <QApplication>
#include <KConfigGroup>
#include <KIcon>
#include <KRun>

DolphinTabWidget::DolphinTabWidget(QWidget* parent) :
    QTabWidget(parent)
{
    connect(this, SIGNAL(tabCloseRequested(int)), SLOT(closeTab(int)));
    connect(this, SIGNAL(currentChanged(int)), SLOT(slotCurrentChanged()));

    DolphinTabBar* tabBar = new DolphinTabBar(this);
    connect(tabBar, SIGNAL(openNewActivatedTab()),         SLOT(openNewActivatedTab()));
    connect(tabBar, SIGNAL(openNewActivatedTab(int)),      SLOT(openNewActivatedTab(int)));
    connect(tabBar, SIGNAL(openNewActivatedTab(KUrl)),     SLOT(openNewActivatedTab(KUrl)));
    connect(tabBar, SIGNAL(tabDropEvent(int,QDropEvent*)), SLOT(slotTabDropEvent(int,QDropEvent*)));
    connect(tabBar, SIGNAL(tabDetachRequested(int)),       SLOT(slotDetachTab(int)));
    tabBar->setAutoActivationDelay(750);
    tabBar->hide();

    setTabBar(tabBar);
    setDocumentMode(true);
    setElideMode(Qt::ElideLeft);
    setUsesScrollButtons(true);
    setTabsClosable(true);
}

DolphinViewContainer* DolphinTabWidget::activeViewContainer() const
{
    DolphinTabPage* tabPage = currentTabPage();
    return tabPage ? tabPage->activeViewContainer() : 0;
}

DolphinTabPage* DolphinTabWidget::currentTabPage() const
{
    return tabPageAt(currentIndex());
}

DolphinTabPage* DolphinTabWidget::tabPageAt(const int index) const
{
    if (index < 0 || index >= count()) {
        return 0;
    }
    return static_cast<DolphinTabPage*>(widget(index));
}

void DolphinTabWidget::saveProperties(KConfigGroup& group) const
{
    const int tabCount = count();
    group.writeEntry("Tab Count", tabCount);
    group.writeEntry("Active Tab Index", currentIndex());

    for (int index = 0; index < tabCount; ++index) {
        tabPageAt(index)->saveProperties(index, group);
    }
}

void DolphinTabWidget::readProperties(const KConfigGroup& group)
{
    const int tabCount = group.readEntry("Tab Count", 1);
    for (int index = 0; index < tabCount; ++index) {
        if (index >= count()) {
            openNewTab();
        }
        tabPageAt(index)->readProperties(index, group);
    }

    const int index = group.readEntry("Active Tab Index", 0);
    setCurrentIndex(index);
}

void DolphinTabWidget::openNewTab()
{
    bool isUrlEditable = false;
    if (activeViewContainer()) {
        isUrlEditable = activeViewContainer()->urlNavigator()->isUrlEditable();
    }

    if (activeViewContainer()) {
        openNewActivatedTab(activeViewContainer()->url());
    } else {
        openNewActivatedTab(GeneralSettings::self()->homeUrl());
    }

    // The URL navigator of the new tab should have the same editable state
    // as the current tab
    KUrlNavigator* navigator = activeViewContainer()->urlNavigator();
    navigator->setUrlEditable(isUrlEditable);

    if (isUrlEditable) {
        // If a new tab is opened and the URL is editable, assure that
        // the user can edit the URL without manually setting the focus
        navigator->setFocus();
    }
}

void DolphinTabWidget::openNewTab(const KUrl& url)
{
    QWidget* focusWidget = QApplication::focusWidget();

    DolphinTabPage* tabPage = new DolphinTabPage(url, this);
    connect(tabPage, SIGNAL(activeViewChanged()),
            this, SIGNAL(activeViewChanged()));
    connect(tabPage, SIGNAL(tabPropertiesChanged(DolphinTabPage*)),
            this, SLOT(slotTabPropertiesChanged(DolphinTabPage*)));
    addTab(tabPage, tabPage->tabIcon(), tabPage->tabText());

    if (focusWidget) {
        // The DolphinViewContainer grabbed the keyboard focus. As the tab is opened
        // in background, assure that the previous focused widget gets the focus back.
        focusWidget->setFocus();
    }
}

void DolphinTabWidget::openNewActivatedTab()
{
    openNewTab();
    setCurrentIndex(count() - 1);
}

void DolphinTabWidget::openNewActivatedTab(const KUrl& url)
{
    openNewTab(url);
    setCurrentIndex(count() - 1);
}

void DolphinTabWidget::closeTab()
{
    closeTab(currentIndex());
}

void DolphinTabWidget::closeTab(const int index)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < count());

    if (count() < 2) {
        // Never close the last tab.
        return;
    }

    DolphinTabPage* tabPage = tabPageAt(index);
    if (!tabPage) {
        return;
    }

    removeTab(index);

    ClosedTab tab;
    tab.text = tabPage->tabText();
    tab.icon = tabPage->tabIcon();
    tab.isSplit = tabPage->isSplitViewEnabled();
    tab.primaryUrl = tabPage->viewContainer(DolphinTabPage::PrimaryView)->url();
    if (tabPage->isSplitViewEnabled()) {
        tab.secondaryUrl = tabPage->viewContainer(DolphinTabPage::SecondaryView)->url();
    }
    emit rememberClosedTab(tab);

    tabPage->deleteLater();
}

void DolphinTabWidget::activateNextTab()
{
    const int index = currentIndex() + 1;
    setCurrentIndex(index < count() ? index : 0);
}

void DolphinTabWidget::activatePrevTab()
{
    const int index = currentIndex() - 1;
    setCurrentIndex(index > -1 ? index : (count() - 1));
}

void DolphinTabWidget::toggleSplitView()
{
    if (currentTabPage()) {
        currentTabPage()->toggleSplitViewEnabled();
    }
}

void DolphinTabWidget::restoreClosedTab(const ClosedTab& tab)
{
    openNewActivatedTab(tab.primaryUrl);
    if (tab.isSplit) {
        currentTabPage()->setSplitViewEnabled(true, tab.secondaryUrl);
    }
}

void DolphinTabWidget::slotDetachTab(int index)
{
    if (index >= 0) {
        const QString separator(QLatin1Char(' '));

        const DolphinTabPage* tabPage = tabPageAt(index);

        const DolphinViewContainer* primaryView = tabPage->viewContainer(DolphinTabPage::PrimaryView);
        QString command = QLatin1String("dolphin") % separator % primaryView->url().url();

        if (tabPage->isSplitViewEnabled()) {
            const DolphinViewContainer* secondaryView = tabPage->viewContainer(DolphinTabPage::SecondaryView);
            command += separator % secondaryView->url().url() % separator % QLatin1String("-split");
        }

        KRun::runCommand(command, this);
        closeTab(index);
    }
}

void DolphinTabWidget::openNewActivatedTab(int index)
{
    if (index >= 0) {
        const DolphinViewContainer* viewContainer = tabPageAt(index)->activeViewContainer();
        openNewActivatedTab(viewContainer->url());
    }
}

void DolphinTabWidget::slotTabDropEvent(int index, QDropEvent* event)
{
    if (index >= 0) {
        // The files/folders are dropped over a tab, so show
        // the drop urls context menu.
        const DolphinView* view = tabPageAt(index)->activeViewContainer()->view();
        QString error;
        DragAndDropHelper::dropUrls(view->rootItem(), view->url(), event, error);
        if (!error.isEmpty()) {
            emit errorMessage(error);
        }
    }
}

void DolphinTabWidget::slotCurrentChanged()
{
    const DolphinTabPage* tabPage = currentTabPage();
    if (tabPage && tabPage->activeView() != DolphinTabPage::NoView) {
        emit activeViewChanged();
    }
}

void DolphinTabWidget::slotTabPropertiesChanged(DolphinTabPage* tabPage)
{
    const int index = indexOf(tabPage);
    if (index >= 0) {
        tabBar()->setTabText(index, tabPage->tabText());
        tabBar()->setTabIcon(index, tabPage->tabIcon());
    }
}

void DolphinTabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);

    if (count() > 1) {
        tabBar()->show();

    }

    emit tabsCountChanged(count());
}

void DolphinTabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);

    // If only one tab is left, then remove the tab entry so that
    // closing the last tab is not possible.
    if (count() < 2) {
        tabBar()->hide();
    }

    emit tabsCountChanged(count());
}