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

#include "searchcriterionselector.h"

#include "searchcriterionvalue.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QList>
#include <QPushButton>

#include <kicon.h>
#include <klocale.h>

SearchCriterionSelector::SearchCriterionSelector(QWidget* parent) :
    QWidget(parent),
    m_layout(0),
    m_descriptionsBox(0),
    m_comparatorBox(0),
    m_valueWidget(0),
    m_removeButton(0),
    m_descriptions()
{
    m_descriptionsBox = new QComboBox(this);
    m_descriptionsBox->addItem(i18nc("@label", "Select..."), -1);
    createDescriptions();
    connect(m_descriptionsBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotDescriptionChanged(int)));

    m_comparatorBox = new QComboBox(this);
    m_comparatorBox->hide();
    connect(m_comparatorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(updateQuery()));

    QWidget* filler = new QWidget(this);
    filler->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_removeButton = new QPushButton(this);
    m_removeButton->setIcon(KIcon("list-remove"));
    m_removeButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(m_removeButton, SIGNAL(clicked()), this, SIGNAL(removeCriterion()));

    m_layout = new QHBoxLayout(this);
    m_layout->setMargin(0);
    m_layout->addWidget(m_descriptionsBox);
    m_layout->addWidget(m_comparatorBox);
    m_layout->addWidget(filler);
    m_layout->addWidget(m_removeButton);

    setLayout(m_layout);
}

SearchCriterionSelector::~SearchCriterionSelector()
{
}

void SearchCriterionSelector::createDescriptions()
{
    // TODO: maybe this creation should be forwarded to a factory if
    // the number of items increases in future

    QList<SearchCriterionDescription::Comparator> comparators;
    comparators.append(SearchCriterionDescription::Comparator(i18nc("@label", "greater than"), ">", "+"));
    comparators.append(SearchCriterionDescription::Comparator(i18nc("@label", "greater than or equal to"), ">=", "+"));
    comparators.append(SearchCriterionDescription::Comparator(i18nc("@label", "less than"), "<", "+"));
    comparators.append(SearchCriterionDescription::Comparator(i18nc("@label", "less than or equal to"), "<=", "+"));

    // add "Date" description
    DateValue* dateValue = new DateValue(this);
    dateValue->hide();
    SearchCriterionDescription date(i18nc("@label", "Date Modified"),
                                    "sourceModified",
                                    comparators,
                                    dateValue);

    // add "File Size" description
    FileSizeValue* fileSizeValue = new FileSizeValue(this);
    fileSizeValue->hide();
    SearchCriterionDescription size(i18nc("@label", "File Size"),
                                    "contentSize",
                                    comparators,
                                    fileSizeValue);

    m_descriptions.append(date);
    m_descriptions.append(size);

    // add all descriptions to the combo box
    const int count = m_descriptions.count();
    for (int i = 0; i < count; ++i) {
        m_descriptionsBox->addItem(m_descriptions[i].name(), i);
    }
}

void SearchCriterionSelector::slotDescriptionChanged(int index)
{
    m_comparatorBox->clear();
    m_comparatorBox->show();
    if (m_valueWidget != 0) {
        m_layout->removeWidget(m_valueWidget);
        // the value widget is obtained by the Search Criterion
        // Selector instance and may not get deleted
    }

    // adjust the comparator box and the value widget dependent from the selected description
    m_comparatorBox->addItem(i18nc("@label", "Select..."), -1);
    const int descrIndex = m_descriptionsBox->itemData(index).toInt();
    if (descrIndex >= 0) {
        // add comparator items
        const SearchCriterionDescription& description = m_descriptions[descrIndex];
        foreach (const SearchCriterionDescription::Comparator& comp, description.comparators()) {
            m_comparatorBox->addItem(comp.name);
        }

        // add value widget
        m_valueWidget = description.valueWidget();
        const int layoutIndex = m_layout->count() - 2;
        m_layout->insertWidget(layoutIndex, m_valueWidget);
        m_valueWidget->show();
    }
}

void SearchCriterionSelector::updateQuery()
{
    const SearchCriterionDescription* descr = description();
    if (descr == 0) {
        // no description has been selected
        return;
    }

    // get selected comparator related to the description
    const int compBoxIndex = m_comparatorBox->currentIndex();
    const int compIndex = m_comparatorBox->itemData(compBoxIndex).toInt();
    if (compIndex < 0) {
        // no comparator has been selected
        return;
    }

    // create query string
    const SearchCriterionDescription::Comparator& comp = descr->comparators()[compIndex];
    const QString queryString = comp.prefix + descr->identifier() + comp.operation + m_valueWidget->value();
    emit criterionChanged(queryString);
}

const SearchCriterionDescription* SearchCriterionSelector::description() const
{
    const int descrBoxIndex = m_descriptionsBox->currentIndex();
    const int descrIndex = m_descriptionsBox->itemData(descrBoxIndex).toInt();
    return (descrIndex < 0) ? 0 : &m_descriptions[descrIndex];
}

#include "searchcriterionselector.moc"
