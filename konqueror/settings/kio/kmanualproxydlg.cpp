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
#include <klistbox.h>
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

    connect( dlg->lbExceptions, SIGNAL(selectionChanged()), SLOT(updateButtons()) );
    connect( dlg->lbExceptions, SIGNAL(doubleClicked (QListBoxItem *)), SLOT(changePressed()));

    connect( dlg->cbSameProxy, SIGNAL( toggled(bool) ), SLOT( sameProxy(bool) ) );
    connect( dlg->pbCopyDown, SIGNAL( clicked() ), SLOT( copyDown() ) );

    connect( dlg->leHttp, SIGNAL(textChanged(const QString&)), SLOT(textChanged(const QString&)) );
    connect( dlg->sbHttp, SIGNAL(valueChanged(int)), SLOT(valueChanged (int)) );
}

void KManualProxyDlg::setProxyData( const KProxyData &data )
{
    KURL url;
        
    // Set the HTTP proxy...
    if (!isValidURL(data.proxyList["http"], &url))
        dlg->sbHttp->setValue( DEFAULT_PROXY_PORT );
    else
    {
        int port = url.port();
        if ( port <= 0 )
            port = DEFAULT_PROXY_PORT;
            
        url.setPort( 0 );
        dlg->leHttp->setText( url.url() );
        dlg->sbHttp->setValue( port );
    }

    bool useSameProxy = (!dlg->leHttp->text().isEmpty () &&
                         data.proxyList["http"] == data.proxyList["https"] &&
                         data.proxyList["http"] == data.proxyList["ftp"]);

    dlg->cbSameProxy->setChecked ( useSameProxy );

    if ( useSameProxy )
    {
      dlg->leHttps->setText ( dlg->leHttp->text() );
      dlg->leFtp->setText ( dlg->leHttp->text() );
      dlg->sbHttps->setValue( dlg->sbHttp->value() );
      dlg->sbFtp->setValue( dlg->sbHttp->value() );

      sameProxy ( true );
    }
    else
    {
      // Set the HTTPS proxy...
      if( !isValidURL( data.proxyList["https"], &url ) )
          dlg->sbHttps->setValue( DEFAULT_PROXY_PORT );
      else
      {
          int port = url.port();
          if ( port <= 0 )
              port = DEFAULT_PROXY_PORT;

          url.setPort( 0 );
          dlg->leHttps->setText( url.url() );
          dlg->sbHttps->setValue( port );
      }

      // Set the FTP proxy...
      if( !isValidURL( data.proxyList["ftp"], &url ) )
          dlg->sbFtp->setValue( DEFAULT_PROXY_PORT );
      else
      {
          int port = url.port();
          if ( port <= 0 )
              port = DEFAULT_PROXY_PORT;

          url.setPort( 0 );
          dlg->leFtp->setText( url.url() );
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
        if( isValidURL( *it ) )
          dlg->lbExceptions->insertItem( *it );
      }
    }

    dlg->cbReverseProxy->setChecked( data.useReverseProxy );
}

const KProxyData KManualProxyDlg::data() const
{
    KProxyData data;

    if (!m_bHasValidData)
      return data;

    data.proxyList["http"] = urlFromInput( dlg->leHttp, dlg->sbHttp );
    
    if ( dlg->cbSameProxy->isChecked () )
    {
        data.proxyList["https"] = data.proxyList["http"];
        data.proxyList["ftp"] = data.proxyList["http"];
    }
    else
    {
        data.proxyList["https"] = urlFromInput( dlg->leHttps, dlg->sbHttps );
        data.proxyList["ftp"] = urlFromInput( dlg->leFtp, dlg->sbFtp );
    }

    if ( dlg->lbExceptions->count() )
    {
        QListBoxItem* item = dlg->lbExceptions->firstItem();
        for( ; item != 0L; item = item->next() )
            data.noProxyFor << item->text();
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
      
      if (dlg->lbHttps->font().bold())
        setHighLight( dlg->lbHttps, false );
      
      if (dlg->lbFtp->font().bold())
        setHighLight( dlg->lbFtp, false );
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
    KURL filteredURL;
    unsigned short count = 0;

    if ( isValidURL( dlg->leHttp->text(), &filteredURL ) )
    {
        dlg->leHttp->setText( filteredURL.url() );
        count++;
    }
    else
        setHighLight( dlg->lbHttp, true );

    if ( !dlg->cbSameProxy->isChecked () )
    {
        if ( isValidURL( dlg->leHttps->text(), &filteredURL ) )
        {
            dlg->leHttps->setText( filteredURL.url() );
            count++;
        }
        else
            setHighLight( dlg->lbHttps, true );

        if ( isValidURL( dlg->leFtp->text(), &filteredURL ) )
        {
            dlg->leFtp->setText( filteredURL.url() );
            count++;
        }
        else
            setHighLight( dlg->lbFtp, true );
    }

    if ( count == 0 )
    {
        showErrorMsg( i18n("Invalid Proxy Setting"),
                      i18n("One or more of the specified proxy settings are "
                           "invalid. The incorrect entries are highlighted.") );
    }

    return (count > 0);
}

void KManualProxyDlg::textChanged(const QString& text)
{
    if (!dlg->cbSameProxy->isChecked())
        return;

    dlg->leFtp->setText( text );
    dlg->leHttps->setText( text );
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
    //qDebug("m_bHasValidData: %s" , m_bHasValidData ? "true" : "false");
    if ( m_bHasValidData || validate() )
    {
      KDialogBase::slotOk();
      m_bHasValidData = true;
    }
}

bool KManualProxyDlg::handleDuplicate( const QString& site )
{
    QListBoxItem* item = dlg->lbExceptions->firstItem();
    while ( item != 0 )
    {
        if ( item->text().findRev( site ) != -1 &&
             item != dlg->lbExceptions->selectedItem() )
        {
            QString msg = i18n("You entered a duplicate address. "
                               "Please try again.");
            QString details = i18n("<qt><center><b>%1</b></center> "
                                   "is already in the list.</qt>").arg(site);
            KMessageBox::detailedError( this, msg, details, i18n("Duplicate Entry") );
            return true;
        }
        
        item = item->next();
    }
    return false;
}

void KManualProxyDlg::newPressed()
{
  QString result;
  if( getException(result, i18n("New Exception")) && !handleDuplicate(result) )
    dlg->lbExceptions->insertItem( result );
}

void KManualProxyDlg::changePressed()
{
  QString result;
  if( getException( result, i18n("Change Exception"), 
                    dlg->lbExceptions->currentText() ) &&
      !handleDuplicate( result ) )
      dlg->lbExceptions->changeItem( result, dlg->lbExceptions->currentItem() );
}

void KManualProxyDlg::deletePressed()
{
    dlg->lbExceptions->removeItem( dlg->lbExceptions->currentItem() );
    dlg->lbExceptions->setSelected( dlg->lbExceptions->currentItem(), true );
    updateButtons();
}

void KManualProxyDlg::deleteAllPressed()
{
    dlg->lbExceptions->clear();
    updateButtons();
}

void KManualProxyDlg::updateButtons()
{
    bool hasItems = dlg->lbExceptions->count() > 0;
    bool itemSelected = (hasItems && dlg->lbExceptions->selectedItem()!=0);
    
    dlg->pbDeleteAll->setEnabled( hasItems );    
    dlg->pbDelete->setEnabled( itemSelected );
    dlg->pbChange->setEnabled( itemSelected );
}

QString KManualProxyDlg::urlFromInput(const KLineEdit* edit, 
                                      const QSpinBox* spin) const
{
  if (!edit)
    return QString::null;
    
  KURL u( edit->text() );
  
  if (spin)
    u.setPort( spin->value() );
  
  return u.url();
}

bool KManualProxyDlg::isValidURL( const QString& _url, KURL* result ) const
{
    KURL url (_url);
    
    QStringList filters;
    filters << "kshorturifilter" << "localdomainfilter";
    
    // If the typed URL is malformed, and the filters cannot filter it
    // then it must be an invalid entry, 
    if( !(url.isValid() || KURIFilter::self()->filterURI(url, filters)) && 
        !url.hasHost() )
      return false;
      
    QString host (url.host());
    
    // We only check for a relevant subset of characters that are 
    // not allowed in <authority> component of a URL.
    if ( host.contains ('*') || host.contains (' ') || host.contains ('?') )
      return false;
      
    if ( result )
      *result = url;
          
    return true;
}

void KManualProxyDlg::showErrorMsg( const QString& caption, 
                                    const QString& message )
{
  QString cap( caption );
  QString msg( message );
   
  if ( cap.isNull() )
    cap = i18n("Invalid Entry");
  
  if ( msg.isNull() )
    msg = i18n("The address you have entered is not valid.");
  
  QString details = i18n("<qt>Make sure none of the addresses or URLs you "
                          "specified contain invalid or wildcard characters "
                          "such as spaces, asteriks(*) or question marks(?).<p>"
                          "<u>Examples of VALID entries:</u><br/>"
                          "<code>http://mycompany.com, 192.168.10.1, "
                          "mycompany,com, localhost, http://localhost</code><p>"
                          "<u>Examples of INVALID entries:</u><br/>"
                          "<code>http://my company.com, http:/mycompany,com "
                          "file:/localhost</code>");
  
  KMessageBox::detailedError( this, msg, details, cap );
}

bool KManualProxyDlg::getException ( QString& result,
                                     const QString& caption, 
                                     const QString& value )
{
    QString label;
    
    // Specify the appropriate message...
    if ( dlg->cbReverseProxy->isChecked() )
      label = i18n("Enter the URL or address that should use the above proxy "
                  "settings:");
    else
      label = i18n("Enter the address or URL that should be excluded from "
                  "using the above proxy settings:");
    
    QString whatsThis = i18n("<qt>Enter a valid address or url.<p>"
                            "<b><u>NOTE:</u></b> Wildcard matching such as "
                            "<code>*.kde.org</code> is not supported. If you want "
                            "to match any host in the <code>.kde.org</code> domain, "
                            "e.g. <code>printing.kde.org</code>, then simply enter "
                            "<code>.kde.org</code></qt>");
                            
    bool ok;
    result = KInputDialog::text( caption, label, value, &ok, 0, 0, 0, 
                                QString::null, whatsThis );
    
    // If the user pressed cancel, do nothing...
    if (!ok)
      return false;
    
    // If the typed URL is malformed, and the filters cannot filter it
    // then it must be an invalid entry, 
    if( isValidURL(result) )
      return true;
    
    showErrorMsg();
    return false;
}

#include "kmanualproxydlg.moc"
