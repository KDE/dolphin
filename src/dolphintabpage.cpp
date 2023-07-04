/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 * SPDX-FileCopyrightText: 2020 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintabpage.h"

#include "dolphin_generalsettings.h"
#include "dolphinviewcontainer.h"

#include <QGridLayout>
#include <QStyle>
#include <QVariantAnimation>

DolphinTabPage::DolphinTabPage(const QUrl &primaryUrl, const QUrl &secondaryUrl, QWidget *parent)
    : QWidget(parent)
    , m_expandingContainer{nullptr}
    , m_primaryViewActive(true)
    , m_splitViewEnabled(false)
    , m_active(true)
{
    QGridLayout *layout = new QGridLayout(this);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);

    m_splitter = new DolphinTabPageSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    connect(m_splitter, &QSplitter::splitterMoved, this, &DolphinTabPage::splitterMoved);
    layout->addWidget(m_splitter, 1, 0);
    layout->setRowStretch(1, 1);

    // Create a new primary view
    m_primaryViewContainer = createViewContainer(primaryUrl);
    connect(m_primaryViewContainer->view(), &DolphinView::urlChanged, this, &DolphinTabPage::activeViewUrlChanged);
    connect(m_primaryViewContainer->view(), &DolphinView::redirection, this, &DolphinTabPage::slotViewUrlRedirection);

    m_splitter->addWidget(m_primaryViewContainer);
    m_primaryViewContainer->show();

    if (secondaryUrl.isValid() || GeneralSettings::splitView()) {
        // Provide a secondary view, if the given secondary url is valid or if the
        // startup settings are set this way (use the url of the primary view).
        m_splitViewEnabled = true;
        const QUrl &url = secondaryUrl.isValid() ? secondaryUrl : primaryUrl;
        m_secondaryViewContainer = createViewContainer(url);
        m_splitter->addWidget(m_secondaryViewContainer);
        m_secondaryViewContainer->show();
    }

    m_primaryViewContainer->setActive(true);
}

bool DolphinTabPage::primaryViewActive() const
{
    return m_primaryViewActive;
}

bool DolphinTabPage::splitViewEnabled() const
{
    return m_splitViewEnabled;
}

void DolphinTabPage::setSplitViewEnabled(bool enabled, Animated animated, const QUrl &secondaryUrl)
{
    if (m_splitViewEnabled != enabled) {
        m_splitViewEnabled = enabled;
        if (animated == WithAnimation
            && (style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) < 1 || GlobalConfig::animationDurationFactor() <= 0.0)) {
            animated = WithoutAnimation;
        }
        if (m_expandViewAnimation) {
            m_expandViewAnimation->stop(); // deletes because of QAbstractAnimation::DeleteWhenStopped.
            if (animated == WithoutAnimation) {
                slotAnimationFinished();
            }
        }

        if (enabled) {
            QList<int> splitterSizes = m_splitter->sizes();
            const QUrl &url = (secondaryUrl.isEmpty()) ? m_primaryViewContainer->url() : secondaryUrl;
            m_secondaryViewContainer = createViewContainer(url);

            auto secondaryNavigator = m_navigatorsWidget->secondaryUrlNavigator();
            if (!secondaryNavigator) {
                m_navigatorsWidget->createSecondaryUrlNavigator();
                secondaryNavigator = m_navigatorsWidget->secondaryUrlNavigator();
            }
            m_secondaryViewContainer->connectUrlNavigator(secondaryNavigator);
            m_navigatorsWidget->setSecondaryNavigatorVisible(true);
            m_navigatorsWidget->followViewContainersGeometry(m_primaryViewContainer, m_secondaryViewContainer);

            m_splitter->addWidget(m_secondaryViewContainer);
            m_secondaryViewContainer->setActive(true);

            if (animated == WithAnimation) {
                m_secondaryViewContainer->setMinimumWidth(1);
                splitterSizes.append(1);
                m_splitter->setSizes(splitterSizes);
                startExpandViewAnimation(m_secondaryViewContainer);
            }
            m_secondaryViewContainer->show();
        } else {
            m_navigatorsWidget->setSecondaryNavigatorVisible(false);
            m_secondaryViewContainer->disconnectUrlNavigator();

            DolphinViewContainer *view;
            if (GeneralSettings::closeActiveSplitView()) {
                view = activeViewContainer();
                if (m_primaryViewActive) {
                    m_primaryViewContainer->disconnectUrlNavigator();
                    m_secondaryViewContainer->connectUrlNavigator(m_navigatorsWidget->primaryUrlNavigator());

                    // If the primary view is active, we have to swap the pointers
                    // because the secondary view will be the new primary view.
                    std::swap(m_primaryViewContainer, m_secondaryViewContainer);
                    m_primaryViewActive = false;
                }
            } else {
                view = m_primaryViewActive ? m_secondaryViewContainer : m_primaryViewContainer;
                if (!m_primaryViewActive) {
                    m_primaryViewContainer->disconnectUrlNavigator();
                    m_secondaryViewContainer->connectUrlNavigator(m_navigatorsWidget->primaryUrlNavigator());

                    // If the secondary view is active, we have to swap the pointers
                    // because the secondary view will be the new primary view.
                    std::swap(m_primaryViewContainer, m_secondaryViewContainer);
                    m_primaryViewActive = true;
                }
            }
            m_primaryViewContainer->setActive(true);
            m_navigatorsWidget->followViewContainersGeometry(m_primaryViewContainer, nullptr);

            if (animated == WithoutAnimation) {
                view->close();
                view->deleteLater();
            } else {
                // Kill it but keep it as a zombie for the closing animation.
                m_secondaryViewContainer = nullptr;
                view->blockSignals(true);
                view->view()->blockSignals(true);
                view->setDisabled(true);
                startExpandViewAnimation(m_primaryViewContainer);
            }

            m_primaryViewContainer->slotSplitTabDisabled();
        }
    }
}

DolphinViewContainer *DolphinTabPage::primaryViewContainer() const
{
    return m_primaryViewContainer;
}

DolphinViewContainer *DolphinTabPage::secondaryViewContainer() const
{
    return m_secondaryViewContainer;
}

DolphinViewContainer *DolphinTabPage::activeViewContainer() const
{
    return m_primaryViewActive ? m_primaryViewContainer : m_secondaryViewContainer;
}

DolphinViewContainer *DolphinTabPage::inactiveViewContainer() const
{
    if (!splitViewEnabled()) {
        return nullptr;
    }

    return primaryViewActive() ? secondaryViewContainer() : primaryViewContainer();
}

KFileItemList DolphinTabPage::selectedItems() const
{
    KFileItemList items = m_primaryViewContainer->view()->selectedItems();
    if (m_splitViewEnabled) {
        items += m_secondaryViewContainer->view()->selectedItems();
    }
    return items;
}

int DolphinTabPage::selectedItemsCount() const
{
    int selectedItemsCount = m_primaryViewContainer->view()->selectedItemsCount();
    if (m_splitViewEnabled) {
        selectedItemsCount += m_secondaryViewContainer->view()->selectedItemsCount();
    }
    return selectedItemsCount;
}

void DolphinTabPage::connectNavigators(DolphinNavigatorsWidgetAction *navigatorsWidget)
{
    insertNavigatorsWidget(navigatorsWidget);
    m_navigatorsWidget = navigatorsWidget;
    auto primaryNavigator = navigatorsWidget->primaryUrlNavigator();
    m_primaryViewContainer->connectUrlNavigator(primaryNavigator);
    if (m_splitViewEnabled) {
        auto secondaryNavigator = navigatorsWidget->secondaryUrlNavigator();
        m_secondaryViewContainer->connectUrlNavigator(secondaryNavigator);
    }
    m_navigatorsWidget->followViewContainersGeometry(m_primaryViewContainer, m_secondaryViewContainer);
}

void DolphinTabPage::disconnectNavigators()
{
    m_navigatorsWidget = nullptr;
    m_primaryViewContainer->disconnectUrlNavigator();
    if (m_splitViewEnabled) {
        m_secondaryViewContainer->disconnectUrlNavigator();
    }
}

void DolphinTabPage::insertNavigatorsWidget(DolphinNavigatorsWidgetAction *navigatorsWidget)
{
    QGridLayout *gridLayout = static_cast<QGridLayout *>(layout());
    if (navigatorsWidget->isInToolbar()) {
        gridLayout->setRowMinimumHeight(0, 0);
    } else {
        // We set a row minimum height, so the height does not visibly change whenever
        // navigatorsWidget is inserted which happens every time the current tab is changed.
        gridLayout->setRowMinimumHeight(0, navigatorsWidget->primaryUrlNavigator()->height());
        gridLayout->addWidget(navigatorsWidget->requestWidget(this), 0, 0);
    }
}

void DolphinTabPage::markUrlsAsSelected(const QList<QUrl> &urls)
{
    m_primaryViewContainer->view()->markUrlsAsSelected(urls);
    if (m_splitViewEnabled) {
        m_secondaryViewContainer->view()->markUrlsAsSelected(urls);
    }
}

void DolphinTabPage::markUrlAsCurrent(const QUrl &url)
{
    m_primaryViewContainer->view()->markUrlAsCurrent(url);
    if (m_splitViewEnabled) {
        m_secondaryViewContainer->view()->markUrlAsCurrent(url);
    }
}

void DolphinTabPage::refreshViews()
{
    m_primaryViewContainer->readSettings();
    if (m_splitViewEnabled) {
        m_secondaryViewContainer->readSettings();
    }
}

QByteArray DolphinTabPage::saveState() const
{
    QByteArray state;
    QDataStream stream(&state, QIODevice::WriteOnly);

    stream << quint32(2); // Tab state version

    stream << m_splitViewEnabled;

    stream << m_primaryViewContainer->url();
    stream << m_primaryViewContainer->urlNavigatorInternalWithHistory()->isUrlEditable();
    m_primaryViewContainer->view()->saveState(stream);

    if (m_splitViewEnabled) {
        stream << m_secondaryViewContainer->url();
        stream << m_secondaryViewContainer->urlNavigatorInternalWithHistory()->isUrlEditable();
        m_secondaryViewContainer->view()->saveState(stream);
    }

    stream << m_primaryViewActive;
    stream << m_splitter->saveState();

    return state;
}

void DolphinTabPage::restoreState(const QByteArray &state)
{
    if (state.isEmpty()) {
        return;
    }

    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);

    // Read the version number of the tab state and check if the version is supported.
    quint32 version = 0;
    stream >> version;
    if (version != 2) {
        // The version of the tab state isn't supported, we can't restore it.
        return;
    }

    bool isSplitViewEnabled = false;
    stream >> isSplitViewEnabled;
    setSplitViewEnabled(isSplitViewEnabled, WithoutAnimation);

    QUrl primaryUrl;
    stream >> primaryUrl;
    m_primaryViewContainer->setUrl(primaryUrl);
    bool primaryUrlEditable;
    stream >> primaryUrlEditable;
    m_primaryViewContainer->urlNavigatorInternalWithHistory()->setUrlEditable(primaryUrlEditable);
    m_primaryViewContainer->view()->restoreState(stream);

    if (isSplitViewEnabled) {
        QUrl secondaryUrl;
        stream >> secondaryUrl;
        m_secondaryViewContainer->setUrl(secondaryUrl);
        bool secondaryUrlEditable;
        stream >> secondaryUrlEditable;
        m_secondaryViewContainer->urlNavigatorInternalWithHistory()->setUrlEditable(secondaryUrlEditable);
        m_secondaryViewContainer->view()->restoreState(stream);
    }

    stream >> m_primaryViewActive;
    if (m_primaryViewActive) {
        m_primaryViewContainer->setActive(true);
    } else {
        Q_ASSERT(m_splitViewEnabled);
        m_secondaryViewContainer->setActive(true);
    }

    QByteArray splitterState;
    stream >> splitterState;
    m_splitter->restoreState(splitterState);
}

void DolphinTabPage::setActive(bool active)
{
    if (active) {
        m_active = active;
    } else {
        // we should bypass changing active view in split mode
        m_active = !m_splitViewEnabled;
    }
    // we want view to fire activated when goes from false to true
    activeViewContainer()->setActive(active);
}

void DolphinTabPage::slotAnimationFinished()
{
    for (int i = 0; i < m_splitter->count(); ++i) {
        QWidget *viewContainer = m_splitter->widget(i);
        if (viewContainer != m_primaryViewContainer && viewContainer != m_secondaryViewContainer) {
            viewContainer->close();
            viewContainer->deleteLater();
        }
    }
    for (int i = 0; i < m_splitter->count(); ++i) {
        QWidget *viewContainer = m_splitter->widget(i);
        viewContainer->setMinimumWidth(viewContainer->minimumSizeHint().width());
    }
    m_expandingContainer = nullptr;
}

void DolphinTabPage::slotAnimationValueChanged(const QVariant &value)
{
    Q_CHECK_PTR(m_expandingContainer);
    const int indexOfExpandingContainer = m_splitter->indexOf(m_expandingContainer);
    int indexOfNonExpandingContainer = -1;
    if (m_expandingContainer == m_primaryViewContainer) {
        indexOfNonExpandingContainer = m_splitter->indexOf(m_secondaryViewContainer);
    } else {
        indexOfNonExpandingContainer = m_splitter->indexOf(m_primaryViewContainer);
    }
    std::vector<QWidget *> widgetsToRemove;
    const QList<int> oldSplitterSizes = m_splitter->sizes();
    QList<int> newSplitterSizes{oldSplitterSizes};
    int expansionWidthNeeded = value.toInt() - oldSplitterSizes.at(indexOfExpandingContainer);

    // Reduce the size of the other widgets to make space for the expandingContainer.
    for (int i = m_splitter->count() - 1; i >= 0; --i) {
        if (m_splitter->widget(i) == m_primaryViewContainer || m_splitter->widget(i) == m_secondaryViewContainer) {
            continue;
        }
        newSplitterSizes[i] = oldSplitterSizes.at(i) - expansionWidthNeeded;
        expansionWidthNeeded = 0;
        if (indexOfNonExpandingContainer != -1) {
            // Make sure every zombie container is at least slightly reduced in size
            // so it doesn't seem like they are here to stay.
            newSplitterSizes[i]--;
            newSplitterSizes[indexOfNonExpandingContainer]++;
        }
        if (newSplitterSizes.at(i) <= 0) {
            expansionWidthNeeded -= newSplitterSizes.at(i);
            newSplitterSizes[i] = 0;
            widgetsToRemove.emplace_back(m_splitter->widget(i));
        }
    }
    if (expansionWidthNeeded > 1 && indexOfNonExpandingContainer != -1) {
        Q_ASSERT(m_splitViewEnabled);
        newSplitterSizes[indexOfNonExpandingContainer] -= expansionWidthNeeded;
    }
    newSplitterSizes[indexOfExpandingContainer] = value.toInt();
    m_splitter->setSizes(newSplitterSizes);
    while (!widgetsToRemove.empty()) {
        widgetsToRemove.back()->close();
        widgetsToRemove.back()->deleteLater();
        widgetsToRemove.pop_back();
    }
}

void DolphinTabPage::slotViewActivated()
{
    const DolphinView *oldActiveView = activeViewContainer()->view();

    // Set the view, which was active before, to inactive
    // and update the active view type, if tab is active
    if (m_active) {
        if (m_splitViewEnabled) {
            activeViewContainer()->setActive(false);
            m_primaryViewActive = !m_primaryViewActive;
        } else {
            m_primaryViewActive = true;
            if (m_secondaryViewContainer) {
                m_secondaryViewContainer->setActive(false);
            }
        }
    }

    const DolphinView *newActiveView = activeViewContainer()->view();

    if (newActiveView == oldActiveView) {
        return;
    }

    disconnect(oldActiveView, &DolphinView::urlChanged, this, &DolphinTabPage::activeViewUrlChanged);
    disconnect(oldActiveView, &DolphinView::redirection, this, &DolphinTabPage::slotViewUrlRedirection);
    connect(newActiveView, &DolphinView::urlChanged, this, &DolphinTabPage::activeViewUrlChanged);
    connect(newActiveView, &DolphinView::redirection, this, &DolphinTabPage::slotViewUrlRedirection);
    Q_EMIT activeViewChanged(activeViewContainer());
    Q_EMIT activeViewUrlChanged(activeViewContainer()->url());
}

void DolphinTabPage::slotViewUrlRedirection(const QUrl &oldUrl, const QUrl &newUrl)
{
    Q_UNUSED(oldUrl)

    Q_EMIT activeViewUrlChanged(newUrl);
}

void DolphinTabPage::switchActiveView()
{
    if (!m_splitViewEnabled) {
        return;
    }
    if (m_primaryViewActive) {
        m_secondaryViewContainer->setActive(true);
    } else {
        m_primaryViewContainer->setActive(true);
    }
}

DolphinViewContainer *DolphinTabPage::createViewContainer(const QUrl &url) const
{
    DolphinViewContainer *container = new DolphinViewContainer(url, m_splitter);
    container->setActive(false);

    const DolphinView *view = container->view();
    connect(view, &DolphinView::activated, this, &DolphinTabPage::slotViewActivated);

    connect(view, &DolphinView::toggleActiveViewRequested, this, &DolphinTabPage::switchActiveView);

    return container;
}

void DolphinTabPage::startExpandViewAnimation(DolphinViewContainer *expandingContainer)
{
    Q_CHECK_PTR(expandingContainer);
    Q_ASSERT(expandingContainer == m_primaryViewContainer || expandingContainer == m_secondaryViewContainer);
    m_expandingContainer = expandingContainer;

    m_expandViewAnimation = new QVariantAnimation(m_splitter);
    m_expandViewAnimation->setDuration(2 * style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, this) * GlobalConfig::animationDurationFactor());
    for (int i = 0; i < m_splitter->count(); ++i) {
        m_splitter->widget(i)->setMinimumWidth(1);
    }
    connect(m_expandViewAnimation, &QAbstractAnimation::finished, this, &DolphinTabPage::slotAnimationFinished);
    connect(m_expandViewAnimation, &QVariantAnimation::valueChanged, this, &DolphinTabPage::slotAnimationValueChanged);

    m_expandViewAnimation->setStartValue(expandingContainer->width());
    if (m_splitViewEnabled) { // A new viewContainer is being opened.
        m_expandViewAnimation->setEndValue(m_splitter->width() / 2);
        m_expandViewAnimation->setEasingCurve(QEasingCurve::OutCubic);
    } else { // A viewContainer is being closed.
        m_expandViewAnimation->setEndValue(m_splitter->width());
        m_expandViewAnimation->setEasingCurve(QEasingCurve::InCubic);
    }
    m_expandViewAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

DolphinTabPageSplitterHandle::DolphinTabPageSplitterHandle(Qt::Orientation orientation, QSplitter *parent)
    : QSplitterHandle(orientation, parent)
    , m_mouseReleaseWasReceived(false)
{
}

bool DolphinTabPageSplitterHandle::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        m_mouseReleaseWasReceived = false;
        break;
    case QEvent::MouseButtonRelease:
        if (m_mouseReleaseWasReceived) {
            resetSplitterSizes();
        }
        m_mouseReleaseWasReceived = !m_mouseReleaseWasReceived;
        break;
    case QEvent::MouseButtonDblClick:
        m_mouseReleaseWasReceived = false;
        resetSplitterSizes();
        break;
    default:
        break;
    }

    return QSplitterHandle::event(event);
}

void DolphinTabPageSplitterHandle::resetSplitterSizes()
{
    QList<int> splitterSizes = splitter()->sizes();
    std::fill(splitterSizes.begin(), splitterSizes.end(), 0);
    splitter()->setSizes(splitterSizes);
}

DolphinTabPageSplitter::DolphinTabPageSplitter(Qt::Orientation orientation, QWidget *parent)
    : QSplitter(orientation, parent)
{
}

QSplitterHandle *DolphinTabPageSplitter::createHandle()
{
    return new DolphinTabPageSplitterHandle(orientation(), this);
}

#include "moc_dolphintabpage.cpp"
