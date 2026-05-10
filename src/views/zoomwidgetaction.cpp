/*
 * SPDX-FileCopyrightText: 2025 Gleb Kasachou <gkosachov99@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "zoomwidgetaction.h"

#include <KLocalizedString>

#include <QEvent>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QMenu>
#include <QPaintEvent>
#include <QStyleOptionMenuItem>
#include <QStylePainter>
#include <QToolBar>
#include <QToolButton>

class ZoomWidget : public QWidget
{
public:
    ZoomWidget(QWidget *parent)
        : QWidget(parent)
    {
    }

protected:
    void paintEvent(QPaintEvent *event) override
    {
        Q_UNUSED(event);

        QStylePainter painter(this);
        QStyleOptionMenuItem option;
        option.initFrom(this);
        option.menuHasCheckableItems = true;
        option.checkType = QStyleOptionMenuItem::NotCheckable;
        option.text = i18nc("@action:inmenu", "Zoom");
        option.icon = QIcon::fromTheme(QStringLiteral("zoom"));
        option.reservedShortcutWidth = 0;
        option.menuItemType = QStyleOptionMenuItem::Normal;

        option.maxIconWidth = 0;
        for (QAction *action : qobject_cast<QMenu *>(parent())->actions()) {
            if (!action->icon().isNull()) {
                option.maxIconWidth = style()->pixelMetric(QStyle::PM_SmallIconSize) + 4;
                break;
            }
        }

        painter.drawControl(QStyle::CE_MenuItem, option);
    }
};

ZoomWidgetAction::ZoomWidgetAction(QAction *zoomInAction, QAction *zoomResetAction, QAction *zoomOutAction, QObject *parent)
    : KToolBarPopupAction(QIcon::fromTheme(QStringLiteral("zoom")), i18nc("@action:intoolbar", "Zoom"), parent)
    , m_zoomInAction(zoomInAction)
    , m_zoomResetAction(zoomResetAction)
    , m_zoomOutAction(zoomOutAction)
{
    // This is a property that KXMLGui reads to determine whether this action
    // should be included in the shortcut configuration UI
    setProperty("isShortcutConfigurable", false);
    setPopupMode(InstantPopup);
    popupMenu()->addActions({zoomInAction, zoomResetAction, zoomOutAction});
}

bool ZoomWidgetAction::eventFilter(QObject *object, QEvent *event)
{
    if (event->type() != QEvent::KeyPress) {
        return false;
    }

    QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
    QWidget *widget = qobject_cast<QWidget *>(object);

    if (keyEvent->keyCombination() == QKeyCombination(Qt::Modifier::SHIFT, Qt::Key_Backtab) || keyEvent->key() == Qt::Key_Left
        || keyEvent->key() == Qt::Key_Up) {
        QWidget *previous = widget->previousInFocusChain();
        if (previous == widget->parentWidget() || !qobject_cast<QToolButton *>(previous)->isEnabled()) {
            return false;
        }

        previous->setFocus(Qt::BacktabFocusReason);
        event->accept();
        return true;
    }

    if (keyEvent->keyCombination() == QKeyCombination(Qt::Key_Tab) || keyEvent->key() == Qt::Key_Right || keyEvent->key() == Qt::Key_Down) {
        QWidget *next = widget->nextInFocusChain();
        if (next->parentWidget() != widget->parentWidget() || !qobject_cast<QToolButton *>(next)->isEnabled()) {
            return false;
        }

        next->setFocus(Qt::TabFocusReason);
        event->accept();
        return true;
    }

    return false;
}

QWidget *ZoomWidgetAction::createWidget(QWidget *parent)
{
    if (qobject_cast<QToolBar *>(parent)) {
        return KToolBarPopupAction::createWidget(parent);
    }

    ZoomWidget *zoomWidget = new ZoomWidget(parent);
    QHBoxLayout *zoomWidgetLayout = new QHBoxLayout;
    zoomWidgetLayout->setContentsMargins(0, 2, 0, 2);
    zoomWidget->setLayout(zoomWidgetLayout);
    zoomWidget->setFocusPolicy(Qt::StrongFocus);

    QSpacerItem *zoomSpacer = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
    zoomWidgetLayout->addSpacerItem(zoomSpacer);

    QStyleOptionMenuItem option;
    option.initFrom(zoomWidget);
    option.menuItemType = QStyleOptionMenuItem::Normal;
    option.icon = icon();
    option.text = text();
    int maxButtonSize = parent->style()->sizeFromContents(QStyle::CT_MenuItem, &option, QSize()).height() - 4;

    QToolButton *zoomOutButton = new QToolButton(zoomWidget);
    zoomOutButton->setMaximumSize(maxButtonSize, maxButtonSize);
    zoomOutButton->setDefaultAction(m_zoomOutAction);
    zoomOutButton->installEventFilter(this);
    zoomWidgetLayout->addWidget(zoomOutButton);
    zoomWidget->setFocusProxy(zoomOutButton);

    QIcon zoomOutIcon;
    QPixmap zoomOutPixmapNormal = m_zoomOutAction->icon().pixmap(zoomOutButton->iconSize(), QIcon::Normal);
    QPixmap zoomOutPixmapDisabled = m_zoomOutAction->icon().pixmap(zoomOutButton->iconSize(), QIcon::Disabled);
    QPixmap zoomOutPixmapActive = m_zoomOutAction->icon().pixmap(zoomOutButton->iconSize(), QIcon::Active);
    zoomOutIcon.addPixmap(zoomOutPixmapNormal, QIcon::Normal);
    zoomOutIcon.addPixmap(zoomOutPixmapDisabled, QIcon::Disabled);
    zoomOutIcon.addPixmap(zoomOutPixmapActive, QIcon::Active);
    zoomOutIcon.addPixmap(zoomOutPixmapNormal, QIcon::Selected);
    m_zoomOutAction->setIcon(zoomOutIcon);

    QToolButton *zoomResetButton = new QToolButton(zoomWidget);
    zoomResetButton->setMaximumSize(maxButtonSize, maxButtonSize);
    zoomResetButton->setDefaultAction(m_zoomResetAction);
    zoomResetButton->installEventFilter(this);
    zoomWidgetLayout->addWidget(zoomResetButton);

    QIcon zoomResetIcon;
    QPixmap zoomResetPixmapNormal = m_zoomResetAction->icon().pixmap(zoomResetButton->iconSize(), QIcon::Normal);
    QPixmap zoomResetPixmapDisabled = m_zoomResetAction->icon().pixmap(zoomResetButton->iconSize(), QIcon::Disabled);
    QPixmap zoomResetPixmapActive = m_zoomResetAction->icon().pixmap(zoomResetButton->iconSize(), QIcon::Active);
    zoomResetIcon.addPixmap(zoomResetPixmapNormal, QIcon::Normal);
    zoomResetIcon.addPixmap(zoomResetPixmapDisabled, QIcon::Disabled);
    zoomResetIcon.addPixmap(zoomResetPixmapActive, QIcon::Active);
    zoomResetIcon.addPixmap(zoomResetPixmapNormal, QIcon::Selected);
    m_zoomResetAction->setIcon(zoomResetIcon);

    QToolButton *zoomInButton = new QToolButton(zoomWidget);
    zoomInButton->setMaximumSize(maxButtonSize, maxButtonSize);
    zoomInButton->setDefaultAction(m_zoomInAction);
    zoomInButton->installEventFilter(this);
    zoomWidgetLayout->addWidget(zoomInButton);

    QIcon zoomInIcon;
    QPixmap zoomInPixmapNormal = m_zoomInAction->icon().pixmap(zoomInButton->iconSize(), QIcon::Normal);
    QPixmap zoomInPixmapDisabled = m_zoomInAction->icon().pixmap(zoomInButton->iconSize(), QIcon::Disabled);
    QPixmap zoomInPixmapActive = m_zoomInAction->icon().pixmap(zoomInButton->iconSize(), QIcon::Active);
    zoomInIcon.addPixmap(zoomInPixmapNormal, QIcon::Normal);
    zoomInIcon.addPixmap(zoomInPixmapDisabled, QIcon::Disabled);
    zoomInIcon.addPixmap(zoomInPixmapActive, QIcon::Active);
    zoomInIcon.addPixmap(zoomInPixmapNormal, QIcon::Selected);
    m_zoomInAction->setIcon(zoomInIcon);

    return zoomWidget;
}
