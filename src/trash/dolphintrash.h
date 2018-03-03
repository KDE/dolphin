/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2018 by Roman Inflianskas <infroma@gmail.com>           *
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

#ifndef DOLPHINTRASH_H
#define DOLPHINTRASH_H

#include <QWidget>

#include <KIO/EmptyTrashJob>
#include <KIOWidgets/KDirLister>

class Trash: public QObject
{
    Q_OBJECT

public:
    // delete copy and move constructors and assign operators
    Trash(Trash const&) = delete;
    Trash(Trash&&) = delete;
    Trash& operator=(Trash const&) = delete;
    Trash& operator=(Trash &&) = delete;

    static Trash& instance();
    static KIO::Job* empty(QWidget *window);
    static bool isEmpty();

signals:
    void emptinessChanged(bool isEmpty);

private:
    KDirLister *m_trashDirLister;

    Trash();
    ~Trash();
};

#endif // DOLPHINTRASH_H
