// (c) 2001, Daniel Naber, based on javaopts.cpp
// (c) 2000 Stefan Schimanski <1Stein@gmx.de>, Netscape parts

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <qregexp.h>
#include <qlayout.h>
#include <qvbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qwhatsthis.h>
#include <kconfig.h>
#include <kglobal.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kdialog.h>
#include <kdebug.h>
#include <kapplication.h>
#include <qtextstream.h>
#include <kiconloader.h>
#include <qlineedit.h>
#include <kfiledialog.h>

#include <qlayout.h>
#include <qtoolbutton.h>
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <kconfig.h>
#include <kdebug.h>
#include <X11/Xlib.h>

#include <kapplication.h>
#include <dcopclient.h>
#include <kprocio.h>
#include <qprogressdialog.h>
#include <kmessagebox.h>
#include <qlistview.h>
#include <qlistbox.h>
#include <kfile.h>

#include "htmlopts.h"
#include "pluginopts.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>

#include "pluginopts.moc"

KPluginOptions::KPluginOptions( KConfig* config, QString group, QWidget *parent,
                            const char *)
    : KCModule( parent, "kcmkonqhtml" ),
      m_pConfig( config ),
      m_groupname( group )
{
    QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

    /***************************************************************************
     ********************* Global Settings *************************************
     **************************************************************************/
    QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enablePluginsGloballyCB = new QCheckBox( i18n( "Enable plugins globally" ), globalGB );
    connect( enablePluginsGloballyCB, SIGNAL( clicked() ), this, SLOT( changed() ) );

    /***************************************************************************
     ********************** WhatsThis? items ***********************************
     **************************************************************************/
    QWhatsThis::add( enablePluginsGloballyCB, i18n("Enables the execution of plugins "
          "that can be contained in HTML pages, e.g. Macromedia Flash. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );

/***********************************************************************************/

    QVGroupBox* netscapeGB = new QVGroupBox( i18n( "Netscape Plugins" ), this );
    toplevel->addWidget( netscapeGB );

    // create Designer made widget
    m_widget = new NSConfigWidget( netscapeGB, "configwidget" );

    // setup widgets
    connect( m_widget->scanAtStartup, SIGNAL(clicked()), SLOT(change()) );
    connect( m_widget->scanButton, SIGNAL(clicked()), SLOT(scan()) );

    m_changed = false;

    dirInit();
    pluginInit();

    // Finally do the loading
    load();
}

KPluginOptions::~KPluginOptions()
{
delete m_pConfig;
}

void KPluginOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);
    bool bPluginGlobal = m_pConfig->readBoolEntry( "EnablePlugins", true );

    // *** apply to GUI ***
    enablePluginsGloballyCB->setChecked( bPluginGlobal );

/***********************************************************************************/

  KConfig *config = new KConfig("kcmnspluginrc", true);

  config->setGroup("Misc");
  m_widget->scanAtStartup->setChecked( config->readBoolEntry( "startkdeScan", false ) );

  dirLoad( config );
  pluginLoad( config );

  delete config;

  change( false );
}

void KPluginOptions::defaults()
{
    enablePluginsGloballyCB->setChecked( true );

/***********************************************************************************/

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

void KPluginOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "EnablePlugins", enablePluginsGloballyCB->isChecked() );

  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

/***********************************************************************************/

    KConfig *config= new KConfig("kcmnspluginrc", false);

    dirSave( config );
    pluginSave( config );

    config->setGroup("Misc");
    config->writeEntry( "startkdeScan", m_widget->scanAtStartup->isChecked() );
    config->sync();
    delete config;

    change( false );
}

QString KPluginOptions::quickHelp() const
{
      return i18n("<h1>Plugins</h1> The Konqueror web browser can use Netscape"
        " plugins to show special content, just like the Navigator does. Please note that,"
        " the way you have to install Netscape plugins may depend on your distribution. A typical"
        " place to install them is, for example, '/opt/netscape/plugins'.");
}

void KPluginOptions::changed()
{
    emit changed(true);
}

/***********************************************************************************/

void KPluginOptions::scan()
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
    m_progress = new QProgressDialog( i18n("Scanning for plugins"), i18n("Cancel"), 100, this );
    KProcIO* nspluginscan = new KProcIO;
    QString scanExe = KGlobal::dirs()->findExe("nspluginscan");
    if (!scanExe) {
        kdDebug() << "can't find nspluginviewer" << endl;
        delete nspluginscan;

        KMessageBox::sorry ( this,
                             i18n("The nspluginscan executable cannot be found. "
                                  "Netscape plugins will not be scanned.") );
        return;
    }
    m_progress->setProgress( 5 );

    // start nspluginscan
    *nspluginscan << scanExe;
    kdDebug() << "Running nspluginscan" << endl;
    connect(nspluginscan, SIGNAL(readReady(KProcIO*)),
            this, SLOT(progress(KProcIO*)));
    connect(nspluginscan, SIGNAL(processExited(KProcess *)),
            this, SLOT(scanDone()));
    if (nspluginscan->start())
       kapp->enter_loop();

    delete nspluginscan;

    // update dialog
    m_progress->setProgress(100);
    load();
    delete m_progress;
}

void KPluginOptions::progress(KProcIO *proc)
{
    QString line;
    while(proc->readln(line) > 0);
    m_progress->setProgress(line.stripWhiteSpace().toInt());
}

void KPluginOptions::scanDone()
{
    kapp->exit_loop();
}


extern "C"
{
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


void KPluginOptions::dirInit()
{
    connect( m_widget->dirBrowse, SIGNAL(clicked()), SLOT(dirBrowse()) );
    connect( m_widget->dirNew, SIGNAL(clicked()), SLOT(dirNew()) );
    connect( m_widget->dirRemove, SIGNAL(clicked()), SLOT(dirRemove()) );
    connect( m_widget->dirUp, SIGNAL(clicked()), SLOT(dirUp()) );
    connect( m_widget->dirDown, SIGNAL(clicked()), SLOT(dirDown()) );
    connect( m_widget->useArtsdsp, SIGNAL(clicked ()),SLOT(change()));
    connect( m_widget->dirEdit,
             SIGNAL(textChanged(const QString&)),
             SLOT(dirEdited(const QString &)) );

    connect( m_widget->dirList,
             SIGNAL(currentChanged(QListBoxItem*)),
             SLOT(dirSelect(QListBoxItem*)) );
}


void KPluginOptions::dirLoad( KConfig *config )
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


void KPluginOptions::dirSave( KConfig *config )
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


void KPluginOptions::dirSelect( QListBoxItem *item )
{
    m_widget->dirEdit->setEnabled( item!=0 );
    m_widget->dirBrowse->setEnabled( item!=0 );
    m_widget->dirRemove->setEnabled( item!=0 );

    unsigned cur = m_widget->dirList->currentItem();
    m_widget->dirDown->setEnabled( item!=0 && cur<m_widget->dirList->count()-1 );
    m_widget->dirUp->setEnabled( item!=0 && cur>0 );

    m_widget->dirEdit->setText( item!=0 ? item->text() : QString::null );

}


void KPluginOptions::dirBrowse()
{
    QString path = KFileDialog::getExistingDirectory( QString::null,  this,
                                                i18n("Select Plugin Scan Directory") );
    if ( !path.isEmpty() ) {
        m_widget->dirEdit->setText( path );
        change();
    }
}


void KPluginOptions::dirNew()
{
    m_widget->dirList->insertItem( QString::null, 0 );
    m_widget->dirList->setCurrentItem( 0 );
    m_widget->dirEdit->setFocus();
    change();
}


void KPluginOptions::dirRemove()
{
    m_widget->dirList->removeItem( m_widget->dirList->currentItem() );
    change();
}


void KPluginOptions::dirUp()
{
    unsigned cur = m_widget->dirList->currentItem();
    if ( cur>0 ) {
        QString txt = m_widget->dirList->text(cur-1);
        m_widget->dirList->removeItem( cur-1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( cur-1>0 );
        m_widget->dirDown->setEnabled( true );
        change();
    }
}


void KPluginOptions::dirDown()
{
    unsigned cur = m_widget->dirList->currentItem();
    if ( cur<m_widget->dirList->count()-1 ) {
        QString txt = m_widget->dirList->text(cur+1);
        m_widget->dirList->removeItem( cur+1 );
        m_widget->dirList->insertItem( txt, cur );

        m_widget->dirUp->setEnabled( true );
        m_widget->dirDown->setEnabled( cur+1<m_widget->dirList->count()-1 );
        change();
    }
}


void KPluginOptions::dirEdited(const QString &txt )
{
    if ( m_widget->dirList->currentText()!=txt ) {
        m_widget->dirList->changeItem( txt, m_widget->dirList->currentItem() );
        change();
    }
}


/***********************************************************************************/


void KPluginOptions::pluginInit()
{
}


void KPluginOptions::pluginLoad( KConfig */*config*/ )
{
    kdDebug() << "-> KPluginOptions::fillPluginList" << endl;
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

    kdDebug() << "<- KPluginOptions::fillPluginList" << endl;
}


void KPluginOptions::pluginSave( KConfig */*config*/ )
{

}
