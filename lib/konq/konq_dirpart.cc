/* This file is part of the KDE projects
   Copyright (C) 2000-2006 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_dirpart.h"
#include "konq_bgnddlg.h"
#include "konq_propsview.h"
#include "konq_settings.h"
#include "konqmimedata.h"

#include <kio/paste.h>
#include <kapplication.h>
#include <kaction.h>
#include <kdatastream.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kmimetypetrader.h>
#include <kparts/browserextension.h>
#include <kurifilter.h>
#include <kglobalsettings.h>

#include <QApplication>
#include <QClipboard>
#include <QFile>
//Added by qt3to4:
#include <assert.h>

#include <Q3ScrollView>

class KonqDirPart::KonqDirPartPrivate
{
public:
    KonqDirPartPrivate() : dirLister( 0 ) {}
    QStringList mimeFilters;
    KAction *aEnormousIcons;
    KAction *aSmallMediumIcons;
    QVector<int> iconSize;

    KDirLister* dirLister;
    bool dirSizeDirty;

    void findAvailableIconSizes(void);
    int findNearestIconSize(int size);
    int nearestIconSizeError(int size);
};

void KonqDirPart::KonqDirPartPrivate::findAvailableIconSizes(void)
{
    KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
    iconSize.resize(1);
    if (root) {
	QList<int> avSizes = root->querySizes(K3Icon::Desktop);
       // kDebug(1203) << "The icon theme handles the sizes:" << avSizes << endl;
	qStableSort(avSizes);
	int oldSize = -1;
	if (avSizes.count() < 10) {
	    // Fixed or threshold type icons
	    QList<int>::const_iterator i;
	    for (i = avSizes.begin(); i != avSizes.end(); i++) {
		// Skip duplicated values (sanity check)
		if (*i != oldSize) iconSize.append(*i);
		oldSize = *i;
	    }
	} else {
	    // Scalable icons.
	    const int progression[] = {16, 22, 32, 48, 64, 96, 128, 192, 256};

	    QList<int>::const_iterator j = avSizes.begin();
	    for (uint i = 0; i < 9; i++) {
		while (j++ != avSizes.end()) {
		    if (*j >= progression[i]) {
			iconSize.append(*j);
			kDebug(1203) << "appending " << *j << " size." << endl;
			break;
		    }
		}
	    }
	}
    } else {
	iconSize.append(K3Icon::SizeSmall); // 16
	iconSize.append(K3Icon::SizeMedium); // 32
	iconSize.append(K3Icon::SizeLarge); // 48
	iconSize.append(K3Icon::SizeHuge); // 64
    }
    kDebug(1203) << "Using " << iconSize.count() << " icon sizes." << endl;
}

int KonqDirPart::KonqDirPartPrivate::findNearestIconSize(int preferred)
{
    int s1 = iconSize[1];
    if (preferred == 0) return KGlobal::iconLoader()->currentSize(K3Icon::Desktop);
    if (preferred <= s1) return s1;
    for (int i = 2; i <= iconSize.count(); i++) {
        if (preferred <= iconSize[i]) {
	    if (preferred - s1 <  iconSize[i] - preferred) return s1;
	    else return iconSize[i];
	} else {
	    s1 = iconSize[i];
	}
    }
    return s1;
}

int KonqDirPart::KonqDirPartPrivate::nearestIconSizeError(int size)
{
    return QABS(size - findNearestIconSize(size));
}

KonqDirPart::KonqDirPart( QObject *parent )
            :KParts::ReadOnlyPart( parent ),
    m_pProps( 0L ),
    m_findPart( 0L )
{
    d = new KonqDirPartPrivate;
    resetCount();
    //m_bMultipleItemsSelected = false;

    connect( QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(slotClipboardDataChanged()) );

    m_paIncIconSize = new KAction( KIcon( "viewmag+" ), i18n( "Enlarge Icons" ), actionCollection(), "incIconSize" );
    connect( m_paIncIconSize, SIGNAL( triggered() ), this, SLOT( slotIncIconSize() ) );
    m_paDecIconSize = new KAction( KIcon( "viewmag-" ), i18n( "Shrink Icons" ), actionCollection(), "decIconSize" );
    connect( m_paDecIconSize, SIGNAL( triggered() ), this, SLOT( slotDecIconSize() ) );


    m_paDefaultIcons = new KAction( i18n( "&Default Size" ), actionCollection(), "modedefault" );
    d->aEnormousIcons = new KAction( i18n( "&Huge" ), actionCollection(), "modeenormous" );
    m_paHugeIcons = new KAction( i18n( "&Very Large" ), actionCollection(), "modehuge" );
    m_paLargeIcons = new KAction( i18n( "&Large" ), actionCollection(), "modelarge" );
    m_paMediumIcons = new KAction( i18n( "&Medium" ), actionCollection(), "modemedium" );
    d->aSmallMediumIcons = new KAction( i18n( "&Small" ), actionCollection(), "modesmallmedium" );
    m_paSmallIcons = new KAction( i18n( "&Tiny" ), actionCollection(), "modesmall" );

    QActionGroup* viewModeGroup = new QActionGroup(this);
    viewModeGroup->setExclusive(true);
    m_paDefaultIcons->setActionGroup( viewModeGroup );
    d->aEnormousIcons->setActionGroup( viewModeGroup );
    m_paHugeIcons->setActionGroup( viewModeGroup );
    m_paLargeIcons->setActionGroup( viewModeGroup );
    m_paMediumIcons->setActionGroup( viewModeGroup );
    d->aSmallMediumIcons->setActionGroup( viewModeGroup );
    m_paSmallIcons->setActionGroup( viewModeGroup );

    connect( m_paDefaultIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( d->aEnormousIcons, SIGNAL( toggled( bool ) ),
	    this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paHugeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paMediumIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( d->aSmallMediumIcons, SIGNAL( toggled( bool ) ),
	    this, SLOT( slotIconSizeToggled( bool ) ) );
    connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotIconSizeToggled( bool ) ) );

    connect( kapp, SIGNAL(iconChanged(int)), SLOT(slotIconChanged(int)) );
#if 0
    // Extract 6 icon sizes from the icon theme.
    // Use 16,22,32,48,64,128 as default.
    // Use these also if the icon theme is scalable.
    int i;
    d->iconSize[0] = 0; // Default value
    d->iconSize[1] = K3Icon::SizeSmall; // 16
    d->iconSize[2] = K3Icon::SizeSmallMedium; // 22
    d->iconSize[3] = K3Icon::SizeMedium; // 32
    d->iconSize[4] = K3Icon::SizeLarge; // 48
    d->iconSize[5] = K3Icon::SizeHuge; // 64
    d->iconSize[6] = K3Icon::SizeEnormous; // 128
    d->iconSize[7] = 192;
    d->iconSize[8] = 256;
    KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
    if (root)
    {
      QList<int> avSizes = root->querySizes(K3Icon::Desktop);
      kDebug(1203) << "the icon theme handles the following sizes:" << avSizes << endl;
      if (avSizes.count() < 10) {
	// Use the icon sizes supplied by the theme.
	// If avSizes contains more than 10 entries, assume a scalable
	// icon theme.
	QList<int>::Iterator it;
	for (i=1, it=avSizes.begin(); (it!=avSizes.end()) && (i<7); it++, i++)
	{
	  d->iconSize[i] = *it;
	  kDebug(1203) << "m_iIconSize[" << i << "] = " << *it << endl;
	}
	// Generate missing sizes
	for (; i < 7; i++) {
	  d->iconSize[i] = d->iconSize[i - 1] + d->iconSize[i - 1] / 2 ;
	  kDebug(1203) << "m_iIconSize[" << i << "] = " << d->iconSize[i] << endl;
	}
      }
    }
#else
    d->iconSize.reserve(10);
    d->iconSize.append(0); // Default value
    adjustIconSizes();
#endif

    KAction *a = new KAction( KIcon( "background" ), i18n( "Configure Background..." ), actionCollection(), "bgsettings" );
    connect( a, SIGNAL( triggered() ), this, SLOT( slotBackgroundSettings() ) );

    a->setToolTip( i18n( "Allows choosing of background settings for this view" ) );
}

KonqDirPart::~KonqDirPart()
{
    // Close the find part with us
    delete m_findPart;
    delete d;
}

void KonqDirPart::adjustIconSizes()
{
    d->findAvailableIconSizes();
    m_paSmallIcons->setEnabled(d->findNearestIconSize(16) < 20);
    d->aSmallMediumIcons->setEnabled(d->nearestIconSizeError(22) < 2);
    m_paMediumIcons->setEnabled(d->nearestIconSizeError(32) < 6);
    m_paLargeIcons->setEnabled(d->nearestIconSizeError(48) < 8);
    m_paHugeIcons->setEnabled(d->nearestIconSizeError(64) < 12);
    d->aEnormousIcons->setEnabled(d->findNearestIconSize(128) > 110);

    if (m_pProps) {
	int size = m_pProps->iconSize();
	int nearSize = d->findNearestIconSize(size);

	if (size != nearSize) {
	    m_pProps->setIconSize(nearSize);
	}
	newIconSize(nearSize);
    }
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

Q3ScrollView * KonqDirPart::scrollWidget()
{
    return static_cast<Q3ScrollView *>(widget());
}

void KonqDirPart::slotBackgroundSettings()
{
    QColor bgndColor = m_pProps->bgColor( widget() );
    QColor defaultColor = KGlobalSettings::baseColor();
    KonqBgndDialog dlg( widget(), m_pProps->bgPixmapFile(), bgndColor, defaultColor );
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
    KUrl url = fileItem->url();
    if ( !fileItem->isReadable() )
    {
        // No permissions or local file that doesn't exist - need to find out which
        if ( ( !fileItem->isLocalFile() ) || QFile::exists( url.path() ) )
        {
            KMessageBox::error( widget(), i18n("<p>You do not have enough permissions to read <b>%1</b></p>", url.prettyUrl()) );
            return;
        }
        KMessageBox::error( widget(), i18n("<p><b>%1</b> does not seem to exist anymore</p>", url.prettyUrl()) );
        return;
    }

    KParts::URLArgs args;
    fileItem->determineMimeType();
    if ( fileItem->isMimeTypeKnown() )
        args.serviceType = fileItem->mimetype();
    args.trustedSource = true;

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
        kDebug() << "emit m_extension->openURLRequest( " << url.url() << "," << args.serviceType << ")" << endl;
        emit m_extension->openURLRequest( url, args );
    }
}

void KonqDirPart::mmbClicked( KFileItem * fileItem )
{
    if ( fileItem )
    {
        // Optimisation to avoid KRun to call kfmclient that then tells us
        // to open a window :-)
        KService::Ptr offer = KMimeTypeTrader::self()->preferredService(fileItem->mimetype(), "Application");
        //if (offer) kDebug(1203) << "KonqDirPart::mmbClicked: got service " << offer->desktopEntryName() << endl;
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
    // This is very related to KDIconView::slotClipboardDataChanged in kdesktop

    KUrl::List lst;
    const QMimeData *data = QApplication::clipboard()->mimeData();
    if ( data->hasFormat( "application/x-kde-cutselection" ) && data->hasFormat( "text/uri-list" ) )
        if ( KonqMimeData::decodeIsCutSelection( data ) )
            lst = KUrl::List::fromMimeData( data );

    disableIcons( lst );

    QString actionText = KIO::pasteActionText();
    bool paste = !actionText.isEmpty();
    if ( paste )
      emit m_extension->setActionText( "paste", actionText );
    emit m_extension->enableAction( "paste", paste );
}

void KonqDirPart::newItems( const KFileItemList & entries )
{
    d->dirSizeDirty = true;
    if ( m_findPart )
        emitTotalCount();

    emit itemsAdded( entries );
}

void KonqDirPart::deleteItem( KFileItem * fileItem )
{
    d->dirSizeDirty = true;
    emit itemRemoved( fileItem );
}

void KonqDirPart::emitTotalCount()
{
    if ( !d->dirLister || d->dirLister->url().isEmpty() )
        return;
    if ( d->dirSizeDirty ) {
        m_lDirSize = 0;
        m_lFileCount = 0;
        m_lDirCount = 0;
        const KFileItemList entries = d->dirLister->items();
        KFileItemList::const_iterator it = entries.begin();
        const KFileItemList::const_iterator end = entries.end();
        for ( ; it != end; ++it )
        {
            if ( !(*it)->isDir() )
            {
                if (!(*it)->isLink()) // symlinks don't contribute to the size
                    m_lDirSize += (*it)->size();
                m_lFileCount++;
            }
            else
                m_lDirCount++;
        }
        d->dirSizeDirty = false;
    }

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
    //kDebug(1203) << "KonqDirPart::emitTotalCount bShowsResult=" << bShowsResult << endl;
    emit setStatusBarText( bShowsResult ? i18n("Search result: %1", summary) : summary );
}

void KonqDirPart::emitCounts( const KFileItemList & lst )
{
    if ( lst.count() == 1 )
        emit setStatusBarText( lst.first()->getStatusBarInfo() );
    else
    {
        long long fileSizeSum = 0;
        uint fileCount = 0;
        uint dirCount = 0;
        KFileItemList::const_iterator it = lst.begin();
        const KFileItemList::const_iterator end = lst.end();
        for ( ; it != end; ++it )
        {
            if ( (*it)->isDir() )
                dirCount++;
            else
            {
                if ( !(*it)->isLink() ) // ignore symlinks
                    fileSizeSum += (*it)->size();
                fileCount++;
            }
        }

        emit setStatusBarText( KIO::itemsSummaryString( fileCount + dirCount,
                                                        fileCount, dirCount,
                                                        fileSizeSum, true ) );
    }
}

void KonqDirPart::emitCounts( const KFileItemList & lst, bool selectionChanged )
{
    if ( lst.count() == 0 )
        emitTotalCount();
    else
        emitCounts( lst );

    // Yes, the caller could do that too :)
    // But this bool could also be used to cache the QString for the last
    // selection, as long as selectionChanged is false.
    // Not sure it's worth it though.
    // MiB: no, I don't think it's worth it. Especially regarding the
    //      loss of readability of the code. Thus, this will be removed in
    //      KDE 4.0.
    if ( selectionChanged )
        emit m_extension->selectionInfo( lst );
}

void KonqDirPart::emitMouseOver( const KFileItem* item )
{
    emit m_extension->mouseOverInfo( item );
}

void KonqDirPart::slotIconSizeToggled( bool toggleOn )
{
    //kDebug(1203) << "KonqDirPart::slotIconSizeToggled" << endl;

    // This slot is called when an iconsize action is checked or by calling
    // action->setChecked(false) (previously true). So we must filter out
    // the 'untoggled' case to prevent odd results here (repaints/loops!)
    if ( !toggleOn )
        return;

    if ( m_paDefaultIcons->isChecked() )
        setIconSize(0);
    else if ( d->aEnormousIcons->isChecked() )
        setIconSize(d->findNearestIconSize(K3Icon::SizeEnormous));
    else if ( m_paHugeIcons->isChecked() )
        setIconSize(d->findNearestIconSize(K3Icon::SizeHuge));
    else if ( m_paLargeIcons->isChecked() )
        setIconSize(d->findNearestIconSize(K3Icon::SizeLarge));
    else if ( m_paMediumIcons->isChecked() )
        setIconSize(d->findNearestIconSize(K3Icon::SizeMedium));
    else if ( d->aSmallMediumIcons->isChecked() )
        setIconSize(d->findNearestIconSize(K3Icon::SizeSmallMedium));
    else if ( m_paSmallIcons->isChecked() )
        setIconSize(d->findNearestIconSize(K3Icon::SizeSmall));
}

void KonqDirPart::slotIncIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );
    int sizeIndex = 0;
    for ( int idx = 1; idx < d->iconSize.count() ; ++idx )
        if (s == d->iconSize[idx]) {
            sizeIndex = idx;
	    break;
	}
    if ( sizeIndex > 0 && sizeIndex < d->iconSize.count() - 1 )
    {
        setIconSize( d->iconSize[sizeIndex + 1] );
    }
}

void KonqDirPart::slotDecIconSize()
{
    int s = m_pProps->iconSize();
    s = s ? s : KGlobal::iconLoader()->currentSize( K3Icon::Desktop );
    uint sizeIndex = 0;
    for ( int idx = 1; idx < d->iconSize.count() ; ++idx )
        if (s == d->iconSize[idx]) {
            sizeIndex = idx;
	    break;
	}
    if ( sizeIndex > 1 )
    {
        setIconSize( d->iconSize[sizeIndex - 1] );
    }
}

// Only updates Actions, a GUI update is done in the views by reimplementing this
void KonqDirPart::newIconSize( int size /*0=default, or 16,32,48....*/ )
{
    int realSize = (size==0) ? KGlobal::iconLoader()->currentSize( K3Icon::Desktop ) : size;
    m_paDecIconSize->setEnabled(realSize > d->iconSize[1]);
    m_paIncIconSize->setEnabled(realSize < d->iconSize.back());

    m_paDefaultIcons->setChecked(size == 0);
    d->aEnormousIcons->setChecked(size == d->findNearestIconSize(K3Icon::SizeEnormous));
    m_paHugeIcons->setChecked(size == d->findNearestIconSize(K3Icon::SizeHuge));
    m_paLargeIcons->setChecked(size == d->findNearestIconSize(K3Icon::SizeLarge));
    m_paMediumIcons->setChecked(size == d->findNearestIconSize(K3Icon::SizeMedium));
    d->aSmallMediumIcons->setChecked(size == d->findNearestIconSize(K3Icon::SizeSmallMedium));
    m_paSmallIcons->setChecked(size == d->findNearestIconSize(K3Icon::SizeSmall));
}

// Stores the new icon size and updates the GUI
void KonqDirPart::setIconSize( int size )
{
    //kDebug(1203) << "KonqDirPart::setIconSize " << size << " -> updating props and GUI" << endl;
    m_pProps->setIconSize( size );
    newIconSize( size );
}

bool KonqDirPart::closeURL()
{
    // Tell all the childern objects to clean themselves up for dinner :)
    return doCloseURL();
}

bool KonqDirPart::openURL(const KUrl& url)
{
    if ( m_findPart )
    {
        kDebug(1203) << "KonqDirPart::openURL -> emit findClosed " << this << endl;
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
    kDebug(1203) << "KonqDirPart::slotFindClosed -> emit findClosed " << this << endl;
    delete m_findPart;
    m_findPart = 0L;
    emit findClosed( this );
    // reload where we were before
    openURL( url() );
}

void KonqDirPart::slotIconChanged( int group )
{
    if (group != K3Icon::Desktop) return;
    adjustIconSizes();
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
    assert( ! ( hasFindPart && !strcmp(m_dirPart->metaObject()->className(), "KFindPart") ) );
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
    assert( ! ( hasFindPart && !strcmp(m_dirPart->metaObject()->className(), "KFindPart") ) );
    if ( !hasFindPart )
        // This calls openURL, that's why we don't want to call it in case of a find part
        KParts::BrowserExtension::restoreState( stream );
    else {
        m_dirPart->restoreFindState( stream );
    }
}


void KonqDirPart::resetCount()
{
    m_lDirSize = 0;
    m_lFileCount = 0;
    m_lDirCount = 0;
    d->dirSizeDirty = true;
}

void KonqDirPart::setDirLister( KDirLister* lister )
{
    d->dirLister = lister;
}

#include "konq_dirpart.moc"
