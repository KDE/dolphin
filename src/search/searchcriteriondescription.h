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

#include <QList>
#include <QString>

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
        Comparator(const QString& n, const QString& o = QString(),
                   const QString& p = QString(), const QString& a = QString()) :
            name(n), operation(o), prefix(p), autoValueType(a) {}
        QString name;          // user visible and translated name
        QString operation;     // Nepomuk operation that represents the comparator
        QString prefix;        // prefix like "+" or "-" that is part of the Nepomuk query
        QString autoValueType; // type for an automatically calculated value of the value widget
    };

    SearchCriterionDescription(const QString& name,
                               const QString& identifier,
                               const QList<Comparator>& comparators,
                               SearchCriterionValue* valueWidget);

    virtual ~SearchCriterionDescription();

    QString name() const;
    QString identifier() const;
    const QList<Comparator>& comparators() const;
    SearchCriterionValue* valueWidget() const;

private:
    QString m_name;       // user visible name that gets translated
    QString m_identifier; // internal Nepomuk identifier
    QList<Comparator> m_comparators;
    SearchCriterionValue* m_valueWidget;
};

#endif // SEARCHCRITERIONDESCRIPTION_H
