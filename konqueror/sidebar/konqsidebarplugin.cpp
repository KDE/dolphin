/* This file is part of the KDE project
   Copyright (C) 2001,2002 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konqsidebarplugin.moc"
#include "konqsidebariface_p.h"
#include <kdebug.h>


KonqSidebarPlugin::KonqSidebarPlugin(KInstance *instance,QObject *parent,
	QWidget * /*widgetParent*/, QString &desktopName_, const char* name)
	: QObject(parent), desktopName(desktopName_)
{
  setObjectName( name );
  m_parentInstance=instance;
}

KonqSidebarPlugin::~KonqSidebarPlugin() { }

KInstance *KonqSidebarPlugin::parentInstance(){return m_parentInstance;}

void KonqSidebarPlugin::openUrl(const KUrl& url){handleURL(url);}

void KonqSidebarPlugin::openPreview(const KFileItemList& items)
{
  handlePreview(items);
}

void KonqSidebarPlugin::openPreviewOnMouseOver(const KFileItem& item)
{
  handlePreviewOnMouseOver(item);
}

void KonqSidebarPlugin::handlePreview(const KFileItemList & /*items*/) {}

void KonqSidebarPlugin::handlePreviewOnMouseOver(const KFileItem& /*items*/) {}


bool KonqSidebarPlugin::universalMode() {
	if (!parent()) return false;
	KonqSidebarIface *ksi=dynamic_cast<KonqSidebarIface*>(parent());
	if (!ksi) return false;
	kDebug()<<"calling KonqSidebarIface->universalMode()"<<endl;
	return ksi->universalMode();
}
