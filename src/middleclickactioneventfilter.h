/***************************************************************************
 *   Copyright (C) 2017 Kai Uwe Broulik <kde@privat.broulik.de>            *
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

#pragma once

#include "dolphin_export.h"

#include <QObject>
#include <QPointer>

class QAction;

/**
 * An event filter that allows to detect a middle click
 * to trigger e.g. opening something in a new tab.
 */
class DOLPHIN_EXPORT MiddleClickActionEventFilter : public QObject
{
    Q_OBJECT

public:
    MiddleClickActionEventFilter(QObject *parent);
    ~MiddleClickActionEventFilter() override;

signals:
    void actionMiddleClicked(QAction *action);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    QPointer<QAction> m_lastMiddlePressedAction;

};
