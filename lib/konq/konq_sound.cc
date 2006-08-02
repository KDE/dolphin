/* This file is part of the KDE Project
   Copyright (c) 2001 Malte Starostik <malte@kde.org>
   Copyright (c) 2006 Matthias Kretz <kretz@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <phonon/audioplayer.h>
#include <phonon/backendcapabilities.h>
#include <kdebug.h>

#include "konq_sound.h"
#include <kurl.h>

using namespace std;

class KonqSoundPlayerImpl : public KonqSoundPlayer
{
public:
	KonqSoundPlayerImpl();
	virtual ~KonqSoundPlayerImpl() {}

	virtual bool isMimeTypeKnown(const QString& mimeType);
	virtual void setUrl(const KUrl &url);
	virtual void play();
	virtual void stop();
	virtual bool isPlaying();

private:
	Phonon::AudioPlayer *m_player;
};

KonqSoundPlayerImpl::KonqSoundPlayerImpl()
	: m_player(0)
{
}

bool KonqSoundPlayerImpl::isMimeTypeKnown(const QString& mimeType)
{
	kDebug() << k_funcinfo << mimeType << Phonon::BackendCapabilities::isMimeTypeKnown(mimeType) << endl;
	return Phonon::BackendCapabilities::isMimeTypeKnown(mimeType);
}

void KonqSoundPlayerImpl::setUrl(const KUrl &url)
{
	kDebug() << k_funcinfo << endl;
	if (!m_player) {
		kDebug() << "create AudioPlayer" << endl;
		m_player = new Phonon::AudioPlayer(Phonon::MusicCategory, this);
	}
	m_player->load(url);
}

void KonqSoundPlayerImpl::play()
{
	kDebug() << k_funcinfo << endl;
	if (m_player)
		m_player->play();
}

void KonqSoundPlayerImpl::stop()
{
	kDebug() << k_funcinfo << endl;
	if (m_player)
		m_player->stop();
}

bool KonqSoundPlayerImpl::isPlaying()
{
	if (m_player) {
		kDebug() << k_funcinfo << m_player->isPlaying() << endl;
		return m_player->isPlaying();
	}
	kDebug() << k_funcinfo << false << endl;
	return false;
}

class KonqSoundFactory : public KLibFactory
{
public:
	KonqSoundFactory(QObject *parent = 0)
		: KLibFactory(parent) {};
	virtual ~KonqSoundFactory() {};

protected:
	virtual QObject *createObject(QObject * = 0,
		const char *className = "QObject", const QStringList &args = QStringList());
};

QObject *KonqSoundFactory::createObject(QObject *,
	const char *className, const QStringList &)
{
	if (qstrcmp(className, "KonqSoundPlayer") == 0)
		return new KonqSoundPlayerImpl();
	return 0;
}

extern "C"
{
	KDE_EXPORT KLibFactory *init_konq_sound()
	{
		return new KonqSoundFactory();
	}
}

// vim: ts=4 sw=4 noet
