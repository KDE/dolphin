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

#ifndef __konq_sound_h__
#define __konq_sound_h__

#include <qobject.h>

#include <klibloader.h>
#include <soundserver.h>

class KURL;
class KPlayObject;
class KPlayObjectFactory;

class KonqSoundFactory : public KLibFactory
{
	Q_OBJECT
public:
	KonqSoundFactory(QObject *parent = 0, const char *name = 0);
	virtual ~KonqSoundFactory();

protected:
	virtual QObject *createObject(QObject * = 0, const char * = 0,
		const char *className = "QObject", const QStringList &args = QStringList());

private:
	Arts::Dispatcher m_dispatcher;
	Arts::SoundServerV2 m_soundServer;
	KPlayObjectFactory *m_factory;
};

#endif

// vim: ts=4 sw=4 noet
