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
    QVBoxLayout* mainLayout = new QVBoxLayout( this, KDialog::marginHint(),
        KDialog::spacingHint() );

    dlg = new KProxyDialogUI(this);
    mainLayout->addWidget(dlg);
    mainLayout->addStretch();

    // signals and slots connections
    connect( dlg->rbNoProxy, SIGNAL( toggled(bool) ),
             SLOT( slotUseProxyChanged() ) );

    connect( dlg->rbAutoDiscover, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );
    connect( dlg->rbAutoScript, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );

    connect( dlg->rbPrompt, SIGNAL( toggled(bool) ),
             SLOT( slotChanged() ) );
    connect( dlg->rbPresetLogin, SIGNAL( toggled(bool) ),
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
  delete _data;
  _data = 0;
}

void KProxyDialog::load()
{
  _defaultData = false;
  _data = new KProxyData;

  KProtocolManager proto;
  bool useProxy = proto.useProxy();
  _data->type = proto.proxyType();
  _data->proxyList["http"] = proto.proxyFor( "http" );
  _data->proxyList["https"] = proto.proxyFor( "https" );
  _data->proxyList["ftp"] = proto.proxyFor( "ftp" );
  _data->proxyList["script"] = proto.proxyConfigScript();
  _data->useReverseProxy = proto.useReverseProxy();
  _data->noProxyFor = QStringList::split( QRegExp("[',''\t'' ']"),
                                           proto.noProxyFor() );

  dlg->gbAuth->setEnabled( useProxy );
  dlg->gbOptions->setEnabled( useProxy );

  dlg->cbPersConn->setChecked( proto.persistentProxyConnection() );

  if ( !_data->proxyList["script"].isEmpty() )
    dlg->location->lineEdit()->setText( _data->proxyList["script"] );

  switch ( _data->type )
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
    case KProtocolManager::NoProxy:
    default:
      dlg->rbNoProxy->setChecked( true );
      break;
  }

  switch( proto.proxyAuthMode() )
  {
    case KProtocolManager::Prompt:
      dlg->rbPrompt->setChecked( true );
      break;
    case KProtocolManager::Automatic:
      dlg->rbPresetLogin->setChecked( true );
    default:
      break;
  }
}

void KProxyDialog::save()
{
  bool updateProxyScout = false;

  if (_defaultData)
    _data->reset ();

  if ( dlg->rbNoProxy->isChecked() )
  {
    KSaveIOConfig::setProxyType( KProtocolManager::NoProxy );
  }
  else
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
                           "configuration script is invalid: please "
                           "correct this problem before proceeding; "
                           "otherwise, the changes you have made will be "
                           "ignored.</qt>");
        KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
        return;
      }
      else
      {
        KSaveIOConfig::setProxyType( KProtocolManager::PACProxy );
        _data->proxyList["script"] = u.url();
        updateProxyScout = true;
      }
    }
    else if ( dlg->rbManual->isChecked() )
    {
      if ( _data->type != KProtocolManager::ManualProxy )
      {
        // Let's try a bit harder to determine if the previous
        // proxy setting was indeed a manual proxy
        KURL u ( _data->proxyList["http"] );
        bool validProxy = (u.isValid() && u.port() != 0);
        u= _data->proxyList["https"];
        validProxy |= (u.isValid() && u.port() != 0);
        u= _data->proxyList["ftp"];
        validProxy |= (u.isValid() && u.port() != 0);

        if (!validProxy)
        {
          QString msg = i18n("<qt>Proxy information was not setup "
                             "properly: please click on the <em>"
                             "Setup...</em> button to correct this "
                             "problem before proceeding; otherwise, "
                             "the changes you have made will be ignored."
                             "</qt>");
          KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
          return;
        }

        _data->type = KProtocolManager::ManualProxy;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::ManualProxy );
    }
    else if ( dlg->rbEnvVar->isChecked() )
    {
      if ( _data->type != KProtocolManager::EnvVarProxy )
      {
        QString msg = i18n("<qt>Proxy information was not setup "
                           "properly: please click on the <em>"
                           "Setup...</em> button to correct this "
                           "problem before proceeding; otherwise. "
                           "the changes you have made will be ignored."
                           "</qt>");
        KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
        return;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::EnvVarProxy );
    }

    if ( dlg->rbPrompt->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Prompt );
    else if ( dlg->rbPresetLogin->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Automatic );
  }

  KSaveIOConfig::setPersistentProxyConnection( dlg->cbPersConn->isChecked() );

  // Save the common proxy setting...
  KSaveIOConfig::setProxyFor( "ftp", _data->proxyList["ftp"] );
  KSaveIOConfig::setProxyFor( "http", _data->proxyList["http"] );
  KSaveIOConfig::setProxyFor( "https", _data->proxyList["https"] );

  KSaveIOConfig::setProxyConfigScript( _data->proxyList["script"] );
  KSaveIOConfig::setUseReverseProxy( _data->useReverseProxy );
  KSaveIOConfig::setNoProxyFor( _data->noProxyFor.join(",") );


  KSaveIOConfig::updateRunningIOSlaves (this);
  if ( updateProxyScout )
    KSaveIOConfig::updateProxyScout( this );

  emit changed( false );
}

void KProxyDialog::defaults()
{
  _defaultData = true;
  dlg->rbNoProxy->setChecked( true );
  dlg->location->lineEdit()->clear();
  dlg->cbPersConn->setChecked( false );
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

  dlg->setProxyData( *_data );

  if ( dlg->exec() == QDialog::Accepted )
  {
    *_data = dlg->data();
    emit changed( true );
  }

  delete dlg;
}

void KProxyDialog::setupEnvProxy()
{
  dlg->rbEnvVar->setChecked(true);

  KEnvVarProxyDlg* dlg = new KEnvVarProxyDlg( this );

  dlg->setProxyData( *_data );

  if ( dlg->exec() == QDialog::Accepted )
  {
    *_data = dlg->data();
    emit changed( true );
  }

  delete dlg;
}

void KProxyDialog::slotChanged()
{
  _defaultData = false;
  emit changed( true );
}

void KProxyDialog::slotUseProxyChanged()
{
  _defaultData = false;
  bool useProxy = !(dlg->rbNoProxy->isChecked());
  dlg->gbAuth->setEnabled(useProxy);
  dlg->gbOptions->setEnabled(useProxy);
  emit changed( true );
}

#include "kproxydlg.moc"
