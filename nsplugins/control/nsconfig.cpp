/**
 *  nsconfig.cpp
 *
 *  Copyright (c) 2000 Stefan Schimanski <1Stein@gmx.de>
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
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <qlayout.h>
#include <qvbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>
#include <kfile.h>
#include <qtextstream.h>
#include <kiconloader.h>

#include "nsconfig.h"

NSPluginConfig::NSPluginConfig(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  QVBoxLayout *topLayout = new QVBoxLayout(this, 5);

  // plugin scan
  QGroupBox *scanGrp = new QGroupBox( i18n("Plugin Scan"), this );
  topLayout->addWidget( scanGrp );
  QBoxLayout *scanLayout = 
     new QVBoxLayout( scanGrp, KDialog::marginHint(), KDialog::spacingHint());
  scanLayout->addSpacing( fontMetrics().lineSpacing() );

  // start in startkde?
  m_startkdeScan = new QCheckBox( i18n("Scan for new plugins at KDE startup"), scanGrp );
  scanLayout->addWidget( m_startkdeScan );
  connect( m_startkdeScan, SIGNAL(clicked(int)), this, SLOT(configChanged()) );

  // scan buttons
  QHBoxLayout *butLayout = new QHBoxLayout( scanLayout, 5 );

  QPushButton *scanButton = new QPushButton( i18n("&Scan"), scanGrp );  
  butLayout->addWidget( scanButton );
  connect( scanButton, SIGNAL(clicked()), this, SLOT(scan()) );

  QPushButton *dirButton = new QPushButton( i18n("Scan &Directories..."), scanGrp );
  butLayout->addWidget( dirButton );
  connect( dirButton, SIGNAL(clicked()), this, SLOT(changeDirs()) );
  connect( dirButton, SIGNAL(clicked()), this, SLOT(configChanged()) );

  // plugin list
  QGroupBox *listGrp = new QGroupBox( i18n("Registered Plugins"), this );
  topLayout->addWidget( listGrp, 1 );
  QBoxLayout *listLayout = 
     new QVBoxLayout( listGrp, KDialog::marginHint(), KDialog::spacingHint());
  listLayout->addSpacing( fontMetrics().lineSpacing() );
 
  m_pluginList = new QListView( listGrp );
  listLayout->addWidget( m_pluginList );
  m_pluginList->addColumn( i18n("Information") );
  m_pluginList->addColumn( i18n("Value") );
 
  load();  
}

NSPluginConfig::~NSPluginConfig() {}

void NSPluginConfig::configChanged()
{
  emit changed(true);
}

void NSPluginConfig::load()
{
  KConfig *config = new KConfig("kcmnspluginrc", true);
  
  config->setGroup("Misc");
  m_startkdeScan->setChecked( config->readBoolEntry( "startkdeScan", true ) );
  delete config;

  fillPluginList();

  emit changed(false);
}

void NSPluginConfig::save()
{

  KConfig *config= new KConfig("kcmnspluginrc", false);

  config->setGroup("Misc");
  config->writeEntry( "startkdeScan", m_startkdeScan->isChecked() );
  config->sync();
  delete config;

  emit changed(false);
}

void NSPluginConfig::defaults()
{
  m_startkdeScan->setChecked( true );
  fillPluginList();
  emit changed(true);
}

void NSPluginConfig::fillPluginList()
{
   kdDebug() << "-> NSPluginConfig::fillPluginList" << endl;
   m_pluginList->clear();
   QRegExp version(";version=[^:]*:");

   // open the cache file
   QFile cachef( locate("data", "nsplugins/cache") );
   if ( !cachef.open(IO_ReadOnly) )
   {
      kdDebug() << "Could not load plugin cache file!" << endl;
      return;
   }

   QTextStream cache(&cachef);


   // root object
   QListViewItem *root = new QListViewItem( m_pluginList, i18n("Netscape Plugins") );
   root->setOpen( true );
   root->setSelectable( false );
   root->setExpandable( true );
   root->setPixmap(0, SmallIcon("nsplugin"));

   // read in cache
   QString line, plugin;
   QListViewItem *next = 0;
   QListViewItem *lastMIME = 0;
   while ( !cache.atEnd() )
   {
      line = cache.readLine();
      kdDebug() << line << endl;
      if (line.isEmpty() || (line.left(1) == "#"))
	 continue;

      if (line.left(1) == "[")
      {
	 plugin = line.mid(1,line.length()-2);
	 kdDebug() << "plugin=" << plugin << endl;

	 // add plugin root item
	 next = new QListViewItem( root, i18n("Plugin"), plugin );
	 next->setOpen( true );
	 next->setSelectable( false );
	 next->setExpandable( true );

	 lastMIME = 0;

	 continue;
      }
     
      QStringList desc = QStringList::split(':', line, TRUE);
      QString mime = desc[0].stripWhiteSpace();
      QString name = desc[2];
      QString suffixes = desc[1];      

      if (!mime.isEmpty())
      {	 
	 kdDebug() << "mime=" << mime << " desc=" << name << " suffix=" << suffixes << endl;
	 lastMIME = new QListViewItem( next, lastMIME, i18n("MIME type"), mime );
	 lastMIME->setOpen( true );
	 lastMIME->setSelectable( false );
	 lastMIME->setExpandable( true );

	 QListViewItem *last = new QListViewItem( lastMIME, 0, i18n("Description"), name );
	 last->setOpen( false );
	 last->setSelectable( false );
	 last->setExpandable( false );

	 last = new QListViewItem( lastMIME, last, i18n("Suffixes"), suffixes );
	 last->setOpen( false );
	 last->setSelectable( false );
	 last->setExpandable( false );
      }
   } 
   kdDebug() << "<- NSPluginConfig::fillPluginList" << endl;
}

void NSPluginConfig::changeDirs()
{
}

void NSPluginConfig::scan()
{
   fillPluginList();
}

extern "C"
{
  KCModule *create_nsplugin(QWidget *parent, const char *name)
  {
    KGlobal::locale()->insertCatalogue("kcmnsplugin");
    return new NSPluginConfig(parent, name);
  };
}
#include "nsconfig.moc"
