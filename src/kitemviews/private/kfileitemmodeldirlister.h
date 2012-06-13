/***************************************************************************
 *   Copyright (C) 2006-2012 by Peter Penz <peter.penz19@gmail.com>        *
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

#ifndef KFILEITEMMODELDIRLISTER_H
#define KFILEITEMMODELDIRLISTER_H

#include <libdolphin_export.h>
#include <KDirLister>

/**
 * @brief Extends the class KDirLister by emitting a signal when an
 *        error occurred instead of showing an error dialog.
 *        KDirLister::autoErrorHandlingEnabled() is set to false.
 */
class LIBDOLPHINPRIVATE_EXPORT KFileItemModelDirLister : public KDirLister
{
    Q_OBJECT

public:
    KFileItemModelDirLister(QObject* parent = 0);
    virtual ~KFileItemModelDirLister();

signals:
    /** Is emitted whenever an error has occurred. */
    void errorMessage(const QString& msg);

    /**
     * Is emitted when the URL of the directory lister represents a file.
     * In this case no signal errorMessage() will be emitted.
     */
    void urlIsFileError(const KUrl& url);

protected:
    virtual void handleError(KIO::Job* job);
};

#endif
