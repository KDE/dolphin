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

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <qlayout.h>
#include <qvbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>
#include <kapp.h>
#include <kfile.h>
#include <qtextstream.h>
#include <kiconloader.h>
#include <kprocess.h>
#include <kmessagebox.h>
#include <qprogressdialog.h>
#include <qlistbox.h>
#include <qtoolbutton.h>
#include <qlineedit.h>
#include <kfiledialog.h>

#include "nsconfig.h"


NSPluginConfig::NSPluginConfig(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
    // create Designer made widget
    m_widget = new ConfigWidget( this, "configwidget" );
    QVBoxLayout *layout = new QVBoxLayout( this );
    layout->addWidget( m_widget );

    // setup widgets
    connect( m_widget->scanAtStartup, SIGNAL(clicked()), SLOT(change()) );
    connect( m_widget->scanButton, SIGNAL(clicked()), SLOT(scan()) );

    m_changed = false;

    dirInit();
    pluginInit();

    load();


}


NSPluginConfig::~NSPluginConfig()
{
}


void NSPluginConfig::load()
{
  KConfig *config = new KConfig("kcmnspluginrc", true);

  config->setGroup("Misc");
  m_widget->scanAtStartup->setChecked( config->readBoolEntry( "startkdeScan", false ) );

  dirLoad( config );
  pluginLoad( config );

  delete config;

  change( false );
}


void NSPluginConfig::save()
{
    KConfig *config= new KConfig("kcmnspluginrc", false);

    dirSave( config );
    pluginSave( config );

    config->setGroup("Misc");
    config->writeEntry( "startkdeScan", m_widget->scanAtStartup->isChecked() );
    config->sync();
    delete config;

    change( false );
}


void NSPluginConfig::defaults()
{
    KConfig *config = new KConfig( QString::null, true, false );

    m_widget->scanAtStartup->setChecked( false );

    m_widget->dirEdit->setText("");
    m_widget->dirEdit->setEnabled( false );
    m_widget->dirBrowse->setEnabled( false );
    m_widget->dirRemove->setEnabled( false );

    dirLoad( config );
    pluginLoad( config );

    delete config;

    change();
}


void NSPluginConfig::scan()
{
    if ( m_changed ) {
        int ret = KMessageBox::warningYesNoCancel( this,
                                                    i18n("Do you want to apply your changes "
                                                         "before the scan? Otherwise the "
                                                         "changes will be lost.") );
        if ( ret==KMessageBox::Cancel ) return;
        if ( ret==KMessageBox::Yes )
             save();
    }

    // find nspluginscan executable
    QProgressDialog progress( i18n("Scanning for plugins"), i18n("Cancel"), 4, this );
    KProcess* nspluginscan = new KProcess;
    QString scanExe = KGlobal::dirs()->findExe("nspluginscan");
    if (!scanExe) {
        kdDebug() << "can't find nspluginviewer" << endl;
        delete nspluginscan;

        KMessageBox::sorry ( this,
                             i18n("The nspluginscan executable can't be found. "
                                  "Netscape plugins won't be scanned.") );
        return;
    }
    progress.setProgress( 1 );

    // start nspluginscan
    *nspluginscan << scanExe;
    kdDebug() << "Running nspluginscan" << endl;
    nspluginscan->start();
    progress.setProgress( 2 );

    // wait for termination of nspluginscan
    while ( nspluginscan->isRunning() ) {
        if ( progress.wasCancelled() ) break;
        kapp->processEvents();
    }
    progress.setProgress( 2 );

    delete nspluginscan;

    // update dialog
    load();
    progress.setProgress( 4 );
}


QString NSPluginConfig::quickHelp() const
{
      return i18n("<h1>Netscape Plugins</h1> The Konqueror web browser can use netscape"
        " plugins to show special content, just like the Navigator does. Please note that"
        " how you have to install netscape plugins may depend on your distribution. A typical"
        " place to install them is for example '/opt/netscape/plugins'.");
}


extern "C"
{
    KCModule *create_nsplugin(QWidget *parent, const char *name)
    {
        KGlobal::locale()->insertCatalogue("nsplugin");
        return new NSPluginConfig(parent, name);
    };

    void init_nsplugin()
    {
        KConfig *config = new KConfig("kcmnspluginrc", true);
        config->setGroup("Misc");
        bool scan = config->readBoolEntry( "startkdeScan", false );
        bool firstTime = config->readBoolEntry( "firstTime", true );
        delete config;

        if ( scan || firstTime )
        {
            system( "nspluginscan" );

            config= new KConfig("kcmnspluginrc", false);
            config->setGroup("Misc");
            config->writeEntry( "firstTime", false );
            config->sync();
            delete config;
        }
    }
}


/***********************************************************************************/


void NSPluginConfig::dirInit()
{
    connect( m_widget->dirBrowse, SIGNAL(clicked()), SLOT(dirBrowse()) );
    connect( m_widget->dirNew, SIGNAL(clicked()), SLOT(dirNew()) );
    connect( m_widget->dirRemove, SIGNAL(clicked()), SLOT(dirRemove()) );
    connect( m_widget->dirUp, SIGNAL(clicked()), SLOT(dirUp()) );
    connect( m_widget->dirDown, SIGNAL(clicked()), SLOT(dirDown()) );

    connect( m_widget->dirEdit,
             SIGNAL(textChanged(const QString&)),
             SLOT(dirEdited(const QString &)) );

    connect( m_widget->dirList,
             SIGNAL(currentChanged(QListBoxItem*)),
             SLOT(dirSelect(QListBoxItem*)) );

    // XXX disabled to avoid new strings
    m_widget->useArtsdsp->hide();
}


void NSPluginConfig::dirLoad( KConfig *config )
{
    QStringList paths;

    // read search paths
    config->setGroup("Misc");
    if ( config->hasKey( "scanPaths" ) )
        paths = config->readListEntry( "scanPaths" );
    else {
        paths.append("$HOME/.netscape/plugins");
        paths.append("/usr/local/netscape/plugins");
        paths.append("/opt/netscape/plugins");
        paths.append("/opt/netscape/communicator/plugins");
        paths.append("/usr/lib/netscape/plugins");
        paths.append("/usr/lib/netscape/plugins-libc5");
        paths.append("/usr/lib/netscape/plugins-libc6");
        paths.append("/usr/lib/mozilla/plugins");
        paths.append("$MOZILLA_HOME/plugins");
    }

    // fill list
    m_widget->dirList->clear();
    m_widget->dirList->insertStringList( paths );

    // setup other widgets
    bool useArtsdsp = config->readBoolEntry( "useArtsdsp", true );
    m_widget->useArtsdsp->setChecked( useArtsdsp );
}


void NSPluginConfig::dirSave( KConfig *config )
{
    // create stringlist
    QStringList paths;
    QListBoxItem *item = m_widget->dirList->firstItem();
    for ( ; item!=0; item=item->next() )
        if ( !item->text().isEmpty() )
            paths << item->text();

    // write entry
    config->setGroup( "Misc" );
    config->writeEntry( "scanPaths", paths );
    config->writeEntry( "useArtsdsp", m_widget->useArtsdsp->isOn() );
}


void NSPluginConfig::dirSelect( QListBoxItem *item )
{
    m_widget->dirEdit->setEnabled( item!=0 );
    m_widget->dirBrowse->setEnabled( item!=0 );
    m_widget->dirRemove->setEnabled( item!=0 );

    unsigned cur = m_widget->dirList->currentItem();
    m_widget->dirDown->setEnabled( item!=0 && cur<m_widget->dirList->count()-1 );
    m_widget->dirUp->setEnabled( item!=0 && cur>0 );

    m_widget->dirEdit->setText( item!=0 ? item->text() : QString::null );

}


void NSPluginConfig::dirBrowse()
{
    QString path = KFileDialog::getExistingDirectory( QString::null,  this,
                                                i18n("Select plugin scan directory") );
    if ( !path.isEmpty() ) {
        m_widget->dirEdit->setText( path );
        change();
    }
}


void NSPluginConfig::dirNew()
{
    m_widget->dirList->insertItem( QString::null, 0 );
    m_widget->dirList->setCurrentItem( 0 );
    m_widget->dirEdit->setFocus();
    change();
}


void NSPluginConfig::dirRemove()
{
    m_widget->dirList->removeItem( m_widget->dirList->currentItem() );
    change();
}


void NSPluginConfig::dirUp()
{
    unsigned cur = m_widget->dirList->currentItem();
    if ( cur>0 ) {
        QString txt = m_widget->dirList->text(cur-1);
        m_widget->dirList->removeItem( cur-1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( cur-1>0 );
        m_widget->dirDown->setEnabled( true );
    }
}


void NSPluginConfig::dirDown()
{
    unsigned cur = m_widget->dirList->currentItem();
    if ( cur<m_widget->dirList->count()-1 ) {
        QString txt = m_widget->dirList->text(cur+1);
        m_widget->dirList->removeItem( cur+1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( true );
        m_widget->dirDown->setEnabled( cur+1<m_widget->dirList->count()-1 );
    }
}


void NSPluginConfig::dirEdited(const QString &txt )
{
    if ( m_widget->dirList->currentText()!=txt ) {
        m_widget->dirList->changeItem( txt, m_widget->dirList->currentItem() );
        change();
    }
}


/***********************************************************************************/


void NSPluginConfig::pluginInit()
{
}


void NSPluginConfig::pluginLoad( KConfig */*config*/ )
{
    kdDebug() << "-> NSPluginConfig::fillPluginList" << endl;
    m_widget->pluginList->clear();
    QRegExp version(";version=[^:]*:");

    // open the cache file
    QFile cachef( locate("data", "nsplugins/cache") );
    if ( !cachef.open(IO_ReadOnly) ) {
        kdDebug() << "Could not load plugin cache file!" << endl;
        return;
    }

    QTextStream cache(&cachef);

    // root object
    QListViewItem *root = new QListViewItem( m_widget->pluginList, i18n("Netscape Plugins") );
    root->setOpen( true );
    root->setSelectable( false );
    root->setExpandable( true );
    root->setPixmap(0, SmallIcon("netscape"));

    // read in cache
    QString line, plugin;
    QListViewItem *next = 0;
    QListViewItem *lastMIME = 0;
    while ( !cache.atEnd() ) {

        line = cache.readLine();
        kdDebug() << line << endl;
        if (line.isEmpty() || (line.left(1) == "#"))
            continue;

        if (line.left(1) == "[") {

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

        if (!mime.isEmpty()) {
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


void NSPluginConfig::pluginSave( KConfig */*config*/ )
{

}


#include "nsconfig.moc"
