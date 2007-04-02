/*  This file is part of the KDE project
    Copyright (C) 2007 Kevin Ottens <ervin@kde.org>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License version 2 as published by the Free Software Foundation.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.

*/
#ifndef KFILEPLACESVIEW_H
#define KFILEPLACESVIEW_H

#include <kdelibs_export.h>

#include <QListView>

#include <kurl.h>

class QAbstractItemModel;

/**
 * This class allows to display a KFilePlacesModel.
 */
class KIO_EXPORT KFilePlacesView : public QListView
{
    Q_OBJECT
public:
    KFilePlacesView(QWidget *parent = 0);
    ~KFilePlacesView();

public Q_SLOTS:
    void setUrl(const KUrl &url);

Q_SIGNALS:
    void urlChanged(const KUrl &url);

private:
    Q_PRIVATE_SLOT(d, void _k_placeClicked(const QModelIndex &))

    class Private;
    Private * const d;
    friend class Private;
};

#endif
