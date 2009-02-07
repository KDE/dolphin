/*  This file is part of the KDE project
    Copyright (C) 2007 Matthias Kretz <kretz@kde.org>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
    02110-1301, USA.

*/

#ifndef PHONONWIDGET_H
#define PHONONWIDGET_H

#include <kurl.h>

#include <QtGui/QWidget>

#include <Phonon/Global>

namespace Phonon
{
    class MediaObject;
    class SeekSlider;
} // namespace Phonon

class QToolButton;

class PhononWidget : public QWidget
{
    Q_OBJECT
    public:
        PhononWidget(QWidget *parent = 0);
        void setUrl(const KUrl &url);

    private slots:
        void stateChanged(Phonon::State);
        void play();
        void stop();

    private:
        void requestMedia();

    private:
        KUrl m_url;
        QToolButton *m_playButton;
        QToolButton *m_stopButton;
        Phonon::MediaObject *m_media;
        Phonon::SeekSlider *m_seekSlider;
};

#endif // PHONONWIDGET_H
