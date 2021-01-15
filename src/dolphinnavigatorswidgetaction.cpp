/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2020 Felix Ernst <fe.a.ernst@gmail.com>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "dolphinnavigatorswidgetaction.h"

#include "trash/dolphintrash.h"

#include <KLocalizedString>
#include <KNotificationJobUiDelegate>
#include <KService>
#include <KXMLGUIFactory>
#include <KXmlGuiWindow>

#include <KIO/ApplicationLauncherJob>

#include <QApplication>
#include <QDomDocument>
#include <QHBoxLayout>
#include <QPushButton>
#include <QSplitter>
#include <QToolBar>

#include <limits>

DolphinNavigatorsWidgetAction::DolphinNavigatorsWidgetAction(QWidget *parent) :
    QWidgetAction{parent},
    m_splitter{new QSplitter(Qt::Horizontal)},
    m_adjustSpacingTimer{new QTimer(this)},
    m_viewGeometriesHelper{m_splitter.get(), this}
{
    updateText();
    setIcon(QIcon::fromTheme(QStringLiteral("dialog-scripts")));

    m_splitter->setChildrenCollapsible(false);

    m_splitter->addWidget(createNavigatorWidget(Primary));

    m_adjustSpacingTimer->setInterval(100);
    m_adjustSpacingTimer->setSingleShot(true);
    connect(m_adjustSpacingTimer.get(), &QTimer::timeout,
            this, &DolphinNavigatorsWidgetAction::adjustSpacing);
}

void DolphinNavigatorsWidgetAction::adjustSpacing()
{
    auto viewGeometries = m_viewGeometriesHelper.viewGeometries();
    const int widthOfSplitterPrimary = viewGeometries.globalXOfPrimary + viewGeometries.widthOfPrimary - viewGeometries.globalXOfNavigatorsWidget;
    const QList<int> splitterSizes = {widthOfSplitterPrimary,
                                      m_splitter->width() - widthOfSplitterPrimary};
    m_splitter->setSizes(splitterSizes);

    // primary side of m_splitter
    int leadingSpacing = viewGeometries.globalXOfPrimary - viewGeometries.globalXOfNavigatorsWidget;
    if (leadingSpacing < 0) {
        leadingSpacing = 0;
    }
    int trailingSpacing = (viewGeometries.globalXOfNavigatorsWidget + m_splitter->width())
                          - (viewGeometries.globalXOfPrimary + viewGeometries.widthOfPrimary);
    if (trailingSpacing < 0 || emptyTrashButton(Primary)->isVisible()
                            || networkFolderButton(Primary)->isVisible()
    ) {
        trailingSpacing = 0;
    }
    const int widthLeftForUrlNavigator = m_splitter->widget(0)->width() - leadingSpacing - trailingSpacing;
    const int widthNeededForUrlNavigator = primaryUrlNavigator()->sizeHint().width() - widthLeftForUrlNavigator;
    if (widthNeededForUrlNavigator > 0) {
        trailingSpacing -= widthNeededForUrlNavigator;
        if (trailingSpacing < 0) {
            leadingSpacing += trailingSpacing;
            trailingSpacing = 0;
        }
        if (leadingSpacing < 0) {
            leadingSpacing = 0;
        }
    }
    spacing(Primary, Leading)->setMinimumWidth(leadingSpacing);
    spacing(Primary, Trailing)->setFixedWidth(trailingSpacing);

    // secondary side of m_splitter
    if (viewGeometries.globalXOfSecondary == INT_MIN) {
        Q_ASSERT(viewGeometries.widthOfSecondary == INT_MIN);
        return;
    }
    spacing(Primary, Trailing)->setFixedWidth(0);

    trailingSpacing = (viewGeometries.globalXOfNavigatorsWidget + m_splitter->width())
                      - (viewGeometries.globalXOfSecondary + viewGeometries.widthOfSecondary);
    if (trailingSpacing < 0 || emptyTrashButton(Secondary)->isVisible()
                            || networkFolderButton(Secondary)->isVisible()
    ) {
        trailingSpacing = 0;
    } else {
        const int widthLeftForUrlNavigator2 = m_splitter->widget(1)->width() - trailingSpacing;
        const int widthNeededForUrlNavigator2 = secondaryUrlNavigator()->sizeHint().width() - widthLeftForUrlNavigator2;
        if (widthNeededForUrlNavigator2 > 0) {
            trailingSpacing -= widthNeededForUrlNavigator2;
            if (trailingSpacing < 0) {
                trailingSpacing = 0;
            }
        }
    }
    spacing(Secondary, Trailing)->setMinimumWidth(trailingSpacing);
}

void DolphinNavigatorsWidgetAction::createSecondaryUrlNavigator()
{
    Q_ASSERT(m_splitter->count() == 1);
    m_splitter->addWidget(createNavigatorWidget(Secondary));
    Q_ASSERT(m_splitter->count() == 2);
    updateText();
}

void DolphinNavigatorsWidgetAction::followViewContainersGeometry(QWidget *primaryViewContainer,
                                                                 QWidget *secondaryViewContainer)
{
    m_viewGeometriesHelper.setViewContainers(primaryViewContainer, secondaryViewContainer);
    adjustSpacing();
}

bool DolphinNavigatorsWidgetAction::isInToolbar() const
{
    return qobject_cast<QToolBar *>(m_splitter->parentWidget());
}

DolphinUrlNavigator* DolphinNavigatorsWidgetAction::primaryUrlNavigator() const
{
    Q_ASSERT(m_splitter);
    return m_splitter->widget(0)->findChild<DolphinUrlNavigator *>();
}

DolphinUrlNavigator* DolphinNavigatorsWidgetAction::secondaryUrlNavigator() const
{
    Q_ASSERT(m_splitter);
    if (m_splitter->count() < 2) {
        return nullptr;
    }
    return m_splitter->widget(1)->findChild<DolphinUrlNavigator *>();
}

void DolphinNavigatorsWidgetAction::setSecondaryNavigatorVisible(bool visible)
{
    if (visible) {
        Q_ASSERT(m_splitter->count() == 2);
        m_splitter->widget(1)->setVisible(true);
    } else if (m_splitter->count() > 1) {
        m_splitter->widget(1)->setVisible(false);
        // Fix an unlikely event of wrong trash button visibility.
        emptyTrashButton(Secondary)->setVisible(false);
    }
    updateText();
}

QWidget *DolphinNavigatorsWidgetAction::createWidget(QWidget *parent)
{
    QWidget *oldParent = m_splitter->parentWidget();
    if (oldParent && oldParent->layout()) {
        oldParent->layout()->removeWidget(m_splitter.get());
        QGridLayout *layout = qobject_cast<QGridLayout *>(oldParent->layout());
        if (qobject_cast<QToolBar *>(parent) && layout) {
            // in DolphinTabPage::insertNavigatorsWidget the minimumHeight of this row was
            // set to fit the m_splitter. Since we are now removing it again, the
            // minimumHeight can be reset to 0.
            layout->setRowMinimumHeight(0, 0);
        }
    }
    m_splitter->setParent(parent);
    return m_splitter.get();
}

void DolphinNavigatorsWidgetAction::deleteWidget(QWidget *widget)
{
    Q_UNUSED(widget)
    m_splitter->setParent(nullptr);
}

QWidget *DolphinNavigatorsWidgetAction::createNavigatorWidget(Side side) const
{
    auto navigatorWidget = new QWidget(m_splitter.get());
    auto layout = new QHBoxLayout{navigatorWidget};
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    if (side == Primary) {
        auto leadingSpacing = new QWidget{navigatorWidget};
        layout->addWidget(leadingSpacing);
    }
    auto urlNavigator = new DolphinUrlNavigator(navigatorWidget);
    layout->addWidget(urlNavigator);

    auto emptyTrashButton = newEmptyTrashButton(urlNavigator, navigatorWidget);
    layout->addWidget(emptyTrashButton);

    auto networkFolderButton = newNetworkFolderButton(urlNavigator, navigatorWidget);
    layout->addWidget(networkFolderButton);

    connect(urlNavigator, &KUrlNavigator::urlChanged, this, [urlNavigator, this]() {
        // Update URL navigator to show a server URL entry placeholder text if we
        // just loaded the remote:/ page, to make it easier for users to figure out
        // that they can enter arbitrary remote URLs. See bug 414670
        if (urlNavigator->locationUrl().scheme() == QLatin1String("remote")) {
            if (!urlNavigator->isUrlEditable()) {
                urlNavigator->setUrlEditable(true);
            }
            urlNavigator->clearText();
            urlNavigator->setPlaceholderText(i18n("Enter server URL (e.g. smb://[ip address])"));
        } else {
            urlNavigator->setPlaceholderText(QString());
        }

        // We have to wait for DolphinUrlNavigator::sizeHint() to update which
        // happens a little bit later than when urlChanged is emitted.
        this->m_adjustSpacingTimer->start();
    }, Qt::QueuedConnection);

    auto trailingSpacing = new QWidget{navigatorWidget};
    layout->addWidget(trailingSpacing);
    return navigatorWidget;
}

QPushButton * DolphinNavigatorsWidgetAction::emptyTrashButton(DolphinNavigatorsWidgetAction::Side side)
{
    int sideIndex = (side == Primary ? 0 : 1);
    if (side == Primary) {
        return static_cast<QPushButton *>(m_splitter->widget(sideIndex)->layout()->itemAt(2)->widget());
    }
    return static_cast<QPushButton *>(m_splitter->widget(sideIndex)->layout()->itemAt(1)->widget());
}

QPushButton *DolphinNavigatorsWidgetAction::newEmptyTrashButton(const DolphinUrlNavigator *urlNavigator, QWidget *parent) const
{
    auto emptyTrashButton = new QPushButton(QIcon::fromTheme(QStringLiteral("user-trash")),
                                        i18nc("@action:button", "Empty Trash"), parent);
    emptyTrashButton->setFlat(true);
    connect(emptyTrashButton, &QPushButton::clicked,
            this, [parent]() { Trash::empty(parent); });
    connect(&Trash::instance(), &Trash::emptinessChanged,
            emptyTrashButton, &QPushButton::setDisabled);
    emptyTrashButton->hide();
    connect(urlNavigator, &KUrlNavigator::urlChanged, this, [emptyTrashButton, urlNavigator]() {
        emptyTrashButton->setVisible(urlNavigator->locationUrl().scheme() == QLatin1String("trash"));
    });
    emptyTrashButton->setDisabled(Trash::isEmpty());
    return emptyTrashButton;
}

QPushButton *DolphinNavigatorsWidgetAction::networkFolderButton(DolphinNavigatorsWidgetAction::Side side)
{
    int sideIndex = (side == Primary ? 0 : 1);
    if (side == Primary) {
        return static_cast<QPushButton *>(m_splitter->widget(sideIndex)->layout()->itemAt(3)->widget());
    }
    return static_cast<QPushButton *>(m_splitter->widget(sideIndex)->layout()->itemAt(2)->widget());
}

QPushButton *DolphinNavigatorsWidgetAction::newNetworkFolderButton(const DolphinUrlNavigator *urlNavigator, QWidget *parent) const
{
    auto networkFolderButton = new QPushButton(QIcon::fromTheme(QStringLiteral("folder-add")),
                                        i18nc("@action:button", "Add Network Folder"), parent);
    networkFolderButton->setFlat(true);
    KService::Ptr service = KService::serviceByDesktopName(QStringLiteral("org.kde.knetattach"));
    connect(networkFolderButton, &QPushButton::clicked,
            this, [networkFolderButton, service]() {
                auto *job = new KIO::ApplicationLauncherJob(service, networkFolderButton);
                auto *delegate = new KNotificationJobUiDelegate;
                delegate->setAutoErrorHandlingEnabled(true);
                job->setUiDelegate(delegate);
                job->start();
            });
    networkFolderButton->hide();
    connect(urlNavigator, &KUrlNavigator::urlChanged, this, [networkFolderButton, urlNavigator, service]() {
        networkFolderButton->setVisible(service && urlNavigator->locationUrl().scheme() == QLatin1String("remote"));
    });
    return networkFolderButton;
}

QWidget *DolphinNavigatorsWidgetAction::spacing(Side side, Position position) const
{
    int sideIndex = (side == Primary ? 0 : 1);
    if (position == Leading) {
        Q_ASSERT(side == Primary); // The secondary side of the splitter has no leading spacing.
        return m_splitter->widget(sideIndex)->layout()->itemAt(0)->widget();
    }
    if (side == Primary) {
        return m_splitter->widget(sideIndex)->layout()->itemAt(4)->widget();
    }
    return m_splitter->widget(sideIndex)->layout()->itemAt(3)->widget();
}

void DolphinNavigatorsWidgetAction::updateText()
{
    const int urlNavigatorsAmount = m_splitter->count() > 1 && m_splitter->widget(1)->isVisible() ?
                                    2 : 1;
    setText(i18ncp("@action:inmenu", "Location Bar", "Location Bars", urlNavigatorsAmount));
}

DolphinNavigatorsWidgetAction::ViewGeometriesHelper::ViewGeometriesHelper
    (QWidget *navigatorsWidget, DolphinNavigatorsWidgetAction *navigatorsWidgetAction) :
    m_navigatorsWidget{navigatorsWidget},
    m_navigatorsWidgetAction{navigatorsWidgetAction}
{
    Q_CHECK_PTR(navigatorsWidget);
    Q_CHECK_PTR(navigatorsWidgetAction);
}

bool DolphinNavigatorsWidgetAction::ViewGeometriesHelper::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::Resize) {
        m_navigatorsWidgetAction->adjustSpacing();
        return false;
    }
    return QObject::eventFilter(watched, event);
}

void DolphinNavigatorsWidgetAction::ViewGeometriesHelper::setViewContainers(QWidget *primaryViewContainer,
                                                                            QWidget *secondaryViewContainer)
{
    Q_CHECK_PTR(primaryViewContainer);
    if (m_primaryViewContainer) {
        m_primaryViewContainer->removeEventFilter(this);
    }
    primaryViewContainer->installEventFilter(this);
    m_primaryViewContainer = primaryViewContainer;

    // It is not possible to resize the secondaryViewContainer without simultaneously
    // resizing the primaryViewContainer so we don't have to installEventFilter() here.
    m_secondaryViewContainer = secondaryViewContainer;
}

DolphinNavigatorsWidgetAction::ViewGeometriesHelper::Geometries
        DolphinNavigatorsWidgetAction::ViewGeometriesHelper::viewGeometries()
{
    Q_ASSERT(m_primaryViewContainer);
    Geometries geometries;

    // width
    geometries.widthOfPrimary = m_primaryViewContainer->width();
    if (m_secondaryViewContainer) {
        geometries.widthOfSecondary = m_secondaryViewContainer->width();
    } else {
        geometries.widthOfSecondary = INT_MIN;
    }

    // globalX
    if (QApplication::layoutDirection() == Qt::LeftToRight) {
        geometries.globalXOfNavigatorsWidget = m_navigatorsWidget->mapToGlobal(QPoint(0,0)).x();
        geometries.globalXOfPrimary = m_primaryViewContainer->mapToGlobal(QPoint(0,0)).x();
        geometries.globalXOfSecondary = !m_secondaryViewContainer ? INT_MIN :
                m_secondaryViewContainer->mapToGlobal(QPoint(0,0)).x();
    } else {
        // When the direction is reversed, globalX does not change.
        // For the adjustSpacing() code to work we need globalX to measure from right to left
        // and to measure up to the rightmost point of a widget instead of the leftmost.
        geometries.globalXOfNavigatorsWidget =
                (-1) * (m_navigatorsWidget->mapToGlobal(QPoint(0,0)).x() + m_navigatorsWidget->width());
        geometries.globalXOfPrimary =
                (-1) * (m_primaryViewContainer->mapToGlobal(QPoint(0,0)).x() + geometries.widthOfPrimary);
        geometries.globalXOfSecondary = !m_secondaryViewContainer ? INT_MIN :
                (-1) * (m_secondaryViewContainer->mapToGlobal(QPoint(0,0)).x() + geometries.widthOfSecondary);
    }
    return geometries;
}
