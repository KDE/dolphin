/*
  SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef MEDIAWIDGET_H
#define MEDIAWIDGET_H

#include <QSize>
#include <QSlider>
#include <QUrl>
#include <QWidget>

#include <QMediaPlayer>

class QMediaPlaylist;
class QMediaPlayer;
class SeekSlider;

class EmbeddedVideoPlayer;
class QToolButton;
class QVBoxLayout;

class MediaWidget : public QWidget
{
    Q_OBJECT
public:
    enum MediaKind { Video, Audio };

    explicit MediaWidget(QWidget *parent = nullptr);

    void setUrl(const QUrl &url, MediaKind kind);
    QUrl url() const;
    void clearUrl();

    void setVideoSize(const QSize &size);
    QSize videoSize() const;
    QMediaPlayer::State state() const;

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
    void stop();
    void finished();
    void setPosition(qint64 position);
    void onStateChanged(QMediaPlayer::State newState);
    void onPositionChanged(qint64 position);
    void onDurationChanged(qint64 position);

private:
    void applyVideoSize();
    void togglePlayback();
    void initPlayer();

    QUrl m_url;
    QSize m_videoSize;

    QToolButton *m_playButton;
    QToolButton *m_pauseButton;

    QVBoxLayout *m_topLayout;
    QMediaPlayer *m_player;
    QSlider *m_seekSlider;
    EmbeddedVideoPlayer *m_videoWidget;
    QMediaPlaylist *m_playlist;
    bool m_autoPlay;
    bool m_isVideo;
};

#endif // MEDIAWIDGET_H
