/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
 *   Copyright (C) 2012 by Stuart Citrin <ctrn3e8@gmail.com>               *
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
#include "filterbar.h"

#include <QKeyEvent>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>

#include <KLocalizedString>
#include <QLineEdit>

FilterBar::FilterBar(QWidget* parent) :
    QWidget(parent)
{
    // Create close button
    QToolButton *closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(QIcon::fromTheme(QStringLiteral("dialog-close")));
    closeButton->setToolTip(i18nc("@info:tooltip", "Hide Filter Bar"));
    connect(closeButton, &QToolButton::clicked, this, &FilterBar::closeRequest);

    // Create button to lock text when changing folders
    m_lockButton = new QToolButton(this);
    m_lockButton->setAutoRaise(true);
    m_lockButton->setCheckable(true);
    m_lockButton->setIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
    m_lockButton->setToolTip(i18nc("@info:tooltip", "Keep Filter When Changing Folders"));
    connect(m_lockButton, &QToolButton::toggled, this, &FilterBar::slotToggleLockButton);

    // Create label
    QLabel* filterLabel = new QLabel(i18nc("@label:textbox", "Filter:"), this);

    // Create filter editor
    m_filterInput = new QLineEdit(this);
    m_filterInput->setLayoutDirection(Qt::LeftToRight);
    m_filterInput->setClearButtonEnabled(true);
    connect(m_filterInput, &QLineEdit::textChanged,
            this, &FilterBar::filterChanged);
    setFocusProxy(m_filterInput);

    // Apply layout
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->addWidget(closeButton);
    hLayout->addWidget(filterLabel);
    hLayout->addWidget(m_filterInput);
    hLayout->addWidget(m_lockButton);

    filterLabel->setBuddy(m_filterInput);
}

FilterBar::~FilterBar()
{
}

void FilterBar::closeFilterBar()
{
    hide();
    clear();
    if (m_lockButton) {
        m_lockButton->setChecked(false);
    }
}

void FilterBar::selectAll()
{
    m_filterInput->selectAll();
}

void FilterBar::clear()
{
    m_filterInput->clear();
}

void FilterBar::slotUrlChanged()
{
    if (!m_lockButton || !(m_lockButton->isChecked())) {
        clear();
    }
}

void FilterBar::slotToggleLockButton(bool checked)
{
    if (checked) {
        m_lockButton->setIcon(QIcon::fromTheme(QStringLiteral("object-locked")));
    } else {
        m_lockButton->setIcon(QIcon::fromTheme(QStringLiteral("object-unlocked")));
        clear();
    }
}

void FilterBar::showEvent(QShowEvent* event)
{
    if (!event->spontaneous()) {
        m_filterInput->setFocus();
    }
}

void FilterBar::keyReleaseEvent(QKeyEvent* event)
{
    QWidget::keyReleaseEvent(event);

    switch (event->key()) {
    case Qt::Key_Escape:
        if (m_filterInput->text().isEmpty()) {
            emit closeRequest();
        } else {
            m_filterInput->clear();
        }
        break;

    case Qt::Key_Enter:
    case Qt::Key_Return:
        emit focusViewRequest();
        break;

    default:
        break;
    }
}

