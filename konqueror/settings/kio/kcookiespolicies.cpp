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
#include <qwhatsthis.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>

#include <kmessagebox.h>
#include <klistview.h>
#include <klocale.h>
#include <kconfig.h>
#include <kdebug.h>
#include <dcopclient.h>

#include "kcookiespolicies.h"


KCookiesPolicies::KCookiesPolicies(QWidget *parent, const char *name)
                 :KCModule(parent, name)
{
    QVBoxLayout *lay;
    // This is the layout for the "Policy" tab.
    lay = new QVBoxLayout( this, KDialog::marginHint(),
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

    rb_gbPolicyAsk = new QRadioButton( i18n("A&sk for confirmation before "
                                            "accepting cookies."), bg_default );
    bg_default->insert (rb_gbPolicyAsk, KCookieAdvice::Ask);

    rb_gbPolicyAccept = new QRadioButton( i18n("Acce&pt all cookies by "
                                               "default"), bg_default );
    bg_default->insert (rb_gbPolicyAccept, KCookieAdvice::Accept);

    rb_gbPolicyReject = new QRadioButton( i18n("Re&ject all cookies by "
                                               "default"), bg_default );
    bg_default->insert (rb_gbPolicyReject, KCookieAdvice::Reject);

    // Create Group Box for specific settings
    gb_domainSpecific = new QGroupBox( i18n("Domain Specific Policy"), this);
    lay->setStretchFactor( gb_domainSpecific, 10 );
    QGridLayout *ds_lay = new QGridLayout( gb_domainSpecific, 3, 2,
                                           KDialog::marginHint(),
                                           KDialog::spacingHint() );
    ds_lay->addRowSpacing( 0, fontMetrics().lineSpacing() );
    ds_lay->setColStretch( 0, 2 ); // only resize the listbox horizontally, not the buttons
    ds_lay->setRowStretch( 2, 2 );

    // CREATE SPLIT LIST BOX
    lv_domainPolicy = new KListView( gb_domainSpecific );
    lv_domainPolicy->setSelectionMode (QListView::Extended);

    lv_domainPolicy->addColumn(i18n("Domain"));
    lv_domainPolicy->addColumn(i18n("Policy"), 100);
    ds_lay->addMultiCellWidget( lv_domainPolicy, 1, 2, 0, 0 );
    connect( lv_domainPolicy, SIGNAL(selectionChanged()), SLOT(selectionChanged()) );
    connect( lv_domainPolicy, SIGNAL(doubleClicked (QListViewItem *)),SLOT(changePressed() ) );
    QString wtstr = i18n("This box contains the domains you have set a specific "
                         "cookie policy for. This policy will be used instead "
                         "of the default policy for any cookie sent by these "
                         "domains. <p>Select a policy and use the controls on "
                         "the right to modify it.");
    QWhatsThis::add( lv_domainPolicy, wtstr );
    QWhatsThis::add( gb_domainSpecific, wtstr );

    QVBox* vbox = new QVBox( gb_domainSpecific );
    vbox->setSpacing( KDialog::spacingHint() );
    pb_domPolicyAdd = new QPushButton( i18n("&New..."), vbox );
    QWhatsThis::add( pb_domPolicyAdd, i18n("Click on this button to manually "
                                           "add a domain specific policy.") );
    connect( pb_domPolicyAdd, SIGNAL(clicked()), SLOT( addPressed() ) );

    pb_domPolicyChange = new QPushButton( i18n("C&hange..."), vbox );
    pb_domPolicyChange->setEnabled( false );
    QWhatsThis::add( pb_domPolicyChange, i18n("Click on this button to change "
                                              "the policy for the domain "
                                              "selected in the list box.") );
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
                                              "the cookie policies. These policies will be merged "
                                              "with the existing ones. Duplicate entries are ignored.") );
    connect( pb_domPolicyImport, SIGNAL( clicked() ), this, SLOT( importPressed() ) );
    pb_domPolicyImport->hide();

    pb_domPolicyExport = new QPushButton( i18n("Export..."), vbox );
    QWhatsThis::add( pb_domPolicyExport, i18n("Click this button to save the cookie policy to a zipped "
                                              "file. The file, named <b>cookie_policy.tgz</b>, will be "
                                              "saved to a location of your choice." ) );

    connect( pb_domPolicyExport, SIGNAL( clicked() ), this, SLOT( exportPressed() ) );
    pb_domPolicyExport->hide();
#endif
    ds_lay->addWidget( vbox, 1, 1 );

    QWhatsThis::add( gb_domainSpecific, i18n("Here you can set specific cookie policies for any particular "
                                             "domain. To add a new policy, simply click on the <i>Add...</i> "
                                             "button and supply the necessary information requested by the "
                                             "dialog box. To change an existing policy, click on the <i>Change...</i> "
                                             "button and choose the new policy from the policy dialog box. Clicking "
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
  int globalPolicy;
  KCookiePolicyDlg* dlg;

  globalPolicy = bg_default->id (bg_default->selected());
  dlg = new KCookiePolicyDlg (i18n("New Cookie Policy"), this);

  if( dlg->exec() && !dlg->domain().isEmpty())
  {
    QString domain = dlg->domain();
    int advice = dlg->advice();

    if ( !handleDuplicate(domain, advice) )
    {
      const char* strAdvice = KCookieAdvice::adviceToStr(advice);
      QListViewItem* index = new QListViewItem (lv_domainPolicy, domain,
                                                strAdvice);
      domainPolicy.insert (index, strAdvice);
      lv_domainPolicy->setCurrentItem (index);
      changed();
    }
  }

  delete dlg;
}

void KCookiesPolicies::changePressed()
{
  int globalPolicy;
  QString oldDomain;
  QString newDomain;
  QListViewItem *index;
  KCookiePolicyDlg* dlg;
  KCookieAdvice::Value advice;

  index = lv_domainPolicy->currentItem();

  if (!index)
    return;

  globalPolicy = bg_default->id (bg_default->selected());
  advice = KCookieAdvice::strToAdvice(domainPolicy[index]);
  dlg = new KCookiePolicyDlg (i18n("Change Cookie Policy"), this);

  oldDomain = index->text(0);
  dlg->setPolicy (advice);
  dlg->setEnableHostEdit (true, oldDomain);

  if( dlg->exec() && !dlg->domain().isEmpty())
  {
    newDomain = dlg->domain();
    int advice = dlg->advice();
    if (newDomain == oldDomain || !handleDuplicate(newDomain, advice))
    {
      domainPolicy[index] = KCookieAdvice::adviceToStr(advice);
      index->setText(0, newDomain);
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
        domainPolicy[item]= KCookieAdvice::adviceToStr(advice);
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
  QListViewItem* item;
  QListViewItem* nextItem;

  item = lv_domainPolicy->firstChild ();

  while (item != 0L)
  {
    if (lv_domainPolicy->isSelected (item))
    {
      nextItem = item->itemBelow();
      if ( !nextItem )
        nextItem = item->itemAbove();

      delete item;
      item = nextItem;
    }
    else
    {
      item = item->itemBelow();
    }
  }

  if (nextItem)
    lv_domainPolicy->setSelected (nextItem, true);

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
  bool hasItems = lv_domainPolicy->childCount() > 0;

  pb_domPolicyDelete->setEnabled ((hasItems && d_itemsSelected > 0));
  pb_domPolicyDeleteAll->setEnabled ((hasItems && d_itemsSelected > 0));
  pb_domPolicyChange->setEnabled ((hasItems && d_itemsSelected == 1));
}

void KCookiesPolicies::changeCookiesEnabled()
{
    bool enabled = cb_enableCookies->isChecked();
    bg_default->setEnabled( enabled );
    gb_domainSpecific->setEnabled( enabled );
}

void KCookiesPolicies::updateDomainList(const QStringList &domainConfig)
{
    QStringList::ConstIterator it = domainConfig.begin();
    for (; it != domainConfig.end(); ++it)
    {
      QString domain;
      KCookieAdvice::Value advice;
      QListViewItem *index;

      splitDomainAdvice(*it, domain, advice);
      index = new QListViewItem( lv_domainPolicy, domain,
                                 i18n(KCookieAdvice::adviceToStr(advice)) );
      domainPolicy[index] = KCookieAdvice::adviceToStr(advice);
    }
}

void KCookiesPolicies::selectionChanged ()
{
  int itemCount;
  QListViewItem* item;

  d_itemsSelected = 0;
  item = lv_domainPolicy->firstChild ();

  while (item != 0L)
  {
    if (lv_domainPolicy->isSelected (item))
      d_itemsSelected++;
    item = item->nextSibling ();
  }

  updateButtons ();
}

void KCookiesPolicies::load()
{
  KConfig* cfg;
  KCookieAdvice::Value advice;

  d_itemsSelected = 0;

  cfg = new KConfig ("kcookiejarrc");
  cfg->setGroup ("Cookie Policy");
  cb_enableCookies->setChecked (cfg->readBoolEntry("Cookies", true));

  advice = KCookieAdvice::strToAdvice (cfg->readEntry("CookieGlobalAdvice",
                                                      "Ask"));
  switch (advice)
  {
    case KCookieAdvice::Accept:
      rb_gbPolicyAccept->setChecked (true);
      break;
    case KCookieAdvice::Reject:
      rb_gbPolicyReject->setChecked (true);
      break;
    case KCookieAdvice::Ask:
    case KCookieAdvice::Dunno:
    default:
      rb_gbPolicyAsk->setChecked (true);
  }

  updateDomainList(cfg->readListEntry("CookieDomainAdvice"));
  changeCookiesEnabled();

  updateButtons();
  delete cfg;
}

void KCookiesPolicies::save()
{
  KConfig *cfg;
  QString advice;
  bool b_useCookies;
  QStringList domainConfig;

  cfg = new KConfig( "kcookiejarrc" );
  cfg->setGroup( "Cookie Policy" );

  b_useCookies = cb_enableCookies->isChecked();

  cfg->writeEntry( "Cookies", b_useCookies );

  if (rb_gbPolicyAccept->isChecked())
      advice = KCookieAdvice::adviceToStr(KCookieAdvice::Accept);
  else if (rb_gbPolicyReject->isChecked())
      advice = KCookieAdvice::adviceToStr(KCookieAdvice::Reject);
  else
      advice = KCookieAdvice::adviceToStr(KCookieAdvice::Ask);

  cfg->writeEntry("CookieGlobalAdvice", advice);


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
        if (!m_dcopClient->send ("kcookiejar", "kcookiejar", "reloadPolicy()",
                                 QString::null))
           kdDebug(7104) << "Can't communicate with the cookiejar!" << endl;
     }
     else
     {
        if (!m_dcopClient->send ("kcookiejar", "kcookiejar", "shutdown()",
                                 QString::null))
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
  {
     kdDebug(7103) << "Can't connect with the DCOP server." << endl;
  }

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

void KCookiesPolicies::splitDomainAdvice (const QString& cfg, QString &domain,
                                          KCookieAdvice::Value &advice)
{
  int pos;

  pos = cfg.find(':');

  if ( pos == -1)
  {
    domain = cfg;
    advice = KCookieAdvice::Dunno;
  }
  else
  {
    domain = cfg.left(pos);
    advice = KCookieAdvice::strToAdvice (cfg.mid (pos+1, cfg.length()));
  }
}

void KCookiesPolicies::changed()
{
  emit KCModule::changed(true);
}

QString KCookiesPolicies::quickHelp() const
{
  return i18n("<h1>Cookies</h1> Cookies contain information that Konqueror"
              " (or any other KDE application using the HTTP protocol) stores"
              " on your computer from a remote Internet server. This means"
              " that a web server can store information about you and your"
              " browsing activities on your machine for later use. You might"
              " consider this an invasion of privacy.<p>However, cookies are"
              " useful in certain situations. For example, they are often used"
              " by Internet shops, so you can 'put things into a shopping"
              " basket'. Some sites require you have a browser that supports"
              " cookies.<p>Because most people want a compromise between privacy"
              " and the benefits cookies offer, KDE offers you the ability to"
              " customize the way it handles cookies. You might, for example"
              " want to set KDE's default policy to ask you whenever a server"
              " wants to set a cookie or simply reject or accept everything."
              " For example, you might choose to accept all cookies from your"
              " favorite shopping web site. For this all you have to do is"
              " either browse to that particular site and when you are presented"
              " with the cookie dialog box, click on <i> This domain </i> under"
              " the 'apply to' tab and choose accept or simply specify the name"
              " of the site in the <i> Domain Specific Policy </i> tab and set"
              " it to accept. This enables you to receive cookies from trusted"
              " web sites without being asked everytime KDE receives a cookie."
             );
}

#include "kcookiespolicies.moc"
