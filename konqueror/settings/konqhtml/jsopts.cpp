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

// == class KJavaScriptOptions =====

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
  connect( enableJavaScriptGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChangeJSEnabled() ) );

//  enableJavaScriptDebugCB = new QCheckBox( i18n( "Enable debu&gging" ), globalGB );
//  QWhatsThis::add( enableJavaScriptDebugCB, i18n("Enables the reporting of errors that occur when JavaScript "
//        "code is executed, and allows the use of the JavaScript debugger to trace through code execution. "
//        "Note that this has a small performance impact and is mainly only useful for developers.") );
//  connect( enableJavaScriptDebugCB, SIGNAL( clicked() ), this, SLOT( slotChanged() ) );

  // the domain-specific listview
  domainSpecific = new JSDomainListView(m_pConfig,m_groupname,this);
  connect(domainSpecific,SIGNAL(changed(bool)),SLOT(slotChanged()));
  toplevel->addWidget( domainSpecific, 2 );

  QWhatsThis::add( domainSpecific, i18n("Here you can set specific JavaScript policies for any particular "
                                          "host or domain. To add a new policy, simply click the <i>New...</i> "
                                          "button and supply the necessary information requested by the "
                                          "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                          "button and choose the new policy from the policy dialog box. Clicking "
                                          "on the <i>Delete</i> button will remove the selected policy causing the default "
                                          "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                          "button allows you to easily share your policies with other people by allowing "
                                          "you to save and retrive them from a zipped file.") );

  QString wtstr = i18n("This box contains the domains and hosts you have set "
                       "a specific JavaScript policy for. This policy will be used "
                       "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
                       "domains or hosts. <p>Select a policy and use the controls on "
                       "the right to modify it.");
  QWhatsThis::add( domainSpecific->listView(), wtstr );

  QWhatsThis::add( domainSpecific->importButton(), i18n("Click this button to choose the file that contains "
                                        "the JavaScript policies. These policies will be merged "
                                        "with the existing ones. Duplicate entries are ignored.") );
  QWhatsThis::add( domainSpecific->exportButton(), i18n("Click this button to save the JavaScript policy to a zipped "
                                        "file. The file, named <b>javascript_policy.tgz</b>, will be "
                                        "saved to a location of your choice." ) );

  // the frame containing the JavaScript policies settings
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
	domainSpecific->initialize(m_pConfig->readListEntry("ECMADomains"));
    else if( m_pConfig->hasKey( "ECMADomainSettings" ) ) {
        domainSpecific->updateDomainListLegacy( m_pConfig->readListEntry( "ECMADomainSettings" ) );
	_removeECMADomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy(m_pConfig->readListEntry("JavaScriptDomainAdvice") );
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

    domainSpecific->save(m_groupname,"ECMADomains");
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

void KJavaScriptOptions::slotChangeJSEnabled() {
  js_global_policies.setFeatureEnabled(enableJavaScriptGloballyCB->isChecked());
}

// == class JSDomainListView =====

JSDomainListView::JSDomainListView(KConfig *config,const QString &group,
	QWidget *parent,const char *name)
	: DomainListView(config,i18n( "Do&main-Specific" ), parent, name),
	group(group) {
}

JSDomainListView::~JSDomainListView() {
}

void JSDomainListView::updateDomainListLegacy(const QStringList &domainConfig)
{
    domainSpecificLV->clear();
    JSPolicies pol(config,group,false);
    pol.defaults();
    for (QStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it) {
      QString domain;
      KHTMLSettings::KJavaScriptAdvice javaAdvice;
      KHTMLSettings::KJavaScriptAdvice javaScriptAdvice;
      KHTMLSettings::splitDomainAdvice(*it, domain, javaAdvice, javaScriptAdvice);
      if (javaScriptAdvice != KHTMLSettings::KJavaScriptDunno) {
        QListViewItem *index =
          new QListViewItem( domainSpecificLV, domain,
                i18n(KHTMLSettings::adviceToStr(javaScriptAdvice)) );

        pol.setDomain(domain);
        pol.setFeatureEnabled(javaScriptAdvice != KHTMLSettings::KJavaScriptReject);
        domainPolicies[index] = new JSPolicies(pol);
      }
    }
}

void JSDomainListView::setupPolicyDlg(PushButton trigger,PolicyDialog &pDlg,
		Policies *pol) {
  JSPolicies *jspol = static_cast<JSPolicies *>(pol);
  QString caption;
  switch (trigger) {
    case AddButton: caption = i18n( "New JavaScript Policy" ); break;
    case ChangeButton: caption = i18n( "Change JavaScript Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("JavaScript policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a JavaScript policy for "
                                          "the above host or domain."));
  JSPoliciesFrame *panel = new JSPoliciesFrame(jspol,i18n("Domain-specific "
  				"JavaScript policies"),&pDlg);
  panel->refresh();
  pDlg.addPolicyPanel(panel);
  pDlg.refresh();
}

JSPolicies *JSDomainListView::createPolicies() {
  return new JSPolicies(config,group,false);
}

JSPolicies *JSDomainListView::copyPolicies(Policies *pol) {
  return new JSPolicies(*static_cast<JSPolicies *>(pol));
}


