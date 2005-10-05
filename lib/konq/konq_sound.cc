/* This file is part of the KDE Project
   Copyright (c) 2001 Malte Starostik <malte@kde.org>

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

#include <kartsdispatcher.h>
#include <kdebug.h>
#include <kplayobjectfactory.h>
#include <soundserver.h>

#include "konq_sound.h"

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
	QStringList m_mimeTypes;

	KArtsDispatcher     m_dispatcher;
	Arts::SoundServerV2 m_soundServer;
	KDE::PlayObjectFactory *m_factory;
	KDE::PlayObject        *m_player;
};

KonqSoundPlayerImpl::KonqSoundPlayerImpl()
	: m_player(0)
{
	m_soundServer = Arts::Reference("global:Arts_SoundServerV2");
	m_factory = new KDE::PlayObjectFactory(m_soundServer);
}

KonqSoundPlayerImpl::~KonqSoundPlayerImpl()
{
	delete m_player;
	delete m_factory;
}

const QStringList &KonqSoundPlayerImpl::mimeTypes()
{
	if (m_mimeTypes.isEmpty())
	{
		Arts::TraderQuery query;
		vector<Arts::TraderOffer> *offers = query.query();

		for (vector<Arts::TraderOffer>::iterator it = offers->begin();
			it != offers->end(); ++it)
		{
			vector<string> *prop = (*it).getProperty("MimeType");
			for (vector<string>::iterator mt = prop->begin();
				mt != prop->end(); ++mt)
				if ((*mt).length()) // && (*mt).find("video/") == string::npos)
					m_mimeTypes << (*mt).c_str();
			delete prop;
		}
		delete offers;
	}
	return m_mimeTypes;
}

void KonqSoundPlayerImpl::play(const QString &fileName)
{
	if (m_soundServer.isNull())
		return;

	delete m_player;
	if ((m_player = m_factory->createPlayObject(fileName, true)))
	{
		if (m_player->isNull())
			stop();
		else
			m_player->play();
	}
}

void KonqSoundPlayerImpl::stop()
{
	delete m_player;
	m_player = 0;
}

bool KonqSoundPlayerImpl::isPlaying()
{
	return m_player ? (m_player->state() == Arts::posPlaying) : false;
}

class KonqSoundFactory : public KLibFactory
{
public:
	KonqSoundFactory(QObject *parent = 0, const char *name = 0)
		: KLibFactory(parent, name) {};
	virtual ~KonqSoundFactory() {};

protected:
	virtual QObject *createObject(QObject * = 0, const char * = 0,
		const char *className = "QObject", const QStringList &args = QStringList());
};

QObject *KonqSoundFactory::createObject(QObject *, const char *,
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
