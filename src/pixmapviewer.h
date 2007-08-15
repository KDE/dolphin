/***************************************************************************
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

#ifndef PIXMAPVIEWER_H
#define PIXMAPVIEWER_H

#include <QWidget>
#include <QPixmap>
#include <QQueue>
#include <QTimeLine>

class QPaintEvent;

/**
 * @brief Widget which shows a pixmap centered inside the boundaries.
 *
 * When the pixmap is changed, a smooth transition is done from the old pixmap
 * to the new pixmap.
 */
class PixmapViewer : public QWidget
{
    Q_OBJECT

public:
    enum Transition
    {
        /** No transition is done when the pixmap is changed. */
        NoTransition,

        /**
         * The old pixmap is replaced by the new pixmap and the size is
         * adjusted smoothly to the size of the new pixmap.
         */
        DefaultTransition,

        /**
         * If the old pixmap and the new pixmap have the same content, but
         * a different size it is recommended to use Transition::SizeTransition
         * instead of Transition::DefaultTransition. In this case it is assured
         * that the larger pixmap is used for downscaling, which leads
         * to an improved scaling output.
         */
        SizeTransition
    };

    explicit PixmapViewer(QWidget* parent,
                          Transition transition = DefaultTransition);

    virtual ~PixmapViewer();
    void setPixmap(const QPixmap& pixmap);
    inline const QPixmap& pixmap() const;

protected:
    virtual void paintEvent(QPaintEvent* event);

private Q_SLOTS:
    void checkPendingPixmaps();

private:
    QPixmap m_pixmap;
    QPixmap m_oldPixmap;
    QQueue<QPixmap> m_pendingPixmaps;
    QTimeLine m_animation;
    Transition m_transition;
    int m_animationStep;
};

const QPixmap& PixmapViewer::pixmap() const
{
    return m_pixmap;
}


#endif
