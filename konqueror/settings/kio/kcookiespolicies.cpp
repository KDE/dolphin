// kcookiespolicies.cpp - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#include <qlayout.h>
#include <qwhatsthis.h>

#include <kkeydialog.h> // for ksplitlist
#include <kbuttonbox.h>
#include <kmessagebox.h>

#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>
#include <kprocess.h>
#include <kdebug.h>
#include <ksimpleconfig.h>
#include <dcopclient.h>

#include "kcookiespolicies.h"
#include "policydlg.h"

/*
* The functions below are utility functions used to
* properly split and change a cookie policy information
* from a string format to one appropriate  
*
*/

enum KCookieAdvice {
    KCookieDunno=0,
    KCookieAccept,
    KCookieReject,
    KCookieAsk
};

static const char *adviceToStr(KCookieAdvice _advice)
{
    switch( _advice )
    {
        case KCookieAccept: return "Accept";
        case KCookieReject: return "Reject";
        case KCookieAsk: return "Ask";
        default: return "Dunno";
    }
}

static KCookieAdvice strToAdvice(const QString& _str)
{
    if (!_str)
        return KCookieDunno;

    if (strcasecmp(_str, "Accept") == 0)
        return KCookieAccept;
    else if (strcasecmp(_str, "Reject") == 0)
        return KCookieReject;
    else if (strcasecmp(_str, "Ask") == 0)
        return KCookieAsk;

    return KCookieDunno;	
}

static void splitDomainAdvice(const char *configStr, QString &domain, KCookieAdvice &advice)
{
    QString tmp(configStr);
    int splitIndex = tmp.find(':');
    if ( splitIndex == -1)
    {
        domain = configStr;
        advice = KCookieDunno;
    }
    else
    {
        domain = tmp.left(splitIndex);
        advice = strToAdvice( tmp.mid( splitIndex+1, tmp.length()).ascii() );
    }
}

/************************************ KCookiesPolicies **********************************/

KCookiesPolicies::KCookiesPolicies(QWidget *parent, const char *name)
                 :KCModule(parent, name)
{
    // This is the layout for the "Policy" tab.
    QGridLayout *lay = new QGridLayout( this, 5, 3, 10, 5);
    lay->addRowSpacing(0,5);
    lay->addRowSpacing(2,5);
    lay->addRowSpacing(4,5);
        
    lay->setRowStretch(0,0);
    lay->setRowStretch(1,0);
    lay->setRowStretch(2,0);  
    lay->setRowStretch(3,1);
    lay->setRowStretch(4,0);
            
    lay->addColSpacing(0,10);
    lay->addColSpacing(2,10);
    
    lay->setColStretch(0,0);
    lay->setColStretch(1,1);
    lay->setColStretch(2,0);
    
    // Global Policy group box
    gb_global = new QGroupBox( i18n( "Global"), this );
    QGridLayout *gp_lay = new QGridLayout( gb_global, 5, 3, 10, 5 );
    gp_lay->addRowSpacing(0,5);
    gp_lay->addRowSpacing(2,3);
    gp_lay->addRowSpacing(4,5);
    
    gp_lay->addColSpacing(0,5);
    gp_lay->addColSpacing(2,5);
    
    gp_lay->setColStretch(0,0);
    gp_lay->setColStretch(1,1);
    gp_lay->setColStretch(2,0);    
    lay->addWidget( gb_global, 1, 1 );  // Add it to the policy tab
    
    cb_enableCookies = new QCheckBox( i18n("&Enable Cookies"), gb_global );
    QWhatsThis::add( cb_enableCookies, i18n("This option turns on cookie support. Normally "
                                            "you will want to have cookie support enabled and "
                                            "customize it to suit your need of privacy.") );
    connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changeCookiesEnabled() ) );
    connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changed() ) );
    gp_lay->addWidget( cb_enableCookies, 1, 1 ); // Add it to the global group box
  
    bg_default = new QButtonGroup( i18n("Default Policy"), gb_global );
    QWhatsThis::add( bg_default, i18n("The default policy determines the way KDE will "
                                      "handle cookies sent from a server that does not "
                                      "belong to a domain for which you have set a specific "
                                      "policy. Note that this default policy is overriden for "
                                      " any server which has a domain specific policy. <ul><li>"
                                      "<em>Accept</em> will cause KDE to silently accept all cookies "
                                      "</li><li><em>Ask</em> will cause KDE to ask you for your confirmation "
                                      "everytime a server wants to set a cookie</li><li><em>Reject</em> will "
                                      "cause KDE not to set cookies</li></ul>") );    
    connect(bg_default, SIGNAL(clicked(int)), this, SLOT(changed()));
    bg_default->setExclusive( true );    
    
    QGridLayout *df_lay = new QGridLayout( bg_default, 7, 1, 10, 5 );
    df_lay->addRowSpacing(0,5);
    df_lay->addRowSpacing(2,1);
    df_lay->addRowSpacing(4,1);
    df_lay->addRowSpacing(6,5);
    
    df_lay->setRowStretch(0,0);
    df_lay->setRowStretch(1,1);
    df_lay->setRowStretch(2,0);

    rb_gbPolicyAsk = new QRadioButton( i18n("Ask for confirmation before accepting cookies."), bg_default );
    df_lay->addWidget( rb_gbPolicyAsk, 1, 0);

    rb_gbPolicyAccept = new QRadioButton( i18n("Accept all cookies by default"), bg_default );
    df_lay->addWidget(rb_gbPolicyAccept, 3, 0);

    rb_gbPolicyReject = new QRadioButton( i18n("Reject all cookies by default"), bg_default );
    df_lay->addWidget(rb_gbPolicyReject, 5, 0);
    
    gp_lay->addWidget( bg_default, 3, 1 ); // Add it to the Global group box
    
    // Create Group Box for specific settings
    gb_domainSpecific = new QGroupBox( i18n("Domain Specific"), this);
    QGridLayout *ds_lay = new QGridLayout( gb_domainSpecific, 11, 5, 10, 5 );
    ds_lay->addRowSpacing(0,10);
    ds_lay->addRowSpacing(2,1);
    ds_lay->addRowSpacing(4,1);
    ds_lay->addRowSpacing(6,1);
    ds_lay->addRowSpacing(8,1);
    ds_lay->addRowSpacing(10,7);
    
    ds_lay->addColSpacing(0,5);
    ds_lay->addColSpacing(2,3);
    ds_lay->addColSpacing(4,5);
  
    // CREATE SPLIT LIST BOX
    lb_domainPolicy = new KSplitList( gb_domainSpecific );
    lb_domainPolicy->setMinimumHeight( 0.10 * sizeHint().height() );
    ds_lay->addMultiCellWidget( lb_domainPolicy, 1, 9, 1, 1 );
    QString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific cookie policy for. This policy will be used "
                         "instead of the default policy for any cookie sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    QWhatsThis::add( lb_domainPolicy, wtstr );
    QWhatsThis::add( gb_domainSpecific, wtstr );  
    
    pb_domPolicyAdd = new QPushButton( i18n("Add..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyAdd, i18n("Click on this button to manually add a domain "
                                           "specific policy.") );
    connect( pb_domPolicyAdd, SIGNAL(clicked()), SLOT( addPressed() ) );
    ds_lay->addWidget( pb_domPolicyAdd, 1, 3);
  
    pb_domPolicyChange = new QPushButton( i18n("Change..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyChange, i18n("Click on this button to change the policy for the "
                                              "domain selected in the list box.") );        
    connect( pb_domPolicyChange, SIGNAL( clicked() ), this, SLOT( changePressed() ) );
    ds_lay->addWidget( pb_domPolicyChange, 3, 3);
    
    pb_domPolicyDelete = new QPushButton( i18n("Delete"), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyDelete, i18n("Click on this button to change the policy for the "
                                              "domain selected in the list box.") );   
    connect( pb_domPolicyDelete, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );
    ds_lay->addWidget( pb_domPolicyDelete, 5, 3);
    
    pb_domPolicyImport = new QPushButton( i18n("Import..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyImport, i18n("Click this button to choose the file that contains "
                                              "the cookie policies.  These policies will be merged "
                                              "with the exisiting ones.  Duplicate enteries are ignored.") );   
    connect( pb_domPolicyDelete, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
    ds_lay->addWidget( pb_domPolicyImport, 7, 3);
    pb_domPolicyImport->setEnabled( false );

    pb_domPolicyExport = new QPushButton( i18n("Export..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyExport, i18n("Click this button to save the cookie policy to a zipped "
                                              "file.  The file, named <b>cookie_policy.tgz</b>, will be "
                                              "saved to a location of your choice." ) );   
    
    connect( pb_domPolicyDelete, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
    ds_lay->addWidget( pb_domPolicyExport, 9, 3);   
    pb_domPolicyExport->setEnabled( false );

    QWhatsThis::add( gb_domainSpecific, i18n("Here you can set specific cookie policies for any particular "
                                             "domain. To add a new policy, simply click the <i>Add...</i> "
                                             "button and supply the necessary information requested by the "
                                             "dialog box. To change an exisiting policy, click on the <i>Change...</i> "
                                             "button and choose the new policy from the policy dialog box.  Clicking "
                                             "on the <i>Delete</i> will remove the selected policy causing the default "
                                             "policy setting to be used for that domain. The <i>Import</i> and <i>Export</i> "
                                             "button allows you to easily share your policies with other people by allowing "
                                             "you to save and retrive them from a zipped file.") );
    lay->addWidget( gb_domainSpecific, 3, 1 );                                             
  
    // Pheweee!! Finally read the options
    load();
}

KCookiesPolicies::~KCookiesPolicies()
{
}

void KCookiesPolicies::addPressed()
{
    PolicyDialog pDlg( this, 0L );
    // We subtract one from the enum value because
    // KCookieDunno is not part of the choice list.
    int def_policy = KCookieAsk - 1;
    pDlg.setDefaultPolicy( def_policy );
    pDlg.setCaption( i18n( "New Cookie Policy" ) );
    if( pDlg.exec() )
    {
        QString domain = pDlg.domain();
        const char* new_advice = adviceToStr( (KCookieAdvice) pDlg.policyAdvice() );
        domain.append( ":" ).append( new_advice );
        domainConfig.append( domain );
        updateDomainList();        
        changed();        
    }
}

void KCookiesPolicies::changePressed()
{
    int index = lb_domainPolicy->currentItem();
    if ( index == -1 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to be changed!" ) );
        return;
    }
       
    QString configStr = *domainConfig.at( index );
    
    // Should never happen!!
    if (configStr.isNull())
        return;
      
    QString domain;
    KCookieAdvice advice;
    splitDomainAdvice( configStr, domain, advice );

    PolicyDialog pDlg( this );
    pDlg.setDisableEdit( false, domain );
    pDlg.setCaption( i18n( "Change Cookie Policy" ) );
    // We subtract one from the enum value because
    // KCookieDunno is not part of the choice list.    
    pDlg.setDefaultPolicy( advice - 1 );        
    if( pDlg.exec() )
    {
        const char* new_advice = adviceToStr( (KCookieAdvice) pDlg.policyAdvice() );
        domain.append( ":" ).append( new_advice );
        domainConfig.insert( domainConfig.remove( domainConfig.at( index ) ), domain );
        updateDomainList();
        changed();
    }
}

void KCookiesPolicies::deletePressed()
{
    int index = lb_domainPolicy->currentItem();
    if ( index == -1 )
    {
        KMessageBox::information( 0, i18n("You must first select a policy to delete!" ) );
        return;
    }    
    domainConfig.remove( domainConfig.at( index ) );
    updateDomainList();
    changed();
    lb_domainPolicy->setCurrentItem( index );
}

void KCookiesPolicies::importPressed()
{
}

void KCookiesPolicies::exportPressed()
{
}

void KCookiesPolicies::changeCookiesEnabled()
{
    bool enabled = cb_enableCookies->isChecked();
    bg_default->setEnabled( enabled );  
    gb_domainSpecific->setEnabled( enabled );  
}

void KCookiesPolicies::updateDomainList()
{
    lb_domainPolicy->clear();
    int id = 0;
    for (QStringList::ConstIterator domain = domainConfig.begin();
         domain != domainConfig.end(); ++domain)
    {
        KSplitListItem *sli = new KSplitListItem( *domain, id);
        connect( lb_domainPolicy, SIGNAL( newWidth( int ) ), sli, SLOT( setWidth( int ) ) );
        sli->setWidth( lb_domainPolicy->width() );
        lb_domainPolicy->insertItem( sli );
        ++id;
    }
}

void KCookiesPolicies::load()
{
  KSimpleConfig *g_pConfig = new KSimpleConfig( "kcookiejarrc" );

  // Backwards compatiable reading of domain specific settings
  if( g_pConfig->hasGroup( "Browser Settings/HTTP" ) &&
      !g_pConfig->hasGroup( "Cookie Policy" ) )
     g_pConfig->setGroup( "Browser Settings/HTTP" );
  else
     g_pConfig->setGroup( "Cookie Policy" );

  QString tmp = g_pConfig->readEntry("CookieGlobalAdvice", "Ask");
  KCookieAdvice globalAdvice = strToAdvice(tmp);
  cb_enableCookies->setChecked( g_pConfig->readBoolEntry( "Cookies", true ));

  rb_gbPolicyAccept->setChecked( globalAdvice == KCookieAccept );
  rb_gbPolicyReject->setChecked( globalAdvice == KCookieReject );
  rb_gbPolicyAsk->setChecked( (globalAdvice != KCookieAccept) &&
                              (globalAdvice != KCookieReject) );

  domainConfig = g_pConfig->readListEntry("CookieDomainAdvice");

  updateDomainList();
  changeCookiesEnabled();

  // Remove the old group - R.I.P
  if( g_pConfig->hasGroup( "Browser Settings/HTTP" ) )
     g_pConfig->deleteGroup( "Browser Settings/HTTP" );

  delete g_pConfig;
}

void KCookiesPolicies::save()
{
  KSimpleConfig *g_pConfig = new KSimpleConfig( "kcookiejarrc" );
  const char *advice;

  g_pConfig->setGroup( "Cookie Policy" );
  g_pConfig->writeEntry( "Cookies", cb_enableCookies->isChecked() );

  if (rb_gbPolicyAccept->isChecked())
      advice = adviceToStr(KCookieAccept);
  else if (rb_gbPolicyReject->isChecked())
      advice = adviceToStr(KCookieReject);
  else
      advice = adviceToStr(KCookieAsk);
  g_pConfig->writeEntry("CookieGlobalAdvice", advice);
  g_pConfig->writeEntry("CookieDomainAdvice", domainConfig);

  g_pConfig->sync();

  delete g_pConfig;

  // Create a dcop client object to communicate with the
  // cookiejar if it is alive, if not who cares.  It will
  // read the config info from file the next time...
  DCOPClient *m_dcopClient = new DCOPClient();
  if( m_dcopClient->attach() )
  {
     if( !m_dcopClient->send( "kcookiejar", "kcookiejar", "reloadPolicy", QString::null ) )
		kdDebug(7104) << "Can't communicate with the cookiejar!" << endl;
  }
  else
     kdDebug(7103) << "Can't connect with the DCOP server." << endl;

  delete m_dcopClient;
}


void KCookiesPolicies::defaults()
{
  cb_enableCookies->setChecked( true );
  rb_gbPolicyAsk->setChecked( true );
  rb_gbPolicyAccept->setChecked( false );
  rb_gbPolicyReject->setChecked( false );
  domainConfig.clear();
  updateDomainList();
  changeCookiesEnabled();
}

QString KCookiesPolicies::quickHelp()
{
    return i18n("<h1>Cookies</h1> Cookies contain information that Konqueror "
                " (or other KDE applications using the http protocol) stores on your "
                " computer from a remote internet server. This means, that a web server "
                " can store information about you and your browsing activities "
                " on your machine for later use. You might consider this an attack on your "
                " privacy. <p> However, cookies are useful in certain situations. For example, they "
                " are often used by internet shops, so you can 'put things into a shopping basket'. "
                " Some sites require you have a browser that supports cookies. <p>"
                " Because most people want a compromise between privacy and the benefits cookies offer,"
                " KDE offers you the ability to customize the way it handles cookies. You might, for "
                " example want to set KDE's default policy to ask you whenever a server wants to set "
                " a cookie or simply reject or accept everything.  For example, you might choose to "
                " accept all cookies from your favoriate shopping web site.  For this all you have to "
                " do is either browse to that particular site and when you are presented with the cookie "
                " dialog box, click on <i> This domain </i> under the apply to tab and choose accept or "
                " simply specify the name of the site in the <i> Domain Specific Policy </i> tab and set it "
                " to accept. This allows you to receive cookies from trusted web sites without being asked "
                " everytime KDE receives a cookie." );
}

void KCookiesPolicies::changed()
{
  emit KCModule::changed(true);
}

#include "kcookiespolicies.moc"
