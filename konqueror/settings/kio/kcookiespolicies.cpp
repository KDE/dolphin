// kcookiespolicies.cpp - Cookies configuration
//
// First version of cookies configuration by Waldo Bastian <bastian@kde.org>
// This dialog box created by David Faure <faure@kde.org>

#include <qlayout.h>
#include <qwhatsthis.h>

#include <kkeydialog.h> // for ksplitlist
#include <kbuttonbox.h>
#include <kmessagebox.h>
#include <klistview.h>

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

static const char * adviceToStr(KCookieAdvice _advice)
{
    switch( _advice ) {
    case KCookieAccept: return I18N_NOOP("Accept");
    case KCookieReject: return I18N_NOOP("Reject");
    case KCookieAsk: return I18N_NOOP("Ask");
    default: return I18N_NOOP("Dunno");
    }
}

static KCookieAdvice strToAdvice(const QString& _str)
{
    if ( _str.isEmpty() )
        return KCookieDunno;

    QString advice = _str.lower();

    if ( advice == QString::fromLatin1("accept"))
        return KCookieAccept;
    else if ( advice == QString::fromLatin1("reject"))
        return KCookieReject;
    else if ( advice == QString::fromLatin1("ask"))
        return KCookieAsk;

    return KCookieDunno;
}

static void splitDomainAdvice(const QString& configStr, QString &domain, KCookieAdvice &advice)
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
        advice = strToAdvice( tmp.mid( splitIndex+1, tmp.length()) );
    }
}

/************************************ KCookiesPolicies **********************************/

KCookiesPolicies::KCookiesPolicies(QWidget *parent, const char *name)
                 :KCModule(parent, name)
{
    // This is the layout for the "Policy" tab.
    QVBoxLayout *lay = new QVBoxLayout( this, KDialog::marginHint(), KDialog::spacingHint() );
    lay->setAutoAdd( true );


    cb_enableCookies = new QCheckBox( i18n("&Enable Cookies"), this );
    QWhatsThis::add( cb_enableCookies, i18n("This option turns on cookie support. Normally "
                                            "you will want to have cookie support enabled and "
                                            "customize it to suit your need of privacy.") );
    connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changeCookiesEnabled() ) );
    connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changed() ) );

    bg_default = new QVButtonGroup( i18n("Default Policy"), this );
    lay->setStretchFactor( bg_default, 0 );
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

    rb_gbPolicyAsk = new QRadioButton( i18n("A&sk for confirmation before accepting cookies."), bg_default);
    rb_gbPolicyAccept = new QRadioButton( i18n("A&ccept all cookies by default"), bg_default );
    rb_gbPolicyReject = new QRadioButton( i18n("&Reject all cookies by default"), bg_default );

    // Create Group Box for specific settings
    gb_domainSpecific = new QGroupBox( i18n("Domain Specific Policies"), this);
    lay->setStretchFactor( gb_domainSpecific, 10 );
    QGridLayout *ds_lay = new QGridLayout( gb_domainSpecific, 6, 2,
                       KDialog::marginHint(), KDialog::spacingHint() );
    ds_lay->setColStretch( 0, 2 ); // only resize the listbox horizontally, not the buttons
    ds_lay->addRowSpacing( 0, 2 * KDialog::spacingHint() );
    ds_lay->setRowStretch( 0, 0 );
    ds_lay->setRowStretch( 1, 1 );
    ds_lay->setRowStretch( 2, 1 );
    ds_lay->setRowStretch( 3, 1 );
    ds_lay->setRowStretch( 4, 1 );
    ds_lay->setRowStretch( 5, 1 );

    // CREATE SPLIT LIST BOX
    lv_domainPolicy = new KListView( gb_domainSpecific );
    lv_domainPolicy->addColumn(i18n("Hostname"));
    lv_domainPolicy->addColumn(i18n("Policy"), 100);
    ds_lay->addMultiCellWidget( lv_domainPolicy, 1, 5, 0, 0 );
    QString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific cookie policy for. This policy will be used "
                         "instead of the default policy for any cookie sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    QWhatsThis::add( lv_domainPolicy, wtstr );
    QWhatsThis::add( gb_domainSpecific, wtstr );

    pb_domPolicyAdd = new QPushButton( i18n("Add..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyAdd, i18n("Click on this button to manually add a domain "
                                           "specific policy.") );
    connect( pb_domPolicyAdd, SIGNAL(clicked()), SLOT( addPressed() ) );
    ds_lay->addWidget( pb_domPolicyAdd, 1, 1);

    pb_domPolicyChange = new QPushButton( i18n("Change..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyChange, i18n("Click on this button to change the policy for the "
                                              "domain selected in the list box.") );
    connect( pb_domPolicyChange, SIGNAL( clicked() ), this, SLOT( changePressed() ) );
    ds_lay->addWidget( pb_domPolicyChange, 2, 1);

    pb_domPolicyDelete = new QPushButton( i18n("Delete"), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyDelete, i18n("Click on this button to change the policy for the "
                                              "domain selected in the list box.") );
    connect( pb_domPolicyDelete, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );
    ds_lay->addWidget( pb_domPolicyDelete, 3, 1);

    pb_domPolicyImport = new QPushButton( i18n("Import..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyImport, i18n("Click this button to choose the file that contains "
                                              "the cookie policies.  These policies will be merged "
                                              "with the exisiting ones.  Duplicate enteries are ignored.") );
    connect( pb_domPolicyDelete, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
    ds_lay->addWidget( pb_domPolicyImport, 4, 1);
    pb_domPolicyImport->setEnabled( false );

    pb_domPolicyExport = new QPushButton( i18n("Export..."), gb_domainSpecific );
    QWhatsThis::add( pb_domPolicyExport, i18n("Click this button to save the cookie policy to a zipped "
                                              "file.  The file, named <b>cookie_policy.tgz</b>, will be "
                                              "saved to a location of your choice." ) );

    connect( pb_domPolicyDelete, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
    ds_lay->addWidget( pb_domPolicyExport, 5, 1);
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

    lay->addSpacing( KDialog::spacingHint());
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
    if( pDlg.exec() ) {

        QListViewItem* index = new QListViewItem(lv_domainPolicy, pDlg.domain(),
                                                 adviceToStr( (KCookieAdvice) pDlg.policyAdvice()));
        domainPolicy.insert( index, adviceToStr( (KCookieAdvice)pDlg.policyAdvice() ) );
        lv_domainPolicy->setCurrentItem( index );
        changed();
    }
}

void KCookiesPolicies::changePressed()
{
    QListViewItem *index = lv_domainPolicy->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("<qt>No policy selected!\nPlease choose a policy to <i>modify</i> first.</qt>") );
        return;
    }

    KCookieAdvice advice = strToAdvice(domainPolicy[index]);

    PolicyDialog pDlg( this );
    pDlg.setDisableEdit( false, index->text(0) );
    pDlg.setCaption( i18n( "Change Cookie Policy" ) );
    // We subtract one from the enum value because
    // KCookieDunno is not part of the choice list.
    pDlg.setDefaultPolicy( advice - 1 );
    if( pDlg.exec() )
    {
      domainPolicy[index] = adviceToStr((KCookieAdvice)pDlg.policyAdvice());
      index->setText(1, i18n(domainPolicy[index]));
      changed();
    }
}

void KCookiesPolicies::deletePressed()
{
    QListViewItem *index = lv_domainPolicy->currentItem();
    if ( index == 0 )
    {
        KMessageBox::information( 0, i18n("<qt>No policy selected!\nPlease choose a policy to <i>delete</i> first.</qt>") );
        return;
    }
    domainPolicy.remove(index);
    delete index;
    changed();
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

void KCookiesPolicies::updateDomainList(const QStringList &domainConfig)
{
    for (QStringList::ConstIterator it = domainConfig.begin();
         it != domainConfig.end(); ++it)
    {
      QString domain;
      KCookieAdvice advice;
      splitDomainAdvice(*it, domain, advice);
      QListViewItem *index = new QListViewItem( lv_domainPolicy, domain, i18n( adviceToStr(advice) ) );
      domainPolicy[index] = adviceToStr(advice);
    }
}

void KCookiesPolicies::load()
{
  KSimpleConfig* cfg = new KSimpleConfig( "kcookiejarrc" );

  // Backwards compatiable reading of domain specific settings
  if( cfg->hasGroup( "Browser Settings/HTTP" ) &&
     !cfg->hasGroup( "Cookie Policy" ) )
     cfg->setGroup( "Browser Settings/HTTP" );
  else
    cfg->setGroup( "Cookie Policy" );

  KCookieAdvice globalAdvice = strToAdvice( cfg->readEntry("CookieGlobalAdvice", "Ask") );
  cb_enableCookies->setChecked( cfg->readBoolEntry( "Cookies", true ) );

  rb_gbPolicyAccept->setChecked( globalAdvice == KCookieAccept );
  rb_gbPolicyReject->setChecked( globalAdvice == KCookieReject );
  rb_gbPolicyAsk->setChecked( (globalAdvice != KCookieAccept) &&
                              (globalAdvice != KCookieReject) );

  updateDomainList( cfg->readListEntry("CookieDomainAdvice") );
  changeCookiesEnabled();

  // Remove the old group - R.I.P
  if( cfg->hasGroup( "Browser Settings/HTTP" ) )
     cfg->deleteGroup( "Browser Settings/HTTP" );

  delete cfg;
}

void KCookiesPolicies::save()
{
  KSimpleConfig *cfg = new KSimpleConfig( "kcookiejarrc" );

  cfg->setGroup( "Cookie Policy" );

  bool b_useCookies = cb_enableCookies->isChecked();
  cfg->writeEntry( "Cookies", b_useCookies );

  QString advice;
  if (rb_gbPolicyAccept->isChecked())
      advice = adviceToStr(KCookieAccept);
  else if (rb_gbPolicyReject->isChecked())
      advice = adviceToStr(KCookieReject);
  else
      advice = adviceToStr(KCookieAsk);
  cfg->writeEntry("CookieGlobalAdvice", advice);
  QStringList domainConfig;
  QListViewItem *at = lv_domainPolicy->firstChild();
  while( at )
  {
    domainConfig.append(QString::fromLatin1("%1:%2").arg(at->text(0)).arg(domainPolicy[at]));
    at = at->nextSibling();
  }
  cfg->writeEntry("CookieDomainAdvice", domainConfig);
  cfg->sync();
  delete cfg;

  // Create a dcop client object to communicate with the
  // cookiejar if it is alive, if not who cares.  It will
  // read the cfg info from file the next time...
  DCOPClient *m_dcopClient = new DCOPClient();
  if( m_dcopClient->attach() )
  {
     if (b_useCookies)
     {
        if( !m_dcopClient->send( "kcookiejar", "kcookiejar", "reloadPolicy()", QString::null ) )
           kdDebug(7104) << "Can't communicate with the cookiejar!" << endl;
     }
     else
     {
        if( !m_dcopClient->send( "kcookiejar", "kcookiejar", "shutdown()", QString::null ) )
           kdDebug(7104) << "Can't communicate with the cookiejar!" << endl;
     }

     // Inform http and https slaves about change in cookie policy.
     {
        QByteArray data;
        QDataStream stream( data, IO_WriteOnly );
        stream << QString("http");
        m_dcopClient->send( "*", "KIO::Scheduler", "reparseSlaveConfiguration(QString)", data );
     }
     {
        QByteArray data;
        QDataStream stream( data, IO_WriteOnly );
        stream << QString("https");
        m_dcopClient->send( "*", "KIO::Scheduler", "reparseSlaveConfiguration(QString)", data );
     }
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
  lv_domainPolicy->clear();
  changeCookiesEnabled();
}

QString KCookiesPolicies::quickHelp() const
{
    return i18n("<h1>Cookies</h1> Cookies contain information that Konqueror\n"
                " (or other KDE applications using the http protocol) stores on your\n"
                " computer from a remote internet server. This means, that a web server\n"
                " can store information about you and your browsing activities\n"
                " on your machine for later use. You might consider this an attack on your\n"
                " privacy. <p> However, cookies are useful in certain situations. For example, they\n"
                " are often used by internet shops, so you can 'put things into a shopping basket'.\n"
                " Some sites require you have a browser that supports cookies. <p>\n"
                " Because most people want a compromise between privacy and the benefits cookies offer,\n"
                " KDE offers you the ability to customize the way it handles cookies. You might, for\n"
                " example want to set KDE's default policy to ask you whenever a server wants to set\n"
                " a cookie or simply reject or accept everything.  For example, you might choose to\n"
                " accept all cookies from your favorite shopping web site.  For this all you have to\n"
                " do is either browse to that particular site and when you are presented with the cookie\n"
                " dialog box, click on <i> This domain </i> under the apply to tab and choose accept or\n"
                " simply specify the name of the site in the <i> Domain Specific Policy </i> tab and set it\n"
                " to accept. This allows you to receive cookies from trusted web sites without being asked\n"
                " everytime KDE receives a cookie." );
}

void KCookiesPolicies::changed()
{
  emit KCModule::changed(true);
}

#include "kcookiespolicies.moc"
