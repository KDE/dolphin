/*
 * SPDX-FileCopyrightText: 2014 Emmanuel Pescosta <emmanuelpescosta099@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintabbar.h"
#include "dolphin_generalsettings.h"
#include <KLocalizedString>

#include <QApplication>
#include <QDrag>
#include <QDragEnterEvent>
#include <QInputDialog>
#include <QMenu>
#include <QMimeData>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QStyleOptionTab>
#include <QTimer>
#include <QToolButton>

class PreventFocusWhileHidden : public QObject
{
public:
    PreventFocusWhileHidden(QObject *parent)
        : QObject(parent) {};

protected:
    bool eventFilter(QObject *obj, QEvent *ev) override
    {
        switch (ev->type()) {
        case QEvent::Hide:
            static_cast<QWidget *>(obj)->setFocusPolicy(Qt::NoFocus);
            return false;
        case QEvent::Show:
            static_cast<QWidget *>(obj)->setFocusPolicy(Qt::TabFocus);
            return false;
        default:
            return false;
        }
    };
};

DolphinTabBar::DolphinTabBar(QWidget *parent)
    : QTabBar(parent)
    , m_autoActivationIndex(-1)
{
    setAcceptDrops(true);
    setSelectionBehaviorOnRemove(QTabBar::SelectPreviousTab);
    setMovable(true);
    setTabsClosable(true);

    setFocusPolicy(Qt::NoFocus);
    installEventFilter(new PreventFocusWhileHidden(this));

    m_autoActivationTimer = new QTimer(this);
    m_autoActivationTimer->setSingleShot(true);
    m_autoActivationTimer->setInterval(800);
    connect(m_autoActivationTimer, &QTimer::timeout, this, &DolphinTabBar::slotAutoActivationTimeout);
    connect(GeneralSettings::self(), &GeneralSettings::tabBarChanged, this, &DolphinTabBar::slotTabBarChanged);

    QTimer::singleShot(0, this, &DolphinTabBar::slotTabBarChanged);
}

QSize DolphinTabBar::tabSizeHint(int index) const
{
    if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FixedSize) {
        QSize defaultSize = QTabBar::tabSizeHint(index);
        defaultSize.setWidth(225);
        return defaultSize;
    } else if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FullWidth && count() > 0) {
        QSize defaultSize = QTabBar::tabSizeHint(index);
        const int buttonSpace = (m_newTabButton && m_newTabButton->isVisible()) ? m_newTabButton->width() : 0;
        defaultSize.setWidth(qMax(25, (width() - buttonSpace) / count()));
        return defaultSize;
    }
    return QTabBar::tabSizeHint(index);
}

QSize DolphinTabBar::minimumSizeHint() const
{
    QSize s = QTabBar::minimumSizeHint();

    if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FullWidth) {
        s.setWidth(0); // allow shrinking
    }

    return s;
}

void DolphinTabBar::dragEnterEvent(QDragEnterEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        event->acceptProposedAction();
        updateAutoActivationTimer(index);
    }

    QTabBar::dragEnterEvent(event);
}

void DolphinTabBar::dragLeaveEvent(QDragLeaveEvent *event)
{
    updateAutoActivationTimer(-1);

    QTabBar::dragLeaveEvent(event);
}

void DolphinTabBar::dragMoveEvent(QDragMoveEvent *event)
{
    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        Q_EMIT tabDragMoveEvent(index, event);
        updateAutoActivationTimer(index);
    }

    QTabBar::dragMoveEvent(event);
}

void DolphinTabBar::dropEvent(QDropEvent *event)
{
    // Disable the auto activation timer
    updateAutoActivationTimer(-1);

    const QMimeData *mimeData = event->mimeData();
    const int index = tabAt(event->position().toPoint());

    if (mimeData->hasUrls()) {
        Q_EMIT tabDropEvent(index, event);
    }

    QTabBar::dropEvent(event);
}

void DolphinTabBar::mousePressEvent(QMouseEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0 && event->button() == Qt::MiddleButton) {
        m_tabToBeClosedOnMiddleMouseButtonRelease = index;
        return;
    }

    if (index >= 0 && event->button() == Qt::LeftButton) {
        const QRect rect = tabRect(index);
        m_drag.tabIndex = index;
        m_drag.startPosition = event->pos();
        m_drag.offsetInTabX = event->pos().x() - rect.x();
        m_drag.offsetInTabY = event->pos().y() - rect.y();
        m_drag.tabWidth = rect.width();
        m_drag.tabHeight = rect.height() > 0 ? rect.height() : height();
        m_drag.isOngoing = false;
    }

    QTabBar::mousePressEvent(event);
}

void DolphinTabBar::mouseMoveEvent(QMouseEvent *event)
{
    QTabBar::mouseMoveEvent(event);

    if (!((event->buttons() & Qt::LeftButton) && m_drag.tabIndex >= 0 && count() > 1)) {
        return;
    }

    if (!m_drag.isOngoing) {
        if ((event->pos() - m_drag.startPosition).manhattanLength() >= QApplication::startDragDistance()) {
            m_drag.isOngoing = true;
        }
        return;
    }

    if (count() > 1 && isOutsideUndetachableZone(event->pos())) {
        startTabDrag(event);
    }
}

void DolphinTabBar::startTabDrag(QMouseEvent *event)
{
    const int detachIndex = currentIndex();
    if (detachIndex < 0 || detachIndex >= count()) {
        cancelTabDrag();
        return;
    }

    // Preview tab rendering for drag
    QStyleOptionTab tab;
    initStyleOption(&tab, detachIndex);
    tab.state &= ~QStyle::State_MouseOver;
    tab.position = QStyleOptionTab::OnlyOneTab;
    tab.leftButtonSize = QSize();
    tab.rightButtonSize = QSize();

    QWidget *closeButton = tabButton(detachIndex, QTabBar::RightSide);
    if (!closeButton) {
        closeButton = tabButton(detachIndex, QTabBar::LeftSide);
    }
    if (closeButton) {
        const int width = tab.fontMetrics.horizontalAdvance(tab.text) + closeButton->width();
        tab.text = tab.fontMetrics.elidedText(tabText(detachIndex), Qt::ElideRight, width);
    }

    const qreal dpr = qMax(qreal(1.0), devicePixelRatioF());
    QPixmap tabPixmap(tab.rect.size() * dpr);
    tabPixmap.setDevicePixelRatio(dpr);
    tabPixmap.fill(Qt::transparent);
    tab.rect = QRect(QPoint(0, 0), tab.rect.size());

    QPainter tabPainter(&tabPixmap);
    style()->drawControl(QStyle::CE_TabBarTab, &tab, &tabPainter, this);
    tabPainter.end();

    // Reset QTabBar internal move state before QDrag takes over
    QMouseEvent releaseEvent(QEvent::MouseButtonRelease, event->position(), event->globalPosition(), Qt::LeftButton, Qt::NoButton, event->modifiers());
    QTabBar::mouseReleaseEvent(&releaseEvent);

    QDrag *drag = new QDrag(this);
    drag->setMimeData(new QMimeData());

    const int tabW = tabPixmap.width() / dpr;
    const int tabH = tabPixmap.height() / dpr;

    if (!tabPixmap.isNull()) {
        QPixmap previewPixmap(tabPixmap.size());
        previewPixmap.setDevicePixelRatio(dpr);
        previewPixmap.fill(Qt::transparent);

        QPainter painter(&previewPixmap);
        painter.setOpacity(0.7);
        painter.drawPixmap(0, 0, tabPixmap);
        painter.end();

        drag->setPixmap(previewPixmap);
        const int hotX = std::clamp(m_drag.offsetInTabX, 0, tabW);
        const int hotY = std::clamp(m_drag.offsetInTabY, 0, tabH);
        drag->setHotSpot(QPoint(hotX, hotY));
    }

    drag->exec(Qt::MoveAction);

    const QPoint releasePos = mapFromGlobal(QCursor::pos());
    const bool shouldDetach = isOutsideUndetachableZone(releasePos);
    cancelTabDrag();

    if (shouldDetach) {
        Q_EMIT tabDetachRequested(detachIndex);
    }
}

void DolphinTabBar::mouseReleaseEvent(QMouseEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0 && index == m_tabToBeClosedOnMiddleMouseButtonRelease && event->button() == Qt::MiddleButton) {
        // Mouse middle click on a tab closes this tab.
        Q_EMIT tabCloseRequested(index);
        return;
    }

    if (event->button() == Qt::LeftButton) {
        cancelTabDrag();
    }

    QTabBar::mouseReleaseEvent(event);
}

bool DolphinTabBar::isOutsideUndetachableZone(const QPoint &pos) const
{
    const int tabWidth = m_drag.tabWidth > 0 ? m_drag.tabWidth : 100;
    const int tabHeight = m_drag.tabHeight > 0 ? m_drag.tabHeight : height();

    const QPoint draggedTabCenter(pos.x() - m_drag.offsetInTabX + tabWidth / 2, pos.y() - m_drag.offsetInTabY + tabHeight / 2);
    const QRect safeZone = rect().adjusted(0, -2 * tabHeight, 0, 2 * tabHeight);

    return !safeZone.contains(draggedTabCenter);
}

void DolphinTabBar::cancelTabDrag()
{
    m_drag = {};
}

void DolphinTabBar::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton) {
        int index = tabAt(event->pos());

        if (index < 0) {
            // empty tabbar area case
            index = currentIndex();
        }
        // Double left click on the tabbar opens a new activated tab
        // with the url from the doubleclicked tab or currentTab otherwise.
        Q_EMIT openNewActivatedTab(index);
    }

    QTabBar::mouseDoubleClickEvent(event);
}

void DolphinTabBar::contextMenuEvent(QContextMenuEvent *event)
{
    const int index = tabAt(event->pos());

    if (index >= 0) {
        // Tab context menu
        QMenu menu(this);

        QAction *newTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-new")), i18nc("@action:inmenu", "New Tab"));
        QAction *detachTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-detach")), i18nc("@action:inmenu", "Detach Tab"));
        menu.addSeparator();
        QAction *renameTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("edit-rename")), i18nc("@action:inmenu", "Rename Tab"));
        menu.addSeparator();
        QAction *closeOtherTabsAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close-other")), i18nc("@action:inmenu", "Close Other Tabs"));
        QAction *closeTabsToLeftAction = menu.addAction(QIcon::fromTheme(QStringLiteral("go-previous")), i18nc("@action:inmenu", "Close Tabs to the Left"));
        closeTabsToLeftAction->setEnabled(index > 0);
        QAction *closeTabsToRightAction = menu.addAction(QIcon::fromTheme(QStringLiteral("go-next")), i18nc("@action:inmenu", "Close Tabs to the Right"));
        closeTabsToRightAction->setEnabled(index < count() - 1);
        QAction *closeTabAction = menu.addAction(QIcon::fromTheme(QStringLiteral("tab-close")), i18nc("@action:inmenu", "Close Tab"));

        QAction *selectedAction = menu.exec(event->globalPos());
        if (selectedAction == newTabAction) {
            Q_EMIT openNewActivatedTab(index);
        } else if (selectedAction == detachTabAction) {
            Q_EMIT tabDetachRequested(index);
        } else if (selectedAction == closeOtherTabsAction) {
            const int tabCount = count();
            for (int i = 0; i < index; i++) {
                Q_EMIT tabCloseRequested(0);
            }
            for (int i = index + 1; i < tabCount; i++) {
                Q_EMIT tabCloseRequested(1);
            }
        } else if (selectedAction == closeTabsToLeftAction) {
            for (int i = 0; i < index; i++) {
                Q_EMIT tabCloseRequested(0);
            }
        } else if (selectedAction == closeTabsToRightAction) {
            const int tabCount = count();
            for (int i = index + 1; i < tabCount; i++) {
                Q_EMIT tabCloseRequested(index + 1);
            }
        } else if (selectedAction == closeTabAction) {
            Q_EMIT tabCloseRequested(index);
        } else if (selectedAction == renameTabAction) {
            bool renamed = false;
            const QString tabNewName = QInputDialog::getText(this,
                                                             i18nc("@title:window for text input", "Rename Tab"),
                                                             i18n("New tab name:"),
                                                             QLineEdit::Normal,
                                                             tabText(index),
                                                             &renamed);

            if (renamed) {
                Q_EMIT tabRenamed(index, tabNewName);
            }
        }

        return;
    }

    QTabBar::contextMenuEvent(event);
}

void DolphinTabBar::slotTabBarChanged()
{
    if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FixedSize) {
        setExpanding(false);
        setUsesScrollButtons(true);
    } else if (GeneralSettings::tabStyle() == GeneralSettings::EnumTabStyle::FullWidth) {
        setExpanding(true);
        setUsesScrollButtons(false);
    } else {
        setExpanding(false);
        setUsesScrollButtons(true);
    }

    updateGeometry();
}

void DolphinTabBar::slotAutoActivationTimeout()
{
    if (m_autoActivationIndex >= 0) {
        setCurrentIndex(m_autoActivationIndex);
        updateAutoActivationTimer(-1);
    }
}

void DolphinTabBar::updateAutoActivationTimer(const int index)
{
    if (m_autoActivationIndex != index) {
        m_autoActivationIndex = index;

        if (m_autoActivationIndex < 0) {
            m_autoActivationTimer->stop();
        } else {
            m_autoActivationTimer->start();
        }
    }
}

void DolphinTabBar::setNewTabButtonVisible(bool visible)
{
    if (visible) {
        if (m_newTabButton) {
            return;
        }
        m_newTabButton = new QToolButton(this);
        m_newTabButton->setObjectName(QStringLiteral("AddTabButton"));
        m_newTabButton->setIcon(QIcon::fromTheme(QIcon::ThemeIcon::ListAdd));
        m_newTabButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
        m_newTabButton->setToolTip(i18nc("@info:tooltip", "Open a new tab"));
        m_newTabButton->setAutoRaise(true);
        m_newTabButton->setFocusPolicy(Qt::NoFocus);
        connect(m_newTabButton, &QToolButton::clicked, this, &DolphinTabBar::newTabRequested);
        m_newTabButton->show();
    } else {
        if (!m_newTabButton) {
            return;
        }
        delete m_newTabButton;
        m_newTabButton = nullptr;
    }
    QResizeEvent resizeEv(size(), size());
    QApplication::sendEvent(this, &resizeEv);
}

void DolphinTabBar::resizeEvent(QResizeEvent *event)
{
    QTabBar::resizeEvent(event);
    updateNewTabButtonGeometry();
}

void DolphinTabBar::tabLayoutChange()
{
    QTabBar::tabLayoutChange();
    updateNewTabButtonGeometry();
}

void DolphinTabBar::tabInserted(int index)
{
    QTabBar::tabInserted(index);
    QTimer::singleShot(0, this, [this]() {
        updateNewTabButtonGeometry();
    });
}

void DolphinTabBar::tabRemoved(int index)
{
    QTabBar::tabRemoved(index);
    QTimer::singleShot(0, this, [this]() {
        updateNewTabButtonGeometry();
    });
}

void DolphinTabBar::updateNewTabButtonGeometry()
{
    if (!m_newTabButton || !m_newTabButton->isVisible() || count() == 0) {
        return;
    }

    const QRect lastTabRect = tabRect(count() - 1);
    const int btnSize = lastTabRect.height();
    const int proposedX = lastTabRect.right() + 1;

    if (proposedX + btnSize <= width()) {
        m_newTabButton->setGeometry(proposedX, lastTabRect.top(), btnSize, btnSize);
    } else {
        int scrollLeftEdge = width();
        const auto *scrollLeftButton = findChild<QToolButton *>(QStringLiteral("ScrollLeftButton"));
        if (scrollLeftButton && scrollLeftButton->isVisible()) {
            scrollLeftEdge = scrollLeftButton->x();
        }
        m_newTabButton->setGeometry(qMax(0, scrollLeftEdge - btnSize), 0, btnSize, btnSize);
    }
    m_newTabButton->raise();
}

#include "moc_dolphintabbar.cpp"
