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

#include <kdatewidget.h>
#include <klineedit.h>
#include <klocale.h>

#include <nepomuk/tag.h>

#include <QComboBox>
#include <QDate>
#include <QIntValidator>
#include <QLabel>
#include <QHBoxLayout>
#include <QShowEvent>

SearchCriterionValue::SearchCriterionValue(QWidget* parent) :
    QWidget(parent)
{
}

SearchCriterionValue::~SearchCriterionValue()
{
}

// -------------------------------------------------------------------------

DateValue::DateValue(QWidget* parent) :
    SearchCriterionValue(parent),
    m_dateWidget(0)
{
    m_dateWidget = new KDateWidget(QDate::currentDate(), this);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_dateWidget);
}

DateValue::~DateValue()
{
}

QString DateValue::value() const
{
    return m_dateWidget->date().toString(Qt::ISODate);
}

// -------------------------------------------------------------------------

TagValue::TagValue(QWidget* parent) :
    SearchCriterionValue(parent),
    m_tags(0)
{
    m_tags = new QComboBox(this);
    m_tags->setInsertPolicy(QComboBox::InsertAlphabetically);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_tags);

    connect(m_tags, SIGNAL(activated(QString)),
            this, SIGNAL(valueChanged(QString)));
}

TagValue::~TagValue()
{
}

QString TagValue::value() const
{
    return m_tags->currentText();
}

void TagValue::showEvent(QShowEvent* event)
{
    if (!event->spontaneous() && (m_tags->count() == 0)) {
        const QList<Nepomuk::Tag> tags = Nepomuk::Tag::allTags();
        foreach (const Nepomuk::Tag& tag, tags) {
            m_tags->addItem(tag.label());
        }

        if (tags.count() == 0) {
            m_tags->addItem(i18nc("@label", "No Tags Available"));
        }
    }
    SearchCriterionValue::showEvent(event);
}

// -------------------------------------------------------------------------

SizeValue::SizeValue(QWidget* parent) :
    SearchCriterionValue(parent),
    m_lineEdit(0),
    m_units(0)
{
    m_lineEdit = new KLineEdit(this);
    m_lineEdit->setClearButtonShown(true);
    m_lineEdit->setValidator(new QIntValidator(this));
    m_lineEdit->setAlignment(Qt::AlignRight);

    m_units = new QComboBox(this);
    // TODO: check the KByte vs. KiByte dilemma :-/
    m_units->addItem(i18nc("@label", "Byte"));
    m_units->addItem(i18nc("@label", "KByte"));
    m_units->addItem(i18nc("@label", "MByte"));
    m_units->addItem(i18nc("@label", "GByte"));

    // set 1 MByte as default
    m_lineEdit->setText("1");
    m_units->setCurrentIndex(2);

    QHBoxLayout* layout = new QHBoxLayout(this);
    layout->setMargin(0);
    layout->addWidget(m_lineEdit);
    layout->addWidget(m_units);
}

SizeValue::~SizeValue()
{
}

QString SizeValue::value() const
{
    return QString();
}

#include "searchcriterionvalue.moc"
