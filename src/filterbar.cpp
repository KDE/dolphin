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

#include <qlabel.h>
#include <qlayout.h>
//Added by qt3to4:
#include <Q3VBoxLayout>
#include <QKeyEvent>
#include <Q3HBoxLayout>

#include <kdialog.h>
#include <klocale.h>
#include <kpushbutton.h>
#include <klineedit.h>
#include <kiconloader.h>

#include "dolphin.h"

FilterBar::FilterBar(QWidget *parent, const char *name) :
    QWidget(parent, name)
{
    const int gap = 3;

    Q3VBoxLayout* foo = new Q3VBoxLayout(this);
    foo->addSpacing(gap);

    Q3HBoxLayout* layout = new Q3HBoxLayout(foo);
    layout->addSpacing(gap);

    m_filter = new QLabel(i18n("Filter:"), this);
    layout->addWidget(m_filter);
    layout->addSpacing(KDialog::spacingHint());

    m_filterInput = new KLineEdit(this);
    layout->addWidget(m_filterInput);

    m_close = new KPushButton(this);
    m_close->setIconSet(SmallIcon("fileclose"));
    m_close->setFlat(true);
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

void FilterBar::hide()
{
    m_filterInput->clear();
    m_filterInput->clearFocus();
    QWidget::hide();
}

void FilterBar::show()
{
    m_filterInput->setFocus();
    QWidget::show();
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
