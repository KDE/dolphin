/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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
#include "dolphinsearchbox.h"

#include <kdialog.h>
#include <kglobalsettings.h>
#include <klineedit.h>
#include <klocale.h>
#include <kicon.h>
#include <kiconloader.h>

#include <QEvent>
#include <QKeyEvent>
#include <QHBoxLayout>
#include <QToolButton>

DolphinSearchBox::DolphinSearchBox(QWidget* parent) :
    QWidget(parent),
    m_searchInput(0),
    m_searchButton(0)
{
    QHBoxLayout* hLayout = new QHBoxLayout(this);
    hLayout->setMargin(0);
    hLayout->setSpacing(0);

    m_searchInput = new KLineEdit(this);
    m_searchInput->setClearButtonShown(true);
    m_searchInput->setMinimumWidth(150);
    m_searchInput->setClickMessage(i18nc("@label:textbox", "Search..."));
    hLayout->addWidget(m_searchInput);
    connect(m_searchInput, SIGNAL(returnPressed()),
            this, SLOT(emitSearchSignal()));

    m_searchButton = new QToolButton(this);
    m_searchButton->setAutoRaise(true);
    m_searchButton->setIcon(KIcon("edit-find"));
    m_searchButton->setToolTip(i18nc("@info:tooltip", "Click to begin the search"));
    hLayout->addWidget(m_searchButton);
    connect(m_searchButton, SIGNAL(clicked()),
            this, SLOT(emitSearchSignal()));
}

DolphinSearchBox::~DolphinSearchBox()
{
}

bool DolphinSearchBox::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        m_searchInput->setFont(KGlobalSettings::generalFont());
    } else if (event->type() == QEvent::KeyPress) {
        if (static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape) {
            m_searchInput->clear();
        }
    }
    return QWidget::event(event);
}

void DolphinSearchBox::emitSearchSignal()
{
    emit search(KUrl("nepomuksearch:/" + m_searchInput->text()));
}

#include "dolphinsearchbox.moc"
