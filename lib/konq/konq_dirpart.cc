/* This file is part of the KDE projects
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_dirpart.h"
#include "konq_bgnddlg.h"
#include "konq_propsview.h"
#include "konq_settings.h"

#include <kparts/browserextension.h>
#include <kfileitem.h>
#include <kcolordlg.h>
#include <kdebug.h>
#include <kuserprofile.h>
#include <konq_drag.h>
#include <kurldrag.h>
#include <klocale.h>

#include <qapplication.h>
#include <qclipboard.h>

KonqDirPart::KonqDirPart( QObject *parent, const char *name )
  : KParts::ReadOnlyPart( parent, name ),
    m_pProps( 0L )
{
    m_lDirSize = 0;
    m_lFileCount = 0;
    m_lDirCount = 0;

    connect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );
}

KonqDirPart::~KonqDirPart()
{
}

QScrollView * KonqDirPart::scrollWidget()
{
    return static_cast<QScrollView *>(widget());
}

void KonqDirPart::slotBackgroundColor()
{
    QColor bgndColor = m_pProps->bgColor( widget() );
    if ( KColorDialog::getColor( bgndColor ) == KColorDialog::Accepted )
    {
        m_pProps->setBgColor( bgndColor );
        m_pProps->setBgPixmapFile( "" );
        m_pProps->applyColors( scrollWidget()->viewport() );
        scrollWidget()->viewport()->repaint();
    }
}

void KonqDirPart::slotBackgroundImage()
{
    KonqBgndDialog dlg( m_pProps->bgPixmapFile(), instance() );
    if ( dlg.exec() == KonqBgndDialog::Accepted )
    {
        m_pProps->setBgPixmapFile( dlg.pixmapFile() );
        m_pProps->applyColors( scrollWidget()->viewport() );
        scrollWidget()->viewport()->repaint();
    }
}

void KonqDirPart::mmbClicked( KFileItem * fileItem )
{
    // Optimisation to avoid KRun to call kfmclient that then tells us
    // to open a window :-)
    KService::Ptr offer = KServiceTypeProfile::preferredService(fileItem->mimetype(), true);
    if (offer) kdDebug() << "KonqDirPart::mmbClicked: got service " << offer->desktopEntryName() << endl;
    if ( offer && offer->desktopEntryName() == "kfmclient" )
    {
        KParts::URLArgs args;
        args.serviceType = fileItem->mimetype();
        emit m_extension->createNewWindow( fileItem->url(), args );
    }
    else
        fileItem->run();
}

void KonqDirPart::saveState( QDataStream &stream )
{
    //kdDebug(1203) << "void KonqDirPart::saveState( QDataStream &stream )" << endl;
    stream << m_nameFilter;
}

void KonqDirPart::restoreState( QDataStream &stream )
{
    // Warning: see comment in IconViewBrowserExtension::restoreState about order
    //kdDebug(1203) << "void KonqDirPart::restoreState( QDataStream &stream )" << endl;
    stream >> m_nameFilter;
}

void KonqDirPart::slotClipboardDataChanged()
{
    KURL::List lst;
    QMimeSource *data = QApplication::clipboard()->data();
    if ( data->provides( "application/x-kde-cutselection" ) && data->provides( "text/uri-list" ) )
        if ( KonqDrag::decodeIsCutSelection( data ) )
            (void) KURLDrag::decode( data, lst );

    disableIcons( lst );
}

void KonqDirPart::newItems( const KFileItemList & entries )
{
    for (KFileItemListIterator it(entries); it.current(); ++it)
    {
        if ( !S_ISDIR( it.current()->mode() ) )
        {
            m_lDirSize += it.current()->size();
            m_lFileCount++;
        }
        else
            m_lDirCount++;
    }
}

void KonqDirPart::deleteItem( KFileItem * fileItem )
{
    if ( !S_ISDIR( fileItem->mode() ) )
    {
        m_lDirSize -= fileItem->size();
        m_lFileCount--;
    }
    else
        m_lDirCount--;
}

void KonqDirPart::emitTotalCount()
{
    emit setStatusBarText(
        KIO::itemsSummaryString(m_lFileCount + m_lDirCount,
                                m_lFileCount,
                                m_lDirCount,
                                m_lDirSize,
                                true));
}

void KonqDirPart::emitCounts( const KFileItemList & lst, bool selectionChanged )
{
    if ( !lst.isEmpty() )
    {
        unsigned long fileSizeSum = 0;
        uint fileCount = 0;
        uint dirCount = 0;

        for (KFileItemListIterator it( lst ); it.current(); ++it )
            if ( S_ISDIR( it.current()->mode() ) )
                dirCount++;
            else // what about symlinks ?
            {
                fileSizeSum += it.current()->size();
                fileCount++;
            }

        emit setStatusBarText( KIO::itemsSummaryString(fileCount + dirCount,
                                                       fileCount,
                                                       dirCount,
                                                       fileSizeSum,
                                                       true));
    }
    else
        emitTotalCount();

    // Yes, the caller could do that too :)
    // But this bool could also be used to cache the QString for the last
    // selection, as long as selectionChanged is false.
    // Not sure it's worth it though.
    if ( selectionChanged )
        emit m_extension->selectionInfo( lst );
}

#include "konq_dirpart.moc"
