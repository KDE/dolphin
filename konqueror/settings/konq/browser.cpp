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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QLayout>
#include <QTabWidget>
#include <QFile>
//Added by qt3to4:
#include <QVBoxLayout>

#include <klocale.h>
#include <kdialog.h>
#include <fixx11h.h>
#include <kcmoduleloader.h>

#include "behaviour.h"
#include "fontopts.h"
#include "previews.h"
#include "browser.h"

KBrowserOptions::KBrowserOptions(KConfig *config, QString group, KInstance *inst, QWidget *parent)
    : KCModule( inst, parent ) 
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  QTabWidget *tab = new QTabWidget(this);
  layout->addWidget(tab);

  appearance = new KonqFontOptions(config, group, false, inst, tab);
  appearance->layout()->setMargin( KDialog::marginHint() );

  behavior = new KBehaviourOptions(config, group, inst, tab);
  behavior->layout()->setMargin( KDialog::marginHint() );

  previews = new KPreviewOptions( inst, tab);
  previews->layout()->setMargin( KDialog::marginHint() );

  kuick = KCModuleLoader::loadModule("kcmkuick", KCModuleLoader::None,tab);

  tab->addTab(appearance, i18n("&Appearance"));
  tab->addTab(behavior, i18n("&Behavior"));
  tab->addTab(previews, i18n("&Previews && Meta-Data"));
  if (kuick)
  {
    kuick->layout()->setMargin( KDialog::marginHint() );
    tab->addTab(kuick, i18n("&Quick Copy && Move"));
  }

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
  QWidget *w = m_tab->currentWidget();
  if (w->inherits("KCModule"))
  {
     KCModule *m = static_cast<KCModule *>(w);
     return m->quickHelp();
  }
  return QString();
}

#include "browser.moc"
