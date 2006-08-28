/*
 * main.cpp
 *
 * Copyright (c) 2000 Yves Arrouye <yves@realnames.com>
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <unistd.h>

#include <QLayout>
#include <QMap>
#include <QTabWidget>
//Added by qt3to4:
#include <QVBoxLayout>

#include <kdialog.h>
#include <kurifilter.h>
#include <kgenericfactory.h>

#include "filteropts.h"
#include "main.h"

typedef KGenericFactory<KURIFilterModule, QWidget> KURIFactory;
K_EXPORT_COMPONENT_FACTORY( kurifilt, KURIFactory("kcmkurifilt") )

class FilterOptions;

KURIFilterModule::KURIFilterModule(QWidget *parent, const QStringList &)
                 :KCModule(KURIFactory::instance(), parent)
{

    filter = KUriFilter::self();

    setQuickHelp( i18n("<h1>Enhanced Browsing</h1> In this module you can configure some enhanced browsing"
      " features of KDE. <h2>Internet Keywords</h2>Internet Keywords let you"
      " type in the name of a brand, a project, a celebrity, etc... and go to the"
      " relevant location. For example you can just type"
      " \"KDE\" or \"K Desktop Environment\" in Konqueror to go to KDE's homepage."
      "<h2>Web Shortcuts</h2>Web Shortcuts are a quick way of using Web search engines. For example, type \"altavista:frobozz\""
      " or \"av:frobozz\" and Konqueror will do a search on AltaVista for \"frobozz\"."
      " Even easier: just press Alt+F2 (if you have not"
      " changed this shortcut) and enter the shortcut in the KDE Run Command dialog."));

    QVBoxLayout *layout = new QVBoxLayout(this);

#if 0
    opts = new FilterOptions(this);
    tab->addTab(opts, i18n("&Filters"));
    connect(opts, SIGNAL(changed(bool)), SIGNAL(changed(bool)));
#endif

    modules.setAutoDelete(true);

    QMap<QString,KCModule*> helper;
    Q3PtrListIterator<KUriFilterPlugin> it = filter->pluginsIterator();
    for (; it.current(); ++it)
    {
        KCModule *module = it.current()->configModule(this, 0);
        if (module)
        {
            modules.append(module);
            helper.insert(it.current()->configName(), module);
            connect(module, SIGNAL(changed(bool)), SIGNAL(changed(bool)));
        }
    }

    if (modules.count() > 1)
    {
        QTabWidget *tab = new QTabWidget(this);

        QMap<QString,KCModule*>::iterator it2;
        for (it2 = helper.begin(); it2 != helper.end(); ++it2)
        {
            tab->addTab(it2.value(), it2.key());
        }

        tab->showPage(modules.first());
        widget = tab;
    }
    else if (modules.count() == 1)
    {
        widget = modules.first();
        layout->setMargin(-KDialog::marginHint());
    }

    layout->addWidget(widget);
}

void KURIFilterModule::load()
{
    Q3PtrListIterator<KCModule> it(modules);
    for (; it.current(); ++it)
    {
	  it.current()->load();
    }
}

void KURIFilterModule::save()
{
    Q3PtrListIterator<KCModule> it(modules);
    for (; it.current(); ++it)
    {
	  it.current()->save();
    }
}

void KURIFilterModule::defaults()
{
    Q3PtrListIterator<KCModule> it(modules);
    for (; it.current(); ++it)
    {
	  it.current()->defaults();
    }
}

#include "main.moc"
