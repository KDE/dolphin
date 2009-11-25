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

#ifndef SEARCHCRITERIONDESCRIPTION_H
#define SEARCHCRITERIONDESCRIPTION_H

#define DISABLE_NEPOMUK_LEGACY
#include <nepomuk/comparisonterm.h>

#include <QList>
#include <QString>
#include <QUrl>

class SearchCriterionValue;
class QWidget;

/**
 * @brief Helper class for SearchCriterionSelector.
 *
 * Describes a search criterion including the used
 * widget for editing.
 */
class SearchCriterionDescription
{
public:
    struct Comparator
    {
        Comparator(const QString& n) :
            name(n),
            isActive(false),
            value(Nepomuk::Query::ComparisonTerm::Smaller),
            autoValueType()
        {
        }

        Comparator(const QString& n, Nepomuk::Query::ComparisonTerm::Comparator c,
                   const QString& a = QString()) :
            name(n),
            isActive(true),
            value(c),
            autoValueType(a)
        {
        }

        QString name;          // user visible and translated name
        bool isActive;
        Nepomuk::Query::ComparisonTerm::Comparator value;
        QString autoValueType; // type for an automatically calculated value of the value widget
    };

    SearchCriterionDescription(const QString& name,
                               const QUrl& identifier,
                               const QList<Comparator>& comparators,
                               SearchCriterionValue* valueWidget);

    virtual ~SearchCriterionDescription();

    QString name() const;
    QUrl identifier() const;
    const QList<Comparator>& comparators() const;
    SearchCriterionValue* valueWidget() const;

private:
    QString m_name;       // user visible name that gets translated
    QUrl m_identifier;    // internal Nepomuk identifier URL
    QList<Comparator> m_comparators;
    SearchCriterionValue* m_valueWidget;
};

#endif // SEARCHCRITERIONDESCRIPTION_H
