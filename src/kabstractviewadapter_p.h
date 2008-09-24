/*******************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>                   *
 *                                                                             *
 *   This library is free software; you can redistribute it and/or             *
 *   modify it under the terms of the GNU Library General Public               *
 *   License as published by the Free Software Foundation; either              *
 *   version 2 of the License, or (at your option) any later version.          *
 *                                                                             *
 *   This library is distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 *   Library General Public License for more details.                          *
 *                                                                             *
 *   You should have received a copy of the GNU Library General Public License *
 *   along with this library; see the file COPYING.LIB.  If not, write to      *
 *   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 *   Boston, MA 02110-1301, USA.                                               *
 *******************************************************************************/

#ifndef KABSTRACTVIEWADAPTER_H
#define KABSTRACTVIEWADAPTER_H

#include <QObject>

class QModelIndex;
class KDirModel;
class QPalette;
class QRect;
class QSize;

/*
 * Interface used by KFilePreviewGenerator to generate previews
 * for files. The interface allows KFilePreviewGenerator to be
 * independent from the view implementation.
 */
class KAbstractViewAdapter : public QObject
{
public:
    enum Signal { ScrollBarValueChanged };

    KAbstractViewAdapter(QObject *parent) : QObject(parent) {}
    virtual ~KAbstractViewAdapter() {}
    virtual QObject *createMimeTypeResolver(KDirModel*) const = 0;
    virtual QSize iconSize() const = 0;
    virtual QPalette palette() const = 0;
    virtual QRect visibleArea() const = 0;
    virtual QRect visualRect(const QModelIndex &index) const = 0;
    virtual void connect(Signal signal, QObject *receiver, const char *slot) = 0;
};

#endif

