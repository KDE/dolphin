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

#include "dolphinsearchoptionsconfigurator.h"

#include <kcombobox.h>
#include <klocale.h>

#include <QButtonGroup>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

DolphinSearchOptionsConfigurator::DolphinSearchOptionsConfigurator(QWidget* parent) :
    QWidget(parent)
{
    QVBoxLayout* vBoxLayout = new QVBoxLayout(this);

    // add "search" configuration
    QLabel* searchLabel = new QLabel(i18nc("@label", "Search:"));

    KComboBox* searchFromBox = new KComboBox();
    searchFromBox->addItem(i18nc("label", "Everywhere"));
    searchFromBox->addItem(i18nc("label", "From Here"));

    // add "what" configuration
    QLabel* whatLabel = new QLabel(i18nc("@label", "What:"));

    KComboBox* searchWhatBox = new KComboBox();
    searchWhatBox->addItem(i18nc("label", "All"));
    searchWhatBox->addItem(i18nc("label", "Images"));
    searchWhatBox->addItem(i18nc("label", "Texts"));
    searchWhatBox->addItem(i18nc("label", "File Names"));

    QWidget* filler = new QWidget();

    // add button "Save"
    QPushButton* saveButton = new QPushButton();
    saveButton->setText(i18nc("@action:button", "Save"));

    QHBoxLayout* hBoxLayout = new QHBoxLayout(this);
    hBoxLayout->addWidget(searchLabel);
    hBoxLayout->addWidget(searchFromBox);
    hBoxLayout->addWidget(whatLabel);
    hBoxLayout->addWidget(searchWhatBox);
    hBoxLayout->addWidget(filler, 1);
    hBoxLayout->addWidget(saveButton);

    vBoxLayout->addLayout(hBoxLayout);
}

DolphinSearchOptionsConfigurator::~DolphinSearchOptionsConfigurator()
{
}

#include "dolphinsearchoptionsconfigurator.moc"
