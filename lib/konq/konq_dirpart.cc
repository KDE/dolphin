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
#include <qmap.h>

#include "konq_dirpart.h"
#include "konq_bgnddlg.h"
#include "konq_propsview.h"
#include "konq_settings.h"

#include <kaction.h>
#include <kdatastream.h>
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
#include <assert.h>

class KonqDirPart::KonqDirPartPrivate
{
public:
    QMap<QString,QString> mimeFilterList;
};

KonqDirPart::KonqDirPart( QObject *parent, const char *name )
            :KParts::ReadOnlyPart( parent, name ),
    m_pProps( 0L ),
    m_findPart( 0L )
{
    d = new KonqDirPartPrivate;
    m_lDirSize = 0;
    m_lFileCount = 0;
    m_lDirCount = 0;
    //m_bMultipleItemsSelected = false;

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
        //kdDebug(1203) << "m_iIconSize[" << i << "] = " << *it << endl;
      }
    }

    new KAction( i18n( "Background Color..." ), 0, this, SLOT( slotBackgroundColor() ), actionCollection(), "bgcolor" );
    new KAction( i18n( "Background Image..." ), "background", 0, this, SLOT( slotBackgroundImage() ), actionCollection(), "bgimage" );
}

KonqDirPart::~KonqDirPart()
{
    // Close the find part with us
    if ( m_findPart )
    {
        delete m_findPart;
    }
    delete d;
}

void KonqDirPart::setMimeFilter( const QString& mime )
{
    QString u = url().url();
    if ( !u.isEmpty() )
    {
        if ( mime.isEmpty() )
            d->mimeFilterList.remove( u );
        else
            d->mimeFilterList[u] = mime;
    }
}

QString KonqDirPart::mimeFilter() const
{
    QString u = url().url();
    if ( d->mimeFilterList.contains( u ) )
        return d->mimeFilterList[u];
    else
        return QString::null;
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
    //if (offer) kdDebug(1203) << "KonqDirPart::mmbClicked: got service " << offer->desktopEntryName() << endl;
    if ( offer && offer->desktopEntryName().startsWith("kfmclient") )
    {
        KParts::URLArgs args;
        args.serviceType = fileItem->mimetype();
        emit m_extension->createNewWindow( fileItem->url(), args );
    }
    else
        fileItem->run();
}

void KonqDirPart::saveNameFilter( QDataStream &stream )
{
    stream << m_nameFilter;
}

void KonqDirPart::saveState( QDataStream &stream )
{
    //kdDebug(1203) << " -- void KonqDirPart::saveState( QDataStream &stream )" << endl;
    if ( !m_findPart )
        stream << false;
    else
    {
        //kdDebug(1203) << "KonqDirPart::saveState -> saving TRUE" << endl;
        stream << true;
        // TODO save the kfindpart in there
        KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( m_findPart );
        if(m_findPart==NULL) {
                kdDebug() << "m_findpart is null\n";
                return;
        }
        if(ext==NULL) {
                kdDebug() << "ext is null\n";
                return;
        }
        else
                ext->saveState( stream );
    }
}

void KonqDirPart::restoreNameFilter( QDataStream &stream )
{
    stream >> m_nameFilter;
    //kdDebug(1203) << "KonqDirPart::restoreNameFilter " << m_nameFilter << endl;
}

void KonqDirPart::restoreState( QDataStream &stream )
{
    // Warning: see comment in IconViewBrowserExtension::restoreState about order
    //kdDebug(1203) << " -- void KonqDirPart::restoreState( QDataStream &stream )" << endl;
    bool bFindPart;
    stream >> bFindPart;
    //kdDebug(1203) << "KonqDirPart::restoreState " << bFindPart << endl;
    if ( bFindPart )
    {
        emit findOpen( this );
        // TODO restore the kfindpart data
        KParts::BrowserExtension * ext = KParts::BrowserExtension::childObject( m_findPart );
        slotClear();
        if(m_findPart==NULL)
                kdDebug() << "\n*************\nrestore m_findpart is null\n";
        if(ext==NULL)
                kdDebug() << "\n*************\nrestore ext is null\n";
        else
                ext->restoreState( stream );
    }
}

void KonqDirPart::slotClipboardDataChanged()
{
    KURL::List lst;
    QMimeSource *data = QApplication::clipboard()->data();
    if ( data->provides( "application/x-kde-cutselection" ) && data->provides( "text/uri-list" ) )
        if ( KonqDrag::decodeIsCutSelection( data ) )
            (void) KURLDrag::decode( data, lst );

    disableIcons( lst );

    updatePasteAction();
}

void KonqDirPart::updatePasteAction()
{
    QMimeSource *data = QApplication::clipboard()->data();
    bool paste = ( data->encodedData( data->format() ).size() != 0 );

    emit m_extension->enableAction( "paste", paste ); // TODO : if only one url, check that it's a dir
}

void KonqDirPart::newItems( const KFileItemList & entries )
{
    for (KFileItemListIterator it(entries); it.current(); ++it)
    {
        if ( !it.current()->isDir() )
        {
            if (!it.current()->isLink()) // ignore symlinks
                m_lDirSize += it.current()->size();
            m_lFileCount++;
        }
        else
            m_lDirCount++;
    }
    if ( m_findPart )
        emitTotalCount();

    emit itemsAdded( entries );
}

void KonqDirPart::deleteItem( KFileItem * fileItem )
{
    if ( !fileItem->isDir() )
    {
        if ( !fileItem->isLink() )
            m_lDirSize -= fileItem->size();
        m_lFileCount--;
    }
    else
        m_lDirCount--;

    emit itemRemoved( fileItem );
}

void KonqDirPart::emitTotalCount()
{
    QString summary =
        KIO::itemsSummaryString(m_lFileCount + m_lDirCount,
                                m_lFileCount,
                                m_lDirCount,
                                m_lDirSize,
                                true);
    bool bShowsResult = false;
    if (m_findPart)
    {
        QVariant prop = m_findPart->property( "showsResult" );
        bShowsResult = prop.isValid() && prop.toBool();
    }
    //kdDebug(1203) << "KonqDirPart::emitTotalCount bShowsResult=" << bShowsResult << endl;
    emit setStatusBarText( bShowsResult ? i18n("Search result: %1").arg(summary) : summary );
}

void KonqDirPart::emitCounts( const KFileItemList & lst, bool selectionChanged )
{
    // Compare the new value with our cache
    /*bool multiple = lst.count()>1;
    if (multiple != m_bMultipleItemsSelected)
    {
        m_bMultipleItemsSelected = multiple;
        updatePasteAction();
    }*/

    if ( lst.count()==1)
    {
        emit setStatusBarText( ((KFileItemList)lst).first()->getStatusBarInfo() );
    }
    else if ( lst.count()>1)
    {
        unsigned long fileSizeSum = 0;
        uint fileCount = 0;
        uint dirCount = 0;

        for (KFileItemListIterator it( lst ); it.current(); ++it )
            if ( it.current()->isDir() )
                dirCount++;
            else
            {
                if (!it.current()->isLink()) // ignore symlinks
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
    //kdDebug(1203) << "KonqDirPart::slotIconSizeToggled" << endl;
    if ( m_paDefaultIcons->isChecked() )
        setIconSize(0);
    else if ( m_paLargeIcons->isChecked() )
        setIconSize(m_iIconSize[3]);
    else if ( m_paMediumIcons->isChecked() )
        setIconSize(m_iIconSize[2]);
    else if ( m_paSmallIcons->isChecked() )
        setIconSize(m_iIconSize[1]);
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
    setIconSize( m_iIconSize[sizeIndex + 1] );
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
    setIconSize( m_iIconSize[sizeIndex - 1] );
}

// Only updates the GUI (that's the one that is reimplemented by the views, too)
void KonqDirPart::newIconSize( int size /*0=default, or 16,32,48....*/ )
{
    m_paDecIconSize->setEnabled(size > m_iIconSize[1]);
    m_paIncIconSize->setEnabled(size < m_iIconSize[3]);
    m_paDefaultIcons->setChecked( size == 0 );
    m_paLargeIcons->setChecked( size == m_iIconSize[3] );
    m_paMediumIcons->setChecked( size == m_iIconSize[2] );
    m_paSmallIcons->setChecked( size == m_iIconSize[1] );
}

// Stores the new icon size and updates the GUI
void KonqDirPart::setIconSize( int size )
{
    //kdDebug(1203) << "KonqDirPart::setIconSize " << size << " -> updating props and GUI" << endl;
    m_pProps->setIconSize( size );
    newIconSize( size );
}

void KonqDirPart::beforeOpenURL()
{
    if ( m_findPart )
    {
        kdDebug(1203) << "KonqDirPart::beforeOpenURL -> emit findClosed " << this << endl;
        delete m_findPart;
        m_findPart = 0L;
        emit findClosed( this );
    }
}

void KonqDirPart::setFindPart( KParts::ReadOnlyPart * part )
{
    assert(part);
    m_findPart = part;
    connect( m_findPart, SIGNAL( started() ),
             this, SLOT( slotStarted() ) );
    connect( m_findPart, SIGNAL( started() ),
             this, SLOT( slotStartAnimationSearching() ) );
    connect( m_findPart, SIGNAL( clear() ),
             this, SLOT( slotClear() ) );
    connect( m_findPart, SIGNAL( newItems( const KFileItemList & ) ),
             this, SLOT( slotNewItems( const KFileItemList & ) ) );
    connect( m_findPart, SIGNAL( finished() ), // can't name it completed, it conflicts with a KROP signal
             this, SLOT( slotCompleted() ) );
    connect( m_findPart, SIGNAL( finished() ),
             this, SLOT( slotStopAnimationSearching() ) );
    connect( m_findPart, SIGNAL( canceled() ),
             this, SLOT( slotCanceled() ) );
    connect( m_findPart, SIGNAL( canceled() ),
             this, SLOT( slotStopAnimationSearching() ) );

    connect( m_findPart, SIGNAL( findClosed() ),
             this, SLOT( slotFindClosed() ) );

    // set the initial URL in the find part
    m_findPart->openURL( url() );
}

void KonqDirPart::slotFindClosed()
{
    kdDebug(1203) << "KonqDirPart::slotFindClosed -> emit findClosed " << this << endl;
    delete m_findPart;
    m_findPart = 0L;
    emit findClosed( this );
    // reload where we were before
    openURL( url() );
}

void KonqDirPart::slotStartAnimationSearching()
{
  started(0);
}

void KonqDirPart::slotStopAnimationSearching()
{
  completed();
}


#include "konq_dirpart.moc"
