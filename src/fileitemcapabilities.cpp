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

#include "fileitemcapabilities.h"

#include <kfileitem.h>
#include <kprotocolmanager.h>

#include <QFileInfo>

FileItemCapabilities::FileItemCapabilities(const KFileItemList& items) :
    m_supportsReading(true),
    m_supportsDeleting(true),
    m_supportsWriting(true),
    m_supportsMoving(true),
    m_isLocal(true)
{
    QFileInfo parentDirInfo;
    foreach (KFileItem item, items) {
        const KUrl url = item.url();
        m_isLocal = m_isLocal && url.isLocalFile();
        m_supportsReading  = m_supportsReading  && KProtocolManager::supportsReading(url);
        m_supportsDeleting = m_supportsDeleting && KProtocolManager::supportsReading(url);
        m_supportsWriting  = m_supportsWriting  && KProtocolManager::supportsWriting(url);
        m_supportsMoving   = m_supportsMoving   && KProtocolManager::supportsMoving(url);

        // The following code has been taken from kdebase/apps/lib/konq/konq_popupmenu.cpp:
        // For local files we can do better: check if we have write permission in parent directory
        if (m_isLocal && (m_supportsDeleting || m_supportsMoving)) {
            const QString directory = url.directory();
            if (parentDirInfo.filePath() != directory) {
                parentDirInfo.setFile(directory);
            }
            if (!parentDirInfo.isWritable()) {
                m_supportsDeleting = false;
                m_supportsMoving = false;
            }
        }
    }
}

FileItemCapabilities::~FileItemCapabilities()
{
}
