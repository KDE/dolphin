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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>

#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include "envvarproxy_ui.h"
#include "kenvvarproxydlg.h"


#define ENV_FTP_PROXY     "FTP_PROXY,ftp_proxy,FTPPROXY,ftpproxy,PROXY,proxy"
#define ENV_HTTP_PROXY    "HTTP_PROXY,http_proxy,HTTPPROXY,httpproxy,PROXY,proxy"
#define ENV_HTTPS_PROXY   "HTTPS_PROXY,https_proxy,HTTPSPROXY,httpsproxy,PROXY,proxy"
#define NO_PROXY          "NO_PROXY, no_proxy"


KEnvVarProxyDlg::KEnvVarProxyDlg( QWidget* parent, const char* name )
                :KProxyDialogBase( parent, name, true,
                                   i18n( "Variable Proxy Configuration" ) )
{
  dlg = new EnvVarProxyDlgUI( this );
  setMainWidget (dlg);
  dlg->leHttp->setMinimumWidth( dlg->leHttp->fontMetrics().maxWidth() * 20 );
  init();
}

KEnvVarProxyDlg::~KEnvVarProxyDlg ()
{
}

void KEnvVarProxyDlg::init()
{
  m_bHasValidData = false;

  connect( dlg->cbShowValue, SIGNAL( clicked() ), SLOT( showValue() ) );
  connect( dlg->pbVerify, SIGNAL( clicked() ), SLOT( verifyPressed() ) );
  connect( dlg->pbDetect, SIGNAL( clicked() ), SLOT( autoDetectPressed() ) );
}

void KEnvVarProxyDlg::setProxyData( const KProxyData& data )
{
  // Setup HTTP Proxy...
  KURL u ( data.proxyList["http"] );
  if (!u.isEmpty() && !u.isValid())
  {
    m_mapEnvVars["http"].name = data.proxyList["http"];
    m_mapEnvVars["http"].value = QString::fromLocal8Bit( getenv(data.proxyList["http"].local8Bit()) );
    if ( !m_mapEnvVars["http"].value.isEmpty() )
      dlg->leHttp->setText( data.proxyList["http"] );
  }

  // Setup HTTPS Proxy...
  u = data.proxyList["https"];
  if (!u.isEmpty() && !u.isValid())
  {
    m_mapEnvVars["https"].name = data.proxyList["https"];
    m_mapEnvVars["https"].value = QString::fromLocal8Bit( getenv(data.proxyList["https"].local8Bit()) );
    if ( !m_mapEnvVars["https"].value.isEmpty() )
      dlg->leHttps->setText( data.proxyList["https"] );
  }

  // Setup FTP Proxy...
  u = data.proxyList["ftp"];
  if (!u.isEmpty() && !u.isValid())
  {
    m_mapEnvVars["ftp"].name = data.proxyList["ftp"];
    m_mapEnvVars["ftp"].value = QString::fromLocal8Bit( getenv(data.proxyList["ftp"].local8Bit()) );
    if ( !m_mapEnvVars["ftp"].value.isEmpty() )
      dlg->leFtp->setText( data.proxyList["ftp"] );
  }

  u = data.noProxyFor.join(",");
  if (!u.isEmpty() && !u.isValid())
  {
    QString noProxy = u.url();
    m_mapEnvVars["noProxy"].name = noProxy;
    m_mapEnvVars["noProxy"].value = QString::fromLocal8Bit( getenv(noProxy.local8Bit()) );
    if ( !m_mapEnvVars["noProxy"].value.isEmpty() )
      dlg->leNoProxy->setText( noProxy );
  }

  dlg->cbShowValue->setChecked( data.showEnvVarValue );
}

const KProxyData KEnvVarProxyDlg::data() const
{
  KProxyData data;

  if (m_bHasValidData)
  {
    data.proxyList["http"] = m_mapEnvVars["http"].name;
    data.proxyList["https"] = m_mapEnvVars["https"].name;
    data.proxyList["ftp"] = m_mapEnvVars["ftp"].name;
    data.noProxyFor = m_mapEnvVars["noProxy"].name;
    data.type = KProtocolManager::EnvVarProxy;
    data.showEnvVarValue = dlg->cbShowValue->isChecked();
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

  setHighLight (dlg->lbHttp, false);
  setHighLight (dlg->lbHttps, false);
  setHighLight (dlg->lbFtp, false);

  // Detect HTTP proxy settings...
  QStringList list = QStringList::split( ',', QString::fromLatin1(ENV_HTTP_PROXY) );
  for( it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["http"].name = *it;
      m_mapEnvVars ["http"].value = env;
      dlg->leHttp->setText( (dlg->cbShowValue->isOn() ? env : *it) );
      found |= true;
      break;
    }
  }

  // Detect HTTPS proxy settings...
  list = QStringList::split( ',', QString::fromLatin1(ENV_HTTPS_PROXY));
  for( it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["https"].name = *it;
      m_mapEnvVars ["https"].value = env;
      dlg->leHttps->setText( (dlg->cbShowValue->isOn() ? env : *it) );
      found |= true;
      break;
    }
  }

  // Detect FTP proxy settings...
  list = QStringList::split( ',', QString::fromLatin1(ENV_FTP_PROXY) );
  for(it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["ftp"].name = *it;
      m_mapEnvVars ["ftp"].value = env;
      dlg->leFtp->setText( (dlg->cbShowValue->isOn() ? env : *it) );
      found |= true;
      break;
    }
  }

  // Detect the NO_PROXY settings...
  list = QStringList::split( ',', QString::fromLatin1(NO_PROXY) );
  for(it = list.begin(); it != list.end(); ++it )
  {
    env = QString::fromLocal8Bit( getenv( (*it).local8Bit() ) );
    if ( !env.isEmpty() )
    {
      m_mapEnvVars ["noProxy"].name = *it;
      m_mapEnvVars ["noProxy"].value = env;
      dlg->leNoProxy->setText( (dlg->cbShowValue->isOn() ? env : *it) );
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
  bool enable = dlg->cbShowValue->isChecked();

  dlg->leHttp->setReadOnly (enable);
  dlg->leHttps->setReadOnly (enable);
  dlg->leFtp->setReadOnly (enable);
  dlg->leNoProxy->setReadOnly (enable);

  if (!dlg->leHttp->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["http"].value : m_mapEnvVars["http"].name;
    dlg->leHttp->setText( value );
  }

  if (!dlg->leHttps->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["https"].value : m_mapEnvVars["https"].name;
    dlg->leHttps->setText( value );
  }

  if (!dlg->leFtp->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["ftp"].value : m_mapEnvVars["ftp"].name;
    dlg->leFtp->setText( value );
  }

  if (!dlg->leNoProxy->text().isEmpty())
  {
    QString value = enable ? m_mapEnvVars["noProxy"].value : m_mapEnvVars["noProxy"].name;
    dlg->leNoProxy->setText( value );
  }
}

bool KEnvVarProxyDlg::validate()
{
  int count = 0;

  QString value = m_mapEnvVars["http"].value;
  if ( !value.isEmpty() )
    count++;

  value = m_mapEnvVars["https"].value;
  if ( !value.isEmpty() )
    count++;

  value = m_mapEnvVars["ftp"].value;
  if ( !value.isEmpty() )
    count++;

  m_bHasValidData = (count > 0);

  return m_bHasValidData;
}

void KEnvVarProxyDlg::slotOk()
{
  if ( !validate() )
  {
    setHighLight (dlg->lbHttp, true);
    setHighLight (dlg->lbHttps, true);
    setHighLight (dlg->lbFtp, true);

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

  KDialogBase::slotOk ();
}

#include "kenvvarproxydlg.moc"
