/***************************************************************************
 *   Copyright (C) 2006 by Oscar Blumberg                                  *
 *   o.blumberg@robertlan.eu.org                                           *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "metadataloader.h"

#include <kmetadata/kmetadata.h>
#include <kurl.h>
#include <QString>

MetadataLoader::MetadataLoader()
{
    if (Nepomuk::KMetaData::ResourceManager::instance()->init()) {
        m_up = false;
        Nepomuk::KMetaData::ResourceManager::instance()->setAutoSync(false);
    } else {
        m_up = true;
    }
}

MetadataLoader::~MetadataLoader()
{
}

bool MetadataLoader::storageUp() {
    return m_up;
}

QString MetadataLoader::getAnnotation(const KUrl& file)
{
    if(m_up)
        return Nepomuk::KMetaData::File(file.url()).getAnnotation();
    else
        return QString();
}

void MetadataLoader::setAnnotation(const KUrl& file, const QString& annotation)
{
    if(m_up)
        Nepomuk::KMetaData::File(file.url()).setAnnotation(annotation);
}

