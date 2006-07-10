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
// Big changes to accommodate per-domain settings
// (c) Leo Savernik 2002-2003

#include <QLayout>
#include <q3groupbox.h>
#include <kconfig.h>
#include <k3listview.h>
#include <kdebug.h>
#include <kurlrequester.h>

#include "htmlopts.h"
#include "policydlg.h"

#include <klocale.h>

#include "jsopts.h"

#include "jsopts.moc"

#if defined Q_WS_X11 && !defined K_WS_QTONLY
#include <X11/Xlib.h>
#endif

// == class KJavaScriptOptions =====

KJavaScriptOptions::KJavaScriptOptions( KSharedConfig::Ptr config, QString group, KInstance *inst, QWidget *parent) :
  KCModule( inst, parent ),
  _removeJavaScriptDomainAdvice(false),
   m_pConfig( config ), m_groupname( group ),
  js_global_policies(config,group,true,QString()),
  _removeECMADomainSettings(false)
{
  QVBoxLayout* toplevel = new QVBoxLayout( this );
  toplevel->setMargin( 10 );
  toplevel->setSpacing( 5 );

  // the global checkbox
  Q3GroupBox* globalGB = new Q3GroupBox( 2, Qt::Vertical, i18n( "Global Settings" ), this );
  toplevel->addWidget( globalGB );

  enableJavaScriptGloballyCB = new QCheckBox( i18n( "Ena&ble JavaScript globally" ), globalGB );
  enableJavaScriptGloballyCB->setWhatsThis( i18n("Enables the execution of scripts written in ECMA-Script "
        "(also known as JavaScript) that can be contained in HTML pages. "
        "Note that, as with any browser, enabling scripting languages can be a security problem.") );
  connect( enableJavaScriptGloballyCB, SIGNAL( clicked() ), SLOT( changed() ) );
  connect( enableJavaScriptGloballyCB, SIGNAL( clicked() ), this, SLOT( slotChangeJSEnabled() ) );

  reportErrorsCB = new QCheckBox( i18n( "Report &errors" ), globalGB );
  reportErrorsCB->setWhatsThis( i18n("Enables the reporting of errors that occur when JavaScript "
	"code is executed.") );
  connect( reportErrorsCB, SIGNAL( clicked() ), SLOT( changed() ) );

  jsDebugWindow = new QCheckBox( i18n( "Enable debu&gger" ), globalGB );
  jsDebugWindow->setWhatsThis( i18n( "Enables builtin JavaScript debugger." ) );
  connect( jsDebugWindow, SIGNAL( clicked() ), SLOT( changed() ) );

  // the domain-specific listview
  domainSpecific = new JSDomainListView(m_pConfig,m_groupname,this,this);
  connect(domainSpecific,SIGNAL(changed(bool)),SLOT(changed()));
  toplevel->addWidget( domainSpecific, 2 );

  domainSpecific->setWhatsThis( i18n("Here you can set specific JavaScript policies for any particular "
                                     "host or domain. To add a new policy, simply click the <i>New...</i> "
                                     "button and supply the necessary information requested by the "
                                     "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                     "button and choose the new policy from the policy dialog box. Clicking "
                                     "on the <i>Delete</i> button will remove the selected policy causing the default "
                                     "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                     "button allows you to easily share your policies with other people by allowing "
                                     "you to save and retrieve them from a zipped file.") );

  QString wtstr = i18n("This box contains the domains and hosts you have set "
                       "a specific JavaScript policy for. This policy will be used "
                       "instead of the default policy for enabling or disabling JavaScript on pages sent by these "
                       "domains or hosts. <p>Select a policy and use the controls on "
                       "the right to modify it.");
  domainSpecific->listView()->setWhatsThis( wtstr );

  domainSpecific->importButton()->setWhatsThis( i18n("Click this button to choose the file that contains "
                                                     "the JavaScript policies. These policies will be merged "
                                                     "with the existing ones. Duplicate entries are ignored.") );
  domainSpecific->exportButton()->setWhatsThis( i18n("Click this button to save the JavaScript policy to a zipped "
                                                     "file. The file, named <b>javascript_policy.tgz</b>, will be "
                                                     "saved to a location of your choice." ) );

  // the frame containing the JavaScript policies settings
  js_policies_frame = new JSPoliciesFrame(&js_global_policies,
  		i18n("Global JavaScript Policies"),this);
  toplevel->addWidget(js_policies_frame);
  connect(js_policies_frame, SIGNAL(changed()), SLOT(changed()));

  // Finally do the loading
  load();
}


void KJavaScriptOptions::load()
{
    // *** load ***
    m_pConfig->setGroup(m_groupname);

    if( m_pConfig->hasKey( "ECMADomains" ) )
	domainSpecific->initialize(m_pConfig->readEntry("ECMADomains", QStringList() ));
    else if( m_pConfig->hasKey( "ECMADomainSettings" ) ) {
        domainSpecific->updateDomainListLegacy( m_pConfig->readEntry( "ECMADomainSettings" , QStringList() ) );
	_removeECMADomainSettings = true;
    } else {
        domainSpecific->updateDomainListLegacy(m_pConfig->readEntry("JavaScriptDomainAdvice", QStringList() ) );
	_removeJavaScriptDomainAdvice = true;
    }

    // *** apply to GUI ***
    js_policies_frame->load();
    enableJavaScriptGloballyCB->setChecked(
    		js_global_policies.isFeatureEnabled());
    reportErrorsCB->setChecked( m_pConfig->readEntry("ReportJavaScriptErrors", QVariant(false)).toBool());
    jsDebugWindow->setChecked( m_pConfig->readEntry( "EnableJavaScriptDebug", QVariant(false )).toBool() );
//    js_popup->setButton( m_pConfig->readUnsignedNumEntry("WindowOpenPolicy", 0) );
    emit changed(false);
}

void KJavaScriptOptions::defaults()
{
  js_policies_frame->defaults();
  enableJavaScriptGloballyCB->setChecked(
    		js_global_policies.isFeatureEnabled());
  reportErrorsCB->setChecked( false );
  jsDebugWindow->setChecked( false );
  emit changed(true);
}

void KJavaScriptOptions::save()
{
    m_pConfig->setGroup(m_groupname);
    m_pConfig->writeEntry( "ReportJavaScriptErrors", reportErrorsCB->isChecked() );
    m_pConfig->writeEntry( "EnableJavaScriptDebug", jsDebugWindow->isChecked() );

    domainSpecific->save(m_groupname,"ECMADomains");
    js_policies_frame->save();

    if (_removeECMADomainSettings) {
      m_pConfig->deleteEntry("ECMADomainSettings");
      _removeECMADomainSettings = false;
    }

    // sync moved to KJSParts::save
//    m_pConfig->sync();
    emit changed(false);
}

void KJavaScriptOptions::slotChangeJSEnabled() {
  js_global_policies.setFeatureEnabled(enableJavaScriptGloballyCB->isChecked());
}

// == class JSDomainListView =====

JSDomainListView::JSDomainListView(KSharedConfig::Ptr config,const QString &group,
	KJavaScriptOptions *options, QWidget *parent)
	: DomainListView(config,i18n( "Do&main-Specific" ), parent),
	group(group), options(options) {
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
        Q3ListViewItem *index =
          new Q3ListViewItem( domainSpecificLV, domain,
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
    case AddButton:
      caption = i18n( "New JavaScript Policy" );
      jspol->setFeatureEnabled(!options->enableJavaScriptGloballyCB->isChecked());
      break;
    case ChangeButton: caption = i18n( "Change JavaScript Policy" ); break;
    default: ; // inhibit gcc warning
  }/*end switch*/
  pDlg.setCaption(caption);
  pDlg.setFeatureEnabledLabel(i18n("JavaScript policy:"));
  pDlg.setFeatureEnabledWhatsThis(i18n("Select a JavaScript policy for "
                                          "the above host or domain."));
  JSPoliciesFrame *panel = new JSPoliciesFrame(jspol,i18n("Domain-Specific "
  				"JavaScript Policies"),pDlg.mainWidget());
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


