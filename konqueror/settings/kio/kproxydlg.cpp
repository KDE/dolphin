// File kproxydlg.cpp by Lars Hoss <Lars.Hoss@munich.netsurf.de>
// Port to KControl by David Faure <faure@kde.org>

#include <qlayout.h> //CT
#include <qwhatsthis.h>

#include <qdialog.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qspinbox.h>

#include <kapp.h>
#include <kmessagebox.h>
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

// PROXY AND PORT SETTINGS
#define DEFAULT_MIN_PORT_VALUE      1
#define DEFAULT_MAX_PORT_VALUE      65536
#define DEFAULT_PROXY_PORT          8080

// MAX_CACHE_AGE needs a proper "age" input widget.
#undef MAX_CACHE_AGE

class KMySpinBox : public QSpinBox
{
public:
   KMySpinBox( QWidget* parent )
    : QSpinBox(parent) { }
   KMySpinBox( int minValue, int maxValue, int step, QWidget* parent)
    : QSpinBox(minValue, maxValue, step, parent) { }
   QLineEdit *editor() const { return QSpinBox::editor(); }
   int value() const
   {
      #ifdef __GNUC__
      #warning workaround for a bug of QSpinBox in >= Qt 2.2.0
      #endif
      if ( editor()->edited() )
          const_cast<KMySpinBox*>(this)->interpretText();
      return QSpinBox::value();
   }
};

KProxyOptions::KProxyOptions(QWidget *parent, const char *name)
  : KCModule(parent, name)
{
  cb_useProxy = new QCheckBox( i18n("Use &Proxy"), this );
  QWhatsThis::add( cb_useProxy, i18n("If this option is enabled, Konqueror will use the"
    " proxy servers provided below for HTTP and FTP connections.  Ask your internet service"
    " provider if you don't know if you do have access to proxy servers.<p>Using proxy servers"
    " is optional but can give you faster access to data on the internet.") );

  connect( cb_useProxy, SIGNAL( clicked() ), SLOT( changeProxy() ) );
  connect( cb_useProxy, SIGNAL( clicked() ), this, SLOT( changed() ) );

  le_http_url = new QLineEdit(this);
  connect(le_http_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_http_url = new QLabel( le_http_url, i18n("&HTTP Proxy:"), this);
  lb_http_url->setAlignment(AlignVCenter);

  sb_http_port = new KMySpinBox(1, 1000000, 1, this);
  connect(sb_http_port, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(sb_http_port->editor(), SIGNAL(textChanged(const QString &)), this, SLOT(changed()));

  QLabel * sep = new QLabel(this);
  sep->setFrameStyle(QFrame::HLine|QFrame::Sunken);

  lb_http_port = new QLabel(sb_http_port, i18n("P&ort:"), this);
  lb_http_port->setAlignment(AlignVCenter);

  QString wtstr = i18n("If you want access to an HTTP proxy server, enter its address here.");
  QWhatsThis::add( lb_http_url, wtstr );
  QWhatsThis::add( le_http_url, wtstr );
  wtstr = i18n("If you want access to an HTTP proxy server, enter its port number here. Your system administrator or ISP should be able to provide with the correct value, but 8000 and 8080 are good guesses. " );
  QWhatsThis::add( lb_http_port, wtstr );
  QWhatsThis::add( sb_http_port, wtstr );

  le_ftp_url = new QLineEdit(this);
  connect(le_ftp_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_ftp_url = new QLabel(le_ftp_url, i18n("&FTP Proxy:"), this);
  lb_ftp_url->setAlignment(AlignVCenter);

  sb_ftp_port = new KMySpinBox(1, 1000000, 1, this);
  connect(sb_ftp_port, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(sb_ftp_port->editor(), SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_ftp_port = new QLabel(sb_ftp_port, i18n("Po&rt:"), this);
  lb_ftp_port->setAlignment(AlignVCenter);


  wtstr = i18n("If you want access to an FTP proxy server, enter its address here.");
  QWhatsThis::add( lb_ftp_url, wtstr );
  QWhatsThis::add( le_ftp_url, wtstr );
  wtstr = i18n("If you want access to an FTP proxy server, enter its port number here. Your system administrator or ISP should be able to provide you with the correct value for this." );
  QWhatsThis::add( lb_ftp_port, wtstr );
  QWhatsThis::add( sb_ftp_port, wtstr );

  le_no_prx = new QLineEdit(this);
  connect(le_no_prx, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_no_prx = new QLabel(le_no_prx, i18n("&No Proxy for:"), this);
  lb_no_prx->setAlignment(AlignVCenter);

  wtstr = i18n("Here you can provide a list of comma or space separated hosts or domains that will be directly accessed without"
    " asking a proxy first. Usually, this will be hosts on your local network.");
  QWhatsThis::add( le_no_prx, wtstr );
  QWhatsThis::add( lb_no_prx, wtstr );

  // buddies
  lb_http_url->setBuddy( le_http_url );
  lb_http_port->setBuddy( sb_http_port );
  lb_ftp_url->setBuddy( le_ftp_url );
  lb_ftp_port->setBuddy( sb_ftp_port );
  lb_no_prx->setBuddy( le_no_prx );

  cb_useCache = new QCheckBox( i18n("Use &Cache"), this );
  QWhatsThis::add( cb_useCache, i18n( "If this box is checked, Konqueror will use its cache to display recently loaded web pages again. It is advisable to use the cache, as it makes switching back and forth between web pages a lot faster. The disadvantage is that it takes up disk space." ) );
  connect( cb_useCache, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( cb_useCache, SIGNAL( clicked() ), this, SLOT( changed() ) );

  cb_verify = new QCheckBox( i18n("Keep Cache in Sync"), this);
  QWhatsThis::add(cb_verify, i18n("Enable this to ask Web servers whether a cached page is still valid. If this is disabled, a cached copy of remote files will be used whenever possible. You can still use the reload button to synchronize the cache with the remote host."));
  connect( cb_verify, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( cb_verify, SIGNAL( clicked() ), this, SLOT( changed() ) );

  cb_offlineMode = new QCheckBox( i18n("Offline Browsing mode"), this);
  QWhatsThis::add(cb_offlineMode, i18n("Enable this checkbox to prevent http requests by KDE applications by default."));
  connect( cb_offlineMode, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( cb_offlineMode, SIGNAL( clicked() ), this, SLOT( changed() ) );

  sb_max_cache_size = new KMySpinBox(100, 2000000, 100, this);
  connect(sb_max_cache_size, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(sb_max_cache_size->editor(), SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_max_cache_size = new QLabel(sb_max_cache_size, i18n("Average Cache &Size:"), this);
  lb_max_cache_size->setAlignment(AlignVCenter);
  wtstr = i18n( "This is the average size in KB that the cache will take on your hard disk. Once in a while the oldest pages will be deleted from the cache to reduce it to this size." );
  QWhatsThis::add( sb_max_cache_size, wtstr );
  QWhatsThis::add( lb_max_cache_size, wtstr );

#ifdef MAX_CACHE_AGE
  sb_max_cache_age = new KMySpinBox(this);
  connect(sb_max_cache_age, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(sb_max_cache_age->editor(), SIGNAL(textChanged(const QString&)), this, SLOT(changed()));

  lb_max_cache_age = new QLabel( sb_max_cache_age, XXXi18n("Maximum Cache &Age:"), this);
  lb_max_cache_age->setAlignment(AlignVCenter);
  wtstr = XXXi18n( "Pages that are older than the time entered here will be deleted from the cache automatically."
                   "This feature is not yet implemented." );
  QWhatsThis::add( lb_max_cache_age, wtstr );
  QWhatsThis::add( sb_max_cache_age, wtstr );
#endif

  pb_down = new QPushButton( this );
  pb_down->setPixmap( BarIcon("down", KIcon::SizeSmall) );
  pb_down->setFixedSize(20,20);
  QWhatsThis::add( pb_down, i18n("Click this button to copy the values for the HTTP proxy"
    " server to the fields for the FTP proxy server, if you have one proxy for both protocols.") );

  connect( pb_down, SIGNAL( clicked() ), SLOT( copyDown() ) );
  connect( pb_down, SIGNAL( clicked() ), SLOT( changed() ) );

  pb_clearCache = new QPushButton( i18n("Clear Cache"), this );
  QWhatsThis::add( pb_clearCache, i18n("Click this button to completely clear the HTTP cache."
    " This can be sometimes useful to check if a wrong copy of a website has been cached,"
    " or to quickly free some disk space.") );

  connect( pb_clearCache, SIGNAL( clicked() ), SLOT( clearCache() ) );

  QVBoxLayout * layout =
    new QVBoxLayout(this, KDialog::marginHint(), KDialog::spacingHint());

  layout->addWidget(cb_useProxy);

  QGridLayout * proxyLayout = new QGridLayout(layout, 2, 5);

  proxyLayout->addWidget(lb_http_url, 0, 0);
  proxyLayout->addWidget(le_http_url, 0, 1);
  proxyLayout->addWidget(lb_http_port, 0, 2);
  proxyLayout->addWidget(sb_http_port, 0, 3);
  proxyLayout->addWidget(pb_down, 0, 4);

  proxyLayout->addWidget(lb_ftp_url, 1, 0);
  proxyLayout->addWidget(le_ftp_url, 1, 1);
  proxyLayout->addWidget(lb_ftp_port, 1, 2);
  proxyLayout->addWidget(sb_ftp_port, 1, 3);

  QHBoxLayout * l = new QHBoxLayout(layout);
  l->addWidget(lb_no_prx);
  l->addWidget(le_no_prx);

  layout->addWidget(sep);
 
  QHBoxLayout * l1 = new QHBoxLayout(layout);
  l1->addWidget(cb_useCache);
  l1->addWidget(cb_verify);
  l1->addWidget(cb_offlineMode);

  QHBoxLayout * l2 = new QHBoxLayout(layout);
  l2->addWidget(lb_max_cache_size);
  l2->addWidget(sb_max_cache_size);

  QHBoxLayout * l3 = new QHBoxLayout(layout);
  l3->addWidget(pb_clearCache);
  l3->addStretch(100);

  layout->addStretch(1);

  // finally read the options
  load();
}

KProxyOptions::~KProxyOptions()
{
}


QString KProxyOptions::quickHelp() const
{
  return i18n( "This module lets you configure your proxy and cache settings. A proxy is a program on another computer that receives requests from your machine to access a certain web page (or other Internet resources), retrieves the page and sends it back to you.<br>Proxies can help increase network throughput, but can also be a means of controlling which web pages you access. The  cache is an internal memory in Konqueror where recently read web pages are stored. If you want to retrieve a web page again that you have recently read, it will not be downloaded from the net, but rather retrieved from the cache which is a lot faster." );
}

void KProxyOptions::load()
{
  updateGUI ( KProtocolManager::proxyFor( "http" ),
              KProtocolManager::proxyFor( "ftp" ),
              KProtocolManager::useProxy(),
              KProtocolManager::noProxyFor());

  cb_useCache->setChecked(KProtocolManager::useCache());

  if (KProtocolManager::defaultCacheControl()==KIO::CC_Verify)
  {
      cb_verify->setChecked(true);
      cb_offlineMode->setChecked(false);
  }
  else if (KProtocolManager::defaultCacheControl()==KIO::CC_CacheOnly)
  {
      cb_verify->setChecked(false);
      cb_offlineMode->setChecked(true);
  }
  else
  {
      cb_verify->setChecked(false);
      cb_offlineMode->setChecked(false);
  }

  old_verify = cb_verify->isChecked();

  sb_max_cache_size->setValue(KProtocolManager::maxCacheSize());
#ifdef MAX_CACHE_AGE
  sb_max_cache_age->setText( "Not yet implemented."); // MaxCacheAge
#endif

  setProxy();
  setCache();
}

void KProxyOptions::defaults() {
  cb_useProxy->setChecked(false);
  le_http_url->setText("");
  sb_http_port->setValue(3128);
  le_ftp_url->setText("");
  sb_ftp_port->setValue(3128);
  le_no_prx->setText("");
  setProxy();
  cb_useCache->setChecked(true);
  cb_verify->setChecked(true);
  cb_offlineMode->setChecked(false);
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
    sb_http_port->setValue(port_num);
  }
  else
  {
    le_http_url->setText( QString::null );
    sb_http_port->setValue(DEFAULT_PROXY_PORT);
  }

  if( !ftpProxy.isEmpty() )
  {
    url = ftpProxy;
    le_ftp_url->setText( url.host() );
    uint port_num = url.port();
    if( port_num == 0 )
      port_num = DEFAULT_PROXY_PORT;
    sb_ftp_port->setValue(port_num);
  }
  else
  {
    le_ftp_url->setText( QString::null );
    sb_ftp_port->setValue(DEFAULT_PROXY_PORT);
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
      url += QString::number(sb_http_port->value());
    }
    KProtocolManager::setProxyFor( "http", url );

    url = le_ftp_url->text();
    if( !url.isEmpty() )
    {
      if ( url.left( 6 ) == "ftp://" )
        url.replace( 0, 3, "http" );
      else
        url.prepend( "http://" );

      url += ":";
      url += QString::number(sb_ftp_port->value());
    }
    KProtocolManager::setProxyFor( "ftp", url );
    KProtocolManager::setUseProxy( cb_useProxy->isChecked() );
    KProtocolManager::setNoProxyFor( le_no_prx->text() );

    // Cache stuff.  TODO:needs to be separated from proxy post 2.0 (DA)
    KProtocolManager::setUseCache( cb_useCache->isChecked() );
    KProtocolManager::setMaxCacheSize(sb_max_cache_size->value());

    if (!cb_useCache->isChecked())
	KProtocolManager::setDefaultCacheControl(KIO::CC_Reload);
    else if (cb_verify->isChecked())
	KProtocolManager::setDefaultCacheControl(KIO::CC_Verify);
    else if (!cb_offlineMode->isChecked())
	KProtocolManager::setDefaultCacheControl(KIO::CC_Cache);
    else
	KProtocolManager::setDefaultCacheControl(KIO::CC_CacheOnly);

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
  sb_ftp_port->setValue(sb_http_port->value());
}

void KProxyOptions::setProxy()
{
  bool useProxy = cb_useProxy->isChecked();

  // now set all input fields
  le_http_url->setEnabled( useProxy );
  sb_http_port->setEnabled( useProxy );
  le_ftp_url->setEnabled( useProxy );
  sb_ftp_port->setEnabled( useProxy );
  le_no_prx->setEnabled( useProxy );
  pb_down->setEnabled( useProxy );
  cb_useProxy->setChecked( useProxy );
}

void KProxyOptions::setCache()
{
  bool useCache = cb_useCache->isChecked();

  // now set all input fields
  sb_max_cache_size->setEnabled( useCache );
#ifdef MAX_CACHE_AGE
  sb_max_cache_age->setEnabled( useCache );
#endif
  //pb_down->setEnabled( useCache );
  cb_useCache->setChecked( useCache );
  cb_verify->setEnabled(useCache);
  if (!useCache)
      cb_offlineMode->setChecked(false);
  cb_offlineMode->setEnabled(useCache);
  if (cb_verify->isChecked() && !old_verify)
  {
      cb_offlineMode->setChecked(false);
  }
  if (cb_offlineMode->isChecked())
      cb_verify->setChecked(false);
  // hack to save 2 signal/slot connections. offlinemode and verify
  // are mutually exclusive
  old_verify=cb_verify->isChecked();
}

void KProxyOptions::changeProxy()
{
  setProxy();
}

void KProxyOptions::changeCache()
{
  setCache();
}

void KProxyOptions::clearCache()
{
  QString error;
  if (KApplication::startServiceByDesktopName("http_cache_cleaner", QStringList(), &error ))
  {
    // Error starting kcookiejar.
    KMessageBox::error(this, i18n("Error starting kio_http_cache_cleaner: %1").arg(error));
  }
  // Not so useless. The button seems to do nothing otherwise.
  KMessageBox::information( this, i18n("Cache cleared"));
}

void KProxyOptions::changed()
{
  emit KCModule::changed(true);
}


#include "kproxydlg.moc"
