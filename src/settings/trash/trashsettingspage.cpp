/***************************************************************************
 *   Copyright (C) 2009 by Shaun Reich shaun.reich@kdemail.net             *
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

#include "trashsettingspage.h"

#include <KCModuleProxy>

#include <QVBoxLayout>

TrashSettingsPage::TrashSettingsPage(QWidget* parent) :
        SettingsPageBase(parent)
{
    QVBoxLayout* topLayout = new QVBoxLayout(this);
    QWidget* vBox = new QWidget(this);
    QVBoxLayout *vBoxVBoxLayout = new QVBoxLayout(vBox);
    vBoxVBoxLayout->setMargin(0);

    m_proxy = new KCModuleProxy("kcmtrash");
    topLayout->addWidget(m_proxy);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    QWidget *w = new QWidget(vBox);
    vBoxVBoxLayout->addWidget(w);
    
    topLayout->addWidget(vBox);

    loadSettings();

    connect(m_proxy, static_cast<void(KCModuleProxy::*)(bool)>(&KCModuleProxy::changed), this, &TrashSettingsPage::changed);
}

TrashSettingsPage::~TrashSettingsPage()
{
}

void TrashSettingsPage::applySettings()
{
    m_proxy->save();
}

void TrashSettingsPage::restoreDefaults()
{
    m_proxy->defaults();
}

void TrashSettingsPage::loadSettings()
{
    m_proxy->load();
}

