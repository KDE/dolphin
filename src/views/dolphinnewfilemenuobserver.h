/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef DOLPHINNEWFILEMENUOBSERVER_H
#define DOLPHINNEWFILEMENUOBSERVER_H

#include <QObject>

#include "dolphin_export.h"

class DolphinNewFileMenu;

/**
 * @brief Allows to observe new file items that have been created
 *        by a DolphinNewFileMenu instance.
 *
 * As soon as a DolphinNewFileMenu instance created a new item,
 * the observer will emit the signal itemCreated().
 */
class DOLPHIN_EXPORT DolphinNewFileMenuObserver : public QObject
{
    Q_OBJECT

public:
    static DolphinNewFileMenuObserver& instance();
    void attach(const DolphinNewFileMenu* menu);
    void detach(const DolphinNewFileMenu* menu);

signals:
    void itemCreated(const QUrl& url);
    void errorMessage(const QString& error);

private:
    DolphinNewFileMenuObserver();
    virtual ~DolphinNewFileMenuObserver();

    friend class DolphinNewFileMenuObserverSingleton;
};

#endif
