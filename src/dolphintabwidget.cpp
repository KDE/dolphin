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

#include "dolphintabwidget.h"

#include "dolphintabbar.h"
#include "dolphintabpage.h"
#include "dolphinviewcontainer.h"

#include <KConfigGroup>
#include <KRun>
#include <KShell>
#include <kio/global.h>

#include <QApplication>
#include <QDropEvent>

DolphinTabWidget::DolphinTabWidget(QWidget* parent) :
    QTabWidget(parent),
    m_placesSelectorVisible(true),
    m_previousTab(0)
{
    connect(this, &DolphinTabWidget::tabCloseRequested,
            this, static_cast<void (DolphinTabWidget::*)(int)>(&DolphinTabWidget::closeTab));
    connect(this, &DolphinTabWidget::currentChanged,
            this, &DolphinTabWidget::currentTabChanged);

    DolphinTabBar* tabBar = new DolphinTabBar(this);
    connect(tabBar, &DolphinTabBar::openNewActivatedTab,
            this,  static_cast<void (DolphinTabWidget::*)(int)>(&DolphinTabWidget::openNewActivatedTab));
    connect(tabBar, &DolphinTabBar::tabDropEvent,
            this, &DolphinTabWidget::tabDropEvent);
    connect(tabBar, &DolphinTabBar::tabDetachRequested,
            this, &DolphinTabWidget::detachTab);
    tabBar->hide();

    setTabBar(tabBar);
    setDocumentMode(true);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
}

DolphinTabPage* DolphinTabWidget::currentTabPage() const
{
    return tabPageAt(currentIndex());
}

DolphinTabPage* DolphinTabWidget::tabPageAt(const int index) const
{
    return static_cast<DolphinTabPage*>(widget(index));
}

void DolphinTabWidget::saveProperties(KConfigGroup& group) const
{
    const int tabCount = count();
    group.writeEntry("Tab Count", tabCount);
    group.writeEntry("Active Tab Index", currentIndex());

    for (int i = 0; i < tabCount; ++i) {
        const DolphinTabPage* tabPage = tabPageAt(i);
        group.writeEntry("Tab Data " % QString::number(i), tabPage->saveState());
    }
}

void DolphinTabWidget::readProperties(const KConfigGroup& group)
{
    const int tabCount = group.readEntry("Tab Count", 0);
    for (int i = 0; i < tabCount; ++i) {
        if (i >= count()) {
            openNewActivatedTab();
        }
        if (group.hasKey("Tab Data " % QString::number(i))) {
            // Tab state created with Dolphin > 4.14.x
            const QByteArray state = group.readEntry("Tab Data " % QString::number(i), QByteArray());
            tabPageAt(i)->restoreState(state);
        } else {
            // Tab state created with Dolphin <= 4.14.x
            const QByteArray state = group.readEntry("Tab " % QString::number(i), QByteArray());
            tabPageAt(i)->restoreStateV1(state);
        }
    }

    const int index = group.readEntry("Active Tab Index", 0);
    setCurrentIndex(index);
}

void DolphinTabWidget::refreshViews()
{
    const int tabCount = count();
    for (int i = 0; i < tabCount; ++i) {
        tabPageAt(i)->refreshViews();
    }
}

void DolphinTabWidget::openNewActivatedTab()
{
    const DolphinViewContainer* oldActiveViewContainer = currentTabPage()->activeViewContainer();
    Q_ASSERT(oldActiveViewContainer);

    const bool isUrlEditable = oldActiveViewContainer->urlNavigator()->isUrlEditable();

    openNewActivatedTab(oldActiveViewContainer->url());

    DolphinViewContainer* newActiveViewContainer = currentTabPage()->activeViewContainer();
    Q_ASSERT(newActiveViewContainer);

    // The URL navigator of the new tab should have the same editable state
    // as the current tab
    KUrlNavigator* navigator = newActiveViewContainer->urlNavigator();
    navigator->setUrlEditable(isUrlEditable);

    if (isUrlEditable) {
        // If a new tab is opened and the URL is editable, assure that
        // the user can edit the URL without manually setting the focus
        navigator->setFocus();
    }
}

void DolphinTabWidget::openNewActivatedTab(const QUrl& primaryUrl, const QUrl& secondaryUrl)
{
    openNewTab(primaryUrl, secondaryUrl);
    setCurrentIndex(count() - 1);
}

void DolphinTabWidget::openNewTab(const QUrl& primaryUrl, const QUrl& secondaryUrl)
{
    QWidget* focusWidget = QApplication::focusWidget();

    DolphinTabPage* tabPage = new DolphinTabPage(primaryUrl, secondaryUrl, this);
    tabPage->setPlacesSelectorVisible(m_placesSelectorVisible);
    connect(tabPage, &DolphinTabPage::activeViewChanged,
            this, &DolphinTabWidget::activeViewChanged);
    connect(tabPage, &DolphinTabPage::activeViewUrlChanged,
            this, &DolphinTabWidget::tabUrlChanged);
    addTab(tabPage, QIcon::fromTheme(KIO::iconNameForUrl(primaryUrl)), tabName(primaryUrl));

    if (focusWidget) {
        // The DolphinViewContainer grabbed the keyboard focus. As the tab is opened
        // in background, assure that the previous focused widget gets the focus back.
        focusWidget->setFocus();
    }
}

void DolphinTabWidget::openDirectories(const QList<QUrl>& dirs, bool splitView)
{
    Q_ASSERT(dirs.size() > 0);

    QList<QUrl>::const_iterator it = dirs.constBegin();
    while (it != dirs.constEnd()) {
        const QUrl& primaryUrl = *(it++);
        if (splitView && (it != dirs.constEnd())) {
            const QUrl& secondaryUrl = *(it++);
            openNewTab(primaryUrl, secondaryUrl);
        } else {
            openNewTab(primaryUrl);
        }
    }
}

void DolphinTabWidget::openFiles(const QList<QUrl>& files, bool splitView)
{
    Q_ASSERT(files.size() > 0);

    // Get all distinct directories from 'files' and open a tab
    // for each directory. If the "split view" option is enabled, two
    // directories are shown inside one tab (see openDirectories()).
    QList<QUrl> dirs;
    foreach (const QUrl& url, files) {
        const QUrl dir(url.adjusted(QUrl::RemoveFilename));
        if (!dirs.contains(dir)) {
            dirs.append(dir);
        }
    }

    const int oldTabCount = count();
    openDirectories(dirs, splitView);
    const int tabCount = count();

    // Select the files. Although the files can be split between several
    // tabs, there is no need to split 'files' accordingly, as
    // the DolphinView will just ignore invalid selections.
    for (int i = oldTabCount; i < tabCount; ++i) {
        DolphinTabPage* tabPage = tabPageAt(i);
        tabPage->markUrlsAsSelected(files);
        tabPage->markUrlAsCurrent(files.first());
    }
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
    emit rememberClosedTab(tabPage->activeViewContainer()->url(), tabPage->saveState());

    removeTab(index);
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
    setCurrentIndex(index >= 0 ? index : (count() - 1));
}

void DolphinTabWidget::slotPlacesPanelVisibilityChanged(bool visible)
{
    // The places-selector from the URL navigator should only be shown
    // if the places dock is invisible
    m_placesSelectorVisible = !visible;

    const int tabCount = count();
    for (int i = 0; i < tabCount; ++i) {
        DolphinTabPage* tabPage = tabPageAt(i);
        tabPage->setPlacesSelectorVisible(m_placesSelectorVisible);
    }
}

void DolphinTabWidget::restoreClosedTab(const QByteArray& state)
{
    openNewActivatedTab();
    currentTabPage()->restoreState(state);
}

void DolphinTabWidget::detachTab(int index)
{
    Q_ASSERT(index >= 0);

    QStringList args;

    const DolphinTabPage* tabPage = tabPageAt(index);
    args << tabPage->primaryViewContainer()->url().url();
    if (tabPage->splitViewEnabled()) {
        args << tabPage->secondaryViewContainer()->url().url();
        args << QStringLiteral("--split");
    }

    const QString command = QStringLiteral("dolphin %1").arg(KShell::joinArgs(args));
    KRun::runCommand(command, this);

    closeTab(index);
}

void DolphinTabWidget::openNewActivatedTab(int index)
{
    Q_ASSERT(index >= 0);
    const DolphinTabPage* tabPage = tabPageAt(index);
    openNewActivatedTab(tabPage->activeViewContainer()->url());
}

void DolphinTabWidget::tabDropEvent(int index, QDropEvent* event)
{
    if (index >= 0) {
        DolphinView* view = tabPageAt(index)->activeViewContainer()->view();
        view->dropUrls(view->url(), event, view);
    }
}

void DolphinTabWidget::tabUrlChanged(const QUrl& url)
{
    const int index = indexOf(qobject_cast<QWidget*>(sender()));
    if (index >= 0) {
        tabBar()->setTabText(index, tabName(url));
        tabBar()->setTabIcon(index, QIcon::fromTheme(KIO::iconNameForUrl(url)));

        // Emit the currentUrlChanged signal if the url of the current tab has been changed.
        if (index == currentIndex()) {
            emit currentUrlChanged(url);
        }
    }
}

void DolphinTabWidget::currentTabChanged(int index)
{
    // previous tab deactivation
    if (DolphinTabPage* tabPage = tabPageAt(m_previousTab)) {
        tabPage->setActive(false);
    }
    DolphinTabPage* tabPage = tabPageAt(index);
    DolphinViewContainer* viewContainer = tabPage->activeViewContainer();
    emit activeViewChanged(viewContainer);
    emit currentUrlChanged(viewContainer->url());
    tabPage->setActive(true);
    m_previousTab = index;
}

void DolphinTabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);

    if (count() > 1) {
        tabBar()->show();
    }

    emit tabCountChanged(count());
}

void DolphinTabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);

    // If only one tab is left, then remove the tab entry so that
    // closing the last tab is not possible.
    if (count() < 2) {
        tabBar()->hide();
    }

    emit tabCountChanged(count());
}

QString DolphinTabWidget::tabName(const QUrl& url) const
{
    QString name;
    if (url == QUrl(QStringLiteral("file:///"))) {
        name = '/';
    } else {
        name = url.adjusted(QUrl::StripTrailingSlash).fileName();
        if (name.isEmpty()) {
            name = url.scheme();
        } else {
            // Make sure that a '&' inside the directory name is displayed correctly
            // and not misinterpreted as a keyboard shortcut in QTabBar::setTabText()
            name.replace('&', QLatin1String("&&"));
        }
    }
    return name;
}
