/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef DOLPHINSEARCHINFORMATION_H
#define DOLPHINSEARCHINFORMATION_H

class KUrl;

/**
 * @brief Allows to access search-engine related information.
 */
class DolphinSearchInformation
{
public:
    static DolphinSearchInformation& instance();
    virtual ~DolphinSearchInformation();

    /**
     * @return True if the Nepomuk indexer is enabled. If Nepomuk is
     *         disabled, always false is returned.
     */
    bool isIndexingEnabled() const;

    /**
     * @return True if the complete directory tree specified by path
     *         is indexed by the Nepomuk indexer. If Nepomuk is disabled,
     *         always false is returned.
     */
    bool isPathIndexed(const KUrl& url) const;

protected:
    DolphinSearchInformation();

private:
    bool m_indexingEnabled;

    friend class DolphinSearchInformationSingleton;
};

#endif

