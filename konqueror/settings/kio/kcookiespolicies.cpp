/**
 * kcookiespolicies.cpp - Cookies configuration
 *
 * Original Authors
 * Copyright (c) Waldo Bastian <bastian@kde.org>
 * Copyright (c) 1999 David Faure <faure@kde.org>
 *
 * Re-written by:
 * Copyright (c) 2000- Dawit Alemayehu <adawit@kde.org>
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
#include <qvbox.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qgroupbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>

#include <kmessagebox.h>
#include <klistview.h>
#include <klocale.h>
#include <kapp.h>
#include <kconfig.h>
#include <kdebug.h>
#include <kiconloader.h>
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
    QVBoxLayout *lay = new QVBoxLayout( this, KDialog::marginHint(),
                                        KDialog::spacingHint() );
    lay->setAutoAdd( true );

    cb_enableCookies = new QCheckBox( i18n("Enable Coo&kies"), this );
    QWhatsThis::add( cb_enableCookies, i18n("This option turns on cookie support. Normally "
                                            "you will want to have cookie support enabled and "
                                            "customize it to suit your need of privacy.") );
    connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changeCookiesEnabled() ) );
    connect( cb_enableCookies, SIGNAL( clicked() ), this, SLOT( changed() ) );

    bg_default = new QVButtonGroup( i18n("Default Policy"), this );
    lay->setStretchFactor( bg_default, 0 );
    QWhatsThis::add( bg_default, i18n("The default policy determines how cookies received from "
                                      "a remote machine, which is not associated with a specific "
                                      "policy (see below), will be handled: "
                                      "<ul><li><b>Accept</b> will cause cookies to be accepted "
                                      "without prompting you</li><li><b>Reject</b> will cause the "
                                      "cookiejar to refuse all cookies it receives</li><li><b>Ask</b> "
                                      "will cause KDE to ask you for your confirmation everytime a "
                                      "server wants to set a cookie</li></ul>") );
    connect(bg_default, SIGNAL(clicked(int)), this, SLOT(changed()));
    bg_default->setExclusive( true );

    rb_gbPolicyAsk = new QRadioButton( i18n("A&sk for confirmation before accepting cookies."), bg_default);
    rb_gbPolicyAccept = new QRadioButton( i18n("Acce&pt all cookies by default"), bg_default );
    rb_gbPolicyReject = new QRadioButton( i18n("Re&ject all cookies by default"), bg_default );

    // Create Group Box for specific settings
    gb_domainSpecific = new QGroupBox( i18n("Site/domain specific policy"), this);
    lay->setStretchFactor( gb_domainSpecific, 10 );
    QGridLayout *ds_lay = new QGridLayout( gb_domainSpecific, 3, 2,
                                           KDialog::marginHint(),
                                           KDialog::spacingHint() );
    ds_lay->addRowSpacing( 0, fontMetrics().lineSpacing() );
    ds_lay->setColStretch( 0, 2 ); // only resize the listbox horizontally, not the buttons
    ds_lay->setRowStretch( 2, 2 );

    // CREATE SPLIT LIST BOX
    lv_domainPolicy = new KListView( gb_domainSpecific );
    lv_domainPolicy->addColumn(i18n("Hostname"));
    lv_domainPolicy->addColumn(i18n("Policy"), 100);
    ds_lay->addMultiCellWidget( lv_domainPolicy, 1, 2, 0, 0 );
    connect( lv_domainPolicy, SIGNAL(selectionChanged()), SLOT(updateButtons()) );
    connect( lv_domainPolicy, SIGNAL(doubleClicked ( QListViewItem * )),SLOT(changePressed() ) );
    QString wtstr = i18n("This box contains the domains and hosts you have set "
                         "a specific cookie policy for. This policy will be used "
                         "instead of the default policy for any cookie sent by these "
                         "domains or hosts. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    QWhatsThis::add( lv_domainPolicy, wtstr );
    QWhatsThis::add( gb_domainSpecific, wtstr );


    QVBox* vbox = new QVBox( gb_domainSpecific );
    vbox->setSpacing( KDialog::spacingHint() );
    pb_domPolicyAdd = new QPushButton( i18n("&New..."), vbox );
    QWhatsThis::add( pb_domPolicyAdd, i18n("Click on this button to manually add a domain "
                                           "specific policy.") );
    connect( pb_domPolicyAdd, SIGNAL(clicked()), SLOT( addPressed() ) );


    pb_domPolicyChange = new QPushButton( i18n("C&hange..."), vbox );
    pb_domPolicyChange->setEnabled( false );
    QWhatsThis::add( pb_domPolicyChange, i18n("Click on this button to change the policy for the "
                                              "domain selected in the list box.") );
    connect( pb_domPolicyChange, SIGNAL( clicked() ), this, SLOT( changePressed() ) );


    pb_domPolicyDelete = new QPushButton( i18n("De&lete"), vbox );
    pb_domPolicyDelete->setEnabled( false );
    QWhatsThis::add( pb_domPolicyDelete, i18n("Click on this button to remove the policy for the "
                                              "domain selected in the list box.") );
    connect( pb_domPolicyDelete, SIGNAL( clicked() ), this, SLOT( deletePressed() ) );

    pb_domPolicyDeleteAll = new QPushButton( i18n("D&elete All"), vbox );
    pb_domPolicyDeleteAll->setEnabled( false );
    QWhatsThis::add( pb_domPolicyDeleteAll, i18n("Click on this button to remove all domain policies.") );
    connect( pb_domPolicyDeleteAll, SIGNAL( clicked() ), this, SLOT( deleteAllPressed() ) );

#if 0
    pb_domPolicyImport = new QPushButton( i18n("Import..."), vbox );
    QWhatsThis::add( pb_domPolicyImport, i18n("Click this button to choose the file that contains "
                                              "the cookie policies.  These policies will be merged "
                                              "with the existing ones.  Duplicate entries are ignored.") );
    connect( pb_domPolicyImport, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
    pb_domPolicyImport->hide();

    pb_domPolicyExport = new QPushButton( i18n("Export..."), vbox );
    QWhatsThis::add( pb_domPolicyExport, i18n("Click this button to save the cookie policy to a zipped "
                                              "file.  The file, named <b>cookie_policy.tgz</b>, will be "
                                              "saved to a location of your choice." ) );

    connect( pb_domPolicyExport, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
    pb_domPolicyExport->hide();
#endif
    ds_lay->addWidget( vbox, 1, 1 );

    QWhatsThis::add( gb_domainSpecific, i18n("Here you can set specific cookie policies for any particular "
                                             "domain. To add a new policy, simply click on the <i>Add...</i> "
                                             "button and supply the necessary information requested by the "
                                             "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                             "button and choose the new policy from the policy dialog box.  Clicking "
                                             "on the <i>Delete</i> button will remove the selected policy causing the "
                                             "default policy setting to be used for that domain. ") );
#if 0
                                             "The <i>Import</i> and <i>Export</i> "
                                             "button allows you to easily share your policies with other people by allowing "
                                             "you to save and retrive them from a zipped file."
#endif

    lay->addSpacing( KDialog::spacingHint() );
    load();
}

KCookiesPolicies::~KCookiesPolicies()
{
}

void KCookiesPolicies::addPressed()
{
    PolicyDialog*  dlg = new PolicyDialog( i18n( "New Cookie Policy" ), this );
    // We subtract one from the enum value because
    // KCookieDunno is not part of the choice list.
    int def_policy = KCookieAsk - 1;
    dlg->setDefaultPolicy( def_policy );
    if( dlg->exec() )
    {
      QString domain = dlg->domain();
      int advice = dlg->policyAdvice();

      if ( !handleDuplicate(domain, advice) )
      {
        const char* strAdvice = adviceToStr(static_cast<KCookieAdvice>(advice));
        QListViewItem* index = new QListViewItem(lv_domainPolicy,
                                                 domain, strAdvice);
        domainPolicy.insert(index, strAdvice);
        lv_domainPolicy->setCurrentItem( index );
        changed();
      }
    }
    delete dlg;
}

void KCookiesPolicies::changePressed()
{
    QListViewItem *index = lv_domainPolicy->currentItem();
    if(!index)
        return;
    KCookieAdvice advice = strToAdvice(domainPolicy[index]);
    PolicyDialog* dlg = new PolicyDialog( i18n("Change Cookie Policy"), this );
    QString old_domain = index->text(0);
    dlg->setEnableHostEdit( true, old_domain );
    dlg->setDefaultPolicy( advice - 1 );
    if( dlg->exec() )
    {
      QString new_domain = dlg->domain();
      int advice = dlg->policyAdvice();
      if ( new_domain == old_domain || !handleDuplicate(new_domain, advice) )
      {
        domainPolicy[index] = adviceToStr(static_cast<KCookieAdvice>(advice));
        index->setText(0, new_domain);
        index->setText(1, i18n(domainPolicy[index]) );
        changed();
      }
    }
    delete dlg;
}

bool KCookiesPolicies::handleDuplicate( const QString& domain, int advice )
{
  QListViewItem* item = lv_domainPolicy->firstChild();
  while ( item != 0 )
  {
    if ( item->text(0) == domain )
    {
      QString msg = i18n("<qt>A policy already exists for"
                         "<center><b>%1</b></center>"
                         "Do you want to replace it ?</qt>").arg(domain);
      int res = KMessageBox::warningYesNo(this, msg,
                                          i18n("Duplicate Policy"),
                                          QString::null);
      if ( res == KMessageBox::Yes )
      {
        domainPolicy[item]=adviceToStr(static_cast<KCookieAdvice>(advice));
        item->setText(0, domain);
        item->setText(1, i18n(domainPolicy[item]));
        changed();
        return true;
      }
      else
        return true;  // User Cancelled!!
    }
    item = item->nextSibling();
  }
  return false;
}

void KCookiesPolicies::deletePressed()
{
    QListViewItem* item = lv_domainPolicy->selectedItem()->itemBelow();
    if ( !item )
      item = lv_domainPolicy->selectedItem()->itemAbove();
    QListViewItem* curr = lv_domainPolicy->selectedItem();
    domainPolicy.remove(curr);
    delete curr;
    if ( item )
      lv_domainPolicy->setSelected( item, true );
    updateButtons();
    changed();
}

void KCookiesPolicies::deleteAllPressed()
{
    domainPolicy.clear();
    lv_domainPolicy->clear();
    updateButtons();
    changed();
}

#if 0
void KCookiesPolicies::importPressed()
{
}

void KCookiesPolicies::exportPressed()
{
}
#endif

void KCookiesPolicies::updateButtons()
{
  bool hasSelectedItems = lv_domainPolicy->childCount() > 0;
  bool itemSelected = (hasSelectedItems && lv_domainPolicy->selectedItem()!=0);
  pb_domPolicyChange->setEnabled( itemSelected );
  pb_domPolicyDelete->setEnabled( itemSelected );
  pb_domPolicyDeleteAll->setEnabled( hasSelectedItems );
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
  KConfig* cfg = new KConfig( "kcookiejarrc" );
  cfg->setGroup( "Cookie Policy" );

  KCookieAdvice globalAdvice = strToAdvice( cfg->readEntry("CookieGlobalAdvice", "Ask") );
  cb_enableCookies->setChecked( cfg->readBoolEntry( "Cookies", true ) );

  rb_gbPolicyAccept->setChecked( globalAdvice == KCookieAccept );
  rb_gbPolicyReject->setChecked( globalAdvice == KCookieReject );
  rb_gbPolicyAsk->setChecked( (globalAdvice != KCookieAccept) &&
                              (globalAdvice != KCookieReject) );

  updateDomainList( cfg->readListEntry("CookieDomainAdvice") );
  changeCookiesEnabled();

  delete cfg;
  updateButtons();
}

void KCookiesPolicies::save()
{
  KConfig *cfg = new KConfig( "kcookiejarrc" );
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
                " computer from a remote Internet server. This means, that a web server\n"
                " can store information about you and your browsing activities\n"
                " on your machine for later use. You might consider this an invasion of\n"
                " privacy. <p> However, cookies are useful in certain situations. For example, they\n"
                " are often used by Internet shops, so you can 'put things into a shopping basket'.\n"
                " Some sites require you have a browser that supports cookies. <p>\n"
                " Because most people want a compromise between privacy and the benefits cookies offer,\n"
                " KDE offers you the ability to customize the way it handles cookies. You might, for\n"
                " example want to set KDE's default policy to ask you whenever a server wants to set\n"
                " a cookie or simply reject or accept everything.  For example, you might choose to\n"
                " accept all cookies from your favorite shopping web site.  For this all you have to\n"
                " do is either browse to that particular site and when you are presented with the cookie\n"
                " dialog box, click on <i> This domain </i> under the 'apply to' tab and choose accept or\n"
                " simply specify the name of the site in the <i> Domain Specific Policy </i> tab and set it\n"
                " to accept. This enables you to receive cookies from trusted web sites without being asked\n"
                " everytime KDE receives a cookie." );
}

void KCookiesPolicies::changed()
{
  emit KCModule::changed(true);
}

#include "kcookiespolicies.moc"
