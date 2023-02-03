/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2022 Felix Ernst <felixernst@zohomail.eu>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#include "actionwithwidget.h"

#include <QAbstractButton>
#include <QFrame>
#include <QPushButton>
#include <QToolButton>

using namespace SelectionMode;

ActionWithWidget::ActionWithWidget(QAction *action)
    : m_action{action}
{
}

ActionWithWidget::ActionWithWidget(QAction *action, QAbstractButton *button)
    : m_action{action}
    , m_widget{button}
{
    copyActionDataToButton(button, action);
}

QWidget *ActionWithWidget::newWidget(QWidget *parent)
{
    Q_CHECK_PTR(m_action);
    Q_ASSERT(!m_widget);

    if (m_action->isSeparator()) {
        auto line = new QFrame(parent);
        line->setFrameShape(QFrame::VLine);
        line->setFrameShadow(QFrame::Sunken);

        m_widget = line;
    } else {
        m_widget = newButtonForAction(m_action, parent);
    }
    return m_widget;
}

QAbstractButton *SelectionMode::newButtonForAction(QAction *action, QWidget *parent)
{
    Q_CHECK_PTR(action);
    Q_ASSERT(!action->isSeparator());

    if (action->priority() == QAction::LowPriority) {
        // We don't want the low priority actions to be displayed icon-only so we need trickery.
        auto button = new QPushButton(parent);
        copyActionDataToButton(static_cast<QAbstractButton *>(button), action);
        button->setMinimumWidth(0);
        return button;
    }

    auto *toolButton = new QToolButton(parent);
    toolButton->setToolButtonStyle(Qt::ToolButtonStyle::ToolButtonTextBesideIcon);
    toolButton->setDefaultAction(action);
    toolButton->setPopupMode(QToolButton::ToolButtonPopupMode::InstantPopup);
    toolButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    toolButton->setMinimumWidth(0);
    return toolButton;
}

void SelectionMode::copyActionDataToButton(QAbstractButton *button, QAction *action)
{
    button->setText(action->text());
    button->setIcon(action->icon());
    button->setToolTip(action->toolTip());
    button->setWhatsThis(action->whatsThis());

    button->setVisible(action->isVisible());
    button->setEnabled(action->isEnabled());

    QObject::connect(button, &QAbstractButton::clicked, action, &QAction::trigger);
}
