/* This file is part of the KDE Project
   Copyright (c) 2001 Malte Starostik <malte.starostik@t-online.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// $Id$

#include <kurl.h>
#include <kartsdispatcher.h>
#include <kplayobjectfactory.h>

#include "konq_sound.moc"

KonqSoundFactory::KonqSoundFactory(QObject *parent, const char *name)
	: KLibFactory(parent, name)
{
	KArtsDispatcher::init();
	m_soundServer = Arts::Reference("global:Arts_SoundServerV2");
	m_factory = new KPlayObjectFactory(m_soundServer);
}

KonqSoundFactory::~KonqSoundFactory()
{
	delete m_factory;
	KArtsDispatcher::free();
}

QObject *KonqSoundFactory::createObject(QObject *, const char *,
	const char *className, const QStringList &args)
{
	if (qstrcmp(className, "KPlayObject") == 0 && args.count())
	{
		if (m_soundServer.isNull())
			return 0;
		KPlayObject *playobject = m_factory->createPlayObject(args[0], true);
		if (playobject)
		{
			if (playobject->object().isNull())
				delete playobject;
			else
			{
				playobject->play();
				return playobject;
			}
		}
	}
	return 0;
}

extern "C"
{
	KLibFactory *init_libkonqsound()
	{
		return new KonqSoundFactory();
	}
};

// vim: ts=4 sw=4 noet
