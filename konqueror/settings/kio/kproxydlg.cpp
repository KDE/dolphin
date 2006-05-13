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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <QLabel>
#include <QLayout>
#include <QRegExp>
#include <QCheckBox>
#include <QRadioButton>
#include <QTabWidget>
//Added by qt3to4:
#include <QVBoxLayout>

#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include "ksaveioconfig.h"
#include "kenvvarproxydlg.h"
#include "kmanualproxydlg.h"

#include "socks.h"
#include "kproxydlg.h"
#include "kproxydlg_ui.h"

KProxyOptions::KProxyOptions ( KInstance *inst, QWidget* parent )
              :KCModule ( inst, parent )
{
  QVBoxLayout *layout = new QVBoxLayout(this);
  
  mTab = new QTabWidget(this);
  layout->addWidget(mTab);

  mProxy  = new KProxyDialog(inst, mTab);
  mSocks = new KSocksConfig(inst, mTab);

  mTab->addTab(mProxy, i18n("&Proxy"));
  mTab->addTab(mSocks, i18n("&SOCKS"));

  connect(mProxy, SIGNAL(changed(bool)), SIGNAL(changed(bool)));
  connect(mSocks, SIGNAL(changed(bool)), SIGNAL(changed(bool)));
  connect(mTab, SIGNAL(currentChanged(QWidget *)), SIGNAL(quickHelpChanged()));
}

KProxyOptions::~KProxyOptions()
{
}

void KProxyOptions::load()
{
  mProxy->load();
  mSocks->load();
}

void KProxyOptions::save()
{
  mProxy->save();
  mSocks->save();
}

void KProxyOptions::defaults()
{
  mProxy->defaults();
  mSocks->defaults();
}

QString KProxyOptions::quickHelp() const
{
  QWidget *w = mTab->currentWidget();
  
  if (w && w->inherits("KCModule"))
  {
     KCModule *m = static_cast<KCModule *>(w);
     return m->quickHelp();
  }
  
  return QString();
}


KProxyDialog::KProxyDialog( KInstance *inst, QWidget* parent)
             :KCModule( inst, parent )
{
  QVBoxLayout* mainLayout = new QVBoxLayout( this );
  mainLayout->setMargin( KDialog::marginHint() );
  mainLayout->setSpacing( KDialog::spacingHint() );
  
  mDlg = new KProxyDialogUI( this );
  mainLayout->addWidget( mDlg );
  mainLayout->addStretch();
  
  // signals and slots connections
  connect( mDlg->rbNoProxy, SIGNAL( toggled(bool) ),
            SLOT( slotUseProxyChanged() ) );
  
  connect( mDlg->rbAutoDiscover, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  connect( mDlg->rbAutoScript, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  
  connect( mDlg->rbPrompt, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  connect( mDlg->rbPresetLogin, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  
  connect( mDlg->cbPersConn, SIGNAL( toggled(bool) ),
            SLOT( slotChanged() ) );
  
  connect( mDlg->location, SIGNAL( textChanged(const QString&) ),
            SLOT( slotChanged() ) );
  
  connect( mDlg->pbEnvSetup, SIGNAL( clicked() ), SLOT( setupEnvProxy() ) );
  connect( mDlg->pbManSetup, SIGNAL( clicked() ), SLOT( setupManProxy() ) );
  
  load();
}

KProxyDialog::~KProxyDialog()
{
  delete mData;
  mData = 0;
}

void KProxyDialog::load()
{
  mDefaultData = false;
  mData = new KProxyData;

  KProtocolManager proto;
  bool useProxy = proto.useProxy();
  mData->type = proto.proxyType();
  mData->proxyList["http"] = proto.proxyFor( "http" );
  mData->proxyList["https"] = proto.proxyFor( "https" );
  mData->proxyList["ftp"] = proto.proxyFor( "ftp" );
  mData->proxyList["script"] = proto.proxyConfigScript();
  mData->useReverseProxy = proto.useReverseProxy();
  mData->noProxyFor = proto.noProxyFor().split( QRegExp("[',''\t'' ']"), QString::SkipEmptyParts);

  mDlg->gbAuth->setEnabled( useProxy );
  mDlg->gbOptions->setEnabled( useProxy );

  mDlg->cbPersConn->setChecked( proto.persistentProxyConnection() );

  if ( !mData->proxyList["script"].isEmpty() )
    mDlg->location->lineEdit()->setText( mData->proxyList["script"] );

  switch ( mData->type )
  {
    case KProtocolManager::WPADProxy:
      mDlg->rbAutoDiscover->setChecked( true );
      break;
    case KProtocolManager::PACProxy:
      mDlg->rbAutoScript->setChecked( true );
      break;
    case KProtocolManager::ManualProxy:
      mDlg->rbManual->setChecked( true );
      break;
    case KProtocolManager::EnvVarProxy:
      mDlg->rbEnvVar->setChecked( true );
      break;
    case KProtocolManager::NoProxy:
    default:
      mDlg->rbNoProxy->setChecked( true );
      break;
  }

  switch( proto.proxyAuthMode() )
  {
    case KProtocolManager::Prompt:
      mDlg->rbPrompt->setChecked( true );
      break;
    case KProtocolManager::Automatic:
      mDlg->rbPresetLogin->setChecked( true );
    default:
      break;
  }
}

void KProxyDialog::save()
{
  bool updateProxyScout = false;

  if (mDefaultData)
    mData->reset ();

  if ( mDlg->rbNoProxy->isChecked() )
  {
    KSaveIOConfig::setProxyType( KProtocolManager::NoProxy );
  }
  else
  {
    if ( mDlg->rbAutoDiscover->isChecked() )
    {
      KSaveIOConfig::setProxyType( KProtocolManager::WPADProxy );
      updateProxyScout = true;
    }
    else if ( mDlg->rbAutoScript->isChecked() )
    {
      KUrl u( mDlg->location->lineEdit()->text() );

      if ( !u.isValid() )
      {
        showInvalidMessage( i18n("The address of the automatic proxy "
                                 "configuration script is invalid. Please "
                                 "correct this problem before proceeding. "
                                 "Otherwise, your changes you will be "
                                 "ignored.") );
        return;
      }
      else
      {
        KSaveIOConfig::setProxyType( KProtocolManager::PACProxy );
        mData->proxyList["script"] = u.url();
        updateProxyScout = true;
      }
    }
    else if ( mDlg->rbManual->isChecked() )
    {
      if ( mData->type != KProtocolManager::ManualProxy )
      {
        // Let's try a bit harder to determine if the previous
        // proxy setting was indeed a manual proxy
        KUrl u ( mData->proxyList["http"] );
        bool validProxy = (u.isValid() && u.port() > 0);
        u = mData->proxyList["https"];
        validProxy = validProxy || (u.isValid() && u.port() > 0);
        u = mData->proxyList["ftp"];
        validProxy = validProxy || (u.isValid() && u.port() > 0);

        if (!validProxy)
        {
          showInvalidMessage();
          return;
        }

        mData->type = KProtocolManager::ManualProxy;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::ManualProxy );
    }
    else if ( mDlg->rbEnvVar->isChecked() )
    {
      if ( mData->type != KProtocolManager::EnvVarProxy )
      {
        showInvalidMessage();
        return;
      }

      KSaveIOConfig::setProxyType( KProtocolManager::EnvVarProxy );
    }

    if ( mDlg->rbPrompt->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Prompt );
    else if ( mDlg->rbPresetLogin->isChecked() )
      KSaveIOConfig::setProxyAuthMode( KProtocolManager::Automatic );
  }

  KSaveIOConfig::setPersistentProxyConnection( mDlg->cbPersConn->isChecked() );

  // Save the common proxy setting...
  KSaveIOConfig::setProxyFor( "ftp", mData->proxyList["ftp"] );
  KSaveIOConfig::setProxyFor( "http", mData->proxyList["http"] );
  KSaveIOConfig::setProxyFor( "https", mData->proxyList["https"] );

  KSaveIOConfig::setProxyConfigScript( mData->proxyList["script"] );
  KSaveIOConfig::setUseReverseProxy( mData->useReverseProxy );
  KSaveIOConfig::setNoProxyFor( mData->noProxyFor.join(",") );


  KSaveIOConfig::updateRunningIOSlaves (this);
  if ( updateProxyScout )
    KSaveIOConfig::updateProxyScout( this );

  emit changed( false );
}

void KProxyDialog::defaults()
{
  mDefaultData = true;
  mDlg->rbNoProxy->setChecked( true );
  mDlg->location->lineEdit()->clear();
  mDlg->cbPersConn->setChecked( false );
  emit changed( true );
}

void KProxyDialog::setupManProxy()
{
  KManualProxyDlg dlgManual( this );

  dlgManual.setProxyData( *mData );

  if ( dlgManual.exec() == QDialog::Accepted )
  {
    *mData = dlgManual.data();
    mDlg->rbManual->setChecked(true);
    emit changed( true );
  }
}

void KProxyDialog::setupEnvProxy()
{
  KEnvVarProxyDlg dlgEnv( this );

  dlgEnv.setProxyData( *mData );

  if ( dlgEnv.exec() == QDialog::Accepted )
  {
    *mData = dlgEnv.data();
    mDlg->rbEnvVar->setChecked(true);
    emit changed( true );
  }
}

void KProxyDialog::slotChanged()
{
  mDefaultData = false;
  emit changed( true );
}

void KProxyDialog::slotUseProxyChanged()
{
  mDefaultData = false;
  bool useProxy = !(mDlg->rbNoProxy->isChecked());
  mDlg->gbAuth->setEnabled(useProxy);
  mDlg->gbOptions->setEnabled(useProxy);
  emit changed( true );
}

QString KProxyDialog::quickHelp() const
{
  return i18n( "<h1>Proxy</h1>"
               "<p>A proxy server is an intermediate program that sits between "
               "your machine and the Internet and provides services such as "
               "web page caching and/or filtering.</p>"
               "<p>Caching proxy servers give you faster access to sites you have "
               "already visited by locally storing or caching the content of those "
               "pages; filtering proxy servers, on the other hand, provide the "
               "ability to block out requests for ads, spam, or anything else you "
               "want to block.</p>"
               "<p><u>Note:</u> Some proxy servers provide both services.</p>" );
}

void KProxyDialog::showInvalidMessage( const QString& _msg )
{
  QString msg;

  if( !_msg.isEmpty() )
    msg = _msg;
  else
    msg = i18n( "<qt>The proxy settings you specified are invalid."
                "<p>Please click on the <b>Setup...</b> "
                "button and correct the problem before proceeding; "
                "otherwise your changes will be ignored.</qt>" );

  KMessageBox::error( this, msg, i18n("Invalid Proxy Setup") );
}

#include "kproxydlg.moc"
