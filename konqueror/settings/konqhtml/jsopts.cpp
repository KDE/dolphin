// (c) Martin R. Jones 1996
// (c) Bernd Wuebben 1998
// KControl port & modifications
// (c) Torben Weis 1998
// End of the KControl port, added 'kfmclient configure' call.
// (c) David Faure 1998
// New configuration scheme for JavaScript
// (C) Kalle Dalheimer 2000
// Major cleanup & Java/JS settings splitted
// (c) Daniel Molkentin 2000
// Big changes to accomodate per-domain settings
// (c) Leo Savernik 2002

#include <kfiledialog.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qcolor.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qmessagebox.h>
#include <qwhatsthis.h>
#include <qvgroupbox.h>
#include <qhbox.h>
#include <qvbox.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <klistview.h>
#include <kmessagebox.h>
#include <qlabel.h>
#include <kcharsets.h>
#include <qspinbox.h>
#include <kdebug.h>
#include <kurlrequester.h>
#include <X11/Xlib.h>
#include <klineedit.h>

#include "htmlopts.h"
#include "policydlg.h"

#include <konq_defaults.h> // include default values directly from konqueror
#include <klocale.h>
#include <khtml_settings.h>
#include <khtmldefaults.h>

#include "jsopts.h"
#include "jspolicies.h"

#include "jsopts.moc"

KJavaScriptOptions::KJavaScriptOptions( KConfig* config, QString group, QWidget *parent,
										const char *name ) :
  KCModule( parent, name ),
  _removeJavaScriptDomainAdvice(false),
   m_pConfig( config ), m_groupname( group ),
  js_global_policies(config,group,true,QString::null),
  _removeECMADomainSettings(false)
{
  QVBoxLayout* toplevel = new QVBoxLayout( this, 10, 5 );

  // the global checkbox
  QVGroupBox* globalGB = new QVGroupBox( i18n( "Global Settings" ), this );
  toplevel->addWidget( globalGB );

  enableJavaScriptGloballyCB = new QCheckBox( i18n( "Ena&ble JavaScript globally" ), globalGB );
  QWhatsThis::add( enableJavaScriptGloballyCB, i18n("Enables the execution of scripts written in ECMA-Script "
        "(also known as JavaScript) that can be contained in HTML pages. "
        "Note that, as with any browser, enabling scripting languages can be a security problem.") );
  connect( enableJavaScriptGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );

//  enableJavaScriptDebugCB = new QCheckBox( i18n( "Enable debu&gging" ), globalGB );
//  QWhatsThis::add( enableJavaScriptDebugCB, i18n("Enables the reporting of errors that occur when JavaScript "
//        "code is executed, and allows the use of the JavaScript debugger to trace through code execution. "
//        "Note that this has a small performance impact and is mainly only useful for developers.") );
//  connect( enableJavaScriptDebugCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );

  // the domain-specific listview (copied and modified from Cookies configuration)
  QGroupBox* domainSpecificGB = new QGroupBox( i18n( "Do&main-Specific" ), this );
  domainSpecificGB->setColumnLayout(0, Qt::Vertical );
  domainSpecificGB->layout()->setSpacing( 0 );
  domainSpecificGB->layout()->setMargin( 0 );
  QGridLayout* domainSpecificGBLayout = new QGridLayout( domainSpecificGB->layout() );
  domainSpecificGBLayout->setAlignment( Qt::AlignTop );
  domainSpecificGBLayout->setSpacing( 6 );
  domainSpecificGBLayout->setMargin( 11 );

  domainSpecificLV = new KListView( domainSpecificGB );
  domainSpecificLV->addColumn(i18n("Host/Domain"));
  domainSpecificLV->addColumn(i18n("Policy"), 100);
  QString wtstr = i18n("This box contains the domains and hosts you have set "
                       "a specific JavaScript policy for. This policy will be used "
                       "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
                       "domains or hosts. <p>Select a policy and use the controls on "
                       "the right to modify it.");
  QWhatsThis::add( domainSpecificLV, wtstr );
  QWhatsThis::add( domainSpecificGB, wtstr );
  connect(domainSpecificLV,SIGNAL(doubleClicked ( QListViewItem * )), this, SLOT( changePressed() ) );
  connect(domainSpecificLV,SIGNAL(returnPressed ( QListViewItem * )), this, SLOT( changePressed() ) );


  domainSpecificGBLayout->addMultiCellWidget( domainSpecificLV, 0, 5, 0, 0 );
  QPushButton* addDomainPB = new QPushButton( i18n("&New..."), domainSpecificGB );
  domainSpecificGBLayout->addWidget( addDomainPB, 0, 1 );
  QWhatsThis::add( addDomainPB, i18n("Click on this button to manually add a host or domain "
                                     "specific policy.") );
  connect( addDomainPB, SIGNAL(clicked()), SLOT( addPressed() ) );

  QPushButton* changeDomainPB = new QPushButton( i18n("Chan&ge..."), domainSpecificGB );
  domainSpecificGBLayout->addWidget( changeDomainPB, 1, 1 );
  QWhatsThis::add( changeDomainPB, i18n("Click on this button to change the policy for the "
                                        "host or domain selected in the list box.") );
  connect( changeDomainPB, SIGNAL( clicked() ), this, SLOT( changePressed() ) );

  QPushButton* deleteDomainPB = new QPushButton( i18n("De&lete"), domainSpecificGB );
  domainSpecificGBLayout->addWidget( deleteDomainPB, 2, 1 );
  QWhatsThis::add( deleteDomainPB, i18n("Click on this button to change the policy for the "
                                        "host or domain selected in the list box.") );
  connect( deleteDomainPB, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

  QPushButton* importDomainPB = new QPushButton( i18n("&Import..."), domainSpecificGB );
  domainSpecificGBLayout->addWidget( importDomainPB, 3, 1 );
  QWhatsThis::add( importDomainPB, i18n("Click this button to choose the file that contains "
                                        "the JavaScript policies. These policies will be merged "
                                        "with the existing ones. Duplicate entries are ignored.") );
  connect( importDomainPB, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
  importDomainPB->setEnabled( false );
  importDomainPB->hide();

  QPushButton* exportDomainPB = new QPushButton( i18n("&Export..."), domainSpecificGB );
  domainSpecificGBLayout->addWidget( exportDomainPB, 4, 1 );
  QWhatsThis::add( exportDomainPB, i18n("Click this button to save the JavaScript policy to a zipped "
                                        "file. The file, named <b>javascript_policy.tgz</b>, will be "
                                        "saved to a location of your choice." ) );

  connect( exportDomainPB, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
  exportDomainPB->setEnabled( false );
  exportDomainPB->hide();

  QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum, QSizePolicy::Expanding );
  domainSpecificGBLayout->addItem( spacer, 5, 1 );
  toplevel->addWidget( domainSpecificGB, 2 );

  QWhatsThis::add( domainSpecificGB, i18n("Here you can set specific JavaScript policies for any particular "
                                          "host or domain. To add a new policy, simply click the <i>Add...</i> "
                                          "button and supply the necessary information requested by the "
                                          "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                          "button and choose the new policy from the policy dialog box. Clicking "
                                          "on the <i>Delete</i> button will remove the selected policy causing the default "
                                          "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                          "button allows you to easily share your policies with other people by allowing "
                                          "you to save and retrive them from a zipped file.") );

  js_policies_frame = new JSPoliciesFrame(&js_global_policies,
  		i18n("Global JavaScript Policies"),this);
  toplevel->addWidget(js_policies_frame);
  connect(js_policies_frame, SIGNAL(changed()), SLOT(slotChanged()));

/*
  kdDebug() << "\"Show debugger window\" says: make me useful!" << endl;
  enableDebugOutputCB = new QCheckBox( i18n( "&Show debugger window" ), miscSettingsGB);
  enableDebugOutputCB->hide();

  QWhatsThis::add( enableDebugOutputCB, i18n("Show a window with informations and warnings issued by the JavaScript interpreter. "
                                             "This is extremely useful for both debugging your own html pages and tracing down "
                                             "problems with Konquerors JavaScript support.") );
  connect( enableDebugOutputCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );
*/

  // Finally do the loading
  load();
}


void KJavaScriptOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);

    if( m_pConfig->hasKey( "ECMADomains" ) )
	updateDomainList(m_pConfig->readListEntry("ECMADomains"));
    else if( m_pConfig->hasKey( "ECMADomainSettings" ) ) {
        updateDomainListLegacy( m_pConfig->readListEntry( "ECMADomainSettings" ) );
	_removeECMADomainSettings = true;
    } else {
        updateDomainListLegacy(m_pConfig->readListEntry("JavaScriptDomainAdvice") );
	_removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    js_policies_frame->load();
    enableJavaScriptGloballyCB->setChecked(
    		js_global_policies.isFeatureEnabled());
//    enableJavaScriptDebugCB->setChecked( m_pConfig->readBoolEntry("EnableJavaScriptDebug",false));
//    js_popup->setButton( m_pConfig->readUnsignedNumEntry("WindowOpenPolicy", 0) );

  // enableDebugOutputCB->setChecked( m_pConfig->readBoolEntry("EnableJSDebugOutput") );
}

void KJavaScriptOptions::defaults()
{
  js_policies_frame->defaults();
  enableJavaScriptGloballyCB->setChecked(
    		js_global_policies.isFeatureEnabled());
//  enableJavaScriptDebugCB->setChecked( false );
 // enableDebugOutputCB->setChecked( false );
}

void KJavaScriptOptions::save()
{
    m_pConfig->setGroup(m_groupname);
//    m_pConfig->writeEntry( "EnableJavaScriptDebug", enableJavaScriptDebugCB->isChecked() );

//    m_pConfig->writeEntry( "EnableJSDebugOutput", enableDebugOutputCB->isChecked() );

    QStringList domainList;
    DomainPolicyMap::Iterator it = javaScriptDomainPolicy.begin();
    for (; it != javaScriptDomainPolicy.end(); ++it) {
    	QListViewItem *current = it.key();
	JSPolicies &pol = it.data();
	pol.save();
	domainList.append(current->text(0));
    }
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry("ECMADomains", domainList);

    js_policies_frame->save();

    if (_removeECMADomainSettings) {
      m_pConfig->deleteEntry("ECMADomainSettings");
      _removeECMADomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
}

void KJavaScriptOptions::slotChanged()
{
  emit changed(true);
}

void KJavaScriptOptions::addPressed()
{
    JSPolicies pol_copy(m_pConfig,m_groupname,false);
    pol_copy.defaults();
    PolicyDialog pDlg(&pol_copy, this);
    pDlg.setCaption( i18n( "New JavaScript Policy" ) );
    setupPolicyDlg(pDlg,pol_copy);
    if( pDlg.exec() ) {
        QListViewItem* index = new QListViewItem( domainSpecificLV, pDlg.domain(),
                                                  pDlg.featureEnabledPolicyText() );
	pol_copy.setDomain(pDlg.domain());
        javaScriptDomainPolicy.insert(index, pol_copy);
        domainSpecificLV->setCurrentItem( index );
        slotChanged();
    }
}

void KJavaScriptOptions::changePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to be changed!" ) );
        return;
    }

    JSPolicies pol_copy = javaScriptDomainPolicy[index];

    PolicyDialog pDlg( &pol_copy, this );
    pDlg.setDisableEdit( true, index->text(0) );
    pDlg.setCaption( i18n( "Change JavaScript Policy" ) );
    setupPolicyDlg(pDlg,pol_copy);
    if( pDlg.exec() )
    {
        pol_copy.setDomain(pDlg.domain());
        javaScriptDomainPolicy[index] = pol_copy;
        index->setText(0, pDlg.domain() );
        index->setText(1, pDlg.featureEnabledPolicyText());
        slotChanged();
    }
}

void KJavaScriptOptions::deletePressed()
{
    QListViewItem *index = domainSpecificLV->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to delete!" ) );
        return;
    }
    javaScriptDomainPolicy.remove(index);
    delete index;
    slotChanged();
}

void KJavaScriptOptions::importPressed()
{
  // PENDING(kalle) Implement this.
}

void KJavaScriptOptions::exportPressed()
{
  // PENDING(kalle) Implement this.
}


void KJavaScriptOptions::changeJavaScriptEnabled()
{
  bool enabled = enableJavaScriptGloballyCB->isChecked();
  js_global_policies.setFeatureEnabled(enabled);
  enableJavaScriptGloballyCB->setChecked( enabled );
}

void KJavaScriptOptions::updateDomainList(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JSPolicies pol(m_pConfig,m_groupname,false);
    for (QStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it) {
      QString domain = *it;
      pol.setDomain(domain);
      pol.load();

      QString policy;
      if (pol.isFeatureEnabledPolicyInherited())
        policy = i18n("Use global");
      else if (pol.isFeatureEnabled())
        policy = i18n("Accept");
      else
        policy = i18n("Reject");
      QListViewItem *index =
        new QListViewItem( domainSpecificLV, domain, policy );

      javaScriptDomainPolicy[index] = pol;

    }
}

void KJavaScriptOptions::updateDomainListLegacy(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JSPolicies pol(m_pConfig,m_groupname,false);
    pol.defaults();
    for (QStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it) {
      QString domain;
      KHTMLSettings::KJavaScriptAdvice javaAdvice;
      KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
      KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
      QListViewItem *index =
        new QListViewItem( domainSpecificLV, domain,
                i18n(KHTMLSettings::adviceToStr(javaScriptAdvice)) );

      pol.setDomain(domain);
      pol.setFeatureEnabled(javaScriptAdvice != KHTMLSettings::KJavaScriptReject);
      javaScriptDomainPolicy[index] = pol;
    }
}

void KJavaScriptOptions::setupPolicyDlg(PolicyDialog &pDlg,JSPolicies &pol) {
  pDlg.setFeatureEnabledLabel(i18n("JavaScript policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a JavaScript policy for "
                                          "the above host or domain."));
  JSPoliciesFrame *panel = new JSPoliciesFrame(&pol,i18n("Domain-specific "
  				"JavaScript policies"),&pDlg);
  panel->refresh();
  pDlg.addPolicyPanel(panel);
  pDlg.refresh();
}
