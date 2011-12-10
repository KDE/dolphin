/***************************************************************************
 *   Copyright (C) 2011 by Janardhan Reddy                                 *
 *   <annapareddyjanardhanreddy@gmail.com>                                 *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#include "kfileitemmodelfilter_p.h"

#include <KFileItem>
#include <QRegExp>

KFileItemModelFilter::KFileItemModelFilter() :
    m_useRegExp(false),
    m_regExp(0),
    m_lowerCasePattern(),
    m_pattern()
{
}

KFileItemModelFilter::~KFileItemModelFilter()
{
    delete m_regExp;
    m_regExp = 0;
}

void KFileItemModelFilter::setPattern(const QString& filter)
{
    m_pattern = filter;
    m_lowerCasePattern = filter.toLower();

    m_useRegExp = filter.contains('*') ||
                  filter.contains('?') ||
                  filter.contains('[');
    if (m_useRegExp) {
        if (!m_regExp) {
            m_regExp = new QRegExp();
            m_regExp->setCaseSensitivity(Qt::CaseInsensitive);
            m_regExp->setMinimal(false);
            m_regExp->setPatternSyntax(QRegExp::WildcardUnix);
        }
        m_regExp->setPattern(filter);
    }
}

QString KFileItemModelFilter::pattern() const
{
    return m_pattern;
}

bool KFileItemModelFilter::matches(const KFileItem& item) const
{
    if (m_useRegExp) {
        return m_regExp->exactMatch(item.text());
    } else {
        return item.text().toLower().contains(m_lowerCasePattern);
    }
}
