// File kproxydlg.cpp by Lars Hoss <Lars.Hoss@munich.netsurf.de>
// Port to KControl by David Faure <faure@kde.org>

#include <qlayout.h> //CT

#include <ktablistbox.h>
#include <klocale.h>
#include <kapp.h>
#include <kurl.h>
#include <string.h>
#include "kproxydlg.h"
#include <kconfig.h>
#include <kiconloader.h>
#include <kglobal.h>

#define ROW_USEPROXY 1
#define ROW_HTTP 2
#define ROW_FTP 3
#define ROW_NOPROXY 5

KProxyOptions::KProxyOptions(QWidget *parent, const char *name)
  : KConfigWidget(parent, name)
{

  QGridLayout *lay = new QGridLayout(this,7,8,10,5);
  lay->addRowSpacing(4,20);
  lay->addRowSpacing(6,20);
  lay->addColSpacing(0,10);
  lay->addColSpacing(3,10);
  lay->addColSpacing(7,10);

  lay->setRowStretch(0,1);
  lay->setRowStretch(1,0); // USEPROXY
  lay->setRowStretch(2,0); // HTTP
  lay->setRowStretch(3,0); // FTP
  lay->setRowStretch(4,2);
  lay->setRowStretch(5,0); // NOPROXY
  lay->setRowStretch(6,20);

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,1);
  lay->setColStretch(3,1);
  lay->setColStretch(4,0);
  lay->setColStretch(5,1);
  lay->setColStretch(6,1);
  lay->setColStretch(7,0);

  cb_useProxy = new QCheckBox( i18n("Use proxy"), this );
  lay->addMultiCellWidget(cb_useProxy,ROW_USEPROXY,ROW_USEPROXY,1,6);
  
  connect( cb_useProxy, SIGNAL( clicked() ), SLOT( changeProxy() ) );
  
  lb_http_url = new QLabel( i18n("HTTP Proxy:"), this);
  lb_http_url->setAlignment(AlignVCenter);
  lay->addWidget(lb_http_url,ROW_HTTP,1);

  le_http_url = new QLineEdit(this);
  lay->addWidget(le_http_url,ROW_HTTP,2);

  lb_http_port = new QLabel( i18n("Port:"), this);
  lb_http_port->setAlignment(AlignVCenter);
  lay->addWidget(lb_http_port,ROW_HTTP,4);

  le_http_port = new QLineEdit(this);
  le_http_port->setGeometry(280, 110, 55, 30);
  lay->addWidget(le_http_port,ROW_HTTP,5);

  lb_ftp_url = new QLabel( i18n("FTP Proxy:"), this);
  lb_ftp_url->setAlignment(AlignVCenter);
  lay->addWidget(lb_ftp_url,ROW_FTP,1);

  le_ftp_url = new QLineEdit(this);
  lay->addWidget(le_ftp_url,ROW_FTP,2);

  lb_ftp_port = new QLabel( i18n("Port:"), this);
  lb_ftp_port->setAlignment(AlignVCenter);
  lay->addWidget(lb_ftp_port,ROW_FTP,4);

  le_ftp_port = new QLineEdit(this);
  lay->addWidget(le_ftp_port,ROW_FTP,5);

  lb_no_prx = new QLabel(i18n("No Proxy for:"), this);
  lb_no_prx->setAlignment(AlignVCenter);
  lay->addWidget(lb_no_prx,ROW_NOPROXY,1);

  le_no_prx = new QLineEdit(this);
  lay->addMultiCellWidget(le_no_prx,ROW_NOPROXY,ROW_NOPROXY,2,5);

  QString path;
  cp_down = new QPushButton( this );
  cp_down->setPixmap( ICON("down.xpm") );
  cp_down->setFixedSize(20,20);
  lay->addWidget(cp_down,ROW_HTTP,6);

  lay->activate();

  connect( cp_down, SIGNAL( clicked() ), SLOT( copyDown() ) );

  // finally read the options
  loadSettings();
}

KProxyOptions::~KProxyOptions()
{
  // now delete everything we allocated before
  // delete lb_info;
  delete lb_http_url;
  delete le_http_url;
  delete lb_http_port;
  delete le_http_port;
  delete lb_ftp_url;
  delete le_ftp_url;
  delete lb_ftp_port;
  delete le_ftp_port;
  delete cp_down;
  delete cb_useProxy;
  // time to say goodbye ...
}

void KProxyOptions::loadSettings()
{
  updateGUI (
      g_pConfig->readEntry( "HttpProxy" ),
      g_pConfig->readEntry( "FtpProxy" ),
      g_pConfig->readBoolEntry( "UseProxy" ),
      g_pConfig->readEntry( "NoProxyFor" )
      );
}

void KProxyOptions::defaultSettings() {
  updateGUI ("","",false,"");
}

void KProxyOptions::updateGUI(QString httpProxy, QString ftpProxy, bool bUseProxy,
               QString noProxyFor)
{
  QString port;
  KURL url;

  if( !httpProxy.isEmpty() ) {
    url = httpProxy;
    port.setNum( url.port() );
    le_http_url->setText( url.host() );
    le_http_port->setText( port ); 
  }

  if( !ftpProxy.isEmpty() ) {
    url = ftpProxy;
    port.setNum( url.port() );
    le_ftp_url->setText( url.host() );
    le_ftp_port->setText( port ); 
  }

  useProxy = bUseProxy;
  setProxy();
  
  le_no_prx->setText( noProxyFor );  

}

void KProxyOptions::saveSettings()
{
    QString url;

    url = le_http_url->text();
    if( !url.isEmpty() ) {
        url = "http://";
        url += le_http_url->text();     // host
        url += ":";
        url += le_http_port->text();    // and port
    }
    g_pConfig->writeEntry( "HttpProxy", url );

    url = le_ftp_url->text();
    if( !url.isEmpty() ) {
        url = "ftp://";
        url += le_ftp_url->text();       // host
        url += ":";
        url += le_ftp_port->text();      // and port
    }
    g_pConfig->writeEntry( "FtpProxy", url );

    g_pConfig->writeEntry( "UseProxy", useProxy );
    g_pConfig->writeEntry( "NoProxyFor", le_no_prx->text() );
    g_pConfig->sync();
}

void KProxyOptions::applySettings()
{
    saveSettings();
}

void KProxyOptions::copyDown()
{
  le_ftp_url->setText( le_http_url->text() );
  le_ftp_port->setText( le_http_port->text() );
}

void KProxyOptions::setProxy()
{
  // now set all input fields
  le_http_url->setEnabled( useProxy );
  le_http_port->setEnabled( useProxy );
  le_ftp_url->setEnabled( useProxy );
  le_ftp_port->setEnabled( useProxy );
  le_no_prx->setEnabled( useProxy );
  cp_down->setEnabled( useProxy );
  cb_useProxy->setChecked( useProxy );
}

void KProxyOptions::changeProxy()
{
  useProxy = cb_useProxy->isChecked();
  setProxy();
}

#include "kproxydlg.moc"
