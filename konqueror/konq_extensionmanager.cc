/*
    konq_extensionmanager.cc - Extension Manager for Konqueror

    Copyright (c) 2003      by Martijn Klingens      <klingens@kde.org>
    Copyright (c) 2004      by Arend van Beelen jr.  <arend@auton.nl>

    *************************************************************************
    *                                                                       *
    * This program is free software; you can redistribute it and/or modify  *
    * it under the terms of the GNU General Public License as published by  *
    * the Free Software Foundation; either version 2 of the License, or     *
    * (at your option) any later version.                                   *
    *                                                                       *
    *************************************************************************
*/

#include <qlayout.h>
#include <qtimer.h>

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kparts/componentfactory.h>
#include <kparts/plugin.h>
#include <kplugininfo.h>
#include <kpluginselector.h>
#include <ksettings/dispatcher.h>
#include <dcopref.h>

#include "konq_extensionmanager.h"
#include "konq_mainwindow.h"

class KonqExtensionManagerPrivate
{
	public:
		KPluginSelector *pluginSelector;
		KonqMainWindow *mainWindow;
		KConfig *khtmlConfig;
		bool isChanged;
};

KonqExtensionManager::KonqExtensionManager(QWidget *parent, const char *name, KonqMainWindow *mainWindow) :
  KDialogBase(Plain, i18n("Configure"), Help | Default | Cancel | Apply | Ok | User1,
              Ok, parent, name, false, true, KGuiItem(i18n("&Reset"), "undo"))
{
	d = new KonqExtensionManagerPrivate;
	showButton(User1, false);
	setChanged(false);

	// FIXME: Implement this - Martijn
	enableButton(KDialogBase::Help, false);

	setInitialSize(QSize(640, 480));

	(new QVBoxLayout(plainPage(), 0, 0))->setAutoAdd(true);
	d->pluginSelector = new KPluginSelector(plainPage());
	setMainWidget(d->pluginSelector);
	connect(d->pluginSelector, SIGNAL(changed(bool)), this, SLOT(setChanged(bool)));
	connect(d->pluginSelector, SIGNAL(configCommitted(const QCString &)),
	        KSettings::Dispatcher::self(), SLOT(reparseConfiguration(const QCString &)));

	d->mainWindow = mainWindow;

	d->khtmlConfig = new KConfig("khtmlrc");
	d->pluginSelector->addPlugins("konqueror", i18n("Extensions"), "Extensions", KGlobal::config());
	d->pluginSelector->addPlugins("khtml",     i18n("Tools"),      "Tools",      d->khtmlConfig);
	//d->pluginSelector->addPlugins(KPluginInfo::fromServices(KTrader::self()->query("Browser/View")), i18n("Browser Plugins"), QString::null, KGlobal::config());
}

KonqExtensionManager::~KonqExtensionManager()
{
	delete d->khtmlConfig;
        delete d;
}

void KonqExtensionManager::setChanged(bool c)
{
	d->isChanged = c;
	enableButton(Apply, c);
}

void KonqExtensionManager::slotDefault()
{
	d->pluginSelector->defaults();
	setChanged(false);
}

void KonqExtensionManager::slotUser1()
{
	d->pluginSelector->load();
	setChanged(false);
}

void KonqExtensionManager::apply()
{
	if(d->isChanged)
	{
		d->pluginSelector->save();
		setChanged(false);
		if(d->mainWindow != 0L)
		{
			KParts::Plugin::loadPlugins(d->mainWindow, d->mainWindow, KGlobal::instance());
			QPtrList<KParts::Plugin> plugins = KParts::Plugin::pluginObjects(d->mainWindow);
			QPtrListIterator<KParts::Plugin> it(plugins);
			KParts::Plugin *plugin;
			while((plugin = it.current()) != 0)
			{
				++it;
				d->mainWindow->factory()->addClient(plugin);
			}
		}
		DCOPRef preloader( "kded", "konqy_preloader" );
		preloader.send( "unloadAllPreloaded" );
	}
}

void KonqExtensionManager::slotApply()
{
	apply();
}

void KonqExtensionManager::slotOk()
{
	emit okClicked();
	apply();
	accept();
}

void KonqExtensionManager::slotHelp()
{
	kdWarning() << k_funcinfo << "FIXME: Implement!" << endl;
}

void KonqExtensionManager::show()
{
	d->pluginSelector->load();

	KDialogBase::show();
}

#include "konq_extensionmanager.moc"
