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

#include <Phonon/Global>

#include <QSize>
#include <QUrl>
#include <QWidget>

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

        enum MediaKind {
            Video,
            Audio
        };

        explicit PhononWidget(QWidget *parent = nullptr);

        void setUrl(const QUrl &url, MediaKind kind);
        QUrl url() const;
        void clearUrl();

        void setVideoSize(const QSize& size);
        QSize videoSize() const;
        Phonon::State state() const;

        void setAutoPlay(bool autoPlay);
        bool eventFilter(QObject *object, QEvent *event) override;

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

    public slots:
        void play();

    protected:
        void showEvent(QShowEvent *event) override;
        void hideEvent(QHideEvent *event) override;

    private slots:
        void stateChanged(Phonon::State newstate);
        void stop();
        void finished();

    private:
        void applyVideoSize();

    private:
        void togglePlayback();

        QUrl m_url;
        QSize m_videoSize;

        QToolButton *m_playButton;
        QToolButton *m_pauseButton;

        QVBoxLayout *m_topLayout;
        Phonon::MediaObject *m_media;
        Phonon::SeekSlider *m_seekSlider;
        Phonon::AudioOutput *m_audioOutput;
        EmbeddedVideoPlayer *m_videoPlayer;
        bool m_autoPlay;
        bool m_isVideo;
};

#endif // PHONONWIDGET_H
