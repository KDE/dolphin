/* This file is part of the KDE project
   Copyright (C) 2002 Waldo Bastian <bastian@kde.org>

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

#include <qlayout.h>
#include <qtabwidget.h>
#include <qfile.h>

#include <klocale.h>
#include <klibloader.h> 
#include <kparts/componentfactory.h>
#include <kservice.h>

#include "behaviour.h"
#include "fontopts.h"
#include "previews.h"
#include "browser.h"

static KCModule *load(QWidget *parent, const QString &libname, const QString &library, const QString &handle)
{
    KLibLoader *loader = KLibLoader::self();
    // attempt to load modules with ComponentFactory, only if the symbol init_<lib> exists
    // (this is because some modules, e.g. kcmkio with multiple modules in the library,
    // cannot be ported to KGenericFactory)
    KLibrary *lib = loader->library(QFile::encodeName(libname.arg(library)));
    if (lib) {
        QString initSym("init_");
        initSym += libname.arg(library);

        if ( lib->hasSymbol(QFile::encodeName(initSym)) )
        {
            // Reuse "lib" instead of letting createInstanceFromLibrary recreate it
            //KCModule *module = KParts::ComponentFactory::createInstanceFromLibrary<KCModule>(QFile::encodeName(libname.arg(library)));
            KLibFactory *factory = lib->factory();
            if ( factory )
            {
                KCModule *module = KParts::ComponentFactory::createInstanceFromFactory<KCModule>( factory );
                if (module)
                    return module;
            }
        }
    
	// get the create_ function
	QString factory("create_%1");
	void *create = lib->symbol(QFile::encodeName(factory.arg(handle)));
	
	if (create)
	    {
		// create the module
		KCModule* (*func)(QWidget *, const char *);
		func = (KCModule* (*)(QWidget *, const char *)) create;
		return  func(parent, 0);
	    }

        lib->unload();
    }
    return 0;
}

static KCModule *loadModule(QWidget *parent, const QString &module)
{
    KService::Ptr service = KService::serviceByDesktopName(module);
    if (!service)
       return 0;
    QString library = service->library();
    
    if (library.isEmpty())
       return 0;

    QString handle =  service->property("X-KDE-FactoryName").toString();
    if (handle.isEmpty())
       handle = library;

    KCModule *kcm = load(parent, "kcm_%1", library, handle);
    if (!kcm)
       kcm = load(parent, "libkcm_%1", library, handle);
    return kcm;
}


//-----------------------------------------------------------------------------

KBrowserOptions::KBrowserOptions(KConfig *config, QString group, QWidget *parent, const char *name)
    : KCModule( parent, "kcmkonq" ) 
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  QTabWidget *tab = new QTabWidget(this);
  layout->addWidget(tab);

  appearance = new KonqFontOptions(config, group, false, tab, name);
  behavior = new KBehaviourOptions(config, group, tab, name);
  previews = new KPreviewOptions(tab, name);
  kuick = loadModule(tab, "kcmkuick");

  tab->addTab(appearance, i18n("&Appearance"));
  tab->addTab(behavior, i18n("&Behavior"));
  tab->addTab(previews, i18n("&Previews"));
  if (kuick)
    tab->addTab(kuick, i18n("&Quick Copy && Move"));

  connect(appearance, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  connect(behavior, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  connect(previews, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  if (kuick)
     connect(kuick, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));

  connect(tab, SIGNAL(currentChanged(QWidget *)), 
          this, SIGNAL(quickHelpChanged()));
  m_tab = tab;
}

void KBrowserOptions::load()
{
  appearance->load();
  behavior->load();
  previews->load();
  if (kuick)
     kuick->load();
}

void KBrowserOptions::defaults()
{
  appearance->defaults();
  behavior->defaults();
  previews->defaults();
  if (kuick)
     kuick->defaults();
}

void KBrowserOptions::save()
{
  appearance->save();
  behavior->save();
  previews->save();
  if (kuick)
     kuick->save();
}

QString KBrowserOptions::quickHelp() const
{
  QWidget *w = m_tab->currentPage();
  if (w->inherits("KCModule"))
  {
     KCModule *m = static_cast<KCModule *>(w);
     return m->quickHelp();
  }
  return QString::null;
}

#include "browser.moc"
