// File kproxydlg.cpp by Lars Hoss <Lars.Hoss@munich.netsurf.de>
// Port to KControl by David Faure <faure@kde.org>

#include <qlayout.h> //CT

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
  : KCModule(parent, name)
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
  connect( cb_useProxy, SIGNAL( clicked() ), this, SLOT( changed() ) );
  
  lb_http_url = new QLabel( i18n("HTTP Proxy:"), this);
  lb_http_url->setAlignment(AlignVCenter);
  lay->addWidget(lb_http_url,ROW_HTTP,1);

  le_http_url = new QLineEdit(this);
  lay->addWidget(le_http_url,ROW_HTTP,2);
  connect(le_http_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_http_port = new QLabel( i18n("Port:"), this);
  lb_http_port->setAlignment(AlignVCenter);
  lay->addWidget(lb_http_port,ROW_HTTP,4);

  le_http_port = new QLineEdit(this);
  le_http_port->setGeometry(280, 110, 55, 30);
  lay->addWidget(le_http_port,ROW_HTTP,5);
  connect(le_http_port, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_ftp_url = new QLabel( i18n("FTP Proxy:"), this);
  lb_ftp_url->setAlignment(AlignVCenter);
  lay->addWidget(lb_ftp_url,ROW_FTP,1);

  le_ftp_url = new QLineEdit(this);
  lay->addWidget(le_ftp_url,ROW_FTP,2);
  connect(le_ftp_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_ftp_port = new QLabel( i18n("Port:"), this);
  lb_ftp_port->setAlignment(AlignVCenter);
  lay->addWidget(lb_ftp_port,ROW_FTP,4);

  le_ftp_port = new QLineEdit(this);
  lay->addWidget(le_ftp_port,ROW_FTP,5);
  connect(le_ftp_port, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_no_prx = new QLabel(i18n("No Proxy for:"), this);
  lb_no_prx->setAlignment(AlignVCenter);
  lay->addWidget(lb_no_prx,ROW_NOPROXY,1);

  le_no_prx = new QLineEdit(this);
  lay->addMultiCellWidget(le_no_prx,ROW_NOPROXY,ROW_NOPROXY,2,5);
  connect(le_no_prx, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  QString path;
  cp_down = new QPushButton( this );
  cp_down->setPixmap( BarIcon("down") );
  cp_down->setFixedSize(20,20);
  lay->addWidget(cp_down,ROW_HTTP,6);

  lay->activate();

  connect( cp_down, SIGNAL( clicked() ), SLOT( copyDown() ) );
  connect( cp_down, SIGNAL( clicked() ), SLOT( changed() ) );

  // finally read the options
  load();
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

void KProxyOptions::load()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

  g_pConfig->setGroup( "Proxy Settings" );
  updateGUI (
      g_pConfig->readEntry( "HttpProxy" ),
      g_pConfig->readEntry( "FtpProxy" ),
      g_pConfig->readBoolEntry( "UseProxy" ),
      g_pConfig->readEntry( "NoProxyFor" )
      );

  delete g_pConfig;

  setProxy();
}

void KProxyOptions::defaults() {
  cb_useProxy->setChecked(false);
  le_http_url->setText("");
  le_http_port->setText(""); 
  le_ftp_url->setText("");
  le_ftp_port->setText(""); 
  le_no_prx->setText("");  
  setProxy();
}

void KProxyOptions::updateGUI(QString httpProxy, QString ftpProxy, bool bUseProxy,
               QString noProxyFor)
{
  KURL url;

  if( !httpProxy.isEmpty() ) {
    url = httpProxy;
    le_http_url->setText( url.host() );
    le_http_port->setText( QString::number( url.port() ) ); 
  }

  if( !ftpProxy.isEmpty() ) {
    url = ftpProxy;
    le_ftp_url->setText( url.host() );
    le_ftp_port->setText( QString::number( url.port() ) ); 
  }

  cb_useProxy->setChecked(bUseProxy);
  setProxy();
  
  le_no_prx->setText( noProxyFor );  

}

void KProxyOptions::save()
{
  KConfig *g_pConfig = new KConfig("kioslaverc");

    QString url;

    g_pConfig->setGroup( "Proxy Settings" );
  
    url = le_http_url->text();
    if( !url.isEmpty() ) {
      if ( url.left( 7 ) != "http://" )
        url.prepend( "http://" );
      
      url += ":";
      url += le_http_port->text();    // port
    }
    g_pConfig->writeEntry( "HttpProxy", url );

    url = le_ftp_url->text();
    if( !url.isEmpty() ) {
      if ( url.left( 6 ) != "ftp://" )
        url.prepend( "ftp://" );
	
      url += ":";
      url += le_ftp_port->text();      // port
    }
    g_pConfig->writeEntry( "FtpProxy", url );

    g_pConfig->writeEntry( "UseProxy", cb_useProxy->isChecked() );
    g_pConfig->writeEntry( "NoProxyFor", le_no_prx->text() );
    g_pConfig->sync();
   
    delete g_pConfig;
}


void KProxyOptions::copyDown()
{
  le_ftp_url->setText( le_http_url->text() );
  le_ftp_port->setText( le_http_port->text() );
}

void KProxyOptions::setProxy()
{
  bool useProxy = cb_useProxy->isChecked();

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
  setProxy();
}


void KProxyOptions::changed()
{
  emit KCModule::changed(true);
}


#include "kproxydlg.moc"
