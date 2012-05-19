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

#include <KUrl>

#include <QSize>
#include <QWidget>

#include <Phonon/Global>

namespace Phonon
{
    class AudioOutput;
    class MediaObject;
    class SeekSlider;
    class VideoPlayer;
} // namespace Phonon

class EmbeddedVideoPlayer;
class QToolButton;
class QVBoxLayout;

class PhononWidget : public QWidget
{
    Q_OBJECT
    public:
        PhononWidget(QWidget *parent = 0);

        void setUrl(const KUrl &url);
        KUrl url() const;

        void setVideoSize(const QSize& size);
        QSize videoSize() const;

    signals:
        /**
         * Is emitted whenever the video-state
         * has changed: If true is returned, a video
         * including control-buttons will be shown.
         * If false is returned, no video is shown
         * and the control-buttons are available for
         * audio only.
         */
        void hasVideoChanged(bool hasVideo);

    protected:
        virtual void showEvent(QShowEvent *event);
        virtual void hideEvent(QHideEvent *event);

    private slots:
        void stateChanged(Phonon::State);
        void play();
        void stop();
        void slotHasVideoChanged(bool);

    private:
        void applyVideoSize();

    private:
        KUrl m_url;
        QSize m_videoSize;

        QToolButton *m_playButton;
        QToolButton *m_stopButton;

        QVBoxLayout *m_topLayout;
        Phonon::MediaObject *m_media;
        Phonon::SeekSlider *m_seekSlider;
        Phonon::AudioOutput *m_audioOutput;
        EmbeddedVideoPlayer *m_videoPlayer;
};

#endif // PHONONWIDGET_H
