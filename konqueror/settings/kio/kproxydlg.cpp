// File kproxydlg.cpp by Lars Hoss <Lars.Hoss@munich.netsurf.de>
// Port to KControl by David Faure <faure@kde.org>
// designer dialog and usage by Daniel Molkentin <molkentin@kde.org>
// Proxy autoconfig by Malte Starostik <malte@kde.org>

#include <qlayout.h>
#include <qwhatsthis.h>

#include <qdialog.h>
#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qcheckbox.h>
#include <qspinbox.h>
#include <qradiobutton.h>
#include <qvbuttongroup.h>
#include <qmultilineedit.h>

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
#include <kurlrequester.h>
#include <kio/http_slave_defaults.h>
#include <klistview.h>
#include <kdebug.h>

#include <kproxydlgui.h>

#include "kproxydlg.h"

// PROXY AND PORT SETTINGS
#define DEFAULT_MIN_PORT_VALUE      1
#define DEFAULT_MAX_PORT_VALUE      65536
#define DEFAULT_PROXY_PORT          8080

// MAX_CACHE_AGE needs a proper "age" input widget.
#undef MAX_CACHE_AGE


// ############################################################

KProxySetDlgBase::KProxySetDlgBase(QWidget *parent, const char *name)
 : KDialogBase(parent, name, true, QString::null, Ok|Cancel, Ok)
{

  QWidget *page = new QWidget(this);
  setMainWidget(page);

  QBoxLayout *topLay = new QVBoxLayout(page, 0, spacingHint());

  title = new QLabel(i18n("&Host / IP address:"),page);
  input = new QLineEdit(page);
  title->setBuddy(input);

  topLay->addSpacing(10);
  topLay->addWidget(title);
  topLay->addWidget(input);
  topLay->addSpacing(10);
  connect(input,SIGNAL(textChanged ( const QString & )),this,SLOT(textChanged ( const QString & )));
  enableButtonOK(false);
}

void KProxySetDlgBase::textChanged ( const QString &text )
{
     enableButtonOK( !text.isEmpty() );
}


KAddHostDlg::KAddHostDlg(QWidget *parent, const char* name)
 : KProxySetDlgBase(parent, name)
{
  setCaption(i18n("Add new host / IP address"));

  QString wstr = i18n("<qt>Here you can add a new hostname or IP address that"
                               " should be directly accessed by Konqueror. Valid examples are:"
                               "<ul><li>192.168.0.1 <i>(Exclude IP address)</i></li>"
                               "<li>192.168.0.* <i>(Exclude Subnet)</i></li>"
                               "<li>localhost <i>(Exclude host)</i></li>"
                               "<li>*.kde.org <i>(Exclude all hosts in a domain)</i></li>"
                               "</ul></qt>");

  QWhatsThis::add(title, wstr);
  QWhatsThis::add(input, wstr);
}

void KAddHostDlg::accept()
{
  emit sigHaveNewHost(input->text());
  QDialog::accept();
}

KEditHostDlg::KEditHostDlg(const QString &host, QWidget *parent, const char* name)
 : KProxySetDlgBase(parent, name)
{
  setCaption(i18n("Edit host / IP address"));
  input->setText(host);

  QString wstr = i18n("<qt>Here you can edit the hostname or IP address that"
                               " should be directly accessed by Konqueror. Valid examples are:"
                               "<ul><li>192.168.0.1 <i>(Exclude IP address)</i></li>"
                               "<li>192.168.0.* <i>(Exclude Subnet)</i></li>"
                               "<li>localhost <i>(Exclude host)</i></li>"
                               "<li>*.kde.org <i>(Exclude all hosts in a domain)</i></li>"
                               "</ul></qt>");

  QWhatsThis::add(title, wstr);
  QWhatsThis::add(input, wstr);

}

void KEditHostDlg::accept()
{
  emit sigHaveChangedHost(input->text());
  QDialog::accept();
}

// ############################################################

KProxyOptions::KProxyOptions(QWidget *parent, const char *name)
  : KCModule(parent, name)
{

 QVBoxLayout *layout = new QVBoxLayout(this);

  ui = new KProxyDlgUI(this);
  layout->addWidget(ui);

  ui->klv_no_prx->addColumn(i18n("Host / IP Address"));
  ui->klv_no_prx->setFullWidth();

  ui->pb_rmHost->setEnabled(false);
  ui->pb_editHost->setEnabled(false);

  connect(ui->pb_addHost, SIGNAL(clicked()), SLOT(slotAddHost()));
  connect(ui->pb_rmHost, SIGNAL(clicked()), SLOT(slotRemoveHost()));
  connect(ui->pb_editHost, SIGNAL(clicked()), SLOT(slotEditHost()));
  connect(ui->klv_no_prx, SIGNAL(doubleClicked ( QListViewItem * )), SLOT(slotEditHost()));

  connect(ui->klv_no_prx, SIGNAL(selectionChanged()), SLOT(slotEnableButtons()));

  connect( ui->cb_useProxy, SIGNAL( clicked() ), SLOT( changeProxy() ) );
  connect( ui->cb_useProxy, SIGNAL( clicked() ), this, SLOT( changed() ) );
  connect( ui->cb_autoProxy, SIGNAL( clicked() ), SLOT( changeProxy() ) );
  connect( ui->cb_autoProxy, SIGNAL( clicked() ), SLOT( changed() ) );
  connect( ui->url_autoProxy, SIGNAL( textChanged( const QString & ) ), SLOT( changed() ) );
  connect( ui->le_http_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect( ui->sb_http_port, SIGNAL(valueChanged(int)), this, SLOT(changed()));
 // connect( ui->sb_http_port->editor(), SIGNAL(textChanged(const QString &)), this, SLOT(changed()));
  connect( ui->le_ftp_url, SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect( ui->sb_ftp_port, SIGNAL(valueChanged(int)), this, SLOT(changed()));
 // connect( ui->sb_ftp_port->editor(), SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect( ui->cb_useCache, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( ui->cb_useCache, SIGNAL( clicked() ), this, SLOT( changed() ) );
  connect( ui->rb_verify, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( ui->rb_verify, SIGNAL( clicked() ), this, SLOT( changed() ) );
  connect( ui->rb_cacheIfPossible, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( ui->rb_cacheIfPossible, SIGNAL( clicked() ), this, SLOT( changed() ) );
  connect( ui->rb_offlineMode, SIGNAL( clicked() ), SLOT( changeCache() ) );
  connect( ui->rb_offlineMode, SIGNAL( clicked() ), this, SLOT( changed() ) );
/*
  connect(sb_max_cache_size, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(sb_max_cache_size->editor(), SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
  connect(sb_max_cache_age, SIGNAL(valueChanged(int)), this, SLOT(changed()));
  connect(sb_max_cache_age->editor(), SIGNAL(textChanged(const QString&)), this, SLOT(changed()));
*/


  ui->pb_down->setPixmap( BarIcon("down", KIcon::SizeSmall) );
  ui->pb_down->setFixedSize(20,20);

  connect( ui->pb_down, SIGNAL( clicked() ), SLOT( copyDown() ) );
  connect( ui->pb_down, SIGNAL( clicked() ), SLOT( changed() ) );

  connect( ui->pb_clearCache, SIGNAL( clicked() ), SLOT( clearCache() ) );

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
              KProtocolManager::proxyType(),
              KProtocolManager::noProxyFor(),
              KProtocolManager::proxyConfigScript() );

  ui->cb_useCache->setChecked(KProtocolManager::useCache());
  KIO::CacheControl cc = KProtocolManager::cacheControl();
  if (cc==KIO::CC_Verify)
      ui->rb_verify->setChecked(true);
  else if (cc==KIO::CC_CacheOnly)
      ui->rb_offlineMode->setChecked(true);
  else if (cc==KIO::CC_Cache)
      ui->rb_cacheIfPossible->setChecked(true);

  ui->sb_max_cache_size->setValue(KProtocolManager::maxCacheSize());

  setProxy();
  setCache();
}

void KProxyOptions::defaults() {
  ui->cb_useProxy->setChecked(false);
  ui->le_http_url->setText("");
  ui->sb_http_port->setValue(3128);
  ui->le_ftp_url->setText("");
  ui->sb_ftp_port->setValue(3128);
  ui->klv_no_prx->clear();
  ui->klv_no_prx->insertItem(new QListViewItem(ui->klv_no_prx, "localhost"));
  setProxy();
  ui->cb_useCache->setChecked(true);
  setCache();
  ui->rb_verify->setChecked(true);
  ui->sb_max_cache_size->setValue(DEFAULT_MAX_CACHE_SIZE);
}

void KProxyOptions::updateGUI(QString httpProxy, QString ftpProxy,
                              KProtocolManager::ProxyType proxyType,
                              QString noProxyFor, QString autoProxy)
{
  KURL url;

  if( !httpProxy.isEmpty() )
  {
    url = httpProxy;
    ui->le_http_url->setText( url.host() );
    uint port_num = url.port();
    if( port_num == 0 )
      port_num = DEFAULT_PROXY_PORT;
    ui->sb_http_port->setValue(port_num);
  }
  else
  {
    ui->le_http_url->setText( QString::null );
    ui->sb_http_port->setValue(DEFAULT_PROXY_PORT);
  }

  if( !ftpProxy.isEmpty() )
  {
    url = ftpProxy;
    ui->le_ftp_url->setText( url.host() );
    uint port_num = url.port();
    if( port_num == 0 )
      port_num = DEFAULT_PROXY_PORT;
    ui->sb_ftp_port->setValue(port_num);
  }
  else
  {
    ui->le_ftp_url->setText( QString::null );
    ui->sb_ftp_port->setValue(DEFAULT_PROXY_PORT);
  }

  ui->cb_useProxy->setChecked(proxyType != KProtocolManager::NoProxy);
  ui->cb_autoProxy->setChecked(proxyType > KProtocolManager::ManualProxy);
  ui->url_autoProxy->setURL(autoProxy);
  setProxy();

  QStringList list = QStringList::split(QRegExp("\\s"), noProxyFor);

  for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it )
	  ui->klv_no_prx->insertItem( new QListViewItem( ui->klv_no_prx, *it));

  if (!ui->klv_no_prx->childCount())
      ui->klv_no_prx->insertItem(new QListViewItem(ui->klv_no_prx, "localhost"));
}

void KProxyOptions::save()
{
    QString url = ui->le_http_url->text();
    if( !url.isEmpty() )
    {
      if ( url.left( 7 ) != "http://" )
        url.prepend( "http://" );

      url += ":";
      url += QString::number(ui->sb_http_port->value());
    }
    KProtocolManager::setProxyFor( "http", url );

    url = ui->le_ftp_url->text();
    if( !url.isEmpty() )
    {
      if ( url.left( 6 ) == "ftp://" )
        url.replace( 0, 3, "http" );
      else
        url.prepend( "http://" );

      url += ":";
      url += QString::number(ui->sb_ftp_port->value());
    }
    KProtocolManager::setProxyFor( "ftp", url );
    if ( ui->cb_useProxy->isChecked() )
    {
        if ( ui->cb_autoProxy->isChecked() )
            KProtocolManager::setProxyType( KProtocolManager::PACProxy );
        else
            KProtocolManager::setProxyType( KProtocolManager::ManualProxy );
    }
    else
        KProtocolManager::setProxyType( KProtocolManager::NoProxy );


    QListViewItemIterator it( ui->klv_no_prx );

    QString noProxyFor;

    for ( ; it.current(); ++it ) {
        noProxyFor += it.current()->text(0);
        if (it.current()->nextSibling()) noProxyFor += " ";
    }

    KProtocolManager::setNoProxyFor( noProxyFor );
    KProtocolManager::setProxyConfigScript( ui->url_autoProxy->url() );

    // Cache stuff.  TODO:needs to be separated from proxy post 2.0 (DA)
    KProtocolManager::setUseCache( ui->cb_useCache->isChecked() );
    KProtocolManager::setMaxCacheSize(ui->sb_max_cache_size->value());

    if (!ui->cb_useCache->isChecked())
	KProtocolManager::setCacheControl(KIO::CC_Reload);
    else if (ui->rb_verify->isChecked())
	KProtocolManager::setCacheControl(KIO::CC_Verify);
    else if (ui->rb_offlineMode->isChecked())
	KProtocolManager::setCacheControl(KIO::CC_CacheOnly);
    else if (ui->rb_cacheIfPossible->isChecked())
	KProtocolManager::setCacheControl(KIO::CC_Cache);

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
  ui->le_ftp_url->setText( ui->le_http_url->text() );
  ui->sb_ftp_port->setValue(ui->sb_http_port->value());
}

void KProxyOptions::setProxy()
{
  bool useProxy = ui->cb_useProxy->isChecked();
  bool autoProxy = ui->cb_autoProxy->isChecked();

  // now set all input fields
  ui->cb_autoProxy->setEnabled( useProxy );
  ui->url_autoProxy->setEnabled( useProxy && autoProxy );
  ui->le_http_url->setEnabled( useProxy && !autoProxy );
  ui->sb_http_port->setEnabled( useProxy && !autoProxy );
  ui->le_ftp_url->setEnabled( useProxy && !autoProxy );
  ui->sb_ftp_port->setEnabled( useProxy && !autoProxy );
  ui->gb_no_proxy_for->setEnabled( useProxy && !autoProxy );
  ui->pb_down->setEnabled( useProxy && !autoProxy );
  ui->cb_useProxy->setChecked( useProxy );
}

void KProxyOptions::setCache()
{
  bool useCache = ui->cb_useCache->isChecked();

  // now set all input fields
  ui->sb_max_cache_size->setEnabled( useCache );
  //pb_down->setEnabled( useCache );
  ui->cb_useCache->setChecked( useCache );
  ui->rb_verify->setEnabled(useCache);
  ui->rb_cacheIfPossible->setEnabled(useCache);
  ui->rb_offlineMode->setEnabled(useCache);
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

void KProxyOptions::slotEnableButtons()
{
  ui->pb_rmHost->setEnabled(true);
  ui->pb_editHost->setEnabled(true);
}

void KProxyOptions::slotAddHost()
{
 KAddHostDlg *dlg = new KAddHostDlg(this);
 connect(dlg, SIGNAL(sigHaveNewHost(QString)), SLOT(slotAddToList(QString)));
 dlg->exec();
}

void KProxyOptions::slotRemoveHost()
{
  if(ui->klv_no_prx->selectedItem()) {
    ui->klv_no_prx->takeItem(ui->klv_no_prx->selectedItem());
    emit changed();
  }
}

void KProxyOptions::slotEditHost()
{
 if(ui->klv_no_prx->selectedItem())
 {
   KEditHostDlg *dlg = new KEditHostDlg(ui->klv_no_prx->selectedItem()->text(0),this);
   connect(dlg, SIGNAL(sigHaveChangedHost(QString)), SLOT(slotModifyActiveListEntry(QString)));
   dlg->exec();
 }
}

void KProxyOptions::slotAddToList(QString host)
{
  ui->klv_no_prx->insertItem(new QListViewItem(ui->klv_no_prx, host));
  emit changed();
}

void KProxyOptions::slotModifyActiveListEntry(QString host)
{
  if(ui->klv_no_prx->selectedItem()) {
    ui->klv_no_prx->selectedItem()->setText(0, host);
    emit changed();
  }
}

#include "kproxydlg.moc"
