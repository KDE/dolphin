/* This file is part of the KDE project
   Copyright (C) 2003 George Staikos <staikos@kde.org>

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

#ifndef mozilla_module_h
#define mozilla_module_h

#include <qobject.h>
#include <konqsidebarplugin.h>

class KHTMLPart;

class KonqSidebarMozillaModule : public KonqSidebarPlugin
{
	Q_OBJECT
public:
	KonqSidebarMozillaModule(KInstance *instance, QObject *parent, QWidget *widgetParent, QString &desktopName, const char *name);
	virtual ~KonqSidebarMozillaModule();

	virtual QWidget *getWidget();
	virtual void *provides(const QString &);

protected:
	virtual void handleURL(const KURL &url);
	KHTMLPart *_htmlPart;
};

#endif
