/*
  SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PHONONWIDGET_H
#define PHONONWIDGET_H

#include <phonon/Global>

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
    enum MediaKind { Video, Audio };

    explicit PhononWidget(QWidget *parent = nullptr);

    void setUrl(const QUrl &url, MediaKind kind);
    QUrl url() const;
    void clearUrl();

    void setVideoSize(const QSize &size);
    QSize videoSize() const;
    Phonon::State state() const;

    void setAutoPlay(bool autoPlay);
    bool eventFilter(QObject *object, QEvent *event) override;

Q_SIGNALS:
    /**
     * Is emitted whenever the video-state
     * has changed: If true is returned, a video
     * including control-buttons will be shown.
     * If false is returned, no video is shown
     * and the control-buttons are available for
     * audio only.
     */
    void hasVideoChanged(bool hasVideo);

public Q_SLOTS:
    void play();

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private Q_SLOTS:
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
