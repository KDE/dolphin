/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kserviceselectdlg.h"
#include "kserviceselectdlg.moc"
#include "kservicelistwidget.h"

#include <klocale.h>
#include <kvbox.h>
#include <QLabel>

KServiceSelectDlg::KServiceSelectDlg( const QString& /*serviceType*/, const QString& /*value*/, QWidget *parent )
    : KDialog( parent )
{
    setObjectName( "serviceSelectDlg" );
    setModal( true );
    setCaption( i18n( "Add Service" ) );
    setButtons( Ok | Cancel );

    KVBox *vbox = new KVBox ( this );

    vbox->setSpacing( KDialog::spacingHint() );
    new QLabel( i18n( "Select service:" ), vbox );
    m_listbox=new KListBox( vbox );

    // Can't make a KTrader query since we don't have a servicetype to give,
    // we want all services that are not applications.......
    // So we have to do it the slow way
    // ### Why can't we query for KParts/ReadOnlyPart as the servicetype? Should work fine!
    KService::List allServices = KService::allServices();
    KService::List::const_iterator it(allServices.begin());
    for ( ; it != allServices.end() ; ++it )
      if ( (*it)->hasServiceType( "KParts/ReadOnlyPart" ) )
      {
          m_listbox->insertItem( new KServiceListItem( (*it), KServiceListWidget::SERVICELIST_SERVICES ) );
      }

    m_listbox->sort();
    m_listbox->setMinimumHeight(350);
    m_listbox->setMinimumWidth(300);
    connect(m_listbox,SIGNAL(doubleClicked ( Q3ListBoxItem * )),SLOT(slotOk()));
    setMainWidget(vbox);
}

KServiceSelectDlg::~KServiceSelectDlg()
{
}

KService::Ptr KServiceSelectDlg::service()
{
    unsigned int selIndex = m_listbox->currentItem();
    KServiceListItem *selItem = static_cast<KServiceListItem *>(m_listbox->item(selIndex));
    return KService::serviceByDesktopPath( selItem->desktopPath );
}
