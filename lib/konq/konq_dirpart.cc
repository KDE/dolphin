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
#include <kdatastream.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <konq_drag.h>
#include <kparts/browserextension.h>
#include <kurldrag.h>
#include <kuserprofile.h>
#include <kurifilter.h>
#include <kglobalsettings.h>

#include <qapplication.h>
#include <qclipboard.h>
#include <qfile.h>
#include <assert.h>

class KonqDirPart::KonqDirPartPrivate
{
public:
    QStringList mimeFilters;
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

    actionCollection()->setHighlightingEnabled( true );

    m_paIncIconSize = new KAction( i18n( "Increase Icon Size" ), "viewmag+", 0, this, SLOT( slotIncIconSize() ), actionCollection(), "incIconSize" );
    m_paDecIconSize = new KAction( i18n( "Decrease Icon Size" ), "viewmag-", 0, this, SLOT( slotDecIconSize() ), actionCollection(), "decIconSize" );

    m_paDefaultIcons = new KRadioAction( i18n( "&Default Size" ), 0, actionCollection(), "modedefault" );
    m_paHugeIcons = new KRadioAction( i18n( "&Huge" ), 0, actionCollection(), "modehuge" );
    m_paLargeIcons = new KRadioAction( i18n( "&Large" ), 0, actionCollection(), "modelarge" );
    m_paMediumIcons = new KRadioAction( i18n( "&Medium" ), 0, actionCollection(), "modemedium" );
    m_paSmallIcons = new KRadioAction( i18n( "&Small" ), 0, actionCollection(), "modesmall" );

    m_paDefaultIcons->setExclusiveGroup( "ViewMode" );
    m_paHugeIcons->setExclusiveGroup( "ViewMode" );
    m_paLargeIcons->setExclusiveGroup( "ViewMode" );
    m_paMediumIcons->setExclusiveGroup( "ViewMode" );
    m_paSmallIcons->setExclusiveGroup( "ViewMode" );

    connect( m_paDefaultIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paHugeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paMediumIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );

    // Extract 4 icon sizes from the icon theme. Use 16,32,48,64 as default.
    int i;
    m_iIconSize[0] = 0; // Default value
    m_iIconSize[1] = KIcon::SizeSmall; // 16
    m_iIconSize[2] = KIcon::SizeMedium; // 32
    m_iIconSize[3] = KIcon::SizeLarge; // 48
    m_iIconSize[4] = KIcon::SizeHuge; // 64
    KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
    if (root)
    {
      QValueList<int> avSizes = root->querySizes(KIcon::Desktop);
      QValueList<int>::Iterator it;
      for (i=1, it=avSizes.begin(); (it!=avSizes.end()) && (i<5); it++, i++)
      {
        m_iIconSize[i] = *it;
        //kdDebug(1203) << "m_iIconSize[" << i << "] = " << *it << endl;
      }
    }

    KAction *a = new KAction( i18n( "Background Settings..." ), "background", 0, this, SLOT( slotBackgroundSettings() ),
                              actionCollection(), "bgsettings" );

    a->setStatusText( i18n( "Allows choosing of a background settings for this view" ) );
}

KonqDirPart::~KonqDirPart()
{
    // Close the find part with us
    delete m_findPart;
    delete d;
}

void KonqDirPart::setMimeFilter (const QStringList& mime)
{
    QString u = url().url();

    if ( u.isEmpty () )
        return;

    if ( mime.isEmpty() )
        d->mimeFilters.clear();
    else
        d->mimeFilters = mime;
}

QStringList KonqDirPart::mimeFilter() const
{
    return d->mimeFilters;
}

QScrollView * KonqDirPart::scrollWidget()
{
    return static_cast<QScrollView *>(widget());
}

void KonqDirPart::slotBackgroundSettings()
{
    QColor bgndColor = m_pProps->bgColor( widget() );
    QColor defaultColor = KGlobalSettings::baseColor();
    KonqBgndDialog dlg( m_pProps->bgPixmapFile(), instance(), bgndColor, defaultColor );
    if ( dlg.exec() == KonqBgndDialog::Accepted )
    {
        if ( dlg.color().isValid() )
        {
            m_pProps->setBgColor( dlg.color() );
        m_pProps->setBgPixmapFile( "" );
    }
        else
    {
            m_pProps->setBgColor( defaultColor );
        m_pProps->setBgPixmapFile( dlg.pixmapFile() );
        }
        m_pProps->applyColors( scrollWidget()->viewport() );
        scrollWidget()->viewport()->repaint();
    }
}

void KonqDirPart::lmbClicked( KFileItem * fileItem )
{
    KURL url = fileItem->url();
    if ( !fileItem->isReadable() )
    {
        // No permissions or local file that doesn't exist - need to find out which
        if ( ( !fileItem->isLocalFile() ) || QFile::exists( url.path() ) )
        {
            KMessageBox::error( widget(), i18n("<p>You do not have enough permissions to read <b>%1</b></p>").arg(url.prettyURL()) );
            return;
        }
        KMessageBox::error( widget(), i18n("<p><b>%1</b> doesn't seem to exist anymore</p>").arg(url.prettyURL()) );
        return;
    }

    KParts::URLArgs args;
    fileItem->determineMimeType();
    if ( fileItem->isMimeTypeKnown() )
        args.serviceType = fileItem->mimetype();
    args.trustedSource = true;

    if ( fileItem->isLink() && fileItem->isLocalFile() ) // see KFileItem::run
        url = KURL( url, KURL::encode_string( fileItem->linkDest() ) );

    if (KonqFMSettings::settings()->alwaysNewWin() && fileItem->isDir()) {
        //args.frameName = "_blank"; // open new window
        // We tried the other option, passing the path as framename so that
        // an existing window for that dir is reused (like MSWindows does when
        // the similar option is activated and the sidebar is hidden (!)).
        // But this requires some work, including changing the framename
        // when navigating, etc. Not very much requested yet, in addition.
        KParts::WindowArgs wargs;
        KParts::ReadOnlyPart* dummy;
        emit m_extension->createNewWindow( url, args, wargs, dummy );
    }
    else
    {
        kdDebug() << "emit m_extension->openURLRequest( " << url.url() << "," << args.serviceType << ")" << endl;
        emit m_extension->openURLRequest( url, args );
    }
}

void KonqDirPart::mmbClicked( KFileItem * fileItem )
{
    if ( fileItem )
    {
        // Optimisation to avoid KRun to call kfmclient that then tells us
        // to open a window :-)
        KService::Ptr offer = KServiceTypeProfile::preferredService(fileItem->mimetype(), "Application");
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
    else
    {
        m_extension->pasteRequest();
    }
}

void KonqDirPart::saveState( QDataStream& stream )
{
    stream << m_nameFilter;
}

void KonqDirPart::restoreState( QDataStream& stream )
{
    stream >> m_nameFilter;
}

void KonqDirPart::saveFindState( QDataStream& stream )
{
    // assert only doable in KDE4.
    //assert( m_findPart ); // test done by caller.
    if ( !m_findPart )
        return;

    // When we have a find part, our own URL wasn't saved (see KonqDirPartBrowserExtension)
    // So let's do it here
    stream << m_url;

    KParts::BrowserExtension* ext = KParts::BrowserExtension::childObject( m_findPart );
    if( !ext )
        return;

    ext->saveState( stream );
}

void KonqDirPart::restoreFindState( QDataStream& stream )
{
    // Restore our own URL
    stream >> m_url;

    emit findOpen( this );

    KParts::BrowserExtension* ext = KParts::BrowserExtension::childObject( m_findPart );
    slotClear();

    if( !ext )
        return;

    ext->restoreState( stream );
}

void KonqDirPart::slotClipboardDataChanged()
{
    // This is very related to KDIconView::slotClipboardDataChanged

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
    bool paste = ( data->format() != 0 );

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
        long long fileSizeSum = 0;
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

void KonqDirPart::emitMouseOver( const KFileItem* item )
{
    emit m_extension->mouseOverInfo( item );
}

void KonqDirPart::slotIconSizeToggled( bool )
{
    //kdDebug(1203) << "KonqDirPart::slotIconSizeToggled" << endl;
    if ( m_paDefaultIcons->isChecked() )
        setIconSize(0);
    else if ( m_paHugeIcons->isChecked() )
        setIconSize(m_iIconSize[4]);
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
    for ( int idx=1; idx < 5 ; ++idx )
        if (s == m_iIconSize[idx])
            sizeIndex = idx;
    if ( sizeIndex > 0 && sizeIndex < 4 )
    {
        setIconSize( m_iIconSize[sizeIndex + 1] );
    }
}

void KonqDirPart::slotDecIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : KGlobal::iconLoader()->currentSize( KIcon::Desktop );
    int sizeIndex = 0;
    for ( int idx=1; idx < 5 ; ++idx )
        if (s == m_iIconSize[idx])
            sizeIndex = idx;
    if ( sizeIndex > 1 )
    {
        setIconSize( m_iIconSize[sizeIndex - 1] );
    }
}

// Only updates the GUI (that's the one that is reimplemented by the views, too)
void KonqDirPart::newIconSize( int size /*0=default, or 16,32,48....*/ )
{
    int realSize = (size==0) ? KGlobal::iconLoader()->currentSize( KIcon::Desktop ) : size;
    m_paDecIconSize->setEnabled(realSize > m_iIconSize[1]);
    m_paIncIconSize->setEnabled(realSize < m_iIconSize[4]);

    m_paDefaultIcons->setChecked( size == 0 );
    m_paHugeIcons->setChecked( size == m_iIconSize[4] );
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

bool KonqDirPart::closeURL()
{
    // Tell all the childern objects to clean themselves up for dinner :)
    return doCloseURL();
}

bool KonqDirPart::openURL(const KURL& url)
{
    if ( m_findPart )
    {
        kdDebug(1203) << "KonqDirPart::openURL -> emit findClosed " << this << endl;
        delete m_findPart;
        m_findPart = 0L;
        emit findClosed( this );
    }

    m_url = url;
    emit aboutToOpenURL ();

    return doOpenURL(url);
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

    emit findOpened( this );

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

void KonqDirPartBrowserExtension::saveState( QDataStream &stream )
{
    m_dirPart->saveState( stream );
    bool hasFindPart = m_dirPart->findPart();
    stream << hasFindPart;
    assert( ! ( hasFindPart && m_dirPart->className() == "KFindPart" ) );
    if ( !hasFindPart )
        KParts::BrowserExtension::saveState( stream );
    else {
        m_dirPart->saveFindState( stream );
    }
}

void KonqDirPartBrowserExtension::restoreState( QDataStream &stream )
{
    m_dirPart->restoreState( stream );
    bool hasFindPart;
    stream >> hasFindPart;
    assert( ! ( hasFindPart && m_dirPart->className() == "KFindPart" ) );
    if ( !hasFindPart )
        // This calls openURL, that's why we don't want to call it in case of a find part
        KParts::BrowserExtension::restoreState( stream );
    else {
        m_dirPart->restoreFindState( stream );
    }
}


#include "konq_dirpart.moc"
