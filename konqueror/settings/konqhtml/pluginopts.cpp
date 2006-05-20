// (c) 2002-2003 Leo Savernik, per-domain settings
// (c) 2001, Daniel Naber, based on javaopts.cpp
// (c) 2000 Stefan Schimanski <1Stein@gmx.de>, Netscape parts


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <QLabel>
#include <QLayout>
#include <qprogressdialog.h>
#include <QRegExp>
#include <QSlider>
#include <q3groupbox.h>
#include <QTextStream>

#include <dcopclient.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kiconloader.h>
#include <k3listview.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprocio.h>
#include <kstandarddirs.h>
#include <kurlrequester.h>

#include "htmlopts.h"
#include "ui_nsconfigwidget.h"
#include "pluginopts.h"
#include "policydlg.h"


// == class PluginPolicies =====

PluginPolicies::PluginPolicies(KConfig* config, const QString &group, bool global,
  		const QString &domain) :
	Policies(config,group,global,domain,"plugins.","EnablePlugins") {
}

PluginPolicies::~PluginPolicies() {
}

// == class KPluginOptions =====

KPluginOptions::KPluginOptions( KConfig* config, QString group, KInstance *inst, QWidget *parent)
    : KCModule( inst, parent ),
      m_pConfig( config ),
      m_groupname( group ),
      global_policies(config,group,true)
{
    QVBoxLayout* toplevel = new QVBoxLayout( this );
    toplevel->setSpacing( 0 /* KDialog::spacingHint() */ );
    toplevel->setMargin(0);

    /**************************************************************************
     ******************** Global Settings *************************************
     *************************************************************************/
    QGroupBox* globalGB = new QGroupBox( i18n( "Global Settings" ), this );
    toplevel->addWidget( globalGB );
    enablePluginsGloballyCB = new QCheckBox( i18n( "&Enable plugins globally" ), globalGB );
    enableHTTPOnly = new QCheckBox( i18n( "Only allow &HTTP and HTTPS URLs for plugins" ), globalGB );
    enableUserDemand = new QCheckBox( i18n( "&Load plugins on demand only" ), globalGB );
    priorityLabel = new QLabel(i18n("CPU priority for plugins: %1", QString()), globalGB);
    //priority = new QSlider(5, 100, 5, 100, Qt::Horizontal, globalGB);
    priority = new QSlider(Qt::Horizontal, globalGB);
    priority->setMinimum(5);
    priority->setMaximum(100);
    priority->setPageStep(5);

    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(enablePluginsGloballyCB);
    vbox->addWidget(enableHTTPOnly);
    vbox->addWidget(enableUserDemand);
    vbox->addWidget(priorityLabel);
    vbox->addWidget(priority);

    connect( enablePluginsGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
    connect( enablePluginsGloballyCB, SIGNAL( clicked() ), this, SLOT( slotTogglePluginsEnabled() ) );
    connect( enableHTTPOnly, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
    connect( enableUserDemand, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
    connect( priority, SIGNAL( valueChanged(int) ), this, SLOT( slotChanged() ) );
    connect( priority, SIGNAL( valueChanged(int) ), this, SLOT( updatePLabel(int) ) );

    QFrame *hrule = new QFrame(globalGB);
    hrule->setFrameStyle(QFrame::HLine | QFrame::Sunken);
    hrule->setSizePolicy(QSizePolicy::MinimumExpanding,QSizePolicy::Fixed);

    /**************************************************************************
     ********************* Domain-specific Settings ***************************
     *************************************************************************/
    QPushButton *domainSpecPB = new QPushButton(i18n("Domain-Specific Settin&gs"),
    						globalGB);
    domainSpecPB->setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
    connect(domainSpecPB,SIGNAL(clicked()),SLOT(slotShowDomainDlg()));

    vbox->addWidget(hrule);
    vbox->addWidget(domainSpecPB);

    globalGB->setLayout(vbox);

    domainSpecificDlg = new KDialogBase(KDialogBase::Swallow,
    			i18n("Domain-Specific Policies"),KDialogBase::Close,
			KDialogBase::Close,this,"domainSpecificDlg", true);

    domainSpecific = new PluginDomainListView(config,group,this,domainSpecificDlg);
    domainSpecific->setMinimumSize(320,200);
    connect(domainSpecific,SIGNAL(changed(bool)),SLOT(slotChanged()));

    domainSpecificDlg->setMainWidget(domainSpecific);

    /**************************************************************************
     ********************** WhatsThis? items **********************************
     *************************************************************************/
    enablePluginsGloballyCB->setWhatsThis( i18n("Enables the execution of plugins "
          "that can be contained in HTML pages, e.g. Macromedia Flash. "
          "Note that, as with any browser, enabling active contents can be a security problem.") );

    QString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific plugin policy for. This policy will be used "
                         "instead of the default policy for enabling or disabling plugins on pages sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    domainSpecific->listView()->setWhatsThis( wtstr );
    domainSpecific->importButton()->setWhatsThis( i18n("Click this button to choose the file that contains "
                                          "the plugin policies. These policies will be merged "
                                          "with the existing ones. Duplicate entries are ignored.") );
    domainSpecific->exportButton()->setWhatsThis( i18n("Click this button to save the plugin policy to a zipped "
                                          "file. The file, named <b>plugin_policy.tgz</b>, will be "
                                          "saved to a location of your choice." ) );
    domainSpecific->setWhatsThis( i18n("Here you can set specific plugin policies for any particular "
                                            "host or domain. To add a new policy, simply click the <i>New...</i> "
                                            "button and supply the necessary information requested by the "
                                            "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                            "button and choose the new policy from the policy dialog box. Clicking "
                                            "on the <i>Delete</i> button will remove the selected policy causing the default "
                                            "policy setting to be used for that domain.") );
#if 0
                                            "The <i>Import</i> and <i>Export</i> "
                                            "button allows you to easily share your policies with other people by allowing "
                                            "you to save and retrieve them from a zipped file.") );
#endif

/*****************************************************************************/

    QGroupBox* netscapeGB = new QGroupBox( i18n( "Netscape Plugins" ), this );
    toplevel->addWidget( netscapeGB );

    // create Designer made widget
    QWidget *dummy = new QWidget(netscapeGB);
    m_widget = new Ui::NSConfigWidget();
    m_widget->setupUi( dummy );
    dummy->setObjectName( "configwidget" );
    m_widget->dirEdit->setMode(KFile::ExistingOnly | KFile::LocalOnly | KFile::Directory);

    vbox = new QVBoxLayout();
    vbox->addWidget(dummy);
    vbox->setMargin( KDialog::marginHint());
    netscapeGB->setLayout(vbox);

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


void KPluginOptions::updatePLabel(int p) {
    QString level;
    p = (100 - p)/5;
    if (p > 15) {
            level = i18nc("lowest priority", "lowest");
    } else if (p > 11) {
            level = i18nc("low priority", "low");
    } else if (p > 7) {
            level = i18nc("medium priority", "medium");
    } else if (p > 3) {
            level = i18nc("high priority", "high");
    } else {
            level = i18nc("highest priority", "highest");
    }

    priorityLabel->setText(i18n("CPU priority for plugins: %1", level));
}


void KPluginOptions::load()
{
    // *** load ***
    global_policies.load();
    bool bPluginGlobal = global_policies.isFeatureEnabled();

    // *** apply to GUI ***
    enablePluginsGloballyCB->setChecked( bPluginGlobal );

    domainSpecific->initialize(m_pConfig->readEntry("PluginDomains", QStringList() ));

/****************************************************************************/

  KConfig *config = new KConfig("kcmnspluginrc", true);

  config->setGroup("Misc");
  m_widget->scanAtStartup->setChecked( config->readEntry( "startkdeScan", QVariant(false )).toBool() );

  m_widget->dirEdit->setURL("");
  m_widget->dirEdit->setEnabled( false );
  m_widget->dirRemove->setEnabled( false );
  m_widget->dirUp->setEnabled( false );
  m_widget->dirDown->setEnabled( false );
  enableHTTPOnly->setChecked( config->readEntry("HTTP URLs Only", QVariant(false)).toBool() );
  enableUserDemand->setChecked( config->readEntry("demandLoad", QVariant(false)).toBool() );
  priority->setValue(100 - qBound(0, config->readEntry("Nice Level", 0), 19) * 5);
  updatePLabel(priority->value());

  dirLoad( config );
  pluginLoad( config );

  delete config;

  change( false );
}

void KPluginOptions::defaults()
{
    global_policies.defaults();
    enablePluginsGloballyCB->setChecked( global_policies.isFeatureEnabled() );
    enableHTTPOnly->setChecked(false);
    enableUserDemand->setChecked(false);
    priority->setValue(100);

/*****************************************************************************/

    KConfig *config = new KConfig( QString(), true, false );

    m_widget->scanAtStartup->setChecked( false );

    m_widget->dirEdit->setURL("");
    m_widget->dirEdit->setEnabled( false );
    m_widget->dirRemove->setEnabled( false );

    dirLoad( config );
    pluginLoad( config );

    delete config;

    change();
}

void KPluginOptions::save()
{
    global_policies.save();

    domainSpecific->save(m_groupname,"PluginDomains");

    m_pConfig->sync();	// I need a sync here, otherwise "apply" won't work
    			// instantly

  QByteArray data;
  if ( !kapp->dcopClient()->isAttached() )
    kapp->dcopClient()->attach();
  kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "reparseConfiguration()", data );

/*****************************************************************************/

    KConfig *config= new KConfig("kcmnspluginrc", false);

    dirSave( config );
    pluginSave( config );

    config->setGroup("Misc");
    config->writeEntry( "startkdeScan", m_widget->scanAtStartup->isChecked() );
    config->writeEntry( "HTTP URLs Only", enableHTTPOnly->isChecked() );
    config->writeEntry( "demandLoad", enableUserDemand->isChecked() );
    config->writeEntry("Nice Level", (int)(100 - priority->value()) / 5);
    config->sync();
    delete config;

    change( false );
}

QString KPluginOptions::quickHelp() const
{
      return i18n("<h1>Konqueror Plugins</h1> The Konqueror web browser can use Netscape"
        " plugins to show special content, just like the Navigator does. Please note that"
        " the way you have to install Netscape plugins may depend on your distribution. A typical"
        " place to install them is, for example, '/opt/netscape/plugins'.");
}

void KPluginOptions::slotChanged()
{
    emit changed(true);
}

void KPluginOptions::slotTogglePluginsEnabled() {
  global_policies.setFeatureEnabled(enablePluginsGloballyCB->isChecked());
}

void KPluginOptions::slotShowDomainDlg() {
  domainSpecificDlg->show();
}

/***********************************************************************************/

void KPluginOptions::scan()
{
    m_widget->scanButton->setEnabled(false);
    if ( m_changed ) {
        int ret = KMessageBox::warningYesNoCancel( this,
                                                    i18n("Do you want to apply your changes "
                                                         "before the scan? Otherwise the "
                                                         "changes will be lost."), QString(), KStdGuiItem::save(), KStdGuiItem::discard() );
        if ( ret==KMessageBox::Cancel ) {
            m_widget->scanButton->setEnabled(true);
            return;
        }
        if ( ret==KMessageBox::Yes )
             save();
    }

    KProcIO* nspluginscan = new KProcIO;
    QString scanExe = KGlobal::dirs()->findExe("nspluginscan");
#warning !QString? i guess that meant isEmpty?
    if (scanExe.isEmpty()) {
        kDebug() << "can't find nspluginviewer" << endl;
        delete nspluginscan;

        KMessageBox::sorry ( this,
                             i18n("The nspluginscan executable cannot be found. "
                                  "Netscape plugins will not be scanned.") );
        m_widget->scanButton->setEnabled(true);
        return;
    }

    // find nspluginscan executable
    m_progress = new QProgressDialog( i18n("Scanning for plugins"), i18n("Cancel"), 0, 100, this );
    m_progress->setValue( 5 );

    // start nspluginscan
    *nspluginscan << scanExe << "--verbose";
    kDebug() << "Running nspluginscan" << endl;
    connect(nspluginscan, SIGNAL(readReady(KProcIO*)),
            this, SLOT(progress(KProcIO*)));
    connect(nspluginscan, SIGNAL(processExited(KProcess *)),
            this, SLOT(scanDone()));
    connect(m_progress, SIGNAL(canceled()), this, SLOT(scanDone()));

    if (nspluginscan->start())
       kapp->enter_loop();

    delete nspluginscan;

    // update dialog
    if (m_progress) {
        m_progress->setValue(100);
        load();
        delete m_progress;
        m_progress = 0;
    }
    m_widget->scanButton->setEnabled(true);
}

void KPluginOptions::progress(KProcIO *proc)
{
    QString line;
    while(proc->readln(line) > 0)
        ;
    m_progress->setValue(line.trimmed().toInt());
}

void KPluginOptions::scanDone()
{
    kapp->exit_loop();
}

/***********************************************************************************/


void KPluginOptions::dirInit()
{
    m_widget->dirEdit->setWindowTitle(i18n("Select Plugin Scan Folder"));
    connect( m_widget->dirNew, SIGNAL(clicked()), SLOT(dirNew()));
    connect( m_widget->dirRemove, SIGNAL(clicked()), SLOT(dirRemove()));
    connect( m_widget->dirUp, SIGNAL(clicked()), SLOT(dirUp()));
    connect( m_widget->dirDown, SIGNAL(clicked()), SLOT(dirDown()) );
    connect( m_widget->useArtsdsp, SIGNAL(clicked()),SLOT(change()));
    connect( m_widget->dirEdit,
             SIGNAL(textChanged(const QString&)),
             SLOT(dirEdited(const QString &)) );

    connect( m_widget->dirList,
             SIGNAL(executed(Q3ListBoxItem*)),
             SLOT(dirSelect(Q3ListBoxItem*)) );

    connect( m_widget->dirList,
             SIGNAL(selectionChanged(Q3ListBoxItem*)),
             SLOT(dirSelect(Q3ListBoxItem*)) );
}


void KPluginOptions::dirLoad( KConfig *config )
{
    QStringList paths;

    // read search paths

    config->setGroup("Misc");
    if ( config->hasKey( "scanPaths" ) )
        paths = config->readEntry( "scanPaths" , QStringList() );
    else {//keep sync with kdebase/nsplugins/pluginscan
        paths.append("$HOME/.mozilla/plugins");
        paths.append("$HOME/.netscape/plugins");
        paths.append("/usr/lib64/browser-plugins");
        paths.append("/usr/lib/browser-plugins");
        paths.append("/usr/local/netscape/plugins");
        paths.append("/opt/mozilla/plugins");
	paths.append("/opt/mozilla/lib/plugins");
        paths.append("/opt/netscape/plugins");
        paths.append("/opt/netscape/communicator/plugins");
        paths.append("/usr/lib/netscape/plugins");
        paths.append("/usr/lib/netscape/plugins-libc5");
        paths.append("/usr/lib/netscape/plugins-libc6");
        paths.append("/usr/lib/mozilla/plugins");
	paths.append("/usr/lib64/netscape/plugins");
	paths.append("/usr/lib64/mozilla/plugins");
        paths.append("$MOZILLA_HOME/plugins");
    }

    // fill list
    m_widget->dirList->clear();
    m_widget->dirList->insertStringList( paths );

    // setup other widgets
    bool useArtsdsp = config->readEntry( "useArtsdsp", QVariant(false )).toBool();
    m_widget->useArtsdsp->setChecked( useArtsdsp );
}


void KPluginOptions::dirSave( KConfig *config )
{
    // create stringlist
    QStringList paths;
    Q3ListBoxItem *item = m_widget->dirList->firstItem();
    for ( ; item!=0; item=item->next() )
        if ( !item->text().isEmpty() )
            paths << item->text();

    // write entry
    config->setGroup( "Misc" );
    config->writeEntry( "scanPaths", paths );
    config->writeEntry( "useArtsdsp", m_widget->useArtsdsp->isChecked() );
}


void KPluginOptions::dirSelect( Q3ListBoxItem *item )
{
    m_widget->dirEdit->setEnabled( item!=0 );
    m_widget->dirRemove->setEnabled( item!=0 );

    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    m_widget->dirDown->setEnabled( item!=0 && cur<m_widget->dirList->count()-1 );
    m_widget->dirUp->setEnabled( item!=0 && cur>0 );
    m_widget->dirEdit->setURL( item!=0 ? item->text() : QString() );
 }


void KPluginOptions::dirNew()
{
    m_widget->dirList->insertItem( QString(), 0 );
    m_widget->dirList->setCurrentItem( 0 );
    dirSelect( m_widget->dirList->selectedItem() );
    m_widget->dirEdit->setURL(QString());
    m_widget->dirEdit->setFocus();
    change();
}


void KPluginOptions::dirRemove()
{
    m_widget->dirEdit->setURL(QString());
    delete m_widget->dirList->selectedItem();
    m_widget->dirRemove->setEnabled( false );
    m_widget->dirUp->setEnabled( false );
    m_widget->dirDown->setEnabled( false );
    m_widget->dirEdit->setEnabled( false );
    change();
}


void KPluginOptions::dirUp()
{
    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
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
    unsigned cur = m_widget->dirList->index(m_widget->dirList->selectedItem());
    if ( cur < m_widget->dirList->count()-1 ) {
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
    if ( m_widget->dirList->currentText() != txt ) {
        m_widget->dirList->blockSignals(true);
        m_widget->dirList->changeItem( txt, m_widget->dirList->currentItem() );
        m_widget->dirList->blockSignals(false);
        change();
    }
}


/***********************************************************************************/


void KPluginOptions::pluginInit()
{
}


void KPluginOptions::pluginLoad( KConfig* /*config*/ )
{
    kDebug() << "-> KPluginOptions::fillPluginList" << endl;
    m_widget->pluginList->clear();
    QRegExp version(";version=[^:]*:");

    // open the cache file
    QFile cachef( locate("data", "nsplugins/cache") );
    if ( !cachef.exists() || !cachef.open(QIODevice::ReadOnly) ) {
        kDebug() << "Could not load plugin cache file!" << endl;
        return;
    }

    QTextStream cache(&cachef);

    // root object
    Q3ListViewItem *root = new Q3ListViewItem( m_widget->pluginList, i18n("Netscape Plugins") );
    root->setOpen( true );
    root->setSelectable( false );
    root->setExpandable( true );
    root->setPixmap(0, SmallIcon("netscape"));

    // read in cache
    QString line, plugin;
    Q3ListViewItem *next = 0;
    Q3ListViewItem *lastMIME = 0;
    while ( !cache.atEnd() ) {

        line = cache.readLine();
        //kDebug() << line << endl;
        if (line.isEmpty() || (line.left(1) == "#"))
            continue;

        if (line.left(1) == "[") {

            plugin = line.mid(1,line.length()-2);
            //kDebug() << "plugin=" << plugin << endl;

            // add plugin root item
            next = new Q3ListViewItem( root, i18n("Plugin"), plugin );
            next->setOpen( false );
            next->setSelectable( false );
            next->setExpandable( true );

            lastMIME = 0;

            continue;
        }

        QStringList desc = line.split(':');
        QString mime = desc[0].trimmed();
        QString name = desc[2];
        QString suffixes = desc[1];

        if (!mime.isEmpty() && next) {
            //kDebug() << "mime=" << mime << " desc=" << name << " suffix=" << suffixes << endl;
            lastMIME = new Q3ListViewItem( next, lastMIME, i18n("MIME type"), mime );
            lastMIME->setOpen( false );
            lastMIME->setSelectable( false );
            lastMIME->setExpandable( true );

            Q3ListViewItem *last = new Q3ListViewItem( lastMIME, 0, i18n("Description"), name );
            last->setOpen( false );
            last->setSelectable( false );
            last->setExpandable( false );

            last = new Q3ListViewItem( lastMIME, last, i18n("Suffixes"), suffixes );
            last->setOpen( false );
            last->setSelectable( false );
            last->setExpandable( false );
        }
    }

    //kDebug() << "<- KPluginOptions::fillPluginList" << endl;
}


void KPluginOptions::pluginSave( KConfig* /*config*/ )
{

}

// == class PluginDomainDialog =====

PluginDomainDialog::PluginDomainDialog(QWidget *parent) :
	QWidget(parent) 
{
  setObjectName("PluginDomainDialog");
  setWindowTitle(i18n("Domain-Specific Policies"));

  thisLayout = new QVBoxLayout(this);
  thisLayout->addSpacing(6);
  QFrame *hrule = new QFrame(this);
  hrule->setFrameStyle(QFrame::HLine | QFrame::Sunken);
  thisLayout->addWidget(hrule);
  thisLayout->addSpacing(6);

  QBoxLayout *hl = new QHBoxLayout(this);
  hl->setSpacing(6);
  hl->setMargin(0);
  hl->addStretch(10);

  QPushButton *closePB = new KPushButton(KStdGuiItem::close(),this);
  connect(closePB,SIGNAL(clicked()),SLOT(slotClose()));
  hl->addWidget(closePB);
  thisLayout->addLayout(hl);
}

PluginDomainDialog::~PluginDomainDialog() {
}

void PluginDomainDialog::setMainWidget(QWidget *widget) {
  thisLayout->insertWidget(0,widget);
}

void PluginDomainDialog::slotClose() {
  hide();
}

// == class PluginDomainListView =====

PluginDomainListView::PluginDomainListView(KConfig *config,const QString &group,
	KPluginOptions *options,QWidget *parent)
	: DomainListView(config,i18n( "Doma&in-Specific" ), parent),
	group(group), options(options) {
}

PluginDomainListView::~PluginDomainListView() {
}

void PluginDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies *pol) {
  QString caption;
  switch (trigger) {
    case AddButton:
      caption = i18n( "New Plugin Policy" );
      pol->setFeatureEnabled(!options->enablePluginsGloballyCB->isChecked());
      break;
    case ChangeButton: caption = i18n( "Change Plugin Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setWindowTitle(caption);
  pDlg.setFeatureEnabledLabel(i18n("&Plugin policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a plugin policy for "
                                    "the above host or domain."));
  pDlg.refresh();
}

PluginPolicies *PluginDomainListView::createPolicies() {
  return new PluginPolicies(config,group,false);
}

PluginPolicies *PluginDomainListView::copyPolicies(Policies *pol) {
  return new PluginPolicies(*static_cast<PluginPolicies *>(pol));
}

#include "pluginopts.moc"
