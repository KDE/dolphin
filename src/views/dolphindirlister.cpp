/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "dolphindirlister.h"
#include <KLocale>
#include <kio/jobclasses.h>

DolphinDirLister::DolphinDirLister() :
    KDirLister()
{
    setAutoErrorHandlingEnabled(false, 0);
}

DolphinDirLister::~DolphinDirLister()
{
}

void DolphinDirLister::handleError(KIO::Job* job)
{
    if (job->error() == KIO::ERR_IS_FILE) {
        emit urlIsFileError(url());
    } else {
        const QString errorString = job->errorString();
        if (errorString.isEmpty()) {
            emit errorMessage(i18nc("@info:status", "Unknown error."));
        } else {
            emit errorMessage(errorString);
        }
    }
}

#include "dolphindirlister.moc"
