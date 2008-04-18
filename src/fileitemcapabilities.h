/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef FILEITEMCAPABILITIES_H
#define FILEITEMCAPABILITIES_H

class KFileItemList;

/**
 * @brief Provides information about the access capabilities of file items.
 *
 * As soon as one file item does not support a specific capability, it is
 * marked as unsupported for all items.
 */
class FileItemCapabilities
{
public:
    FileItemCapabilities(const KFileItemList& items);
    virtual ~FileItemCapabilities();
    bool supportsReading() const;
    bool supportsDeleting() const;
    bool supportsWriting() const;
    bool supportsMoving() const;
    bool isLocal() const;

private:
    bool m_supportsReading : 1;
    bool m_supportsDeleting : 1;
    bool m_supportsWriting : 1;
    bool m_supportsMoving : 1;
    bool m_isLocal : 1;
};

inline bool FileItemCapabilities::supportsReading() const
{
    return m_supportsReading;
}

inline bool FileItemCapabilities::supportsDeleting() const
{
    return m_supportsDeleting;
}

inline bool FileItemCapabilities::supportsWriting() const
{
    return m_supportsWriting;
}

inline bool FileItemCapabilities::supportsMoving() const
{
    return m_supportsMoving;
}

inline bool FileItemCapabilities::isLocal() const
{
    return m_isLocal;
}

#endif
