/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef PIXMAPVIEWER_H
#define PIXMAPVIEWER_H

#include <QtGui/QWidget>
#include <QtGui/QPixmap>
#include <QTimeLine>

class QPaintEvent;

/**
 * @brief Widget which shows a pixmap centered inside the boundaries.
 *
 * @see IconsViewSettingsPage
 * @author Peter Penz <peter.penz@gmx.at>
 */
class PixmapViewer : public QWidget
{
    Q_OBJECT
public:
    explicit PixmapViewer(QWidget* parent);
    virtual ~PixmapViewer();
    void setPixmap(const QPixmap& pixmap);
    const QPixmap& pixmap() const
    {
        return m_pixmap;
    }

protected:
    virtual void paintEvent(QPaintEvent* event);

private slots:
    void beginTransition();
    void finishTransition();

private:
    QPixmap m_pixmap;
    QPixmap m_nextPixmap;
    QPixmap m_pendingPixmap;
    QTimeLine m_animation;
    int m_animationStep;
};


#endif
