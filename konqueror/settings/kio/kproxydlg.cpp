/*
   kproxydlg.cpp - Proxy configuration dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qregexp.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qtabwidget.h>

#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include "ksaveioconfig.h"
#include "kenvvarproxydlg.h"
#include "kmanualproxydlg.h"

#include "socks.h"
#include "kproxydlg.h"
#include "kproxydlg_ui.h"

KProxyOptions::KProxyOptions (QWidget* parent )
              :KCModule (parent, "kcmkio")
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  QTabWidget *tab = new QTabWidget(this);
  layout->addWidget(tab);

  proxy  = new KProxyDialog(tab);
  socks = new KSocksConfig(tab);

  tab->addTab(proxy, i18n("&Proxy"));
  tab->addTab(socks, i18n("&SOCKS"));

  connect(proxy, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));
  connect(socks, SIGNAL(changed(bool)), this, SIGNAL(changed(bool)));

  connect(tab, SIGNAL(currentChanged(QWidget *)),
          this, SIGNAL(quickHelpChanged()));
  m_tab = tab;
}

KProxyOptions::~KProxyOptions()
{
}

void KProxyOptions::load()
{
  proxy->load();
  socks->load();
}

void KProxyOptions::save()
{
  proxy->save();
  socks->save();
}

void KProxyOptions::defaults()
{
  proxy->defaults();
  socks->defaults();
}

QString KProxyOptions::quickHelp() const
{
  QWidget *w = m_tab->currentPage();
  if (w && w->inherits("KCModule"))
  {
     KCModule *m = static_cast<KCModule *>(w);
     return m->quickHelp();
  }
  return QString::null;
}


KProxyDialog::KProxyDialog( QWidget* parent)
             :KCModule( parent, "kcmkio" )
{
    QVBoxLayout* mainLayout = new QVBoxLayout( this, 0, 0 );

    dlg = new KProxyDialogUI(this);
    mainLayout->addWidget(dlg);

    QWhatsThis::add( dlg->cbUseProxy, i18n("<qt>Check this box to enable the use "
                                       "of proxy servers for your Internet "
                                       "connection.<p>Please note that using "
                                       "proxy servers is optional, but has the "
                                       "benefit or advantage of giving you "
                                       "faster access to data on the Internet."
                                       "<p>If you are uncertain whether or not "
                                       "you need to use a proxy server to "
                                       "connect to the Internet, consult your "
                                       "Internet service provider's setup "
                                       "guide or your system administrator."
                                       "</qt>") );

    QWhatsThis::add( dlg->gbConfigure, i18n("Options for setting up connections "
                                        "to your proxy server(s)") );

    QWhatsThis::add( dlg->rbAutoDiscover, i18n("<qt> Select this option if you want "
                                           "the proxy setup configuration script "
                                           "file to be automatically detected and "
                                           "downloaded.<p>This option only differs "
                                           "from the next choice in that it "
                                           "<i>does not</i> require you to supply "
                                           "the location of the configuration script "
                                           "file. Instead it will be automatically "
                                           "downloaded using the <b>Web Proxy "
                                           "Auto-Discovery Protocol (WPAD)</b>.<p>"
                                           "<u>NOTE:</u>  If you have a problem "
                                           "using this setup, please consult the FAQ "
                                           "section at http://www.konqueror.org for "
                                           "more information.</qt>") );

    QWhatsThis::add( dlg->rbAutoScript, i18n("Select this option if your proxy "
                                         "support is provided through a script "
                                         "file located at a specific address. "
                                         "You can then enter the address of "
                                         "the location below.") );

    QWhatsThis::add( dlg->rbEnvVar, i18n("<qt>Some systems are setup with "
                                     "environment variables such as "
                                     "<b>$HTTP_PROXY</b> to allow graphical "
                                     "as well as non-graphical application "
                                     "to share the same proxy configuration "
                                     "information.<p>Select this option and "
                                     "click on the <i>Setup...</i> button on "
                                     "the right side to provide the environment "
                                     "variable names used to set the address "
                                     "of the proxy server(s).</qt>") );

    QWhatsThis::add( dlg->rbManual, i18n("<qt>Select this option and click on "
                                     "the <i>Setup...</i> button on the "
                                     "right side to manually setup the "
                                     "location of the proxy servers to be "
                                     "used.</qt>") );

    QWhatsThis::add( dlg->gbAuth, i18n("Setup the way that authorization information "
                                   "should be handled when making proxy "
                                   "connections. The default option is to "
                                   "prompt you for password as needed.") );

    QWhatsThis::add( dlg->rbPrompt, i18n("Select this option if you want to be "
                                      "prompted for the login information "
                                      "as needed. This is default behavior.") );

    QWhatsThis::add( dlg->rbAutoLogin, i18n("Select this option if you have "
                                        "already setup a login entry for "
                                        "your proxy server(s) in <tt>"
                                        "$KDEHOME/share/config/kionetrc"
                                        "</tt> file.") );

    QWhatsThis::add( dlg->cbPersConn, i18n("Using persistent proxy connections is faster but "
                                        "only works correctly with proxies that are fully HTTP 1.1 "
                                        "compliant. Do <b>NOT</b> use this option in combination "
                                        "with programs such as JunkBuster or WWWOfle.") );

    // signals and slots connections
    connect( dlg->cbUseProxy, SIGNAL( toggled(bool) ),
             SLOT( slotUseProxyChanged() ) );

    connect( dlg->rbAutoDiscover, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );
    connect( dlg->rbAutoScript, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );

    connect( dlg->rbPrompt, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );
    connect( dlg->rbAutoLogin, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );

    connect( dlg->cbPersConn, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );

    connect( dlg->location, SIGNAL( textChanged(const QString&) ),
             SLOT( slotChanged() ) );

    connect( dlg->pbEnvSetup, SIGNAL( clicked() ), SLOT( setupEnvProxy() ) );
    connect( dlg->pbManSetup, SIGNAL( clicked() ), SLOT( setupManProxy() ) );

    load();
}

KProxyDialog::~KProxyDialog()
{
  delete m_data;
  m_data = 0;
}

void KProxyDialog::load()
{
  m_data = new KProxyData;

  KProtocolManager proto;
  bool useProxy = proto.useProxy();
  m_data->type = proto.proxyType();
  m_data->httpProxy = proto.proxyFor( "http" );
  m_data->httpsProxy = proto.proxyFor( "https" );
  m_data->ftpProxy = proto.proxyFor( "ftp" );
  m_data->scriptProxy = proto.proxyConfigScript();
  m_data->useReverseProxy = proto.useReverseProxy();
  m_data->noProxyFor = QStringList::split( QRegExp("[',''\t'' ']"),
                                           proto.noProxyFor() );

  dlg->cbUseProxy->setChecked( useProxy );
  dlg->gbConfigure->setEnabled( useProxy );
  dlg->gbAuth->setEnabled( useProxy );
  dlg->gbOptions->setEnabled( useProxy );

  dlg->cbPersConn->setChecked( proto.persistentProxyConnection() );

  if ( !m_data->scriptProxy.isEmpty() )
    dlg->location->lineEdit()->setText( m_data->scriptProxy );

  switch ( m_data->type )
  {
    case KProtocolManager::WPADProxy:
      dlg->rbAutoDiscover->setChecked( true );
      break;
    case KProtocolManager::PACProxy:
      dlg->rbAutoScript->setChecked( true );
      break;
    case KProtocolManager::ManualProxy:
      dlg->rbManual->setChecked( true );
      break;
    case KProtocolManager::EnvVarProxy:
      dlg->rbEnvVar->setChecked( true );
      break;
    default:
      break;
  }

  switch( proto.proxyAuthMode() )
  {
    case KProtocolManager::Prompt:
      dlg->rbPrompt->setChecked( true );
      break;
    case KProtocolManager::Automatic:
      dlg->rbAutoLogin->setChecked( true );
    default:
      break;
  }
}

void KProxyDialog::save()
{
  bool updateProxyScout = false;

  if ( dlg->cbUseProxy->isChecked() )
  {
    if ( dlg->rbAutoDiscover->isChecked() )
    {
      KSaveIOConfig::setProxyType( KProtocolManager::WPADProxy );
      updateProxyScout = true;
    }
    else if ( dlg->rbAutoScript->isChecked() )
    {
      KURL u;

      u = dlg->location->lineEdit()->text();

      if ( !u.isValid() )
      {
        QString msg = i18n("<qt>The address of the automatic proxy "
                           "configuration script is invalid! Please "
                           "correct this problem before proceeding. "
                           "Otherwise the changes you made will be "
                           "ignored!</qt>");
        KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
        return;
      }
      else
      {
        KSaveIOConfig::setProxyType( KProtocolManager::PACProxy );
        m_data->scriptProxy = u.url();
        updateProxyScout = true;
      }
    }
    else if ( dlg->rbManual->isChecked() )
    {
      if ( m_data->type != KProtocolManager::ManualProxy )
      {
        // Let's try a bit harder to determine if the previous
        // proxy setting was indeed a manual proxy
        KURL u = m_data->httpProxy;
        bool validProxy = (u.isValid() && u.port() != 0);
        u= m_data->httpsProxy;
        validProxy |= (u.isValid() && u.port() != 0);
        u= m_data->ftpProxy;
        validProxy |= (u.isValid() && u.port() != 0);

        if (!validProxy)
        {
          QString msg = i18n("<qt>Proxy information was not setup "
                             "properly! Please click on the <em>"
                             "Setup...</em> button to correct this "
                             "problem before proceeding. Otherwise "
                             "the changes you made will be ignored!"
                             "</qt>");
          KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
          return;
        }

        m_data->type = KProtocolManager::ManualProxy;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::ManualProxy );
    }
    else if ( dlg->rbEnvVar->isChecked() )
    {
      if ( m_data->type != KProtocolManager::EnvVarProxy )
      {
        QString msg = i18n("<qt>Proxy information was not setup "
                           "properly! Please click on the <em>"
                           "Setup...</em> button to correct this "
                           "problem before proceeding. Otherwise "
                           "the changes you made will be ignored!"
                           "</qt>");
        KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
        return;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::EnvVarProxy );
    }

    if ( dlg->rbPrompt->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Prompt );
    else if ( dlg->rbAutoLogin->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Automatic );
  }
  else
  {
    KSaveIOConfig::setProxyType( KProtocolManager::NoProxy );
  }
  
  KSaveIOConfig::setPersistentProxyConnection( dlg->cbPersConn->isChecked() );

  // Save the common proxy setting...
  KSaveIOConfig::setProxyFor( "ftp", m_data->ftpProxy );
  KSaveIOConfig::setProxyFor( "http", m_data->httpProxy );
  KSaveIOConfig::setProxyFor( "https", m_data->httpsProxy );

  KSaveIOConfig::setProxyConfigScript( m_data->scriptProxy );
  KSaveIOConfig::setUseReverseProxy( m_data->useReverseProxy );
  KSaveIOConfig::setNoProxyFor( m_data->noProxyFor.join(",") );


  KSaveIOConfig::updateRunningIOSlaves (this);
  if ( updateProxyScout )
    KSaveIOConfig::updateProxyScout( this );

  emit changed( false );
}

void KProxyDialog::defaults()
{
  m_data->reset();
  dlg->cbUseProxy->setChecked( false );
  emit changed( true );
}

QString KProxyDialog::quickHelp() const
{
  return i18n( "<h1>Proxy</h1><p>This module lets you configure your proxy "
               "settings.</p><p>A proxy is a program on another computer "
               "that receives requests from your machine to access "
               "a certain web page (or other Internet resources), "
               "retrieves the page and sends it back to you.</p>" );
}

void KProxyDialog::setupManProxy()
{
  dlg->rbManual->setChecked(true);

  KManualProxyDlg* dlg = new KManualProxyDlg( this );

  dlg->setProxyData( *m_data );

  if ( dlg->exec() == QDialog::Accepted )
  {
    *m_data = dlg->data();
    emit changed( true );
  }

  delete dlg;
}

void KProxyDialog::setupEnvProxy()
{
  dlg->rbEnvVar->setChecked(true);

  KEnvVarProxyDlg* dlg = new KEnvVarProxyDlg( this );

  dlg->setProxyData( *m_data );

  if ( dlg->exec() == QDialog::Accepted )
  {
    *m_data = dlg->data();
    emit changed( true );
  }

  delete dlg;
}

void KProxyDialog::slotChanged()
{
  emit changed( true );
}

void KProxyDialog::slotUseProxyChanged()
{
  bool useProxy = dlg->cbUseProxy->isChecked();
  dlg->gbConfigure->setEnabled(useProxy);
  dlg->lbLocation->setEnabled(dlg->rbAutoScript->isChecked());
  dlg->location->setEnabled(dlg->rbAutoScript->isChecked());
  dlg->gbAuth->setEnabled(useProxy);
  dlg->gbOptions->setEnabled(useProxy);
  emit changed( true );
}

#include "kproxydlg.moc"
