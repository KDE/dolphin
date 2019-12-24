/***************************************************************************
 *   Copyright (C) 2011 by Janardhan Reddy                                 *
 *   <annapareddyjanardhanreddy@gmail.com>                                 *
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

#include "kfileitemmodelfilter.h"

#include <QRegularExpression>

#include <KFileItem>

KFileItemModelFilter::KFileItemModelFilter() :
    m_useRegExp(false),
    m_regExp(nullptr),
    m_lowerCasePattern(),
    m_pattern()
{
}

KFileItemModelFilter::~KFileItemModelFilter()
{
    delete m_regExp;
    m_regExp = nullptr;
}

void KFileItemModelFilter::setPattern(const QString& filter)
{
    m_pattern = filter;
    m_lowerCasePattern = filter.toLower();

    if (filter.contains('*') || filter.contains('?') || filter.contains('[')) {
        if (!m_regExp) {
            m_regExp = new QRegularExpression();
            m_regExp->setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
        m_regExp->setPattern(QRegularExpression::wildcardToRegularExpression(filter));
        m_useRegExp = m_regExp->isValid();
    } else {
        m_useRegExp = false;
    }
}

QString KFileItemModelFilter::pattern() const
{
    return m_pattern;
}

void KFileItemModelFilter::setMimeTypes(const QStringList& types)
{
    m_mimeTypes = types;
}

QStringList KFileItemModelFilter::mimeTypes() const
{
    return m_mimeTypes;
}

bool KFileItemModelFilter::hasSetFilters() const
{
    return (!m_pattern.isEmpty() || !m_mimeTypes.isEmpty());
}


bool KFileItemModelFilter::matches(const KFileItem& item) const
{
    const bool hasPatternFilter = !m_pattern.isEmpty();
    const bool hasMimeTypesFilter = !m_mimeTypes.isEmpty();

    // If no filter is set, return true.
    if (!hasPatternFilter && !hasMimeTypesFilter) {
        return true;
    }

    // If both filters are set, return true when both filters are matched
    if (hasPatternFilter && hasMimeTypesFilter) {
        return (matchesPattern(item) && matchesType(item));
    }

    // If only one filter is set, return true when that filter is matched
    if (hasPatternFilter) {
        return matchesPattern(item);
    }

    return matchesType(item);
}

bool KFileItemModelFilter::matchesPattern(const KFileItem& item) const
{
    if (m_useRegExp) {
        return m_regExp->match(item.text()).hasMatch();
    } else {
        return item.text().toLower().contains(m_lowerCasePattern);
    }
}

bool KFileItemModelFilter::matchesType(const KFileItem& item) const
{
    foreach (const QString& mimeType, m_mimeTypes) {
        if (item.mimetype() == mimeType) {
            return true;
        }
    }

    return m_mimeTypes.isEmpty();
}
