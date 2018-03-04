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

#include "dolphintabpage.h"

#include "dolphin_generalsettings.h"
#include "dolphinviewcontainer.h"

#include <QSplitter>
#include <QVBoxLayout>

DolphinTabPage::DolphinTabPage(const QUrl &primaryUrl, const QUrl &secondaryUrl, QWidget* parent) :
    QWidget(parent),
    m_primaryViewActive(true),
    m_splitViewEnabled(false),
    m_active(true)
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(0);
    layout->setMargin(0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    layout->addWidget(m_splitter);

    // Create a new primary view
    m_primaryViewContainer = createViewContainer(primaryUrl);
    connect(m_primaryViewContainer->view(), &DolphinView::urlChanged,
            this, &DolphinTabPage::activeViewUrlChanged);
    connect(m_primaryViewContainer->view(), &DolphinView::redirection,
            this, &DolphinTabPage::slotViewUrlRedirection);

    m_splitter->addWidget(m_primaryViewContainer);
    m_primaryViewContainer->show();

    if (secondaryUrl.isValid() || GeneralSettings::splitView()) {
        // Provide a secondary view, if the given secondary url is valid or if the
        // startup settings are set this way (use the url of the primary view).
        m_splitViewEnabled = true;
        const QUrl& url = secondaryUrl.isValid() ? secondaryUrl : primaryUrl;
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

void DolphinTabPage::setSplitViewEnabled(bool enabled, const QUrl &secondaryUrl)
{
    if (m_splitViewEnabled != enabled) {
        m_splitViewEnabled = enabled;

        if (enabled) {
            const QUrl& url = (secondaryUrl.isEmpty()) ? m_primaryViewContainer->url() : secondaryUrl;
            m_secondaryViewContainer = createViewContainer(url);

            const bool placesSelectorVisible = m_primaryViewContainer->urlNavigator()->isPlacesSelectorVisible();
            m_secondaryViewContainer->urlNavigator()->setPlacesSelectorVisible(placesSelectorVisible);

            m_splitter->addWidget(m_secondaryViewContainer);
            m_secondaryViewContainer->show();
            m_secondaryViewContainer->setActive(true);
        } else {
            // Close the view which is active.
            DolphinViewContainer* view = activeViewContainer();
            if (m_primaryViewActive) {
                // If the primary view is active, we have to swap the pointers
                // because the secondary view will be the new primary view.
                qSwap(m_primaryViewContainer, m_secondaryViewContainer);
            }
            m_primaryViewContainer->setActive(true);
            view->close();
            view->deleteLater();
        }
    }
}

DolphinViewContainer* DolphinTabPage::primaryViewContainer() const
{
    return m_primaryViewContainer;
}

DolphinViewContainer* DolphinTabPage::secondaryViewContainer() const
{
    return m_secondaryViewContainer;
}

DolphinViewContainer* DolphinTabPage::activeViewContainer() const
{
    return m_primaryViewActive ? m_primaryViewContainer :
                                 m_secondaryViewContainer;
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

void DolphinTabPage::markUrlsAsSelected(const QList<QUrl>& urls)
{
    m_primaryViewContainer->view()->markUrlsAsSelected(urls);
    if (m_splitViewEnabled) {
        m_secondaryViewContainer->view()->markUrlsAsSelected(urls);
    }
}

void DolphinTabPage::markUrlAsCurrent(const QUrl& url)
{
    m_primaryViewContainer->view()->markUrlAsCurrent(url);
    if (m_splitViewEnabled) {
        m_secondaryViewContainer->view()->markUrlAsCurrent(url);
    }
}

void DolphinTabPage::setPlacesSelectorVisible(bool visible)
{
    m_primaryViewContainer->urlNavigator()->setPlacesSelectorVisible(visible);
    if (m_splitViewEnabled) {
        m_secondaryViewContainer->urlNavigator()->setPlacesSelectorVisible(visible);
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
    stream << m_primaryViewContainer->urlNavigator()->isUrlEditable();
    m_primaryViewContainer->view()->saveState(stream);

    if (m_splitViewEnabled) {
        stream << m_secondaryViewContainer->url();
        stream << m_secondaryViewContainer->urlNavigator()->isUrlEditable();
        m_secondaryViewContainer->view()->saveState(stream);
    }

    stream << m_primaryViewActive;
    stream << m_splitter->saveState();

    return state;
}

void DolphinTabPage::restoreState(const QByteArray& state)
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
    setSplitViewEnabled(isSplitViewEnabled);

    QUrl primaryUrl;
    stream >> primaryUrl;
    m_primaryViewContainer->setUrl(primaryUrl);
    bool primaryUrlEditable;
    stream >> primaryUrlEditable;
    m_primaryViewContainer->urlNavigator()->setUrlEditable(primaryUrlEditable);
    m_primaryViewContainer->view()->restoreState(stream);

    if (isSplitViewEnabled) {
        QUrl secondaryUrl;
        stream >> secondaryUrl;
        m_secondaryViewContainer->setUrl(secondaryUrl);
        bool secondaryUrlEditable;
        stream >> secondaryUrlEditable;
        m_secondaryViewContainer->urlNavigator()->setUrlEditable(secondaryUrlEditable);
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

void DolphinTabPage::restoreStateV1(const QByteArray& state)
{
    if (state.isEmpty()) {
        return;
    }

    QByteArray sd = state;
    QDataStream stream(&sd, QIODevice::ReadOnly);

    bool isSplitViewEnabled = false;
    stream >> isSplitViewEnabled;
    setSplitViewEnabled(isSplitViewEnabled);

    QUrl primaryUrl;
    stream >> primaryUrl;
    m_primaryViewContainer->setUrl(primaryUrl);
    bool primaryUrlEditable;
    stream >> primaryUrlEditable;
    m_primaryViewContainer->urlNavigator()->setUrlEditable(primaryUrlEditable);

    if (isSplitViewEnabled) {
        QUrl secondaryUrl;
        stream >> secondaryUrl;
        m_secondaryViewContainer->setUrl(secondaryUrl);
        bool secondaryUrlEditable;
        stream >> secondaryUrlEditable;
        m_secondaryViewContainer->urlNavigator()->setUrlEditable(secondaryUrlEditable);
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

void DolphinTabPage::slotViewActivated()
{
    const DolphinView* oldActiveView = activeViewContainer()->view();

    // Set the view, which was active before, to inactive
    // and update the active view type, if tab is active
    if (m_active) {
        if (m_splitViewEnabled) {
            activeViewContainer()->setActive(false);
            m_primaryViewActive = !m_primaryViewActive;
        } else {
            m_primaryViewActive = true;
        }
    }

    const DolphinView* newActiveView = activeViewContainer()->view();

    if (newActiveView != oldActiveView) {
        disconnect(oldActiveView, &DolphinView::urlChanged,
                   this, &DolphinTabPage::activeViewUrlChanged);
        disconnect(oldActiveView, &DolphinView::redirection,
                   this, &DolphinTabPage::slotViewUrlRedirection);
        connect(newActiveView, &DolphinView::urlChanged,
                this, &DolphinTabPage::activeViewUrlChanged);
        connect(newActiveView, &DolphinView::redirection,
                this, &DolphinTabPage::slotViewUrlRedirection);
    }

    emit activeViewUrlChanged(activeViewContainer()->url());
    emit activeViewChanged(activeViewContainer());
}

void DolphinTabPage::slotViewUrlRedirection(const QUrl& oldUrl, const QUrl& newUrl)
{
    Q_UNUSED(oldUrl);

    emit activeViewUrlChanged(newUrl);
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

DolphinViewContainer* DolphinTabPage::createViewContainer(const QUrl& url) const
{
    DolphinViewContainer* container = new DolphinViewContainer(url, m_splitter);
    container->setActive(false);

    const DolphinView* view = container->view();
    connect(view, &DolphinView::activated,
            this, &DolphinTabPage::slotViewActivated);

    connect(view, &DolphinView::toggleActiveViewRequested,
            this, &DolphinTabPage::switchActiveView);

    return container;
}
