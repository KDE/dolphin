/*
  SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "phononwidget.h"

#include <KLocalizedString>
#include <phonon/AudioOutput>
#include <phonon/MediaObject>
#include <phonon/SeekSlider>
#include <phonon/VideoWidget>

#include <QShowEvent>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

class EmbeddedVideoPlayer : public Phonon::VideoWidget
{
    Q_OBJECT

public:
    EmbeddedVideoPlayer(QWidget *parent = nullptr)
        : Phonon::VideoWidget(parent)
    {
    }

    void setSizeHint(const QSize &size)
    {
        m_sizeHint = size;
        updateGeometry();
    }

    QSize sizeHint() const override
    {
        return m_sizeHint.isValid() ? m_sizeHint : Phonon::VideoWidget::sizeHint();
    }

private:
    QSize m_sizeHint;
};

PhononWidget::PhononWidget(QWidget *parent)
    : QWidget(parent)
    , m_url()
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_topLayout(nullptr)
    , m_media(nullptr)
    , m_seekSlider(nullptr)
    , m_audioOutput(nullptr)
    , m_videoPlayer(nullptr)
{
}

void PhononWidget::setUrl(const QUrl &url, MediaKind kind)
{
    if (m_url != url) {
        m_url = url;
        m_isVideo = kind == MediaKind::Video;
    }
    if (m_autoPlay) {
        play();
    } else {
        stop();
    }
}

void PhononWidget::setAutoPlay(bool autoPlay)
{
    m_autoPlay = autoPlay;
    if (!m_url.isEmpty() && (m_media == nullptr || m_media->state() != Phonon::State::PlayingState) && m_autoPlay && isVisible()) {
        play();
    }
}

QUrl PhononWidget::url() const
{
    return m_url;
}

void PhononWidget::clearUrl()
{
    m_url.clear();
}

void PhononWidget::togglePlayback()
{
    if (m_media && m_media->state() == Phonon::State::PlayingState) {
        m_media->pause();
    } else {
        play();
    }
}

bool PhononWidget::eventFilter(QObject *object, QEvent *event)
{
    Q_UNUSED(object)
    if (event->type() == QEvent::MouseButtonPress) {
        const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            // toggle playback
            togglePlayback();
            return true;
        }
    }
    return false;
}

void PhononWidget::setVideoSize(const QSize &size)
{
    if (m_videoSize != size) {
        m_videoSize = size;
        applyVideoSize();
    }
}

QSize PhononWidget::videoSize() const
{
    return m_videoSize;
}

void PhononWidget::showEvent(QShowEvent *event)
{
    if (event->spontaneous()) {
        QWidget::showEvent(event);
        return;
    }

    if (!m_topLayout) {
        m_topLayout = new QVBoxLayout(this);
        m_topLayout->setContentsMargins(0, 0, 0, 0);

        QHBoxLayout *controlsLayout = new QHBoxLayout();
        controlsLayout->setContentsMargins(0, 0, 0, 0);
        controlsLayout->setSpacing(0);

        m_playButton = new QToolButton(this);
        m_pauseButton = new QToolButton(this);
        m_seekSlider = new Phonon::SeekSlider(this);

        controlsLayout->addWidget(m_playButton);
        controlsLayout->addWidget(m_pauseButton);
        controlsLayout->addWidget(m_seekSlider);

        m_topLayout->addLayout(controlsLayout);

        const int smallIconSize = style()->pixelMetric(QStyle::PM_SmallIconSize);
        const QSize buttonSize(smallIconSize, smallIconSize);

        m_playButton->setToolTip(i18n("play"));
        m_playButton->setIconSize(buttonSize);
        m_playButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
        m_playButton->setAutoRaise(true);
        connect(m_playButton, &QToolButton::clicked, this, &PhononWidget::play);

        m_pauseButton->setToolTip(i18n("pause"));
        m_pauseButton->setIconSize(buttonSize);
        m_pauseButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
        m_pauseButton->setAutoRaise(true);
        m_pauseButton->hide();
        connect(m_pauseButton, &QToolButton::clicked, this, &PhononWidget::togglePlayback);

        m_seekSlider->setIconVisible(false);

        // Creating an audio player or video player instance might take up to
        // 2 seconds when doing it the first time. To prevent that the user
        // interface gets noticeable blocked, the creation is delayed until
        // the play button has been pressed (see PhononWidget::play()).
    }
}

void PhononWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (!event->spontaneous()) {
        stop();
    }
}

void PhononWidget::stateChanged(Phonon::State newstate)
{
    setUpdatesEnabled(false);
    switch (newstate) {
    case Phonon::PlayingState:
    case Phonon::BufferingState:
        m_playButton->hide();
        m_pauseButton->show();
        break;
    default:
        m_pauseButton->hide();
        m_playButton->show();
        break;
    }
    setUpdatesEnabled(true);
}

void PhononWidget::play()
{
    if (!m_media) {
        m_media = new Phonon::MediaObject(this);
        connect(m_media, &Phonon::MediaObject::stateChanged, this, &PhononWidget::stateChanged);
        connect(m_media, &Phonon::MediaObject::finished, this, &PhononWidget::finished);
        m_seekSlider->setMediaObject(m_media);
    }

    if (!m_videoPlayer) {
        m_videoPlayer = new EmbeddedVideoPlayer(this);
        m_videoPlayer->setCursor(Qt::PointingHandCursor);
        m_videoPlayer->installEventFilter(this);
        m_topLayout->insertWidget(0, m_videoPlayer);
        Phonon::createPath(m_media, m_videoPlayer);
        applyVideoSize();
    }

    if (!m_audioOutput) {
        m_audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
        Phonon::createPath(m_media, m_audioOutput);
    }

    if (m_isVideo) {
        Q_EMIT hasVideoChanged(true);
    }

    if (m_url != m_media->currentSource().url()) {
        m_media->setCurrentSource(m_url);
    }
    m_media->play();

    m_videoPlayer->setVisible(m_isVideo);
}

void PhononWidget::finished()
{
    if (m_isVideo) {
        m_videoPlayer->hide();
        Q_EMIT hasVideoChanged(false);
    }
}

Phonon::State PhononWidget::state() const
{
    return m_media == nullptr ? Phonon::State::StoppedState : m_media->state();
}

void PhononWidget::stop()
{
    if (m_media) {
        m_media->stop();
        m_videoPlayer->hide();
        Q_EMIT hasVideoChanged(false);
    }
}

void PhononWidget::applyVideoSize()
{
    if ((m_videoPlayer) && m_videoSize.isValid()) {
        m_videoPlayer->setSizeHint(m_videoSize);
    }
}

#include "phononwidget.moc"
