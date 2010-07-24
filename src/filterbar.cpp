/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
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

#include <QBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QToolButton>

#include <kicon.h>
#include <klocale.h>
#include <klineedit.h>
#include <kiconloader.h>

FilterBar::FilterBar(QWidget* parent) :
    QWidget(parent)
{
    // Create close button
    QToolButton *closeButton = new QToolButton(this);
    closeButton->setAutoRaise(true);
    closeButton->setIcon(KIcon("dialog-close"));
    closeButton->setToolTip(i18nc("@info:tooltip", "Hide Filter Bar"));
    connect(closeButton, SIGNAL(clicked()), this, SIGNAL(closeRequest()));

    // Create label
    QLabel* filterLabel = new QLabel(i18nc("@label:textbox", "Filter:"), this);

    // Create filter editor
    m_filterInput = new KLineEdit(this);
    m_filterInput->setLayoutDirection(Qt::LeftToRight);
    m_filterInput->setClearButtonShown(true);
    connect(m_filterInput, SIGNAL(textChanged(const QString&)),
            this, SIGNAL(filterChanged(const QString&)));

    // Apply layout
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->addWidget(closeButton);
    hLayout->addWidget(filterLabel);
    hLayout->addWidget(m_filterInput);

    filterLabel->setBuddy(m_filterInput);
}

FilterBar::~FilterBar()
{
}

void FilterBar::clear()
{
    m_filterInput->clear();
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
    if ((event->key() == Qt::Key_Escape)) {
        if (m_filterInput->text().isEmpty()) {
            emit closeRequest();
        } else {
            m_filterInput->clear();
        }
    }
}

#include "filterbar.moc"
