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

#include "dolphintabpage.h"

#include "dolphinviewcontainer.h"
#include "dolphin_generalsettings.h"

#include <QSplitter>
#include <KConfigGroup>
#include <KIcon>

DolphinTabPage::DolphinTabPage(const KUrl& url, QWidget* parent) :
    QWidget(parent),
    m_activeView(NoView),
    m_splitViewEnabled(false)
{
    m_layout = new QVBoxLayout(this);
    m_layout->setSpacing(0);
    m_layout->setMargin(0);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setChildrenCollapsible(false);
    m_layout->addWidget(m_splitter);

    // Create a new primary view.
    DolphinViewContainer* primaryView = createViewContainer(url);
    m_splitter->addWidget(primaryView);
    primaryView->show();

    if (GeneralSettings::splitView()) {
        // Provide a split view, if the startup settings are set this way. Use
        // the url of the primary view.
        setSplitViewEnabled(true, url);
    }

    setActiveView(PrimaryView);
}

QString DolphinTabPage::tabText() const
{
    if (activeView() != NoView) {
        QString name;

        const KUrl& url = activeViewContainer()->url();
        if (url.equals(KUrl("file:///"))) {
            name = '/';
        } else {
            name = url.fileName();
            if (name.isEmpty()) {
                name = url.protocol();
            } else {
                // Make sure that a '&' inside the directory name is displayed correctly
                // and not misinterpreted as a keyboard shortcut in QTabBar::setTabText()
                name.replace('&', "&&");
            }
        }

        return squeezedText(name);
    }

    return QString();
}

KIcon DolphinTabPage::tabIcon() const
{
    if (activeView() != NoView) {
        const KUrl& url = activeViewContainer()->url();
        return KIcon(KMimeType::iconNameForUrl(url));
    }

    return KIcon();
}

void DolphinTabPage::setActiveView(const DolphinTabPage::ViewType type)
{
    Q_ASSERT(type > NoView);
    viewContainer(isSplitViewEnabled() ? type : PrimaryView)->setActive(true);
}

DolphinTabPage::ViewType DolphinTabPage::activeView() const
{
    return m_activeView;
}

void DolphinTabPage::setSplitViewEnabled(const bool enabled, const KUrl& secondaryUrl)
{
    if (isSplitViewEnabled() != enabled) {
        m_splitViewEnabled = enabled;

        if (enabled) {
            // If the given secondary url is valid, use it for the secondary view. If the url is
            // not valid, use the url of the primary view instead.
            const bool useSecondaryUrl = !secondaryUrl.isEmpty() && secondaryUrl.isValid();
            const KUrl& url = useSecondaryUrl ? secondaryUrl : viewContainer(PrimaryView)->url();

            DolphinViewContainer* secondaryView = createViewContainer(url);
            m_splitter->addWidget(secondaryView);
            secondaryView->show();
            setActiveView(SecondaryView);
        } else {
            // Close the view which is active.
            DolphinViewContainer* view = activeViewContainer();
            // Re-Add the widget to the splitter layout, after all the other widgets.
            // Because the widget is already in the splitter, it will only be
            // moved to the SecondaryView position. (PrimaryView -> SecondaryView
            // and SecondaryView -> SecondaryView). QSplitter doesn't provide such
            // a function to move widgets - so addWidget() is just a workaround.
            // So we remove always the widget on the position SecondaryView.
            m_splitter->addWidget(view);
            view->close();
            view->deleteLater();
            setActiveView(PrimaryView);
        }
    }
}

bool DolphinTabPage::isSplitViewEnabled() const
{
    return m_splitViewEnabled;
}

void DolphinTabPage::toggleSplitViewEnabled()
{
    setSplitViewEnabled(!isSplitViewEnabled());
}

DolphinViewContainer* DolphinTabPage::viewContainer(const DolphinTabPage::ViewType type) const
{
    Q_ASSERT(type > NoView);
    return static_cast<DolphinViewContainer*>(m_splitter->widget(type));
}

DolphinViewContainer* DolphinTabPage::activeViewContainer() const
{
    return viewContainer(m_activeView);
}

void DolphinTabPage::reload() const
{
    viewContainer(PrimaryView)->view()->reload();
    if (isSplitViewEnabled()) {
        viewContainer(SecondaryView)->view()->reload();
    }
}

void DolphinTabPage::refresh() const
{
    viewContainer(PrimaryView)->view()->readSettings();
    if (isSplitViewEnabled()) {
        viewContainer(SecondaryView)->view()->readSettings();
    }
}

void DolphinTabPage::saveProperties(const int tabIndex, KConfigGroup& group) const
{
    const DolphinViewContainer* primaryView = viewContainer(PrimaryView);
    group.writeEntry(tabProperty("Primary URL", tabIndex), primaryView->url().url());
    group.writeEntry(tabProperty("Primary Editable", tabIndex), primaryView->urlNavigator()->isUrlEditable());

    if (isSplitViewEnabled()) {
        const DolphinViewContainer* secondaryView = viewContainer(SecondaryView);
        group.writeEntry(tabProperty("Secondary URL", tabIndex), secondaryView->url().url());
        group.writeEntry(tabProperty("Secondary Editable", tabIndex), secondaryView->urlNavigator()->isUrlEditable());
    }

    group.writeEntry(tabProperty("Active View", tabIndex), (int)(activeView()));
    group.writeEntry(tabProperty("Splitter Sizes", tabIndex), m_splitter->saveState());
}

void DolphinTabPage::readProperties(const int tabIndex, const KConfigGroup& group)
{
    DolphinViewContainer* primaryView = viewContainer(PrimaryView);
    primaryView->setUrl(group.readEntry(tabProperty("Primary URL", tabIndex)));
    const bool editable = group.readEntry(tabProperty("Primary Editable", tabIndex), false);
    primaryView->urlNavigator()->setUrlEditable(editable);

    const QString secondaryUrl = group.readEntry(tabProperty("Secondary URL", tabIndex));
    if (!secondaryUrl.isEmpty()) {
        setSplitViewEnabled(true);
        DolphinViewContainer* secondaryView = viewContainer(SecondaryView);
        secondaryView->setUrl(secondaryUrl);
        const bool editable = group.readEntry(tabProperty("Secondary Editable", tabIndex), false);
        secondaryView->urlNavigator()->setUrlEditable(editable);
    } else {
        setSplitViewEnabled(false);
    }

    const ViewType activeViewType = (ViewType)(group.readEntry(tabProperty("Active View", tabIndex), (int)(PrimaryView)));
    setActiveView(activeViewType);

    const QByteArray& splitterState = group.readEntry(tabProperty("Splitter Sizes", tabIndex)).toUtf8();
    if (!splitterState.isEmpty()) {
        m_splitter->restoreState(splitterState);
    }
}

void DolphinTabPage::slotViewActivated()
{
    if (m_activeView == NoView) {
        // No view was active before, so get the type
        // of the newly activated view.
        if (isSplitViewEnabled()) {
            if (viewContainer(PrimaryView)->isActive()) {
                m_activeView = PrimaryView;
            } else {
                m_activeView = SecondaryView;
            }
        } else {
            m_activeView = PrimaryView;
        }
    } else {
        // Set the view, which was active before, to inactive
        // and update the active view type.
        if (isSplitViewEnabled()) {
            activeViewContainer()->setActive(false);

            if (m_activeView == PrimaryView) {
                m_activeView = SecondaryView;
            } else {
                m_activeView = PrimaryView;
            }
        } else {
            m_activeView = PrimaryView;
        }
    }

    emit tabPropertiesChanged(this);
    emit activeViewChanged();
}

void DolphinTabPage::slotViewUrlChanged(const KUrl& url)
{
    if (activeViewContainer()->url() == url) {
        // The view container whose url has changed is the active
        // view container, so inform the tab widget about this change,
        // so that the tab text and icon can be updated.
        emit tabPropertiesChanged(this);
    }
}

DolphinViewContainer* DolphinTabPage::createViewContainer(const KUrl& url)
{
    DolphinViewContainer* container = new DolphinViewContainer(url, m_splitter);
    container->setActive(false);

    const DolphinView* view = container->view();
    connect(view, SIGNAL(activated()),
            this, SLOT(slotViewActivated()));
    connect(view, SIGNAL(urlChanged(KUrl)),
            this, SLOT(slotViewUrlChanged(KUrl)));

    return container;
}

QString DolphinTabPage::tabProperty(const QString& property, const int tabIndex)
{
    return "Tab " % QString::number(tabIndex) % " " % property;
}

QString DolphinTabPage::squeezedText(const QString& text) const
{
    const QFontMetrics fm = fontMetrics();
    return fm.elidedText(text, Qt::ElideMiddle, fm.maxWidth() * 10);
}