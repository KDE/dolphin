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

#include "phononwidget.h"

#include <Phonon/AudioOutput>
#include <Phonon/Global>
#include <Phonon/MediaObject>
#include <Phonon/SeekSlider>
#include <Phonon/VideoWidget>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QShowEvent>
#include <QToolButton>

#include <KDialog>
#include <KIcon>
#include <KUrl>
#include <KLocale>

class EmbeddedVideoPlayer : public Phonon::VideoWidget
{
    public:
        EmbeddedVideoPlayer(QWidget *parent = 0) :
            Phonon::VideoWidget(parent)
        {
        }

        void setSizeHint(const QSize& size)
        {
            m_sizeHint = size;
            updateGeometry();
        }

        virtual QSize sizeHint() const
        {
            return m_sizeHint.isValid() ? m_sizeHint : Phonon::VideoWidget::sizeHint();
        }

    private:
        QSize m_sizeHint;
};

PhononWidget::PhononWidget(QWidget *parent)
    : QWidget(parent),
    m_url(),
    m_playButton(0),
    m_stopButton(0),
    m_topLayout(0),
    m_media(0),
    m_seekSlider(0),
    m_audioOutput(0),
    m_videoPlayer(0)
{
}

void PhononWidget::setUrl(const KUrl &url)
{
    if (m_url != url) {
        stop(); // emits playingStopped() signal
        m_url = url;
    }
}

KUrl PhononWidget::url() const
{
    return m_url;
}

void PhononWidget::setVideoSize(const QSize& size)
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
        m_topLayout->setMargin(0);
        m_topLayout->setSpacing(KDialog::spacingHint());
        QHBoxLayout *controlsLayout = new QHBoxLayout(this);
        controlsLayout->setMargin(0);
        controlsLayout->setSpacing(0);

        m_playButton = new QToolButton(this);
        m_stopButton = new QToolButton(this);
        m_seekSlider = new Phonon::SeekSlider(this);

        controlsLayout->addWidget(m_playButton);
        controlsLayout->addWidget(m_stopButton);
        controlsLayout->addWidget(m_seekSlider);

        m_topLayout->addLayout(controlsLayout);

        m_playButton->setToolTip(i18n("play"));
        m_playButton->setIconSize(QSize(16, 16));
        m_playButton->setIcon(KIcon("media-playback-start"));
        connect(m_playButton, SIGNAL(clicked()), this, SLOT(play()));

        m_stopButton->setToolTip(i18n("stop"));
        m_stopButton->setIconSize(QSize(16, 16));
        m_stopButton->setIcon(KIcon("media-playback-stop"));
        m_stopButton->hide();
        connect(m_stopButton, SIGNAL(clicked()), this, SLOT(stop()));

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
        m_stopButton->show();
        m_playButton->hide();
        break;
    default:
        m_stopButton->hide();
        m_playButton->show();
        break;
    }
    setUpdatesEnabled(true);
}

void PhononWidget::play()
{
    if (!m_media) {
        m_media = new Phonon::MediaObject(this);
        connect(m_media, SIGNAL(stateChanged(Phonon::State,Phonon::State)),
                this, SLOT(stateChanged(Phonon::State)));
        connect(m_media, SIGNAL(hasVideoChanged(bool)),
                this, SLOT(slotHasVideoChanged(bool)));
        m_seekSlider->setMediaObject(m_media);
    }

    if (!m_audioOutput) {
        m_audioOutput = new Phonon::AudioOutput(Phonon::MusicCategory, this);
        Phonon::createPath(m_media, m_audioOutput);
    }

    emit hasVideoChanged(false);

    m_media->setCurrentSource(m_url);
    m_media->hasVideo();
    m_media->play();
}

void PhononWidget::stop()
{
    if (m_media) {
        m_media->stop();

        m_stopButton->hide();
        m_playButton->show();
    }

    if (m_videoPlayer) {
        m_videoPlayer->hide();
    }

    emit hasVideoChanged(false);
}

void PhononWidget::slotHasVideoChanged(bool hasVideo)
{
    emit hasVideoChanged(hasVideo);

    if (hasVideo) {
        if (!m_videoPlayer) {
            // Replay the media to apply path changes
            m_media->stop();
            m_videoPlayer = new EmbeddedVideoPlayer(this);
            m_topLayout->insertWidget(0, m_videoPlayer);
            Phonon::createPath(m_media, m_videoPlayer);
            m_media->play();
        }
        applyVideoSize();
        m_videoPlayer->show();
    }
}

void PhononWidget::applyVideoSize()
{
    if ((m_videoPlayer) && m_videoSize.isValid()) {
        m_videoPlayer->setSizeHint(m_videoSize);
    }
}
