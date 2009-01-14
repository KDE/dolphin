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

#ifndef DOLPHINFILEPLACESVIEW_H
#define DOLPHINFILEPLACESVIEW_H

#include <kfileplacesview.h>

/**
 * @brief Combines bookmarks and mounted devices as list which is
 *        embedded as panel.
 */
class DolphinFilePlacesView : public KFilePlacesView
{
    Q_OBJECT

public:
    DolphinFilePlacesView(QWidget* parent);
    virtual ~DolphinFilePlacesView();

signals:
    void urlChanged(const KUrl& url, Qt::MouseButtons buttons);

protected:
    virtual void mousePressEvent(QMouseEvent* event);

private slots:
    void slotUrlsDropped(const KUrl& dest, QDropEvent* event, QWidget* parent);
    void emitExtendedUrlChangedSignal(const KUrl& url);

private:
    Qt::MouseButtons m_mouseButtons;
};

#endif // DOLPHINFILEPLACESVIEW_H
