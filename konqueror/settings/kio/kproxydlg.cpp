// File kproxydlg.cpp by Lars Hoss <Lars.Hoss@munich.netsurf.de>
// Port to KControl by David Faure <faure@kde.org>

#include <qlayout.h> //CT
#include <qwhatsthis.h>

#include <kapp.h>
#include <kurl.h>
#include <kdialog.h>
#include <klocale.h>
#include <string.h>
#include <kprotocolmanager.h>
#include <knumvalidator.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <dcopclient.h>

#include "kproxydlg.h"

#define ROW_USEPROXY            1
#define ROW_HTTP                2
#define ROW_FTP                 3
#define ROW_NOPROXY             5
#define ROW_USECACHE            7
#define ROW_MAXCACHESIZE        8
#define ROW_MAXCACHEAGE         9

// PROXY AND PORT SETTINGS
#define DEFAULT_MIN_PORT_VALUE      1
#define DEFAULT_MAX_PORT_VALUE      65536
#define DEFAULT_PROXY_PORT          8080


KProxyOptions::KProxyOptions(QWidget *parent, const char *name)
  : KCModule(parent, name)
{

  QGridLayout *lay = new QGridLayout(this,11,8,
                                     KDialog::marginHint(),
                                     KDialog::spacingHint());
  lay->addRowSpacing(4,KDialog::spacingHint()*2);

  lay->addColSpacing(0,KDialog::spacingHint());
  lay->addColSpacing(3,KDialog::spacingHint());
  lay->addColSpacing(7,KDialog::spacingHint());

  lay->setRowStretch(0,1);
  lay->setRowStretch(1,0); // USEPROXY
  lay->setRowStretch(2,0); // HTTP
  lay->setRowStretch(3,0); // FTP
  lay->setRowStretch(4,2);
  lay->setRowStretch(5,0); // NOPROXY
  lay->setRowStretch(6,2);
  lay->setRowStretch(7,0); // USECACHE
  lay->setRowStretch(8,0); // MAX CACHE SIZE
  lay->setRowStretch(9,0); // MAX CACHE AGE
  lay->setRowStretch(10,20);

  lay->setColStretch(0,0);
  lay->setColStretch(1,0);
  lay->setColStretch(2,1);
  lay->setColStretch(3,1);
  lay->setColStretch(4,0);
  lay->setColStretch(5,1);
  lay->setColStretch(6,1);
  lay->setColStretch(7,0);

  cb_useProxy = new QCheckBox( i18n("Use &Proxy"), this );
  lay->addMultiCellWidget(cb_useProxy,ROW_USEPROXY,ROW_USEPROXY,1,6);
  QWhatsThis::add( cb_useProxy, i18n("If this option is enabled, Konqueror will use the"
    " proxy servers provided below for HTTP and FTP connections.  Ask your internet service"
    " provider if you don't know if you do have access to proxy servers.<p>Using proxy servers"
    " is optional but can give you faster access to data on the internet.") );

  connect( cb_useProxy, SIGNAL( clicked() ), SLOT( changeProxy() ) );
  connect( cb_useProxy, SIGNAL( clicked() ), this, SLOT( changed() ) );

  le_http_url = new QLineEdit(this);
  lay->addWidget(le_http_url,ROW_HTTP,2);
  connect(le_http_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_http_url = new QLabel( le_http_url, i18n("&HTTP Proxy:"), this);
  lb_http_url->setAlignment(AlignVCenter);
  lay->addWidget(lb_http_url,ROW_HTTP,1);

  le_http_port = new QLineEdit(this);
  le_http_port->setGeometry(280, 110, 55, 30);
  lay->addWidget(le_http_port,ROW_HTTP,5);
  connect(le_http_port, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  QLabel *label = new QLabel(this);
  label->setFrameStyle(QFrame::HLine|QFrame::Sunken);
  lay->addMultiCellWidget(label, 6, 6, 0, 6);

  lb_http_port = new QLabel( le_http_port, i18n("P&ort:"), this);
  lb_http_port->setAlignment(AlignVCenter);
  lay->addWidget(lb_http_port,ROW_HTTP,4);


  QString wtstr = i18n("If you want access to an HTTP proxy server, enter its address here.");
  QWhatsThis::add( lb_http_url, wtstr );
  QWhatsThis::add( le_http_url, wtstr );
  wtstr = i18n("If you want access to an HTTP proxy server, enter its port number here. Your system administrator or ISP should be able to provide with the correct value, but 8000 and 8080 are good guesses. " );
  QWhatsThis::add( lb_http_port, wtstr );
  QWhatsThis::add( le_http_port, wtstr );

  le_ftp_url = new QLineEdit(this);
  lay->addWidget(le_ftp_url,ROW_FTP,2);
  connect(le_ftp_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_ftp_url = new QLabel( le_ftp_url, i18n("&FTP Proxy:"), this);
  lb_ftp_url->setAlignment(AlignVCenter);
  lay->addWidget(lb_ftp_url,ROW_FTP,1);

  le_ftp_port = new QLineEdit(this);
  lay->addWidget(le_ftp_port,ROW_FTP,5);
  le_ftp_port->setValidator( new KIntValidator( DEFAULT_MIN_PORT_VALUE, DEFAULT_MAX_PORT_VALUE, le_ftp_port ) );
  connect(le_ftp_port, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_ftp_port = new QLabel( le_ftp_port, i18n("Po&rt:"), this);
  lb_ftp_port->setAlignment(AlignVCenter);
  lay->addWidget(lb_ftp_port,ROW_FTP,4);


  wtstr = i18n("If you want access to an FTP proxy server, enter its address here.");
  QWhatsThis::add( lb_ftp_url, wtstr );
  QWhatsThis::add( le_ftp_url, wtstr );
  wtstr = i18n("If you want access to an FTP proxy server, enter its port number here. Your system administrator or ISP should be able to provide you with the correct value for this." );
  QWhatsThis::add( lb_ftp_port, wtstr );
  QWhatsThis::add( le_ftp_port, wtstr );

  le_no_prx = new QLineEdit(this);
  lay->addMultiCellWidget(le_no_prx,ROW_NOPROXY,ROW_NOPROXY,2,5);
  connect(le_no_prx, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_no_prx = new QLabel(le_no_prx, i18n("&No Proxy for:"), this);
  lb_no_prx->setAlignment(AlignVCenter);
  lay->addWidget(lb_no_prx,ROW_NOPROXY,1);

  wtstr = i18n("Here you can provide a list of hosts or domains that will be directly accessed without"
    " asking a proxy first. Usually, this will be hosts on your local network.");
  QWhatsThis::add( le_no_prx, wtstr );
  QWhatsThis::add( lb_no_prx, wtstr );

  // buddies
  lb_http_url->setBuddy( le_http_url );
  lb_http_port->setBuddy( le_http_port );
  lb_ftp_url->setBuddy( le_ftp_url );
  lb_ftp_port->setBuddy( le_ftp_port );
  lb_no_prx->setBuddy( le_no_prx );

  cb_useCache = new QCheckBox( i18n("Use &Cache"), this );
  lay->addMultiCellWidget(cb_useCache,ROW_USECACHE,ROW_USECACHE,1,6);
  QWhatsThis::add( cb_useCache, i18n( "If this box is checked, Konqueror will use its cache to display recently loaded web pages again. It is advisable to use the cache, as it makes switching back and forth between web pages a lot faster. The disadvantage is that it takes up disk space." ) );

  connect( cb_useCache, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( cb_useCache, SIGNAL( clicked() ), this, SLOT( changed() ) );

  le_max_cache_size = new QLineEdit(this);
  lay->addWidget(le_max_cache_size,ROW_MAXCACHESIZE,2);
  connect(le_max_cache_size, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_max_cache_size = new QLabel( le_max_cache_size, i18n("Average Cache &Size:"), this);
  lb_max_cache_size->setAlignment(AlignVCenter);
  lay->addWidget(lb_max_cache_size,ROW_MAXCACHESIZE,1);
  wtstr = i18n( "This is the average size in KB that the cache will take on your hard disk. Once in a while the oldest pages will be deleted from the cache to reduce it to this size." );
  QWhatsThis::add( le_max_cache_size, wtstr );
  QWhatsThis::add( lb_max_cache_size, wtstr );

  le_max_cache_age = new QLineEdit(this);
  lay->addWidget(le_max_cache_age,ROW_MAXCACHEAGE,2);
  le_max_cache_age->setValidator( new KIntValidator( le_max_cache_age ) );
  connect(le_max_cache_age, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_max_cache_age = new QLabel( le_max_cache_age, i18n("Maximum Cache &Age:"), this);
  lb_max_cache_age->setAlignment(AlignVCenter);
  lay->addWidget(lb_max_cache_age,ROW_MAXCACHEAGE,1);
  wtstr = i18n( "Pages that are older than the time entered here will be deleted from the cache automatically. This feature is not yet implemented." );
  QWhatsThis::add( lb_max_cache_age, wtstr );
  QWhatsThis::add( le_max_cache_age, wtstr );

  QString path;
  cp_down = new QPushButton( this );
  cp_down->setPixmap( BarIcon("down") );
  cp_down->setFixedSize(20,20);
  lay->addWidget(cp_down,ROW_HTTP,6);
  QWhatsThis::add( cp_down, i18n("Click this button to copy the values for the HTTP proxy"
    " server to the fields for the FTP proxy server, if you have one proxy for both protocols.") );

  lay->activate();

  connect( cp_down, SIGNAL( clicked() ), SLOT( copyDown() ) );
  connect( cp_down, SIGNAL( clicked() ), SLOT( changed() ) );

  // Use a validator to ensure only proper values are entered!!
  KIntValidator* validator = new KIntValidator( DEFAULT_MIN_PORT_VALUE, DEFAULT_MAX_PORT_VALUE, this );
  le_http_port->setValidator(validator);
  le_ftp_port->setValidator(validator);

  // finally read the options
  load();
}

KProxyOptions::~KProxyOptions()
{
/* Why ?? They are automatically deleted!!! (DA)
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
*/
}


QString KProxyOptions::quickHelp() const
{
  return i18n( "This module lets you configure your proxy and cache settings. A proxy is a program on another computer that receives requests from your machine to access a certain web page (or other Internet ressources), retrieves the page and sends it back to you.<br>Proxies can help increase network throughput, but can also be a means of controlling which web pages you access. The  cache is an internal memory in Konqueror where recently read web pages are stored. If you want to retrieve a web page again that you have recently read, it will not be downloaded from the net, but rather retrieved from the cache which is a lot faster." );
}

void KProxyOptions::load()
{
  updateGUI ( KProtocolManager::proxyFor( "http" ),
              KProtocolManager::proxyFor( "ftp" ),
              KProtocolManager::useProxy(),
              KProtocolManager::noProxyFor());

  cb_useCache->setChecked(KProtocolManager::useCache());
  le_max_cache_size->setText( QString::number( KProtocolManager::maxCacheSize() ) );
  le_max_cache_age->setText( "Not yet implemented."); // MaxCacheAge

  setProxy();
  setCache();
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

void KProxyOptions::updateGUI(QString httpProxy, QString ftpProxy,
                              bool bUseProxy, QString noProxyFor)
{
  KURL url;

  if( !httpProxy.isEmpty() )
  {
    url = httpProxy;
    le_http_url->setText( url.host() );
    uint port_num = url.port();
    if( port_num == 0 )
      port_num = DEFAULT_PROXY_PORT;
    le_http_port->setText( QString::number( port_num ) );
  }

  if( !ftpProxy.isEmpty() )
  {
    url = ftpProxy;
    le_ftp_url->setText( url.host() );
    uint port_num = url.port();
    if( port_num == 0 )
      port_num = DEFAULT_PROXY_PORT;
    le_ftp_port->setText( QString::number( port_num ) );
  }

  cb_useProxy->setChecked(bUseProxy);
  setProxy();

  le_no_prx->setText( noProxyFor );

}

void KProxyOptions::save()
{
    QString url = le_http_url->text();
    if( !url.isEmpty() )
    {
      if ( url.left( 7 ) != "http://" )
        url.prepend( "http://" );

      url += ":";
      url += le_http_port->text();    // port
    }
    KProtocolManager::setProxyFor( "http", url );

    url = le_ftp_url->text();
    if( !url.isEmpty() )
    {
      if ( url.left( 6 ) != "ftp://" )
        url.prepend( "ftp://" );

      url += ":";
      url += le_ftp_port->text();      // port
    }
    KProtocolManager::setProxyFor( "ftp", url );
    KProtocolManager::setUseProxy( cb_useProxy->isChecked() );
    KProtocolManager::setNoProxyFor( le_no_prx->text() );

    // Cache stuff.  TODO:needs to be separated from proxy post 2.0 (DA)
    KProtocolManager::setUseCache( cb_useCache->isChecked() );
    KProtocolManager::setMaxCacheSize(le_max_cache_size->text().toInt());

    // Update everyone...
    QByteArray data;
    // This should only be done when the FTP proxy setting is changed (on/off)
    QCString launcher = KApplication::launcher();
    kapp->dcopClient()->send( launcher, launcher, "reparseConfiguration()", data );
    QDataStream stream( data, IO_WriteOnly );
    // ### TODO: setup protocol argument
    stream << QString::null;
    if ( !kapp->dcopClient()->isAttached() )
      kapp->dcopClient()->attach();
    kapp->dcopClient()->send( "*", "KIO::Scheduler", "reparseSlaveConfiguration(QString)", data );
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

void KProxyOptions::setCache()
{
  bool useCache = cb_useCache->isChecked();

  // now set all input fields
  le_max_cache_size->setEnabled( useCache );
  le_max_cache_age->setEnabled( useCache );
  cb_useCache->setChecked( useCache );
}

void KProxyOptions::changeProxy()
{
  setProxy();
}

void KProxyOptions::changeCache()
{
  setCache();
}

void KProxyOptions::changed()
{
  emit KCModule::changed(true);
}


#include "kproxydlg.moc"
