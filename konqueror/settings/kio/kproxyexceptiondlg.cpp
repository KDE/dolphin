/*
   kproxyexceptiondlg.h - Proxy exception dialog

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   (GPL) version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write
   to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <qlabel.h>
#include <qlayout.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>
#include <qpushbutton.h>

#include <kdebug.h>
#include <klocale.h>
#include <klistview.h>
#include <klineedit.h>
#include <kmessagebox.h>

#include "kproxyexceptiondlg.h"

KProxyExceptionDlg::KProxyExceptionDlg( QWidget* parent,  const char* name,
                                        bool modal, const QString &caption)
                   :KDialogBase( parent, name, modal, caption, Ok|Cancel )
{
    QWidget *page = new QWidget( this );
    setMainWidget (page);

    QVBoxLayout* mainLayout = new QVBoxLayout( page, KDialog::marginHint(),
                                               KDialog::spacingHint() );

    QLabel* label = new QLabel( i18n("Enter address (URL) that should be "
                                     "excluded from using a proxy server:"),
                                page, "lb_excpetions" );
    label->setSizePolicy( QSizePolicy( QSizePolicy::Preferred,
                                       QSizePolicy::Fixed,
                                       label->sizePolicy().hasHeightForWidth() ) );
    label->setAlignment( (QLabel::WordBreak | QLabel::AlignTop |
                          QLabel::AlignLeft) );
    mainLayout->addWidget( label );

    m_leExceptions = new KLineEdit( page, "m_leExceptions" );
    m_leExceptions->setMinimumWidth( m_leExceptions->fontMetrics().width('W') * 20 );
    m_leExceptions->setSizePolicy( QSizePolicy( QSizePolicy::Preferred,
                                               QSizePolicy::Fixed,
                                               m_leExceptions->sizePolicy().hasHeightForWidth() ) );
    QWhatsThis::add( m_leExceptions, i18n("<qt>Enter the site name(s) that "
                                         "should be exempted from using the "
                                         "proxy server(s) specified above.<p>"
                                         "Note that the reverse is true if "
                                         "the \"<code>Only use proxy for "
                                         "entries in this list</code>\" "
                                         "box is checked. That is the proxy "
                                         "server will only be used for "
                                         "addresses that match one of the "
                                         "items in this list.") );
    mainLayout->addWidget( m_leExceptions );
    QSpacerItem* spacer = new QSpacerItem( 16, 16, QSizePolicy::Minimum,
                                           QSizePolicy::Fixed );
    mainLayout->addItem( spacer );

    m_leExceptions->setFocus();
    connect( m_leExceptions, SIGNAL( textChanged( const QString& ) ),
             SLOT( slotTextChanged( const QString& ) ) );
}

KProxyExceptionDlg::~KProxyExceptionDlg()
{
}

void KProxyExceptionDlg::slotTextChanged( const QString& )
{
}

QString KProxyExceptionDlg::exception() const
{
    return m_leExceptions->text();
}

void KProxyExceptionDlg::setException( const QString& text )
{
    if ( !text.isEmpty() )
        m_leExceptions->setText( text );
}


KExceptionBox::KExceptionBox( QWidget* parent, const char* name )
              :QGroupBox( parent, name )
{
    setTitle( i18n("Exceptions") );
    QVBoxLayout* mainLayout = new QVBoxLayout( this,
                                               KDialog::marginHint(),
                                               KDialog::spacingHint() );
    mainLayout->setAlignment( Qt::AlignTop );

    QHBoxLayout* hlay = new QHBoxLayout;
    hlay->setSpacing( KDialog::spacingHint() );
    hlay->setMargin( KDialog::marginHint() );

    m_cbReverseproxy = new QCheckBox( i18n("Only use proxy for entries "
                                          "in this list"), this,
                                          "m_cbReverseproxy" );
    QWhatsThis::add( m_cbReverseproxy, i18n("<qt>Check this box to reverse the "
                                           "use of the exception list. i.e. "
                                           "Checking this box will result "
                                           "in the proxy servers being used "
                                           "only when the requested URL matches "
                                           "one of the addresses listed here."
                                           "<p>This feature is useful if all "
                                           "you want or need is to use a proxy "
                                           "server  for a few specific sites.<p>"
                                           "If you have more complex "
                                           "requirements you might want to use "
                                           "a configuration script</qt>") );
    hlay->addWidget( m_cbReverseproxy );
    QSpacerItem* spacer = new QSpacerItem( 20, 20, QSizePolicy::Expanding,
                              QSizePolicy::Minimum );
    hlay->addItem( spacer );
    mainLayout->addLayout( hlay );

    QGridLayout* glay = new QGridLayout;
    glay->setSpacing( KDialog::spacingHint() );
    glay->setMargin( 0 );

    QVBoxLayout* vlay = new QVBoxLayout;
    vlay->setSpacing( KDialog::spacingHint() );
    vlay->setMargin( 0 );

    m_pbNew = new QPushButton( i18n("&New..."), this, "m_pbNew" );
    connect( m_pbNew, SIGNAL( clicked() ), SLOT( newPressed() ) );
    QWhatsThis::add( m_pbNew, i18n("Click this to add an address that should "
                                  "be exempt from using or forced to use, "
                                  "depending on the check box above, a proxy "
                                  "server.") );
    m_pbChange = new QPushButton( i18n("C&hange..."), this,
                                  "m_pbChange" );
    connect( m_pbChange, SIGNAL( clicked() ), SLOT( changePressed() ) );
    m_pbChange->setEnabled( false );
    QWhatsThis::add( m_pbChange, i18n("Click this button to change the "
                                     "selected exception address.") );
    m_pbDelete = new QPushButton( i18n("De&lete"), this,
                                  "m_pbDelete" );
    connect( m_pbDelete, SIGNAL( clicked() ), SLOT( deletePressed() ) );
    m_pbDelete->setEnabled( false );
    QWhatsThis::add( m_pbDelete, i18n("Click this button to delete the "
                                     "selected address.") );
    m_pbDeleteAll = new QPushButton( i18n("D&elete All"), this,
                                     "m_pbDeleteAll" );
    connect( m_pbDeleteAll, SIGNAL( clicked() ), SLOT( deleteAllPressed() ) );
    m_pbDeleteAll->setEnabled( false );
    QWhatsThis::add( m_pbDeleteAll, i18n("Click this button to delete all "
                                     "the address in the exception list.") );
    vlay->addWidget( m_pbNew );
    vlay->addWidget( m_pbChange );
    vlay->addWidget( m_pbDelete );
    vlay->addWidget( m_pbDeleteAll );

    glay->addLayout( vlay, 0, 1 );

    m_lvExceptions = new KListView( this, "m_lvExceptions" );
    connect( m_lvExceptions, SIGNAL(selectionChanged()),
             SLOT(updateButtons()) );
    m_lvExceptions->addColumn( i18n("Address") );
    QWhatsThis::add( m_lvExceptions, i18n("<qt>Contains a list of addresses "
                                          "that should either bypass the use "
                                          "of the proxy server(s) specified "
                                          "above or use these servers based "
                                          "on the state of the <tt>\"Only "
                                          "use proxy for entries in the "
                                          "list\"</tt> checkbox above.<p>"
                                          "If the box is checked, only "
                                          "URLs that match the addresses "
                                          "listed here will be sent through "
                                          "the proxy server(s) shown above. "
                                          "Otherwise, the proxy servers are "
                                          "bypassed for this list.</qt>") );

    glay->addMultiCellWidget( m_lvExceptions, 0, 1, 0, 0 );
    spacer = new QSpacerItem( 20, 20, QSizePolicy::Minimum,
                              QSizePolicy::MinimumExpanding );
    glay->addItem( spacer, 1, 1 );
    mainLayout->addLayout( glay );
}

bool KExceptionBox::handleDuplicate( const QString& site )
{
    QListViewItem* item = m_lvExceptions->firstChild();
    while ( item != 0 )
    {
        if ( item->text(0).findRev( site ) != -1 &&
             item != m_lvExceptions->currentItem() )
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

void KExceptionBox::newPressed()
{
    KProxyExceptionDlg* dlg = new KProxyExceptionDlg( this );
    dlg->setCaption( i18n("New Exception") );
    if ( dlg->exec() == QDialog::Accepted )
    {
        QString exception = dlg->exception();
        if ( !handleDuplicate( exception ) )
        {
            QListViewItem* index = new QListViewItem( m_lvExceptions,
                                                      exception );
            m_lvExceptions->setCurrentItem( index );
        }
    }
    delete dlg;
}

void KExceptionBox::changePressed()
{
    KProxyExceptionDlg* dlg = new KProxyExceptionDlg( this );
    dlg->setCaption( i18n("Change Exception") );
    QString currentItem = m_lvExceptions->currentItem()->text(0);
    dlg->setException( currentItem );
    if ( dlg->exec() == QDialog::Accepted )
    {
        QString exception = dlg->exception();
        if ( !handleDuplicate( exception ) )
        {
            QListViewItem* index = m_lvExceptions->currentItem();
            index->setText( 0, exception );
            m_lvExceptions->setCurrentItem( index );
        }
    }
    delete dlg;
}

void KExceptionBox::deletePressed()
{
    QListViewItem* item = m_lvExceptions->selectedItem()->itemBelow();
    
    if ( !item )
        item = m_lvExceptions->selectedItem()->itemAbove();

    delete m_lvExceptions->selectedItem();

    if ( item )
        m_lvExceptions->setSelected( item, true );

    updateButtons();
}

void KExceptionBox::deleteAllPressed()
{
    m_lvExceptions->clear();
    updateButtons();
}

QStringList KExceptionBox::exceptions() const
{
    QStringList list;
    if ( m_lvExceptions->childCount() )
    {
        QListViewItem* item = m_lvExceptions->firstChild();
        for( ; item != 0L; item = item->nextSibling() )
            list << item->text(0);
    }
    return list;
}

void KExceptionBox::fillExceptions( const QStringList& items )
{
    QStringList::ConstIterator it = items.begin();
    for( ; it != items.end(); ++it )
      (void) new QListViewItem( m_lvExceptions, (*it) );
}

bool KExceptionBox::isReverseProxyChecked() const
{
    return m_cbReverseproxy->isChecked();
}

void KExceptionBox::setCheckReverseProxy(bool checked)
{
    m_cbReverseproxy->setChecked(checked);
}

void KExceptionBox::updateButtons()
{
    bool hasItems = m_lvExceptions->childCount() > 0;
    bool itemSelected = ( hasItems && m_lvExceptions->selectedItem()!=0 );
    m_pbDelete->setEnabled( itemSelected );
    m_pbDeleteAll->setEnabled( hasItems );
    m_pbChange->setEnabled( itemSelected );
}

#include "kproxyexceptiondlg.moc"
