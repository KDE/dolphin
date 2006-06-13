/*
   kenvvarproxydlg.cpp - Proxy configuration dialog

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

#include <stdlib.h>

#include <QLabel>
#include <QLayout>
#include <QCheckBox>
#include <QPushButton>

#include <kdebug.h>
#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>
#include <kurl.h>

#include "envvarproxy_ui.h"
#include "kenvvarproxydlg.h"


#define ENV_FTP_PROXY     "FTP_PROXY,ftp_proxy,FTPPROXY,ftpproxy,PROXY,proxy"
#define ENV_HTTP_PROXY    "HTTP_PROXY,http_proxy,HTTPPROXY,httpproxy,PROXY,proxy"
#define ENV_HTTPS_PROXY   "HTTPS_PROXY,https_proxy,HTTPSPROXY,httpsproxy,PROXY,proxy"
#define NO_PROXY          "NO_PROXY,no_proxy"


KEnvVarProxyDlg::KEnvVarProxyDlg( QWidget* parent, const char* name )
                :KProxyDialogBase( parent, name, true,
                                   i18n( "Variable Proxy Configuration" ) )
{
  mDlg = new EnvVarProxyDlgUI( this );
  setMainWidget( mDlg );
  mDlg->leHttp->setMinimumWidth( mDlg->leHttp->fontMetrics().maxWidth() * 20 );
  init();
}

KEnvVarProxyDlg::~KEnvVarProxyDlg ()
{
}

void KEnvVarProxyDlg::init()
{
  m_bHasValidData = false;

  connect( mDlg->cbShowValue, SIGNAL( clicked() ), SLOT( showValue() ) );
  connect( mDlg->pbVerify, SIGNAL( clicked() ), SLOT( verifyPressed() ) );
  connect( mDlg->pbDetect, SIGNAL( clicked() ), SLOT( autoDetectPressed() ) );
}

void KEnvVarProxyDlg::setProxyData( const KProxyData& data )
{
  // Setup HTTP Proxy...
  KUrl u ( data.proxyList["http"] );
  if (!u.isEmpty() && !u.isValid())
  {
    mEnvVarsMap["http"].name = data.proxyList["http"];
    mEnvVarsMap["http"].value = QString::fromLocal8Bit( getenv(data.proxyList["http"].toLocal8Bit()) );
  }

  // Setup HTTPS Proxy...
  u = data.proxyList["https"];
  if (!u.isEmpty() && !u.isValid())
  {
    mEnvVarsMap["https"].name = data.proxyList["https"];
    mEnvVarsMap["https"].value = QString::fromLocal8Bit( getenv(data.proxyList["https"].toLocal8Bit()) );
  }

  // Setup FTP Proxy...
  u = data.proxyList["ftp"];
  if (!u.isEmpty() && !u.isValid())
  {
    mEnvVarsMap["ftp"].name = data.proxyList["ftp"];
    mEnvVarsMap["ftp"].value = QString::fromLocal8Bit( getenv(data.proxyList["ftp"].toLocal8Bit()) );
  }

  u = data.noProxyFor.join(",");
  if (!u.isEmpty() && !u.isValid())
  {
    QString noProxy = u.url();
    mEnvVarsMap["noProxy"].name = noProxy;
    mEnvVarsMap["noProxy"].value = QString::fromLocal8Bit( getenv(noProxy.toLocal8Bit()) );
  }

  mDlg->cbShowValue->setChecked( data.showEnvVarValue );
  showValue();
}

const KProxyData KEnvVarProxyDlg::data() const
{
  KProxyData data;

  if (m_bHasValidData)
  {
    data.proxyList["http"] = mEnvVarsMap["http"].name;
    data.proxyList["https"] = mEnvVarsMap["https"].name;
    data.proxyList["ftp"] = mEnvVarsMap["ftp"].name;
    data.noProxyFor = QStringList(mEnvVarsMap["noProxy"].name);
    data.type = KProtocolManager::EnvVarProxy;
    data.showEnvVarValue = mDlg->cbShowValue->isChecked();
  }

  return data;
}


void KEnvVarProxyDlg::verifyPressed()
{
  if ( !validate() )
  {
    QString msg = i18n("You must specify at least one valid proxy "
                       "environment variable.");

    QString details = i18n("<qt>Make sure you entered the actual environment "
                           "variable name rather than its value. For "
                           "example, if the environment variable is <br><b>"
                           "HTTP_PROXY=http://localhost:3128</b><br> you need "
                           "to enter <b>HTTP_PROXY</b> here instead of the "
                           "actual value http://localhost:3128.</qt>");

    KMessageBox::detailedSorry( this, msg, details,
                                i18n("Invalid Proxy Setup") );
  }
  else
  {
    KMessageBox::information( this, i18n("Successfully verified."),
                                    i18n("Proxy Setup") );
  }
}

void KEnvVarProxyDlg::autoDetectPressed()
{
  QString env;
  QStringList::ConstIterator it;

  bool found = false;

  setHighLight (mDlg->lbHttp, false);
  setHighLight (mDlg->lbHttps, false);
  setHighLight (mDlg->lbFtp, false);

  // Detect HTTP proxy settings...
  QStringList list = QString::fromLatin1(ENV_HTTP_PROXY).split( ',', QString::SkipEmptyParts );
  for( it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).toLocal8Bit() ) );
    if ( !env.isEmpty() )
    {
      mEnvVarsMap ["http"].name = *it;
      mEnvVarsMap ["http"].value = env;
      mDlg->leHttp->setText( (mDlg->cbShowValue->isChecked() ? env : *it) );
      found = true;
      break;
    }
  }

  // Detect HTTPS proxy settings...
  list = QString::fromLatin1(ENV_HTTPS_PROXY).split( ',', QString::SkipEmptyParts );
  for( it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).toLocal8Bit() ) );
    if ( !env.isEmpty() )
    {
      mEnvVarsMap ["https"].name = *it;
      mEnvVarsMap ["https"].value = env;
      mDlg->leHttps->setText( (mDlg->cbShowValue->isChecked() ? env : *it) );
      found = true;
      break;
    }
  }

  // Detect FTP proxy settings...
  list = QString::fromLatin1(ENV_FTP_PROXY).split( ',', QString::SkipEmptyParts );
  for(it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).toLocal8Bit() ) );
    if ( !env.isEmpty() )
    {
      mEnvVarsMap ["ftp"].name = *it;
      mEnvVarsMap ["ftp"].value = env;
      mDlg->leFtp->setText( (mDlg->cbShowValue->isChecked() ? env : *it) );
      found = true;
      break;
    }
  }

  // Detect the NO_PROXY settings...
  list = QString::fromLatin1(NO_PROXY).split (',', QString::SkipEmptyParts );
  for(it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).toLocal8Bit() ) );
    if ( !env.isEmpty() )
    {
      mEnvVarsMap ["noProxy"].name = *it;
      mEnvVarsMap ["noProxy"].value = env;
      mDlg->leNoProxy->setText( (mDlg->cbShowValue->isChecked() ? env : *it) );
      break;
    }
  }

  if ( !found )
  {
    QString msg = i18n("Did not detect any environment variables "
                       "commonly used to set system wide proxy "
                       "information.");

    QString details = i18n("<qt>To learn about the variable names the "
                           "automatic detection process searches for, "
                           "press OK, click on the quick help button "
                           "on the window title bar of the "
                           "previous dialog and then click on the "
                           "\"<b>Auto Detect</b>\" button.</qt>");

    KMessageBox::detailedSorry( this, msg, details,
                                i18n("Automatic Proxy Variable Detection") );
  }
}

void KEnvVarProxyDlg::showValue()
{
  bool enable = mDlg->cbShowValue->isChecked();

  mDlg->leHttp->setReadOnly (enable);
  mDlg->leHttps->setReadOnly (enable);
  mDlg->leFtp->setReadOnly (enable);
  mDlg->leNoProxy->setReadOnly (enable);

  if (enable)
  {
    mDlg->leHttp->setText( mEnvVarsMap["http"].value );
    mDlg->leHttps->setText( mEnvVarsMap["https"].value );
    mDlg->leFtp->setText( mEnvVarsMap["ftp"].value );
    mDlg->leNoProxy->setText( mEnvVarsMap["noProxy"].value );
  }
  else
  {
    mDlg->leHttp->setText( mEnvVarsMap["http"].name );
    mDlg->leHttps->setText( mEnvVarsMap["https"].name );
    mDlg->leFtp->setText( mEnvVarsMap["ftp"].name );
    mDlg->leNoProxy->setText( mEnvVarsMap["noProxy"].name );
  }
}

bool KEnvVarProxyDlg::validate()
{
  int count = 0;

  QString value = mEnvVarsMap["http"].value;
  if ( !value.isEmpty() )
    count++;

  value = mEnvVarsMap["https"].value;
  if ( !value.isEmpty() )
    count++;

  value = mEnvVarsMap["ftp"].value;
  if ( !value.isEmpty() )
    count++;

  m_bHasValidData = (count > 0);

  return m_bHasValidData;
}

void KEnvVarProxyDlg::accept()
{
  if ( !validate() )
  {
    setHighLight (mDlg->lbHttp, true);
    setHighLight (mDlg->lbHttps, true);
    setHighLight (mDlg->lbFtp, true);

    QString msg = i18n("You must specify at least one valid proxy "
                       "environment variable.");

    QString details = i18n("<qt>Make sure you entered the actual environment "
                           "variable name rather than its value. For "
                           "example, if the environment variable is <br><b>"
                           "HTTP_PROXY=http://localhost:3128</b><br> you need "
                           "to enter <b>HTTP_PROXY</b> here instead of the "
                           "actual value http://localhost:3128.</qt>");

    KMessageBox::detailedError( this, msg, details,
                                 i18n("Invalid Proxy Setup") );
    return;
  }

  KDialog::accept();
}

#include "kenvvarproxydlg.moc"
