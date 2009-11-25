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

#include "searchcriteriondescription.h"

SearchCriterionDescription::SearchCriterionDescription(const QString& name,
                                                       const QUrl& identifier,
                                                       const QList<Comparator>& comparators,
                                                       SearchCriterionValue* valueWidget) :
    m_name(name),
    m_identifier(identifier),
    m_comparators(comparators),
    m_valueWidget(valueWidget)
{
}

SearchCriterionDescription::~SearchCriterionDescription()
{
}

QString SearchCriterionDescription::name() const
{
    return m_name;
}

QUrl SearchCriterionDescription::identifier() const
{
    return m_identifier;
}

const QList<SearchCriterionDescription::Comparator>& SearchCriterionDescription::comparators() const
{
    return m_comparators;
}

SearchCriterionValue* SearchCriterionDescription::valueWidget() const
{
    return m_valueWidget;
}
