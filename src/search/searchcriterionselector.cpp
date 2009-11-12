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

SearchCriterionSelector::SearchCriterionSelector(Type type, QWidget* parent) :
    QWidget(parent),
    m_layout(0),
    m_descriptionsBox(0),
    m_comparatorBox(0),
    m_valueWidget(0),
    m_removeButton(0),
    m_descriptions()
{
    m_descriptionsBox = new QComboBox(this);

    m_comparatorBox = new QComboBox(this);
    m_comparatorBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    createDescriptions();
    const int index = static_cast<int>(type);
    m_descriptionsBox->setCurrentIndex(index);

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

    slotDescriptionChanged(index);
    connect(m_descriptionsBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotDescriptionChanged(int)));
}

SearchCriterionSelector::~SearchCriterionSelector()
{
}

void SearchCriterionSelector::createDescriptions()
{
    Q_ASSERT(m_descriptionsBox != 0);
    Q_ASSERT(m_comparatorBox != 0);

    // TODO: maybe this creation should be forwarded to a factory if
    // the number of items increases in future
    QList<SearchCriterionDescription::Comparator> defaultComps;
    defaultComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "Greater Than"), ">", "+"));
    defaultComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "Greater Than or Equal to"), ">=", "+"));
    defaultComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "Less Than"), "<", "+"));
    defaultComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "Less Than or Equal to"), "<=", "+"));

    // add "Date" description
    QList<SearchCriterionDescription::Comparator> dateComps;
    dateComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "Anytime"))); // TODO
    dateComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "Today"))); // TODO
    dateComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "This week"))); // TODO
    dateComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "This month"))); // TODO
    foreach (const SearchCriterionDescription::Comparator& comp, defaultComps) {
        dateComps.append(comp);
    }

    DateValue* dateValue = new DateValue(this);
    dateValue->hide();
    SearchCriterionDescription date(i18nc("@label", "Date"),
                                    "sourceModified",
                                    dateComps,
                                    dateValue);

    // add "Tag" description
    QList<SearchCriterionDescription::Comparator> tagComps;
    tagComps.append(SearchCriterionDescription::Comparator(i18nc("@label All (tags)", "All")));
    tagComps.append(SearchCriterionDescription::Comparator(i18nc("@label", "Equal to"), "=="));

    TagValue* tagValue = new TagValue(this);
    tagValue->hide();
    SearchCriterionDescription tag(i18nc("@label", "Tag"),
                                   "tag",
                                   tagComps,
                                   tagValue);

    // add "Size" description
    QList<SearchCriterionDescription::Comparator> sizeComps = defaultComps;
    sizeComps.insert(0, SearchCriterionDescription::Comparator(i18nc("@label Any (file size)", "Any")));

    SizeValue* sizeValue = new SizeValue(this);
    sizeValue->hide();
    SearchCriterionDescription size(i18nc("@label", "Size"),
                                    "contentSize",
                                    sizeComps,
                                    sizeValue);

    m_descriptions.append(date);
    m_descriptions.append(tag);
    m_descriptions.append(size);

    // add all descriptions to the combo box
    const int count = m_descriptions.count();
    for (int i = 0; i < count; ++i) {
        m_descriptionsBox->addItem(m_descriptions[i].name(), i);
    }
}

void SearchCriterionSelector::slotDescriptionChanged(int index)
{
    if (m_valueWidget != 0) {
        m_valueWidget->hide();
        m_layout->removeWidget(m_valueWidget);
        m_valueWidget = 0;
        // the value widget is obtained by the Search Criterion
        // Selector instance and may not get deleted
    }

    // add comparator items
    disconnect(m_comparatorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComparatorChanged(int)));
    m_comparatorBox->clear();

    const SearchCriterionDescription& description = m_descriptions[index];
    foreach (const SearchCriterionDescription::Comparator& comp, description.comparators()) {
        m_comparatorBox->addItem(comp.name);
    }

    // add value widget
    m_valueWidget = description.valueWidget();
    m_layout->insertWidget(2, m_valueWidget);

    m_comparatorBox->setCurrentIndex(0);
    slotComparatorChanged(0);
    connect(m_comparatorBox, SIGNAL(currentIndexChanged(int)), this, SLOT(slotComparatorChanged(int)));
}

void SearchCriterionSelector::slotComparatorChanged(int index)
{
    Q_ASSERT(index >= 0);

    // only show the value widget if an operation is defined by the comparator
    const int descIndex = m_descriptionsBox->currentIndex();
    const SearchCriterionDescription& descr = m_descriptions[descIndex];
    const SearchCriterionDescription::Comparator& comp = descr.comparators()[index];
    m_valueWidget->setVisible(!comp.operation.isEmpty());

    // create query string
    const QString queryString = comp.prefix + descr.identifier() + comp.operation + m_valueWidget->value();
    emit criterionChanged(queryString);
}

#include "searchcriterionselector.moc"
