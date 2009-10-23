/***************************************************************************
 *   Copyright (C) 2009 by Adam Kidder <thekidder@gmail.com>               *
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

#include "searchcriterionvalue.h"

#include <klineedit.h>
#include <klocale.h>

#include <QComboBox>
#include <QDateEdit>
#include <QLabel>
#include <QHBoxLayout>

SearchCriterionValue::SearchCriterionValue(QWidget* parent) :
    QWidget(parent)
{
}

SearchCriterionValue::~SearchCriterionValue()
{
}



DateValue::DateValue(QWidget* parent) :
    SearchCriterionValue(parent),
    m_dateEdit(0)
{
    m_dateEdit = new QDateEdit(this);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_dateEdit);
}

DateValue::~DateValue()
{
}

QString DateValue::value() const
{
    return QString();
}



FileSizeValue::FileSizeValue(QWidget* parent) :
    SearchCriterionValue(parent),
    m_lineEdit(0),
    m_units(0)
{
    m_lineEdit = new KLineEdit(this);
    m_lineEdit->setClearButtonShown(true);

    m_units = new QComboBox(this);
    // TODO: check the KByte vs. KiByte dilemma :-/
    m_units->addItem(i18nc("@label", "Byte"));
    m_units->addItem(i18nc("@label", "KByte"));
    m_units->addItem(i18nc("@label", "MByte"));
    m_units->addItem(i18nc("@label", "GByte"));

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_units);
}

FileSizeValue::~FileSizeValue()
{
}

QString FileSizeValue::value() const
{
    return QString();
}

#include "searchcriterionvalue.moc"
