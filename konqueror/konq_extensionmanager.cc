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

#include "konq_extensionmanager.h"

#include <qlayout.h>
#include <qtimer.h>

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kparts/componentfactory.h>
#include <kplugininfo.h>
#include <kpluginselector.h>
#include <ksettings/dispatcher.h>

class KonqExtensionManagerPrivate
{
	public:
		KPluginSelector *pluginSelector;
		KConfig *config;
		bool isChanged;
};

KonqExtensionManager::KonqExtensionManager(QWidget *parent, const char *name) :
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

	d->config = new KConfig("konquerorrc");
	d->pluginSelector->addPlugins("konqueror", i18n("Konqueror Extensions"), QString::null, d->config);
}

KonqExtensionManager::~KonqExtensionManager()
{
	delete d->config;
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
