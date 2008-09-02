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

#include <QtGui/QBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QToolButton>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kiconloader.h>

FilterBar::FilterBar(QWidget* parent) :
    QWidget(parent)
{
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);

    m_close = new QToolButton(this);
    m_close->setAutoRaise(true);
    m_close->setIcon(KIcon("dialog-close"));
    m_close->setToolTip(i18nc("@info:tooltip", "Hide Filter Bar"));
    hLayout->addWidget(m_close);
    hLayout->addSpacing(KDialog::spacingHint());

    m_filter = new QLabel(i18nc("@label:textbox", "Filter:"), this);
    hLayout->addWidget(m_filter);

    m_filterInput = new KLineEdit(this);
    m_filterInput->setLayoutDirection(Qt::LeftToRight);
    m_filterInput->setClearButtonShown(true);
    m_filter->setBuddy(m_filterInput);
    hLayout->addWidget(m_filterInput);

    connect(m_filterInput, SIGNAL(textChanged(const QString&)),
            this, SIGNAL(filterChanged(const QString&)));
    connect(m_close, SIGNAL(clicked()), this, SIGNAL(closeRequest()));
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
        emit closeRequest();
    }
}

#include "filterbar.moc"
