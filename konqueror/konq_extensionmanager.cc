/* -*- indent-tabs-mode: t; tab-width: 4; c-basic-offset:4 -*-
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

#include <QLayout>
#include <QTimer>
//Added by qt3to4:
#include <QVBoxLayout>

#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kinstance.h>
#include <kparts/componentfactory.h>
#include <kparts/plugin.h>
#include <kplugininfo.h>
#include <kpluginselector.h>
#include <ksettings/dispatcher.h>

#include "konq_view.h"
#include "konq_extensionmanager.h"
#include "konq_mainwindow.h"


class KonqExtensionManagerPrivate
{
public:
	KPluginSelector *pluginSelector;
	KonqMainWindow *mainWindow;
	KParts::ReadOnlyPart* activePart;
	bool isChanged;
};

KonqExtensionManager::KonqExtensionManager(QWidget *parent, KonqMainWindow *mainWindow, KParts::ReadOnlyPart* activePart) :
  KDialog( parent )
{
  setCaption( i18n("Configure") );
  setButtons( Default | Cancel | Apply | Ok | User1 );
  setButtonGuiItem( User1, KStdGuiItem::reset() );
  setObjectName( "extensionmanager" );
  showButtonSeparator( true );

	d = new KonqExtensionManagerPrivate;
	showButton(User1, false);
	setChanged(false);

	setInitialSize(QSize(640, 480));

  QFrame *page = new QFrame( this );
  setMainWidget( page );

	QVBoxLayout *vb = new QVBoxLayout( page );
	vb->setAutoAdd(true);
	vb->setSpacing( 0 );
	d->pluginSelector = new KPluginSelector( page );
	setMainWidget(d->pluginSelector);
	connect(d->pluginSelector, SIGNAL(changed(bool)), this, SLOT(setChanged(bool)));
	connect(d->pluginSelector, SIGNAL(configCommitted(const QByteArray &)),
	        KSettings::Dispatcher::self(), SLOT(reparseConfiguration(const QByteArray &)));

	d->mainWindow = mainWindow;
	d->activePart = activePart;

	// There's a limitation of KPluginSelector here... It assumes that all plugins in a given widget (as created by addPlugins)
	// have their config in the same KConfig[Group]. So we can't show konqueror extensions and khtml extensions in the same tab.
	d->pluginSelector->addPlugins("konqueror", i18n("Extensions"), "Extensions", KGlobal::config());
	if ( activePart ) {
		KInstance* instance = activePart->instance();
		d->pluginSelector->addPlugins(instance->instanceName(), i18n("Tools"), "Tools", instance->config());
		d->pluginSelector->addPlugins(instance->instanceName(), i18n("Statusbar"), "Statusbar", instance->config());
	}

  connect( this, SIGNAL( okClicked() ), SLOT( slotOk() ) );
  connect( this, SIGNAL( applyClicked() ), SLOT( slotApply() ) );
  connect( this, SIGNAL( defaultClicked() ), SLOT( slotDefault() ) );
  connect( this, SIGNAL( user1Clicked() ), SLOT( slotUser1() ) );
}

KonqExtensionManager::~KonqExtensionManager()
{
	delete d;
}

void KonqExtensionManager::setChanged(bool c)
{
	d->isChanged = c;
	enableButton(Apply, c);
}

void KonqExtensionManager::apply()
{
	if(d->isChanged)
  {
		d->pluginSelector->save();
  	setChanged(false);
	  if( d->mainWindow )
 		{
  		KParts::Plugin::loadPlugins(d->mainWindow, d->mainWindow, KGlobal::instance());
	  	QList<KParts::Plugin*> plugins = KParts::Plugin::pluginObjects(d->mainWindow);
                         for (int i = 0; i < plugins.size(); ++i) {
 			d->mainWindow->factory()->addClient(plugins.at(i));
	  	}
 		}
  	if ( d->activePart )
	  {
 			KParts::Plugin::loadPlugins( d->activePart, d->activePart, d->activePart->instance() );
  		QList<KParts::Plugin*> plugins = KParts::Plugin::pluginObjects( d->activePart );
	  	for (int i = 0; i < plugins.size(); ++i) {
		  	d->activePart->factory()->addClient(plugins.at(i));
 			}
  	}
 	}
}

void KonqExtensionManager::slotOk()
{
	emit okClicked();
  apply();
 	accept();
}

void KonqExtensionManager::slotApply()
{
  apply();
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

void KonqExtensionManager::show()
{
	d->pluginSelector->load();

	KDialog::show();
}

#include "konq_extensionmanager.moc"
