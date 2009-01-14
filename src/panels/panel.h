/***************************************************************************
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef PANEL_H
#define PANEL_H

#include <QtGui/QWidget>
#include <kurl.h>
#include <kfileitem.h>

/**
 * @brief Base widget for all panels that can be docked on the window borders.
 */
class Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Panel(QWidget* parent = 0);
    virtual ~Panel();

    /** Returns the current set URL of the active Dolphin view. */
    const KUrl& url() const;

public slots:
    /**
     * This is invoked every time the folder being displayed in the
     * active Dolphin view changes.
     */
    virtual void setUrl(const KUrl& url);

private:
    KUrl m_url;
};

#endif // PANEL_H
