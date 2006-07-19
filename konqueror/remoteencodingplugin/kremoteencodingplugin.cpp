/*
    Copyright (c) 2003 Thiago Macieira <thiago.macieira@kdemail.net>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License (LGPL) as published by the Free Software Foundation;
    either version 2 of the License, or (at your option) any later
    version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

/*
 * This code is largely based on the UserAgent changer plugin (uachanger)
 * Copyright © 2001 Dawit Alemayehu <adawit@kde.org>
 * Distributed under the same terms.
 */

#include <kdebug.h>
#include <kactionmenu.h>
#include <klocale.h>
#include <kglobal.h>
#include <kconfig.h>
#include <kcharsets.h>
#include <kmenu.h>
#include <kgenericfactory.h>
#include <kprotocolinfo.h>
#include <kprotocolmanager.h>
#include <kprotocolmanager.h>
#include <kio/slaveconfig.h>
#include <kio/scheduler.h>
#include <konq_dirpart.h>
#include <kparts/browserextension.h>

#include "kremoteencodingplugin.h"

#define DATA_KEY	QLatin1String("Charset")

KRemoteEncodingPlugin::KRemoteEncodingPlugin(QObject * parent,
					     const QStringList &)
  : KParts::Plugin(parent), m_loaded(false), m_idDefault(0)
{
  m_menu = new KActionMenu(KIcon("charset"), i18n("Select Remote Charset"),
			   actionCollection(), "changeremoteencoding"); connect(m_menu->menu(), SIGNAL(aboutToShow()),
	  this, SLOT(slotAboutToShow()));
  m_menu->setEnabled(false);
  m_menu->setDelayed(false);

  m_part = dynamic_cast<KonqDirPart*>(parent);
  if (m_part)
    // if parent is not a KonqDirPart, our menu will never show
    QObject::connect(m_part, SIGNAL(aboutToOpenURL()),
		     this, SLOT(slotAboutToOpenURL()));
}

KRemoteEncodingPlugin::~KRemoteEncodingPlugin()
{
}

void
KRemoteEncodingPlugin::slotReload()
{
  loadSettings();
}

void
KRemoteEncodingPlugin::loadSettings()
{
  m_loaded = true;

  m_encodingDescriptions = KGlobal::charsets()->descriptiveEncodingNames();

  fillMenu();
}

void
KRemoteEncodingPlugin::slotAboutToOpenURL()
{
  KUrl oldURL = m_currentURL;
  m_currentURL = m_part->url();

  if (m_currentURL.protocol() != oldURL.protocol())
    {
      // This plugin works on ftp, fish, etc.
      // everything whose type is T_FILESYSTEM except for local files
      if (!m_currentURL.isLocalFile() &&
	  KProtocolManager::outputType(m_currentURL) == KProtocolInfo::T_FILESYSTEM)
	{
	  m_menu->setEnabled(true);
	  loadSettings();
	}
      else
	m_menu->setEnabled(false);

      return;
    }

  if (m_currentURL.host() != oldURL.host())
    updateMenu();
}

void
KRemoteEncodingPlugin::fillMenu()
{
  KMenu *menu = m_menu->popupMenu();
  menu->clear();

  QStringList::ConstIterator it;
  int count = 0;
  for (it = m_encodingDescriptions.begin(); it != m_encodingDescriptions.end(); ++it)
    menu->insertItem(*it, this, SLOT(slotItemSelected(int)), 0, ++count);
  menu->insertSeparator();

  menu->insertItem(i18n("Reload"), this, SLOT(slotReload()), 0, ++count);
  menu->insertItem(i18n("Default"), this, SLOT(slotDefault()), 0, ++count);
  m_idDefault = count;
}

void
KRemoteEncodingPlugin::updateMenu()
{
  if (!m_loaded)
    loadSettings();

  // uncheck everything
  for (unsigned i =  0; i < m_menu->popupMenu()->count(); i++)
    m_menu->menu()->setItemChecked(m_menu->popupMenu()->idAt(i), false);

  QString charset = KIO::SlaveConfig::self()->configData(m_currentURL.protocol(), m_currentURL.host(),
							 DATA_KEY);
  if (!charset.isEmpty())
    {
      int id = 1;
      QStringList::Iterator it;
      for (it = m_encodingDescriptions.begin(); it != m_encodingDescriptions.end(); ++it, ++id)
	if ((*it).indexOf(charset) != -1)
	  break;

      kDebug() << k_funcinfo << "URL=" << m_currentURL << " charset=" << charset << endl;

      if (it == m_encodingDescriptions.end())
	kWarning() << k_funcinfo << "could not find entry for charset=" << charset << endl;
      else
	m_menu->popupMenu()->setItemChecked(id, true);
    }
  else
    m_menu->popupMenu()->setItemChecked(m_idDefault, true);
}

void
KRemoteEncodingPlugin::slotAboutToShow()
{
  if (!m_loaded)
    loadSettings();
  updateMenu();
}

void
KRemoteEncodingPlugin::slotItemSelected(int id)
{
  KConfig config(("kio_" + m_currentURL.protocol() + "rc").toLatin1());
  QString host = m_currentURL.host();

  if (!m_menu->popupMenu()->isItemChecked(id))
    {
      QString charset = KGlobal::charsets()->encodingForName(m_encodingDescriptions[id - 1]);

      config.setGroup(host);
      config.writeEntry(DATA_KEY, charset);
      config.sync();

      // Update the io-slaves...
      updateBrowser();
    }
}

void
KRemoteEncodingPlugin::slotDefault()
{
  // We have no choice but delete all higher domain level
  // settings here since it affects what will be matched.
  KConfig config(("kio_" + m_currentURL.protocol() + "rc").toLatin1());

  QStringList partList = m_currentURL.host().split('.', QString::SkipEmptyParts);
  if (!partList.isEmpty())
    {
      partList.erase(partList.begin());

      QStringList domains;
      // Remove the exact name match...
      domains << m_currentURL.host();

      while (partList.count())
	{
	  if (partList.count() == 2)
	    if (partList[0].length() <= 2 && partList[1].length() == 2)
	      break;

	  if (partList.count() == 1)
	    break;

	  domains << partList.join(".");
	  partList.erase(partList.begin());
	}

      for (QStringList::Iterator it = domains.begin(); it != domains.end();
	   it++)
	{
	  kDebug() << k_funcinfo << "Domain to remove: " << *it << endl;
	  if (config.hasGroup(*it))
	    config.deleteGroup(*it);
	  else if (config.hasKey(*it))
	    config.deleteEntry(*it);
	}
    }
  config.sync();

  // Update the io-slaves.
  updateBrowser();
}

void
KRemoteEncodingPlugin::updateBrowser()
{
  // Inform running io-slaves about the change...
  // TODO:
#ifdef __GNUC__
#warning enable once KIO::Scheduler::emitReparseSlaveConfiguration is in kdelibs snapshot
#endif
  //KIO::Scheduler::emitReparseSlaveConfiguration();

  // Reload the page with the new charset
  KParts::URLArgs args = m_part->extension()->urlArgs();
  args.reload = true;
  m_part->extension()->setURLArgs(args);
  m_part->openURL(m_currentURL);
}

typedef KGenericFactory < KRemoteEncodingPlugin > KRemoteEncodingPluginFactory;
K_EXPORT_COMPONENT_FACTORY(konq_remoteencoding,
			   KRemoteEncodingPluginFactory("kremoteencodingplugin"))
#include "kremoteencodingplugin.moc"
