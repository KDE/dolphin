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

#include <Phonon/Global>
#include <Phonon/MediaObject>
#include <Phonon/SeekSlider>
#include <QtGui/QHBoxLayout>
#include <QtGui/QToolButton>
#include <kicon.h>
#include <kurl.h>
#include <klocale.h>

PhononWidget::PhononWidget(QWidget *parent)
    : QWidget(parent),
    m_media(0)
{
    QHBoxLayout *innerLayout = new QHBoxLayout(this);
    innerLayout->setMargin(0);
    innerLayout->setSpacing(0);
    m_playButton = new QToolButton(this);
    m_stopButton = new QToolButton(this);
    m_seekSlider = new Phonon::SeekSlider(this);
    innerLayout->addWidget(m_playButton);
    innerLayout->addWidget(m_stopButton);
    innerLayout->addWidget(m_seekSlider);

    m_playButton->setToolTip(i18n("play"));
    m_playButton->setIconSize(QSize(16, 16));
    m_playButton->setIcon(KIcon("media-playback-start"));
    m_stopButton->setToolTip(i18n("stop"));
    m_stopButton->setIconSize(QSize(16, 16));
    m_stopButton->setIcon(KIcon("media-playback-stop"));
    m_stopButton->hide();
    m_seekSlider->setIconVisible(false);
}

void PhononWidget::setUrl(const KUrl &url)
{
    if (m_media) {
        m_media->setCurrentSource(url);
    } else {
        m_media = Phonon::createPlayer(Phonon::MusicCategory, url);
        m_media->setParent(this);
        connect(m_playButton, SIGNAL(clicked()), m_media, SLOT(play()));
        connect(m_stopButton, SIGNAL(clicked()), m_media, SLOT(stop()));
        connect(m_media, SIGNAL(stateChanged(Phonon::State, Phonon::State)), SLOT(stateChanged(Phonon::State)));
        m_seekSlider->setMediaObject(m_media);
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
