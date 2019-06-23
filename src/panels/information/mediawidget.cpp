/*
  SPDX-FileCopyrightText: 2007 Matthias Kretz <kretz@kde.org>

  SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "mediawidget.h"

#include <KLocalizedString>

#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoWidget>

#include <QShowEvent>
#include <QSlider>
#include <QStyle>
#include <QStyleOptionSlider>
#include <QToolButton>
#include <QVBoxLayout>

class EmbeddedVideoPlayer : public QVideoWidget
{
    Q_OBJECT

public:
    EmbeddedVideoPlayer(QWidget *parent = nullptr)
        : QVideoWidget(parent)
    {
    }

    void setSizeHint(const QSize &size)
    {
        m_sizeHint = size;
        updateGeometry();
    }

    QSize sizeHint() const override
    {
        return m_sizeHint.isValid() ? m_sizeHint : QVideoWidget::sizeHint();
    }

private:
    QSize m_sizeHint;
};

class SeekSlider : public QSlider
{
    Q_OBJECT

public:
    SeekSlider(Qt::Orientation orientation, QWidget *parent = nullptr)
        : QSlider(orientation, parent)
    {
    }

protected:
    // Function copied from qslider.cpp
    inline int pick(const QPoint &pt) const
    {
        return orientation() == Qt::Horizontal ? pt.x() : pt.y();
    }

    // Function copied from qslider.cpp and modified to make it compile
    int pixelPosToRangeValue(int pos) const
    {
        QStyleOptionSlider opt;
        initStyleOption(&opt);
        QRect gr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderGroove, this);
        QRect sr = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
        int sliderMin, sliderMax, sliderLength;

        if (orientation() == Qt::Horizontal) {
            sliderLength = sr.width();
            sliderMin = gr.x();
            sliderMax = gr.right() - sliderLength + 1;
        } else {
            sliderLength = sr.height();
            sliderMin = gr.y();
            sliderMax = gr.bottom() - sliderLength + 1;
        }
        return QStyle::sliderValueFromPosition(minimum(), maximum(), pos - sliderMin, sliderMax - sliderMin, opt.upsideDown);
    }

    // Based on code from qslider.cpp
    void mousePressEvent(QMouseEvent *event) override
    {
        if (event->button() == Qt::LeftButton) {
            QStyleOptionSlider opt;
            initStyleOption(&opt);
            const QRect sliderRect = style()->subControlRect(QStyle::CC_Slider, &opt, QStyle::SC_SliderHandle, this);
            const QPoint center = sliderRect.center() - sliderRect.topLeft();
            // to take half of the slider off for the setSliderPosition call we use the center - topLeft

            if (!sliderRect.contains(event->pos())) {
                event->accept();

                int position = pixelPosToRangeValue(pick(event->pos() - center));
                setSliderPosition(position);
                triggerAction(SliderMove);
                setRepeatAction(SliderNoAction);

                Q_EMIT sliderMoved(position);
            } else {
                QSlider::mousePressEvent(event);
            }
        } else {
            QSlider::mousePressEvent(event);
        }
    }
};

MediaWidget::MediaWidget(QWidget *parent)
    : QWidget(parent)
    , m_url()
    , m_playButton(nullptr)
    , m_pauseButton(nullptr)
    , m_topLayout(nullptr)
    , m_player(nullptr)
    , m_seekSlider(nullptr)
    , m_videoWidget(nullptr)
    , m_playlist(nullptr)
{
}

void MediaWidget::setUrl(const QUrl &url, MediaKind kind)
{
    if (m_url != url) {
        m_url = url;
        m_isVideo = kind == MediaKind::Video;
        m_seekSlider->setValue(0);
    }
    if (m_autoPlay) {
        play();
    } else {
        stop();
    }
}

void MediaWidget::setAutoPlay(bool autoPlay)
{
    m_autoPlay = autoPlay;
    if (!m_url.isEmpty() && (m_player == nullptr || m_player->state() != QMediaPlayer::PlayingState) && m_autoPlay && isVisible()) {
        play();
    }
}

QUrl MediaWidget::url() const
{
    return m_url;
}

void MediaWidget::clearUrl()
{
    m_url.clear();
}

void MediaWidget::togglePlayback()
{
    if (m_player && m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        play();
    }
}

bool MediaWidget::eventFilter(QObject *object, QEvent *event)
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

void MediaWidget::setVideoSize(const QSize &size)
{
    if (m_videoSize != size) {
        m_videoSize = size;
        applyVideoSize();
    }
}

QSize MediaWidget::videoSize() const
{
    return m_videoSize;
}

void MediaWidget::showEvent(QShowEvent *event)
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
        m_seekSlider = new SeekSlider(Qt::Orientation::Horizontal, this);
        connect(m_seekSlider, &QAbstractSlider::sliderMoved, this, &MediaWidget::setPosition);

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
        connect(m_playButton, &QToolButton::clicked, this, &MediaWidget::play);

        m_pauseButton->setToolTip(i18n("pause"));
        m_pauseButton->setIconSize(buttonSize);
        m_pauseButton->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
        m_pauseButton->setAutoRaise(true);
        m_pauseButton->hide();
        connect(m_pauseButton, &QToolButton::clicked, this, &MediaWidget::togglePlayback);

        // Creating an audio player or video player instance might take up to
        // 2 seconds when doing it the first time. To prevent that the user
        // interface gets noticeable blocked, the creation is delayed until
        // the play button has been pressed (see PhononWidget::play()).
    }
}

void MediaWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    if (!event->spontaneous()) {
        stop();
    }
}

void MediaWidget::onStateChanged(QMediaPlayer::State newState)
{
    setUpdatesEnabled(false);
    switch (newState) {
    case QMediaPlayer::State::PlayingState:
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

void MediaWidget::initPlayer()
{
    if (!m_playlist) {
        m_playlist = new QMediaPlaylist;

        m_player = new QMediaPlayer;
        m_player->setPlaylist(m_playlist);

        m_videoWidget = new EmbeddedVideoPlayer(this);
        m_videoWidget->setCursor(Qt::PointingHandCursor);

        m_videoWidget->installEventFilter(this);
        m_player->setVideoOutput(m_videoWidget);
        m_topLayout->insertWidget(0, m_videoWidget);

        applyVideoSize();

        connect(m_player, &QMediaPlayer::stateChanged, this, &MediaWidget::onStateChanged);
        connect(m_player, &QMediaPlayer::positionChanged, this, &MediaWidget::onPositionChanged);
        connect(m_player, &QMediaPlayer::durationChanged, this, &MediaWidget::onDurationChanged);
    }

    if (m_url != m_playlist->currentMedia().request().url()) {
        m_playlist->addMedia(m_url);
        m_playlist->next();
        m_seekSlider->setSliderPosition(0);
    }

    Q_EMIT hasVideoChanged(m_isVideo);

    m_videoWidget->setVisible(m_isVideo);
}

void MediaWidget::play()
{
    initPlayer();

    m_player->play();
}

void MediaWidget::finished()
{
    if (m_isVideo) {
        m_videoWidget->hide();
        Q_EMIT hasVideoChanged(false);
    }
}

QMediaPlayer::State MediaWidget::state() const
{
    return m_player == nullptr ? QMediaPlayer::State::StoppedState : m_player->state();
}

void MediaWidget::stop()
{
    if (m_player) {
        m_playlist->clear();
        m_player->stop();
        m_videoWidget->hide();
        Q_EMIT hasVideoChanged(false);
    }
}

void MediaWidget::applyVideoSize()
{
    if ((m_videoWidget) && m_videoSize.isValid()) {
        m_videoWidget->setSizeHint(m_videoSize);
    }
}

void MediaWidget::setPosition(qint64 position)
{
    if (!m_player || m_player->state() == QMediaPlayer::StoppedState) {
        initPlayer();

        auto prevDuration = m_seekSlider->maximum();

        // TODO KF6: use Qt::SingleShotConnection
        QMetaObject::Connection *const connection = new QMetaObject::Connection;
        *connection = connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [connection, prevDuration, position, this](QMediaPlayer::MediaStatus status) {
            if (status == QMediaPlayer::BufferedMedia) {
                m_player->setPosition(float(position) / prevDuration * m_player->duration());
                m_player->pause();

                QObject::disconnect(*connection);
                delete connection;
            }
        });

        m_player->play();
    } else {
        m_player->setPosition(position);
    }
}

void MediaWidget::onPositionChanged(qint64 position)
{
    m_seekSlider->setValue(static_cast<int>(position));
}

void MediaWidget::onDurationChanged(qint64 duration)
{
    m_seekSlider->setMaximum(static_cast<int>(duration));
}

#include "mediawidget.moc"
#include "moc_mediawidget.cpp"
