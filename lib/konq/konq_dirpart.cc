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

#include <kaction.h>
#include <kcolordlg.h>
#include <kdebug.h>
#include <kfileitem.h>
#include <klocale.h>
#include <konq_drag.h>
#include <kparts/browserextension.h>
#include <kurldrag.h>
#include <kuserprofile.h>

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

    m_paIncIconSize = new KAction( i18n( "Increase Icon Size" ), "viewmag+", 0, this, SLOT( slotIncIconSize() ), actionCollection(), "incIconSize" );
    m_paDecIconSize = new KAction( i18n( "Decrease Icon Size" ), "viewmag-", 0, this, SLOT( slotDecIconSize() ), actionCollection(), "decIconSize" );

    m_paDefaultIcons = new KRadioAction( i18n( "&Default Size" ), 0, actionCollection(), "modedefault" );
    m_paLargeIcons = new KRadioAction( i18n( "&Large" ), 0, actionCollection(), "modelarge" );
    m_paMediumIcons = new KRadioAction( i18n( "&Medium" ), 0, actionCollection(), "modemedium" );
    m_paSmallIcons = new KRadioAction( i18n( "&Small" ), 0, actionCollection(), "modesmall" );

    m_paDefaultIcons->setExclusiveGroup( "ViewMode" );
    m_paLargeIcons->setExclusiveGroup( "ViewMode" );
    m_paMediumIcons->setExclusiveGroup( "ViewMode" );
    m_paSmallIcons->setExclusiveGroup( "ViewMode" );

    connect( m_paDefaultIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paMediumIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );

    // Extract 3 icon sizes from the icon theme. Use 16,32,48 as default.
    int i;
    m_iIconSize[0] = 0; // Default value
    m_iIconSize[1] = KIcon::SizeSmall; // 16
    m_iIconSize[2] = KIcon::SizeMedium; // 32
    m_iIconSize[3] = KIcon::SizeLarge; // 48
    KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
    if (root)
    {
      QValueList<int> avSizes = root->querySizes(KIcon::Desktop);
      QValueList<int>::Iterator it;
      for (i=1, it=avSizes.begin(); (it!=avSizes.end()) && (i<4); it++, i++)
      {
        m_iIconSize[i] = *it;
        //kdDebug(1202) << "m_iIconSize[" << i << "] = " << *it << endl;
      }
    }
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

void KonqDirPart::slotIconSizeToggled( bool )
{
    if ( m_paDefaultIcons->isChecked() )
        newIconSize(0);
    else if ( m_paLargeIcons->isChecked() )
        newIconSize(m_iIconSize[3]);
    else if ( m_paMediumIcons->isChecked() )
        newIconSize(m_iIconSize[2]);
    else if ( m_paSmallIcons->isChecked() )
        newIconSize(m_iIconSize[1]);
}

void KonqDirPart::slotIncIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int sizeIndex = 0;
    for ( int idx=1; idx < 4 ; ++idx )
        if (s == m_iIconSize[idx])
            sizeIndex = idx;
    ASSERT( sizeIndex != 0 && sizeIndex < 3 );
    newIconSize( m_iIconSize[sizeIndex + 1] );
}

void KonqDirPart::slotDecIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int sizeIndex = 0;
    for ( int idx=1; idx < 4 ; ++idx )
        if (s == m_iIconSize[idx])
            sizeIndex = idx;
    ASSERT( sizeIndex > 1 );
    newIconSize( m_iIconSize[sizeIndex - 1] );
}

void KonqDirPart::newIconSize( int size /*0=default, or 16,32,48....*/ )
{
    m_pProps->setIconSize( size );

    // Update the actions
    m_paDecIconSize->setEnabled(size > m_iIconSize[1]);
    m_paIncIconSize->setEnabled(size < m_iIconSize[3]);
    m_paDefaultIcons->setChecked( size == 0 );
    m_paLargeIcons->setChecked( size == m_iIconSize[3] );
    m_paMediumIcons->setChecked( size == m_iIconSize[2] );
    m_paSmallIcons->setChecked( size == m_iIconSize[1] );
}

#include "konq_dirpart.moc"
