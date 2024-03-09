/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintabwidget.h"

#include "dolphin_generalsettings.h"
#include "dolphintabbar.h"
#include "dolphinviewcontainer.h"

#include <KAcceleratorManager>
#include <KConfigGroup>
#include <KIO/CommandLauncherJob>
#include <KLocalizedString>
#include <KShell>
#include <KStringHandler>
#include <kio/global.h>

#include <QApplication>
#include <QDropEvent>

DolphinTabWidget::DolphinTabWidget(DolphinNavigatorsWidgetAction *navigatorsWidget, QWidget *parent)
    : QTabWidget(parent)
    , m_dragAndDropHelper{this}
    , m_lastViewedTab(nullptr)
    , m_navigatorsWidget{navigatorsWidget}
{
    KAcceleratorManager::setNoAccel(this);

    connect(this, &DolphinTabWidget::tabCloseRequested, this, QOverload<int>::of(&DolphinTabWidget::closeTab));
    connect(this, &DolphinTabWidget::currentChanged, this, &DolphinTabWidget::currentTabChanged);

    DolphinTabBar *tabBar = new DolphinTabBar(this);
    connect(tabBar, &DolphinTabBar::openNewActivatedTab, this, QOverload<int>::of(&DolphinTabWidget::openNewActivatedTab));
    connect(tabBar, &DolphinTabBar::tabDragMoveEvent, this, &DolphinTabWidget::tabDragMoveEvent);
    connect(tabBar, &DolphinTabBar::tabDropEvent, this, &DolphinTabWidget::tabDropEvent);
    connect(tabBar, &DolphinTabBar::tabDetachRequested, this, &DolphinTabWidget::detachTab);

    setTabBar(tabBar);
    setDocumentMode(true);
    setElideMode(Qt::ElideRight);
    setUsesScrollButtons(true);
    setTabBarAutoHide(true);
}

DolphinTabPage *DolphinTabWidget::currentTabPage() const
{
    return tabPageAt(currentIndex());
}

DolphinTabPage *DolphinTabWidget::nextTabPage() const
{
    const int index = currentIndex() + 1;
    return tabPageAt(index < count() ? index : 0);
}

DolphinTabPage *DolphinTabWidget::prevTabPage() const
{
    const int index = currentIndex() - 1;
    return tabPageAt(index >= 0 ? index : (count() - 1));
}

DolphinTabPage *DolphinTabWidget::tabPageAt(const int index) const
{
    return static_cast<DolphinTabPage *>(widget(index));
}

void DolphinTabWidget::saveProperties(KConfigGroup &group) const
{
    const int tabCount = count();
    group.writeEntry("Tab Count", tabCount);
    group.writeEntry("Active Tab Index", currentIndex());

    for (int i = 0; i < tabCount; ++i) {
        const DolphinTabPage *tabPage = tabPageAt(i);
        group.writeEntry("Tab Data " % QString::number(i), tabPage->saveState());
    }
}

void DolphinTabWidget::readProperties(const KConfigGroup &group)
{
    const int tabCount = group.readEntry("Tab Count", 0);
    for (int i = 0; i < tabCount; ++i) {
        if (i >= count()) {
            openNewActivatedTab();
        }
        const QByteArray state = group.readEntry("Tab Data " % QString::number(i), QByteArray());
        tabPageAt(i)->restoreState(state);
    }

    const int index = group.readEntry("Active Tab Index", 0);
    setCurrentIndex(index);
}

void DolphinTabWidget::refreshViews()
{
    // Left-elision is better when showing full paths, since you care most
    // about the current directory which is on the right
    if (GeneralSettings::showFullPathInTitlebar()) {
        setElideMode(Qt::ElideLeft);
    } else {
        setElideMode(Qt::ElideRight);
    }

    const int tabCount = count();
    for (int i = 0; i < tabCount; ++i) {
        updateTabName(i);
        tabPageAt(i)->refreshViews();
    }
}

void DolphinTabWidget::updateTabName(int index)
{
    Q_ASSERT(index >= 0);
    tabBar()->setTabText(index, tabName(tabPageAt(index)));
}

bool DolphinTabWidget::isUrlOpen(const QUrl &url) const
{
    return viewOpenAtDirectory(url).has_value();
}

bool DolphinTabWidget::isItemVisibleInAnyView(const QUrl &urlOfItem) const
{
    return viewShowingItem(urlOfItem).has_value();
}

void DolphinTabWidget::openNewActivatedTab()
{
    std::unique_ptr<DolphinUrlNavigator::VisualState> oldNavigatorState;
    if (currentTabPage()->primaryViewActive() || !m_navigatorsWidget->secondaryUrlNavigator()) {
        oldNavigatorState = m_navigatorsWidget->primaryUrlNavigator()->visualState();
    } else {
        oldNavigatorState = m_navigatorsWidget->secondaryUrlNavigator()->visualState();
    }

    const DolphinViewContainer *oldActiveViewContainer = currentTabPage()->activeViewContainer();
    Q_ASSERT(oldActiveViewContainer);

    openNewActivatedTab(oldActiveViewContainer->url());

    DolphinViewContainer *newActiveViewContainer = currentTabPage()->activeViewContainer();
    Q_ASSERT(newActiveViewContainer);

    // The URL navigator of the new tab should have the same editable state
    // as the current tab
    newActiveViewContainer->urlNavigator()->setVisualState(*oldNavigatorState.get());

    // Always focus the new tab's view
    newActiveViewContainer->view()->setFocus();
}

void DolphinTabWidget::openNewActivatedTab(const QUrl &primaryUrl, const QUrl &secondaryUrl)
{
    openNewTab(primaryUrl, secondaryUrl);
    if (GeneralSettings::openNewTabAfterLastTab()) {
        setCurrentIndex(count() - 1);
    } else {
        setCurrentIndex(currentIndex() + 1);
    }
}

void DolphinTabWidget::openNewTab(const QUrl &primaryUrl, const QUrl &secondaryUrl, DolphinTabWidget::NewTabPosition position)
{
    QWidget *focusWidget = QApplication::focusWidget();

    DolphinTabPage *tabPage = new DolphinTabPage(primaryUrl, secondaryUrl, this);
    tabPage->setActive(false);
    connect(tabPage, &DolphinTabPage::activeViewChanged, this, &DolphinTabWidget::activeViewChanged);
    connect(tabPage, &DolphinTabPage::activeViewUrlChanged, this, &DolphinTabWidget::tabUrlChanged);
    connect(tabPage->activeViewContainer(), &DolphinViewContainer::captionChanged, this, [this, tabPage]() {
        updateTabName(indexOf(tabPage));
    });

    if (position == NewTabPosition::FollowSetting) {
        if (GeneralSettings::openNewTabAfterLastTab()) {
            position = NewTabPosition::AtEnd;
        } else {
            position = NewTabPosition::AfterCurrent;
        }
    }

    int newTabIndex = -1;
    if (position == NewTabPosition::AfterCurrent || (position == NewTabPosition::FollowSetting && !GeneralSettings::openNewTabAfterLastTab())) {
        newTabIndex = currentIndex() + 1;
    }

    insertTab(newTabIndex, tabPage, QIcon() /* loaded in tabInserted */, tabName(tabPage));

    if (focusWidget) {
        // The DolphinViewContainer grabbed the keyboard focus. As the tab is opened
        // in background, assure that the previous focused widget gets the focus back.
        focusWidget->setFocus();
    }
}

void DolphinTabWidget::openDirectories(const QList<QUrl> &dirs, bool splitView)
{
    Q_ASSERT(dirs.size() > 0);

    bool somethingWasAlreadyOpen = false;

    QList<QUrl>::const_iterator it = dirs.constBegin();
    while (it != dirs.constEnd()) {
        const QUrl &primaryUrl = *(it++);
        const std::optional<ViewIndex> viewIndexAtDirectory = viewOpenAtDirectory(primaryUrl);

        // When the user asks for a URL that's already open,
        // activate it instead of opening a new tab
        if (viewIndexAtDirectory.has_value()) {
            somethingWasAlreadyOpen = true;
            activateViewContainerAt(viewIndexAtDirectory.value());
        } else if (splitView && (it != dirs.constEnd())) {
            const QUrl &secondaryUrl = *(it++);
            if (somethingWasAlreadyOpen) {
                openNewTab(primaryUrl, secondaryUrl);
            } else {
                openNewActivatedTab(primaryUrl, secondaryUrl);
            }
        } else {
            if (somethingWasAlreadyOpen) {
                openNewTab(primaryUrl);
            } else {
                openNewActivatedTab(primaryUrl);
            }
        }
    }
}

void DolphinTabWidget::openFiles(const QList<QUrl> &files, bool splitView)
{
    Q_ASSERT(files.size() > 0);

    // Get all distinct directories from 'files'.
    QList<QUrl> dirsThatNeedToBeOpened;
    QList<QUrl> dirsThatWereAlreadyOpen;
    for (const QUrl &file : files) {
        const QUrl dir(file.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash));
        if (dirsThatNeedToBeOpened.contains(dir) || dirsThatWereAlreadyOpen.contains(dir)) {
            continue;
        }

        // The selecting of files that we do later will not work in views that already have items selected.
        // So we check if dir is already open and clear the selection if it is. BUG: 417230
        // We also make sure the view will be activated.
        auto viewIndex = viewShowingItem(file);
        if (viewIndex.has_value()) {
            viewContainerAt(viewIndex.value())->view()->clearSelection();
            activateViewContainerAt(viewIndex.value());
            dirsThatWereAlreadyOpen.append(dir);
        } else {
            dirsThatNeedToBeOpened.append(dir);
        }
    }

    const int oldTabCount = count();
    // Open a tab for each directory. If the "split view" option is enabled,
    // two directories are shown inside one tab (see openDirectories()).
    if (dirsThatNeedToBeOpened.size() > 0) {
        openDirectories(dirsThatNeedToBeOpened, splitView);
    }
    const int tabCount = count();

    // Select the files. Although the files can be split between several
    // tabs, there is no need to split 'files' accordingly, as
    // the DolphinView will just ignore invalid selections.
    for (int i = 0; i < tabCount; ++i) {
        DolphinTabPage *tabPage = tabPageAt(i);
        tabPage->markUrlsAsSelected(files);
        tabPage->markUrlAsCurrent(files.first());
        if (i < oldTabCount) {
            // Force selection of file if directory was already open, BUG: 417230
            tabPage->activeViewContainer()->view()->updateViewState();
        }
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
        // Close Dolphin when closing the last tab.
        parentWidget()->close();
        return;
    }

    DolphinTabPage *tabPage = tabPageAt(index);
    Q_EMIT rememberClosedTab(tabPage->activeViewContainer()->url(), tabPage->saveState());

    removeTab(index);
    tabPage->deleteLater();
}

void DolphinTabWidget::activateTab(const int index)
{
    if (index < count()) {
        setCurrentIndex(index);
    }
}

void DolphinTabWidget::activateLastTab()
{
    setCurrentIndex(count() - 1);
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

void DolphinTabWidget::restoreClosedTab(const QByteArray &state)
{
    openNewActivatedTab();
    currentTabPage()->restoreState(state);
}

void DolphinTabWidget::copyToInactiveSplitView()
{
    const DolphinTabPage *tabPage = currentTabPage();
    if (!tabPage->splitViewEnabled()) {
        return;
    }

    const KFileItemList selectedItems = tabPage->activeViewContainer()->view()->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    DolphinView *const inactiveView = tabPage->inactiveViewContainer()->view();
    inactiveView->copySelectedItems(selectedItems, inactiveView->url());
}

void DolphinTabWidget::moveToInactiveSplitView()
{
    const DolphinTabPage *tabPage = currentTabPage();
    if (!tabPage->splitViewEnabled()) {
        return;
    }

    const KFileItemList selectedItems = tabPage->activeViewContainer()->view()->selectedItems();
    if (selectedItems.isEmpty()) {
        return;
    }

    DolphinView *const inactiveView = tabPage->inactiveViewContainer()->view();
    inactiveView->moveSelectedItems(selectedItems, inactiveView->url());
}

void DolphinTabWidget::detachTab(int index)
{
    Q_ASSERT(index >= 0);

    QStringList args;

    const DolphinTabPage *tabPage = tabPageAt(index);
    args << tabPage->primaryViewContainer()->url().url();
    if (tabPage->splitViewEnabled()) {
        args << tabPage->secondaryViewContainer()->url().url();
        args << QStringLiteral("--split");
    }
    args << QStringLiteral("--new-window");

    KIO::CommandLauncherJob *job = new KIO::CommandLauncherJob("dolphin", args, this);
    job->setDesktopName(QStringLiteral("org.kde.dolphin"));
    job->start();

    closeTab(index);
}

void DolphinTabWidget::openNewActivatedTab(int index)
{
    Q_ASSERT(index >= 0);
    const DolphinTabPage *tabPage = tabPageAt(index);
    openNewActivatedTab(tabPage->activeViewContainer()->url());
}

void DolphinTabWidget::tabDragMoveEvent(int index, QDragMoveEvent *event)
{
    if (index >= 0) {
        DolphinView *view = tabPageAt(index)->activeViewContainer()->view();
        m_dragAndDropHelper.updateDropAction(event, view->url());
    }
}

void DolphinTabWidget::tabDropEvent(int index, QDropEvent *event)
{
    if (index >= 0) {
        DolphinView *view = tabPageAt(index)->activeViewContainer()->view();
        view->dropUrls(view->url(), event, view);
    } else {
        const auto urls = event->mimeData()->urls();

        for (const QUrl &url : urls) {
            auto *job = KIO::stat(url, KIO::StatJob::SourceSide, KIO::StatDetail::StatBasic, KIO::JobFlag::HideProgressInfo);
            connect(job, &KJob::result, this, [this, job]() {
                if (!job->error() && job->statResult().isDir()) {
                    openNewTab(job->url(), QUrl(), NewTabPosition::AtEnd);
                }
            });
        }
    }
}

void DolphinTabWidget::tabUrlChanged(const QUrl &url)
{
    const int index = indexOf(qobject_cast<QWidget *>(sender()));
    if (index >= 0) {
        updateTabName(index);
        tabBar()->setTabToolTip(index, url.toDisplayString(QUrl::PreferLocalFile));
        if (tabBar()->isVisible()) {
            // ensure the path url ends with a slash to have proper folder icon for remote folders
            const QUrl pathUrl = QUrl(url.adjusted(QUrl::StripTrailingSlash).toString(QUrl::FullyEncoded).append("/"));
            tabBar()->setTabIcon(index, QIcon::fromTheme(KIO::iconNameForUrl(pathUrl)));
        } else {
            // Mark as dirty, actually load once the tab bar actually gets shown
            tabBar()->setTabIcon(index, QIcon());
        }

        // Emit the currentUrlChanged signal if the url of the current tab has been changed.
        if (index == currentIndex()) {
            Q_EMIT currentUrlChanged(url);
        }

        Q_EMIT urlChanged(url);
    }
}

void DolphinTabWidget::currentTabChanged(int index)
{
    DolphinTabPage *tabPage = tabPageAt(index);
    if (tabPage == m_lastViewedTab) {
        return;
    }
    if (m_lastViewedTab) {
        m_lastViewedTab->disconnectNavigators();
        m_lastViewedTab->setActive(false);
    }
    if (tabPage->splitViewEnabled() && !m_navigatorsWidget->secondaryUrlNavigator()) {
        m_navigatorsWidget->createSecondaryUrlNavigator();
    }
    DolphinViewContainer *viewContainer = tabPage->activeViewContainer();
    Q_EMIT activeViewChanged(viewContainer);
    Q_EMIT currentUrlChanged(viewContainer->url());
    tabPage->setActive(true);
    tabPage->connectNavigators(m_navigatorsWidget);
    m_navigatorsWidget->setSecondaryNavigatorVisible(tabPage->splitViewEnabled());
    m_lastViewedTab = tabPage;
}

void DolphinTabWidget::tabInserted(int index)
{
    QTabWidget::tabInserted(index);

    if (tabBar()->isVisible()) {
        // Resolve all pending tab icons
        for (int i = 0; i < count(); ++i) {
            const QUrl url = tabPageAt(i)->activeViewContainer()->url();
            if (tabBar()->tabIcon(i).isNull()) {
                // ensure the path url ends with a slash to have proper folder icon for remote folders
                const QUrl pathUrl = QUrl(url.adjusted(QUrl::StripTrailingSlash).toString(QUrl::FullyEncoded).append("/"));
                tabBar()->setTabIcon(i, QIcon::fromTheme(KIO::iconNameForUrl(pathUrl)));
            }
            if (tabBar()->tabToolTip(i).isEmpty()) {
                tabBar()->setTabToolTip(index, url.toDisplayString(QUrl::PreferLocalFile));
            }
        }
    }

    Q_EMIT tabCountChanged(count());
}

void DolphinTabWidget::tabRemoved(int index)
{
    QTabWidget::tabRemoved(index);

    Q_EMIT tabCountChanged(count());
}

QString DolphinTabWidget::tabName(DolphinTabPage *tabPage) const
{
    if (!tabPage) {
        return QString();
    }
    // clang-format off
    QString name;
    if (tabPage->splitViewEnabled()) {
        if (tabPage->primaryViewActive()) {
            // i18n: %1 is the primary view and %2 the secondary view. For left to right languages the primary view is on the left so we also want it to be on the
            // left in the tab name. In right to left languages the primary view would be on the right so the tab name should match.
            name = i18nc("@title:tab Active primary view | (Inactive secondary view)", "%1 | (%2)", tabPage->primaryViewContainer()->caption(), tabPage->secondaryViewContainer()->caption());
        } else {
            // i18n: %1 is the primary view and %2 the secondary view. For left to right languages the primary view is on the left so we also want it to be on the
            // left in the tab name. In right to left languages the primary view would be on the right so the tab name should match.
            name = i18nc("@title:tab (Inactive primary view) | Active secondary view", "(%1) | %2", tabPage->primaryViewContainer()->caption(), tabPage->secondaryViewContainer()->caption());
        }
    } else {
        name = tabPage->activeViewContainer()->caption();
    }
    // clang-format on

    // Make sure that a '&' inside the directory name is displayed correctly
    // and not misinterpreted as a keyboard shortcut in QTabBar::setTabText()
    return KStringHandler::rsqueeze(name.replace('&', QLatin1String("&&")), 40 /* default maximum visible folder name visible */);
}

DolphinViewContainer *DolphinTabWidget::viewContainerAt(DolphinTabWidget::ViewIndex viewIndex) const
{
    const auto tabPage = tabPageAt(viewIndex.tabIndex);
    if (!tabPage) {
        return nullptr;
    }
    return viewIndex.isInPrimaryView ? tabPage->primaryViewContainer() : tabPage->secondaryViewContainer();
}

DolphinViewContainer *DolphinTabWidget::activateViewContainerAt(DolphinTabWidget::ViewIndex viewIndex)
{
    activateTab(viewIndex.tabIndex);
    auto viewContainer = viewContainerAt(viewIndex);
    if (!viewContainer) {
        return nullptr;
    }
    viewContainer->setActive(true);
    return viewContainer;
}

const std::optional<const DolphinTabWidget::ViewIndex> DolphinTabWidget::viewOpenAtDirectory(const QUrl &directory) const
{
    int i = currentIndex();
    if (i < 0) {
        return std::nullopt;
    }
    // loop over the tabs starting from the current one
    do {
        const auto tabPage = tabPageAt(i);
        if (tabPage->primaryViewContainer()->url() == directory) {
            return std::optional(ViewIndex{i, true});
        }

        if (tabPage->splitViewEnabled() && tabPage->secondaryViewContainer()->url() == directory) {
            return std::optional(ViewIndex{i, false});
        }

        i = (i + 1) % count();
    } while (i != currentIndex());

    return std::nullopt;
}

const std::optional<const DolphinTabWidget::ViewIndex> DolphinTabWidget::viewShowingItem(const QUrl &item) const
{
    // The item might not be loaded yet even though it exists. So instead
    // we check if the folder containing the item is showing its contents.
    const QUrl dirContainingItem(item.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash));

    // The dirContainingItem is either open directly or expanded in a tree-style view mode.
    // Is dirContainingitem the base url of a view?
    auto viewOpenAtContainingDirectory = viewOpenAtDirectory(dirContainingItem);
    if (viewOpenAtContainingDirectory.has_value()) {
        return viewOpenAtContainingDirectory;
    }

    // Is dirContainingItem expanded in some tree-style view?
    // The rest of this method is about figuring this out.

    int i = currentIndex();
    if (i < 0) {
        return std::nullopt;
    }
    // loop over the tabs starting from the current one
    do {
        const auto tabPage = tabPageAt(i);
        if (tabPage->primaryViewContainer()->url().isParentOf(item)) {
            const KFileItem fileItemContainingItem = tabPage->primaryViewContainer()->view()->items().findByUrl(dirContainingItem);
            if (!fileItemContainingItem.isNull() && tabPage->primaryViewContainer()->view()->isExpanded(fileItemContainingItem)) {
                return std::optional(ViewIndex{i, true});
            }
        }

        if (tabPage->splitViewEnabled() && tabPage->secondaryViewContainer()->url().isParentOf(item)) {
            const KFileItem fileItemContainingItem = tabPage->secondaryViewContainer()->view()->items().findByUrl(dirContainingItem);
            if (!fileItemContainingItem.isNull() && tabPage->secondaryViewContainer()->view()->isExpanded(fileItemContainingItem)) {
                return std::optional(ViewIndex{i, false});
            }
        }

        i = (i + 1) % count();
    } while (i != currentIndex());

    return std::nullopt;
}

#include "moc_dolphintabwidget.cpp"
