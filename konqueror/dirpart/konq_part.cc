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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_part.h"

#include <QApplication>

#include <kaboutdata.h>
#include <kdirlister.h>
#include <konq_filetip.h>
#include <konq_settings.h>
#include <kparts/genericfactory.h>
#include <kdirmodel.h>

#include "konq_selectionmodel.h"
#include "konq_iconview.h"
#include "konq_listview.h"

K_EXPORT_COMPONENT_FACTORY( konq_part, KonqFactory )

KonqPart::KonqPart( QWidget* parentWidget, QObject* parent, const QStringList& args )
        : KonqDirPart( parent )
        ,m_model( new KDirModel( this ) )
        ,m_fileTip( new KonqFileTip( 0 /* m_view*/ ) )
{
    KonqFMSettings* settings = KonqFMSettings::settings();
    setInstance( KonqFactory::instance() );
    setBrowserExtension( new KonqDirPartBrowserExtension( this ) );
    setDirLister( m_model->dirLister() );

    QString mode = args.first();
    if ( mode == "DetailedList" ) {
        m_view = new KonqListView( parentWidget );
        setXMLFile( "konq_listview.rc" );
        m_view->setModel( m_model );
        m_view->setSelectionModel( new KonqSelectionModel( m_model ) );
    }
    else /*if ( mode == "Icon" )*/ {
        m_view = new KonqIconView( parentWidget );
        setXMLFile( "konq_iconview.rc" );
        m_view->setModel( m_model );
    }
    QFont font( settings->standardFont() );
    QColor color = settings->normalTextColor();
    m_view->setFont( font );
#if 0 // TODO
    font.setUnderline( settings->underlineLink() );
    m_model->setItemFont( font );
    m_model->setItemColor( color );
#endif

    setWidget( m_view );

    m_model->dirLister()->setMainWindow( widget()->topLevelWidget() );
    m_fileTip->setOptions( settings->showFileTips(), settings->showPreviewsInFileTips(), settings->numFileTips() );

    connect( m_model->dirLister(), SIGNAL( newItems(const KFileItemList&) ),
             SLOT( slotNewItems(const KFileItemList&) ) );
    connect( m_model->dirLister(), SIGNAL( clear() ), SLOT( slotClear() ) );

    connect( m_view, SIGNAL( execute(const QModelIndex&, Qt::MouseButton) ),
             SLOT( slotExecute(const QModelIndex&, Qt::MouseButton) ) );
    connect( m_view, SIGNAL( toolTip(const QModelIndex&) ),
             SLOT( slotToolTip(const QModelIndex&) ) );
    connect( m_view, SIGNAL( contextMenu(const QPoint&,const QModelIndexList&) ),
             SLOT( slotContextMenu(const QPoint&,const QModelIndexList&) ) );

    connect( m_view->selectionModel(), SIGNAL( selectionChanged(const QItemSelection&,const QItemSelection&) ),
             SLOT( slotUpdateActions() ) );
}

KonqPart::~KonqPart()
{
}

KAboutData* KonqPart::createAboutData()
{
    return new KAboutData( "konq_part", I18N_NOOP( "KonqPart" ), "0.1" );
}

void KonqPart::slotNewItems( const KFileItemList& items )
{
    newItems( items );
    KIO::PreviewJob* job = KIO::filePreview( items, 128 );
    connect( job, SIGNAL( gotPreview(const KFileItem*,const QPixmap&) ),
                  SLOT( slotPreview(const KFileItem*,const QPixmap&) ) );
}

void KonqPart::slotCompleted()
{
    emit completed();
}

void KonqPart::slotClear()
{
    kDebug() << k_funcinfo << endl;
    resetCount();
}

const KFileItem* KonqPart::currentItem()
{
    return m_model->itemForIndex( m_view->currentIndex() );
}

bool KonqPart::doOpenURL( const KUrl& url )
{
    emit setWindowCaption( url.pathOrUrl() );
    KParts::URLArgs args = extension()->urlArgs();

    m_model->dirLister()->openUrl( url, false, args.reload );
    return true;
}

void KonqPart::slotExecute( const QModelIndex& index, Qt::MouseButton mb )
{
    KFileItem* item = m_model->itemForIndex( index );
    if ( !item )
        return;

    if ( mb == Qt::LeftButton || mb == Qt::NoButton )
        lmbClicked( item );
    else if ( mb == Qt::MidButton )
        mmbClicked( item );
}

void KonqPart::slotToolTip( const QModelIndex& index )
{
    m_fileTip->setItem( m_model->itemForIndex( index ) );
}

void KonqPart::slotContextMenu( const QPoint& pos, const QModelIndexList& indexes )
{
    KFileItemList items;
    if ( indexes.isEmpty() )
        items.append( m_model->itemForIndex( QModelIndex() ) ); // root item
    else
        foreach( QModelIndex index, indexes )
            if ( index.column() == 0 )
                items.append( m_model->itemForIndex( index ) );
    emit extension()->popupMenu( pos, items );
}

void KonqPart::slotUpdateActions()
{
    bool canDelete = true;

    QModelIndexList indexes = static_cast<KonqListView*>(m_view)->selectedIndexes(); // ### FIXME
    foreach ( QModelIndex index, indexes ) {
       // ### TODO
    }
    emit extension()->enableAction( "copy", !indexes.isEmpty() );
    emit extension()->enableAction( "cut", canDelete );
    emit extension()->enableAction( "trash", canDelete );
    emit extension()->enableAction( "del", canDelete );
    emit extension()->enableAction( "properties", !indexes.isEmpty() );
    emit extension()->enableAction( "editMimeType", indexes.count() == 1 );
    emit extension()->enableAction( "rename", indexes.count() == 1 );

}

void KonqPart::slotPreview( const KFileItem* item, const QPixmap& pixmap )
{
#if 0 // TODO
    //m_model->addPreview( item, pixmap );
#endif
}


#include "konq_part.moc"
