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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "filterbar.h"

#include <QBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QToolButton>

#include <kdialog.h>
#include <klocale.h>
#include <klineedit.h>
#include <kiconloader.h>

#include "dolphin.h"

FilterBar::FilterBar(QWidget *parent, const char *name) :
    QWidget(parent, name)
{
    const int gap = 3;

    QVBoxLayout* foo = new QVBoxLayout(this);
    foo->setMargin(0);
    foo->addSpacing(gap);

    QHBoxLayout* layout = new QHBoxLayout(foo);
    layout->setMargin(0);
    layout->addSpacing(gap);

    m_filter = new QLabel(i18n("Filter:"), this);
    layout->addWidget(m_filter);
    layout->addSpacing(KDialog::spacingHint());

    m_filterInput = new KLineEdit(this);
    m_filter->setBuddy(m_filterInput);
    layout->addWidget(m_filterInput);

    m_close = new QToolButton(this);
    m_close->setAutoRaise(true);
    m_close->setIcon(QIcon(SmallIcon("fileclose")));
    layout->addWidget(m_close);
    layout->addSpacing(gap);

    connect(m_filterInput, SIGNAL(textChanged(const QString&)),
            this, SIGNAL(signalFilterChanged(const QString&)));
    connect(m_close, SIGNAL(clicked()), this, SLOT(hide()));
    connect(m_close, SIGNAL(clicked()),
            &Dolphin::mainWin(), SLOT(slotShowFilterBarChanged()));
}

FilterBar::~FilterBar()
{
}

void FilterBar::hideEvent(QHideEvent* event)
{
    if (!event->spontaneous()) {
        m_filterInput->clear();
        m_filterInput->clearFocus();
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
    if ((event->key() == Qt::Key_Escape)) {
        hide();
        Dolphin::mainWin().slotShowFilterBarChanged();
    }
}

#include "filterbar.moc"
