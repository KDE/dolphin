/* This file is part of the KDE project
   Copyright (c) 2005 Pascal Létourneau <pascal.letourneau@kdemail.net>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_part.h"

#include <QApplication>

#include <kaboutdata.h>
#include <kdirlister.h>
#include <konq_filetip.h>
#include <kparts/genericfactory.h>

#include "konq_model.h"
#include "konq_selectionmodel.h"
#include "konq_iconview.h"
#include "konq_listview.h"

K_EXPORT_COMPONENT_FACTORY( konq_part, KonqListFactory )

KonqPart::KonqPart( QWidget* parentWidget, const char*, QObject* parent, const char* name, const QStringList& )
        : KonqDirPart( parent, name )
        ,m_dirLister( new KDirLister() )
        ,m_view( /*new KonqIconView( parentWidget )*/ new KonqListView( parentWidget ) )
        ,m_model( new KonqModel( parent ) )
        ,m_fileTip( new KonqFileTip( 0 /* m_view*/ ) )
{
    setInstance( KonqListFactory::instance() );
    setBrowserExtension( new KonqDirPartBrowserExtension( this ) );
    setWidget( m_view );
    setDirLister( m_dirLister );
    setXMLFile( "konq_listview.rc" );
    m_dirLister->setMainWindow( widget()->topLevelWidget() );
    m_view->setModel( m_model );
    m_view->setSelectionModel( new KonqSelectionModel( m_model ) );
    m_fileTip->setOptions( true, true, 6 );

    connect( m_dirLister, SIGNAL( newItems(const KFileItemList&) ),
             SLOT( slotNewItems(const KFileItemList&) ) );
    connect( m_dirLister, SIGNAL( clear() ), SLOT( slotClear() ) );
    connect( m_view, SIGNAL( execute(QMouseEvent*) ),
             SLOT( slotExecute(QMouseEvent*) ) );
    connect( m_view, SIGNAL( toolTip(const QModelIndex&) ),
             SLOT( slotToolTip(const QModelIndex&) ) );
    connect( m_view, SIGNAL( contextMenu(const QPoint&,const QModelIndexList&) ),
             SLOT( slotContextMenu(const QPoint&,const QModelIndexList&) ) );
}

KonqPart::~KonqPart()
{
    delete m_dirLister;
}

KAboutData* KonqPart::createAboutData()
{
    return new KAboutData( "konq_part", I18N_NOOP( "KonqPart" ), "0.1" );
}

void KonqPart::slotNewItems( const KFileItemList& items )
{
    m_model->appendFileItems( items );
    m_model->insertRows( m_model->rowCount(), items.count() );
    newItems( items );
}

void KonqPart::slotCompleted()
{
    emit completed();
}

void KonqPart::slotClear()
{
    resetCount();
    m_model->clearFileItems();
    m_model->removeRows( 0, m_model->rowCount()-1 );
}

const KFileItem* KonqPart::currentItem()
{
    return m_model->fileItem( m_view->currentIndex() );
}

bool KonqPart::doOpenURL( const KURL& url )
{
    emit setWindowCaption( url.pathOrURL() );
    KParts::URLArgs args = extension()->urlArgs();

    m_dirLister->openURL( url, false, args.reload );
    return true;
}

void KonqPart::slotExecute( QMouseEvent* ev )
{
    if ( ev->button() == Qt::LeftButton )
        lmbClicked( m_model->fileItem( m_view->indexAt( ev->pos() ) ) );
    else if ( ev->button() == Qt::MidButton )
        mmbClicked( m_model->fileItem( m_view->indexAt( ev->pos() ) ) );
}

void KonqPart::slotToolTip( const QModelIndex& index )
{
    m_fileTip->setItem( m_model->fileItem( index ) );
}

void KonqPart::slotContextMenu( const QPoint& pos, const QModelIndexList& indexes )
{
    KFileItemList items;
    if ( indexes.isEmpty() )
        items.append( m_dirLister->rootItem() );
    else
        foreach( QModelIndex index, indexes )
            if ( index.column() == 0 )
                items.append( m_model->fileItem( index ) );
    emit extension()->popupMenu( pos, items );
}
