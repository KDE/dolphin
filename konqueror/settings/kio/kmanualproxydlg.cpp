/*
   kmanualproxydlg.cpp - Proxy configuration dialog

   Copyright (C) 2001,02,03 - Dawit Alemayehu <adawit@kde.org>

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
#include <qspinbox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <knuminput.h>
#include <klistview.h>
#include <klineedit.h>
#include <kicontheme.h>
#include <kurifilter.h>
#include <kmessagebox.h>
#include <kiconloader.h>
#include <kinputdialog.h>
#include <kio/ioslave_defaults.h>

#include "manualproxy_ui.h"
#include "kmanualproxydlg.h"


KManualProxyDlg::KManualProxyDlg( QWidget* parent, const char* name )
                :KProxyDialogBase( parent, name, true,
                                   i18n("Manual Proxy Configuration") )
{
    dlg = new ManualProxyDlgUI (this);
    setMainWidget (dlg);

    dlg->pbCopyDown->setPixmap( BarIcon("down", KIcon::SizeSmall) );
    QSizePolicy sizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed,
                            dlg->pbCopyDown->sizePolicy().hasHeightForWidth() );
    dlg->pbCopyDown->setSizePolicy( sizePolicy );

    init();
}

void KManualProxyDlg::init()
{
    dlg->sbHttp->setRange( 0, MAX_PORT_VALUE );
    dlg->sbHttps->setRange( 0, MAX_PORT_VALUE );
    dlg->sbFtp->setRange( 0, MAX_PORT_VALUE );

    connect( dlg->pbNew, SIGNAL( clicked() ), SLOT( newPressed() ) );
    connect( dlg->pbChange, SIGNAL( clicked() ), SLOT( changePressed() ) );
    connect( dlg->pbDelete, SIGNAL( clicked() ), SLOT( deletePressed() ) );
    connect( dlg->pbDeleteAll, SIGNAL( clicked() ), SLOT( deleteAllPressed() ) );

    connect( dlg->lvExceptions, SIGNAL(selectionChanged()), SLOT(updateButtons()) );
    connect( dlg->lvExceptions, SIGNAL(doubleClicked (QListViewItem *)), SLOT(changePressed()));

    connect( dlg->cbSameProxy, SIGNAL( toggled(bool) ), SLOT( sameProxy(bool) ) );
    connect( dlg->pbCopyDown, SIGNAL( clicked() ), SLOT( copyDown() ) );

    connect( dlg->leHttp, SIGNAL(textChanged(const QString&)), SLOT(textChanged(const QString&)) );
    connect( dlg->sbHttp, SIGNAL(valueChanged(int)), SLOT(valueChanged (int)) );
}

void KManualProxyDlg::setProxyData( const KProxyData &data )
{
    KURL u;

    QString host;
    int port = DEFAULT_PROXY_PORT;

    // Set the HTTP proxy...
    u = data.proxyList["http"];
    if (!u.isValid())
        dlg->sbHttp->setValue(port);
    else
    {
        int p = u.port();
        if ( p > 0 )
            port = p;

        u.setPort( 0 );
        host = u.url();
        dlg->leHttp->setText( host );
        dlg->sbHttp->setValue( port );
    }

    bool useSameProxy = (!dlg->leHttp->text().isEmpty () &&
                         data.proxyList["http"] == data.proxyList["https"] &&
                         data.proxyList["http"] == data.proxyList["ftp"]);

    dlg->cbSameProxy->setChecked ( useSameProxy );

    if ( useSameProxy )
    {
      dlg->leHttps->setText ( host );
      dlg->leFtp->setText ( host );
      dlg->sbHttps->setValue( port );
      dlg->sbFtp->setValue( port );
      sameProxy ( true );
    }
    else
    {
      // Set the HTTPS proxy...
      u = data.proxyList["https"];
      if (!u.isValid())
          dlg->sbHttps->setValue( DEFAULT_PROXY_PORT );
      else
      {
          int p = u.port();
          if ( p > 0 )
              port = p;

          u.setPort( 0 );
          dlg->leHttps->setText( u.url() );
          dlg->sbHttps->setValue( port );
      }

      // Set the FTP proxy...
      u = data.proxyList["ftp"];
      if (!u.isValid())
          dlg->sbFtp->setValue( DEFAULT_PROXY_PORT );
      else
      {
          int p = u.port();
          if ( p > 0 )
              port = p;

          u.setPort( 0 );

          dlg->leFtp->setText( u.url() );
          dlg->sbFtp->setValue( port );
      }
    }


    QStringList::ConstIterator it = data.noProxyFor.begin();
    for( ; it != data.noProxyFor.end(); ++it )
    {
      // "no_proxy" is a keyword used by the environment variable
      // based configuration. We ignore it here as it is not applicable...
      if ((*it).lower() != "no_proxy" && !(*it).isEmpty())
      {
        // Validate the NOPROXYFOR entries and use only hostnames if the entry is 
        // a valid or legitimate URL. NOTE: needed to catch manual manipulation
        // of the proxy config files...
        QStringList filters;
        filters << "kshorturifilter" << "localdomainfilter";
        KURL url ( *it );        
        if ( !url.isValid() )
          KURIFilter::self()->filterURI(url, filters);
          
        QString exception = (url.isValid() && !url.host().isEmpty()) ? url.host() : url.url();                
        (void) new QListViewItem( dlg->lvExceptions, exception );
      }
    }

    dlg->cbReverseProxy->setChecked( data.useReverseProxy );
}

const KProxyData KManualProxyDlg::data() const
{
    KURL u;
    KProxyData data;

    if (!m_bHasValidData)
      return data;

    u = dlg->leHttp->text();
    if ( u.isValid() )
    {
        u.setPort( dlg->sbHttp->value() );
        data.proxyList["http"] = u.url();
    }

    if ( dlg->cbSameProxy->isChecked () )
    {
        data.proxyList["https"] = data.proxyList["http"];
        data.proxyList["ftp"] = data.proxyList["http"];
    }
    else
    {
        u = dlg->leHttps->text();
        if ( u.isValid() )
        {
            u.setPort( dlg->sbHttps->value() );
            data.proxyList["https"] = u.url();
        }

        u = dlg->leFtp->text();
        if ( u.isValid() )
        {
            u.setPort( dlg->sbFtp->value() );
            data.proxyList["ftp"] = u.url();
        }
    }

    if ( dlg->lvExceptions->childCount() )
    {
        QListViewItem* item = dlg->lvExceptions->firstChild();
        for( ; item != 0L; item = item->nextSibling() )
            data.noProxyFor << item->text(0);
    }

    data.type = KProtocolManager::ManualProxy;
    data.useReverseProxy = dlg->cbReverseProxy->isChecked();

    return data;
}

void KManualProxyDlg::sameProxy( bool enable )
{
    dlg->leHttps->setEnabled (!enable );
    dlg->leFtp->setEnabled (!enable );
    dlg->sbHttps->setEnabled (!enable );
    dlg->sbFtp->setEnabled (!enable );
    dlg->pbCopyDown->setEnabled( !enable );

    if (enable)
    {
      m_oldFtpText = dlg->leFtp->text();
      m_oldHttpsText = dlg->leHttps->text();

      m_oldFtpPort = dlg->sbFtp->value();
      m_oldHttpsPort = dlg->sbHttps->value();

      int port = dlg->sbHttp->value();
      QString text = dlg->leHttp->text();

      dlg->leFtp->setText (text);
      dlg->leHttps->setText (text);

      dlg->sbFtp->setValue (port);
      dlg->sbHttps->setValue (port);
    }
    else
    {
      dlg->leFtp->setText (m_oldFtpText);
      dlg->leHttps->setText (m_oldHttpsText);

      dlg->sbFtp->setValue (m_oldFtpPort);
      dlg->sbHttps->setValue (m_oldHttpsPort);
    }
}

bool KManualProxyDlg::validate()
{
    QFont f;
    QStringList filters;

    unsigned short count = 0;
    filters << "kshorturifilter" << "localdomainfilter";

    KURL url ( dlg->leHttp->text() );

    if (url.isValid() || KURIFilter::self()->filterURI (url, filters))
    {
        dlg->leHttp->setText (url.url());
        count++;
    }
    else
    {
        f = dlg->lbHttp->font();
        f.setBold( true );
        dlg->lbHttp->setFont( f );
    }

    if ( !dlg->cbSameProxy->isChecked () )
    {
        url = dlg->leHttps->text();

        if (url.isValid() || KURIFilter::self()->filterURI (url, filters))
        {
            dlg->leHttps->setText (url.url());
            count++;
        }
        else
        {
            f = dlg->lbHttps->font();
            f.setBold( true );
            dlg->lbHttps->setFont( f );
        }

        url = dlg->leFtp->text();

        if (url.isValid() || KURIFilter::self()->filterURI (url, filters))
        {
            dlg->leFtp->setText (url.url());
            count++;
        }
        else
        {
            f = dlg->lbFtp->font();
            f.setBold( true );
            dlg->lbFtp->setFont( f );
        }
    }

    if ( count == 0 )
    {
        QString msg = i18n("You must specify at least one valid proxy address.");

        QString details = i18n("<qt>Make sure that you have specified at least one "
                               "valid proxy address, eg. <b>http://192.168.1.20</b>."
                               "</qt>");

        KMessageBox::detailedError( this, msg, details,
                                    i18n("Invalid Proxy Setup") );
    }

    return (count > 0);
}

void KManualProxyDlg::textChanged(const QString& text)
{
    if (!dlg->cbSameProxy->isChecked())
        return;

    dlg->leFtp->setText (text);
    dlg->leHttps->setText (text);
}

void KManualProxyDlg::valueChanged(int value)
{
    if (!dlg->cbSameProxy->isChecked())
        return;

    dlg->sbFtp->setValue (value);
    dlg->sbHttps->setValue (value);
 }

void KManualProxyDlg::copyDown()
{
    int action = -1;

    if ( !dlg->leHttp->text().isEmpty() )
        action += 4;
    else if ( !dlg->leHttps->text().isEmpty() )
        action += 3;

    switch ( action )
    {
      case 3:
        dlg->leHttps->setText( dlg->leHttp->text() );
        dlg->sbHttps->setValue( dlg->sbHttp->value() );
        dlg->leFtp->setText( dlg->leHttp->text() );
        dlg->sbFtp->setValue( dlg->sbHttp->value() );
        break;
      case 2:
        dlg->leFtp->setText( dlg->leHttps->text() );
        dlg->sbFtp->setValue( dlg->sbHttps->value() );
        break;
      case 1:
      case 0:
      default:
        break;
    }
}

void KManualProxyDlg::slotOk()
{
    if ( m_bHasValidData || validate() )
    {
      KDialogBase::slotOk();
      m_bHasValidData = true;
    }
}

bool KManualProxyDlg::handleDuplicate( const QString& site )
{
    QListViewItem* item = dlg->lvExceptions->firstChild();
    while ( item != 0 )
    {
        if ( item->text(0).findRev( site ) != -1 &&
             item != dlg->lvExceptions->currentItem() )
        {
            QString msg = i18n("<qt><center><b>%1</b><br/>"
                               "already exists!").arg(site);
            KMessageBox::error( this, msg, i18n("Duplicate Exception") );
            return true;
        }
        item = item->nextSibling();
    }
    return false;
}


void KManualProxyDlg::newPressed()
{
  QString msg;

  // Specify the appropriate message...
  if ( dlg->cbReverseProxy->isChecked() )
      msg = i18n("Enter the address or URL for which the above proxy server "
                 "should be used:");
  else
      msg = i18n("Enter the address or URL that should be excluded from using "
                 "the above proxy server:");

  KURL result ( KInputDialog::getText (i18n("New Exception"), msg) );
  
  // If the user pressed cancel, do nothing...
  if (result.isEmpty())
    return;
  
  QStringList filters;
  filters << "kshorturifilter" << "localdomainfilter";
  
  // If the typed URL is malformed, attempt to filter it in order
  // to check its validity...
  if ( !result.isValid() )
    KURIFilter::self()->filterURI(result, filters);
    
  QString exception = (result.isValid() && !result.host().isEmpty()) ? result.host() : result.url();
  
  if ( !handleDuplicate( exception ) )
  {
    QListViewItem* index = new QListViewItem( dlg->lvExceptions, exception );
    dlg->lvExceptions->setCurrentItem( index );
  }
}

void KManualProxyDlg::changePressed()
{
    if( !dlg->lvExceptions->currentItem() )
        return;

    QString msg;

    // Specify the appropriate message...
    if ( dlg->cbReverseProxy->isChecked() )
        msg = i18n("Enter the address or URL for which the above proxy server "
                   "should be used:");
    else
        msg = i18n("Enter the address or URL that should be excluded from using "
                   "the above proxy server:");

    QString currentItem = dlg->lvExceptions->currentItem()->text(0);
    QString result = KInputDialog::getText ( i18n("Change Exception"), msg, currentItem );
    if ( !result.isNull() )
    {
        if ( !handleDuplicate( result ) )
        {
            QListViewItem* index = dlg->lvExceptions->currentItem();
            index->setText( 0, result );
            dlg->lvExceptions->setCurrentItem( index );
        }
    }
}

void KManualProxyDlg::deletePressed()
{
    QListViewItem* item = dlg->lvExceptions->selectedItem()->itemBelow();

    if ( !item )
        item = dlg->lvExceptions->selectedItem()->itemAbove();

    delete dlg->lvExceptions->selectedItem();

    if ( item )
        dlg->lvExceptions->setSelected( item, true );

    updateButtons();
}

void KManualProxyDlg::deleteAllPressed()
{
    dlg->lvExceptions->clear();
    updateButtons();
}

void KManualProxyDlg::updateButtons()
{
    bool hasItems = dlg->lvExceptions->childCount() > 0;
    bool itemSelected = ( hasItems && dlg->lvExceptions->selectedItem()!=0 );
    dlg->pbDelete->setEnabled( itemSelected );
    dlg->pbDeleteAll->setEnabled( hasItems );
    dlg->pbChange->setEnabled( itemSelected );
}

#include "kmanualproxydlg.moc"
