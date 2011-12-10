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

#ifndef KFILEITEMMODELFILTER_H
#define KFILEITEMMODELFILTER_H

#include <libdolphin_export.h>
#include <QString>

class KFileItem;
class QRegExp;

/**
 * @brief Allows to check whether an item of the KFileItemModel
 *        matches with a set filter-string.
 *
 * Currently the filter is only checked for the KFileItem::text()
 * property of the KFileItem, but this might get extended in
 * future.
 */
class LIBDOLPHINPRIVATE_EXPORT KFileItemModelFilter
{

public:
    KFileItemModelFilter();
    virtual ~KFileItemModelFilter();

    /**
     * Sets the pattern that is used for a comparison with the item
     * in KFileItemModelFilter::matches(). Per default the pattern
     * defines a sub-string. As soon as the pattern contains at least
     * a '*', '?' or '[' the pattern represents a regular expression.
     */
    void setPattern(const QString& pattern);
    QString pattern() const;

    /**
     * @return True if the item matches with the pattern defined by
     *         KFileItemModelFilter::setPattern().
     */
    bool matches(const KFileItem& item) const;

private:
    bool m_useRegExp;           // If true, m_regExp is used for filtering,
                                // otherwise m_lowerCaseFilter is used.
    QRegExp* m_regExp;
    QString m_lowerCasePattern; // Lowercase version of m_filter for
                                // faster comparison in matches().
    QString m_pattern;          // Property set by setFilter().
};
#endif


