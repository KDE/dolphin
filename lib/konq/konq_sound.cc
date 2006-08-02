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
	virtual ~KonqSoundPlayerImpl();

	virtual const QStringList &mimeTypes();
	virtual void play(const QString &fileName);
	virtual void stop();
	virtual bool isPlaying();

private:
	QStringList m_mimetypes;
	Phonon::AudioPlayer m_player;
};

KonqSoundPlayerImpl::KonqSoundPlayerImpl()
	: m_player(Phonon::MusicCategory)
{
}

KonqSoundPlayerImpl::~KonqSoundPlayerImpl()
{
}

const QStringList &KonqSoundPlayerImpl::mimeTypes()
{
	m_mimetypes = Phonon::BackendCapabilities::self()->knownMimeTypes();
	return m_mimetypes;
}

void KonqSoundPlayerImpl::play(const QString &fileName)
{
	kDebug() << k_funcinfo << endl;
	m_player.play(KUrl(fileName));
}

void KonqSoundPlayerImpl::stop()
{
	m_player.stop();
}

bool KonqSoundPlayerImpl::isPlaying()
{
	return m_player.isPlaying();
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
