/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_iconview.h"
#include "konq_bgnddlg.h"
#include "konq_propsview.h"
#include "konq_childview.h"
#include "konq_frame.h"
#include "konq_factory.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kaccel.h>
#include <kaction.h>
#include <kcolordlg.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kfileivi.h>
#include <kfileitem.h>
#include <kio_error.h>
#include <kio_job.h>
#include <klibloader.h>
#include <klineeditdlg.h>
#include <kmimetype.h>
#include <konqiconviewwidget.h>
#include <konqsettings.h>
#include <krun.h>
#include <kstdaction.h>
#include <kurl.h>
#include <kparts/mainwindow.h>

#include <qmessagebox.h>
#include <qfile.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <klocale.h>
#include <qregexp.h>

class KonqIconViewFactory : public KLibFactory
{
public:
    KonqIconViewFactory()
    {
	KonqFactory::instanceRef();
    }
    virtual ~KonqIconViewFactory()
    {
	KonqFactory::instanceUnref();
    }

    virtual QObject* create( QObject *parent, const char *name, const char*, const QStringList &args )
    {
	KonqKfmIconView *obj = new KonqKfmIconView( (QWidget *)parent, parent, name );
	emit objectCreated( obj );

	QStringList::ConstIterator it = args.begin();
	QStringList::ConstIterator end = args.end();
	uint i = 1;
	for (; it != end; ++it, ++i )
	    // This is not used anymore - do we still need something like this ?
	    // (David)
    	    // Hmmm, don't think so (Simon)
	    if ( *it == "-viewMode" && i <= args.count() )
	    {
		++it;
	
		QIconView *iconView = obj->iconViewWidget();
	
		if ( *it == "LargeIcons" )
		{
		    //iconView->setViewMode( QIconSet::Large );
		    iconView->setItemTextPos( QIconView::Bottom );
		}
		else if ( *it == "SmallIcons" )
		{
		    //iconView->setViewMode( QIconSet::Small );
		    iconView->setItemTextPos( QIconView::Bottom );
		}
		else if ( *it == "SmallVerticalIcons" )
		{
		    //iconView->setViewMode( QIconSet::Small );
		    iconView->setItemTextPos( QIconView::Right );
		}

		break;
	    }

	return obj;
    }

};

extern "C"
{
    void *init_libkonqiconview()
    {
	return new KonqIconViewFactory;
    }
};

IconViewBrowserExtension::IconViewBrowserExtension( KonqKfmIconView *iconView )
 : KParts::BrowserExtension( iconView )
{
  m_iconView = iconView;
}

int IconViewBrowserExtension::xOffset()
{
  return m_iconView->iconViewWidget()->contentsX();
}

int IconViewBrowserExtension::yOffset()
{
  return m_iconView->iconViewWidget()->contentsY();
}

void IconViewBrowserExtension::reparseConfiguration()
{
    KonqFMSettings::reparseConfiguration();
    // m_pProps is a problem here (what is local, what is global ?)
    // but settings is easy :
    m_iconView->iconViewWidget()->initConfig();
}

void IconViewBrowserExtension::saveLocalProperties()
{
    m_iconView->m_pProps->saveLocal( KURL( m_iconView->url() ) );
}

void IconViewBrowserExtension::savePropertiesAsDefault()
{
    m_iconView->m_pProps->saveAsDefault();
}


KonqKfmIconView::KonqKfmIconView( QWidget *parentWidget, QObject *parent, const char *name )
    : KParts::ReadOnlyPart( parent, name )
{
    kdebug(0, 1202, "+KonqKfmIconView");
    m_iXOffset = 0;
    m_iYOffset = 0;

    setInstance( KonqFactory::instance() );

    m_extension = new IconViewBrowserExtension( this );

    setXMLFile( "konq_iconview.rc" );

    // Create a properties instance for this view
    // (copying the default values)
    m_pProps = new KonqPropsView( * KonqPropsView::defaultProps() );
    m_pSettings = KonqFMSettings::defaultIconSettings();

    m_pIconView = new KonqIconViewWidget( parentWidget, "qiconview" );

    // pass signals to the extension
    connect( m_pIconView, SIGNAL( enableAction( const char *, bool ) ),
             m_extension, SIGNAL( enableAction( const char *, bool ) ) );

    setWidget( m_pIconView );

    m_ulTotalFiles = 0;

    // Don't repaint on configuration changes during construction
    m_bInit = true;

    m_paDotFiles = new KToggleAction( i18n( "Show &Dot Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );
    m_paImagePreview = new KToggleAction( i18n( "&Image Preview" ), 0, actionCollection(), "image_preview" );

    connect( m_paImagePreview, SIGNAL( toggled( bool ) ), this, SLOT( slotImagePreview( bool ) ) );

    //    m_pamSort = new KActionMenu( i18n( "Sort..." ), actionCollection(), "sort" );

    KToggleAction *aSortByNameCS = new KToggleAction( i18n( "by Name (Case Sensitive)" ), 0, actionCollection(), "sort_nc" );
    KToggleAction *aSortByNameCI = new KToggleAction( i18n( "by Name (Case Insensitive)" ), 0, actionCollection(), "sort_nci" );
    KToggleAction *aSortBySize = new KToggleAction( i18n( "By Size" ), 0, actionCollection(), "sort_size" );

    aSortByNameCS->setExclusiveGroup( "sorting" );
    aSortByNameCI->setExclusiveGroup( "sorting" );
    aSortBySize->setExclusiveGroup( "sorting" );

    aSortByNameCS->setChecked( true );
    aSortByNameCI->setChecked( false );
    aSortBySize->setChecked( false );

    connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseSensitive( bool ) ) );
    connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseInsensitive( bool ) ) );
    connect( aSortBySize, SIGNAL( toggled( bool ) ), this, SLOT( slotSortBySize( bool ) ) );

    KToggleAction *aSortDescending = new KToggleAction( i18n( "Descending" ), 0, actionCollection(), "sort_descend" );

    connect( aSortDescending, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDescending() ) );
    /*
    m_pamSort->insert( aSortByNameCS );
    m_pamSort->insert( aSortByNameCI );
    m_pamSort->insert( aSortBySize );

    m_pamSort->popupMenu()->insertSeparator();

    m_pamSort->insert( aSortDescending );
    */
    m_paSelect = new KAction( i18n( "&Select..." ), CTRL+Key_Slash, this, SLOT( slotSelect() ), actionCollection(), "select" );
    m_paUnselect = new KAction( i18n( "&Unselect..." ), CTRL+Key_Backslash, this, SLOT( slotUnselect() ), actionCollection(), "unselect" );
    m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), this, "selectall" );
    m_paUnselectAll = new KAction( i18n( "U&nselect All" ), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
    m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), CTRL+Key_I, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );

    m_paLargeIcons = new KToggleAction( i18n( "&Large View" ), 0, actionCollection(), "largeview" );
    m_paNormalIcons = new KToggleAction( i18n( "&Normal View" ), 0, actionCollection(), "normalview" );
    m_paSmallIcons = new KToggleAction( i18n( "&Small View" ), 0, actionCollection(), "smallview" );
    //m_paKOfficeMode = new KToggleAction( i18n( "&KOffice mode" ), 0, this );

    m_paLargeIcons->setExclusiveGroup( "ViewMode" );
    m_paNormalIcons->setExclusiveGroup( "ViewMode" );
    m_paSmallIcons->setExclusiveGroup( "ViewMode" );
    //m_paKOfficeMode->setExclusiveGroup( "ViewMode" );

    m_paLargeIcons->setChecked( false );
    m_paNormalIcons->setChecked( true );
    m_paSmallIcons->setChecked( false );
    //m_paKOfficeMode->setChecked( false );
    //m_paKOfficeMode->setEnabled( false );

    m_paBottomText = new KToggleAction( i18n( "Text at the &bottom" ), 0, actionCollection(), "textbottom" );
    m_paRightText = new KToggleAction( i18n( "Text at the &right" ), 0, actionCollection(), "textright" );

    m_paBottomText->setExclusiveGroup( "TextPos" );
    m_paRightText->setExclusiveGroup( "TextPos" );

    m_paBottomText->setChecked( true );
    m_paRightText->setChecked( false );

    /*KAction * paBackgroundColor =*/ new KAction( i18n( "Background Color..." ), 0, this, SLOT( slotBackgroundColor() ), actionCollection(), "bgcolor" );
    /*KAction * paBackgroundImage =*/ new KAction( i18n( "Background Image..." ), 0, this, SLOT( slotBackgroundImage() ), actionCollection(), "bgimage" );

    //

    connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewLarge( bool ) ) );
    connect( m_paNormalIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewNormal( bool ) ) );
    connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewSmall( bool ) ) );
    //connect( m_paKOfficeMode, SIGNAL( toggled( bool ) ), this, SLOT( slotKofficeMode( bool ) ) );

    connect( m_paBottomText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextBottom( bool ) ) );
    connect( m_paRightText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextRight( bool ) ) );

    //
    /*
    actions()->append( BrowserView::ViewAction( m_paImagePreview, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paDotFiles, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_pamSort, BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( paBackgroundColor, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( paBackgroundImage, BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( m_paBottomText, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paRightText, BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( new QActionSeparator( this ), BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( m_paLargeIcons, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paNormalIcons, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paSmallIcons, BrowserView::MenuView ) );
    //actions()->append( BrowserView::ViewAction( m_paKOfficeMode, BrowserView::MenuView ) );


    actions()->append( BrowserView::ViewAction( m_paSelect, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paUnselect, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paSelectAll, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paUnselectAll, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paInvertSelection, BrowserView::MenuEdit ) );
    */

    QObject::connect( m_pIconView, SIGNAL( doubleClicked( QIconViewItem * ) ),
		      this, SLOT( slotReturnPressed( QIconViewItem * ) ) );
    QObject::connect( m_pIconView, SIGNAL( returnPressed( QIconViewItem * ) ),
		      this, SLOT( slotReturnPressed( QIconViewItem * ) ) );
		
    QObject::connect( m_pIconView, SIGNAL( onItem( QIconViewItem * ) ),
		      this, SLOT( slotOnItem( QIconViewItem * ) ) );
		
    QObject::connect( m_pIconView, SIGNAL( onViewport() ),
		      this, SLOT( slotOnViewport() ) );
		
    QObject::connect( m_pIconView, SIGNAL( mouseButtonPressed(int, QIconViewItem*, const QPoint&)),
		      this, SLOT( slotMouseButtonPressed(int, QIconViewItem*, const QPoint&)) );
    QObject::connect( m_pIconView, SIGNAL( rightButtonPressed( QIconViewItem *, const QPoint &) ),
		      this, SLOT( slotViewportRightClicked( QIconViewItem * ) ) );

    // Now we may react to configuration changes
    m_bInit = false;

    m_dirLister = 0L;
    m_bLoading = false;
    m_bNeedAlign = false;

    m_pIconView->setResizeMode( QIconView::Adjust );
    // KDE extension : KIconLoader size
    m_pIconView->setIcons( KIconLoader::Medium ); // TODO : part of KonqPropsView

    m_eSortCriterion = NameCaseInsensitive;

    m_paImagePreview->setChecked( m_pProps->m_bImagePreview );

    m_lDirSize = 0;
    m_lFileCount = 0;
    m_lDirCount = 0;

    connect( m_pIconView, SIGNAL( selectionChanged() ),
	     this, SLOT( slotDisplayFileSelectionInfo() ) );
}

KonqKfmIconView::~KonqKfmIconView()
{
    kdebug(0, 1202, "-KonqKfmIconView");
    if ( m_dirLister ) delete m_dirLister;
    delete m_pProps;
    //no need for that, KParts deletes our widget already ;-)
    //    delete m_pIconView;
}

void KonqKfmIconView::slotImagePreview( bool toggle )
{
    m_pProps->m_bImagePreview = toggle;
    m_pIconView->setImagePreviewAllowed ( m_pProps->m_bImagePreview );
    m_pIconView->arrangeItemsInGrid( true );
}

void KonqKfmIconView::slotShowDot()
{
    kdebug(0, 1202, "KonqKfmIconView::slotShowDot()");
    m_pProps->m_bShowDot = !m_pProps->m_bShowDot;
    m_dirLister->setShowingDotFiles( m_pProps->m_bShowDot );
    //we don't want the non-dot files to remain where they are
    m_bNeedAlign = true;
}

void KonqKfmIconView::slotSelect()
{
    KLineEditDlg l( i18n("Select files:"), "", m_pIconView );
    if ( l.exec() )
    {
	QString pattern = l.text();
	if ( pattern.isEmpty() )
	    return;

	QRegExp re( pattern, true, true );

	m_pIconView->blockSignals( true );

	QIconViewItem *it = m_pIconView->firstItem();
	while ( it )
	{
	    if ( re.match( it->text() ) != -1 )
		it->setSelected( true, true );
	    it = it->nextItem();
	}

	m_pIconView->blockSignals( false );

        // do this once, not for each item
	m_pIconView->slotSelectionChanged();
        slotDisplayFileSelectionInfo();
    }
}

void KonqKfmIconView::slotUnselect()
{
    KLineEditDlg l( i18n("Unselect files:"), "", m_pIconView );
    if ( l.exec() )
    {
	QString pattern = l.text();
	if ( pattern.isEmpty() )
	    return;

	QRegExp re( pattern, true, true );

	m_pIconView->blockSignals( true );

	QIconViewItem *it = m_pIconView->firstItem();
	while ( it )
	{
	    if ( re.match( it->text() ) != -1 )
		it->setSelected( false, true );
	    it = it->nextItem();
	}

	m_pIconView->blockSignals( false );

        // do this once, not for each item
	m_pIconView->slotSelectionChanged();
        slotDisplayFileSelectionInfo();
    }
}

void KonqKfmIconView::slotSelectAll()
{
    m_pIconView->selectAll( true );
}

void KonqKfmIconView::slotUnselectAll()
{
    m_pIconView->selectAll( false );
}

void KonqKfmIconView::slotInvertSelection()
{
    m_pIconView->invertSelection( );
}

void KonqKfmIconView::slotSortByNameCaseSensitive( bool toggle )
{
    if ( !toggle )
	return;

    setupSorting( NameCaseSensitive );
}

void KonqKfmIconView::slotSortByNameCaseInsensitive( bool toggle )
{
    if ( !toggle )
	return;

    setupSorting( NameCaseInsensitive );
}

void KonqKfmIconView::slotSortBySize( bool toggle )
{
    if ( !toggle )
	return;

    setupSorting( Size );
}

void KonqKfmIconView::setupSorting( SortCriterion criterion )
{
    m_eSortCriterion = criterion;

    setupSortKeys();

    m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::slotSortDescending()
{
    if ( m_pIconView->sortDirection() )
	m_pIconView->setSorting( true, false );
    else
	m_pIconView->setSorting( true, true );

    m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
  if ( event->activated() )
    m_pIconView->slotSelectionChanged();
} 

void KonqKfmIconView::slotKofficeMode( bool b )
{
    if ( b )
    {
    /* emit openURLRequest() signal with serviceType argument set instead (Simon)
    QObject *obj = parent();
	while ( obj )
	{
	    if ( obj->inherits( "KonqFrame" ) )
		break;
	    obj = obj->parent();
	}

	if ( obj && obj->inherits( "KonqFrame" ) )
	{
	    KonqChildView *childView = ((KonqFrame *)obj)->childView();
	    // TODO switch to koffice view mode here
	    childView->changeViewMode( "inode/directory", url(), false, "KonqTreeView" );
	}
    */
    }
}

void KonqKfmIconView::slotViewLarge( bool b )
{
    if ( b )
    {
	m_pIconView->setIcons( KIconLoader::Large );
	m_pIconView->arrangeItemsInGrid( true );
    }
}

void KonqKfmIconView::slotViewNormal( bool b )
{
    if ( b )
    {
	m_pIconView->setIcons( KIconLoader::Medium );
	m_pIconView->arrangeItemsInGrid( true );
    }
}

void KonqKfmIconView::slotViewSmall( bool b )
{
    if ( b )
    {
	m_pIconView->setIcons( KIconLoader::Small );
	m_pIconView->arrangeItemsInGrid( true );
    }
}

void KonqKfmIconView::slotTextBottom( bool b )
{
    if ( b ) {
	m_pIconView->setGridX( 70 );
	m_pIconView->setItemTextPos( QIconView::Bottom );
    }
}

void KonqKfmIconView::slotTextRight( bool b )
{
    if ( b ) {
	m_pIconView->setGridX( 120 );
	m_pIconView->setItemTextPos( QIconView::Right );
    }
}

void KonqKfmIconView::slotBackgroundColor()
{
    QColor bgndColor;
    if ( KColorDialog::getColor( bgndColor ) == KColorDialog::Accepted )
    {
	m_pProps->m_bgColor = bgndColor;
	m_pProps->m_bgPixmap = QPixmap();
	m_pIconView->viewport()->setBackgroundColor( m_pProps->m_bgColor );
	m_pIconView->viewport()->setBackgroundPixmap( m_pProps->m_bgPixmap );
	m_pProps->saveLocal( m_dirLister->url() );
	m_pIconView->updateContents();
    }
}

void KonqKfmIconView::slotBackgroundImage()
{
    KonqBgndDialog dlg( m_dirLister->url() );
    if ( dlg.exec() == KonqBgndDialog::Accepted )
    {
	m_pProps->m_bgPixmap = dlg.pixmap();
	m_pIconView->viewport()->setBackgroundColor( m_pProps->m_bgColor );
	m_pIconView->viewport()->setBackgroundPixmap( m_pProps->m_bgPixmap );
	// no need to savelocal, the dialog does it
	m_pIconView->updateContents();
    }
}

bool KonqKfmIconView::closeURL()
{
    debug("KonqKfmIconView::stop()");
    if ( m_dirLister ) m_dirLister->stop();
    return true;
}

void KonqKfmIconView::saveState( QDataStream &stream )
{
    stream << (Q_INT32)m_pIconView->size()
	   << (Q_INT32)m_pIconView->itemTextPos()
	   << (Q_INT32)m_pProps->m_bImagePreview
	   << (Q_INT32)m_pProps->m_bShowDot
	   << (Q_INT32)m_pProps->m_bHTMLAllowed;
}

void KonqKfmIconView::restoreState( QDataStream &stream )
{
    Q_INT32 iIconSize, iTextPos, iImagePreview, iShowDot, iHTMLAllowed;

    stream >> iIconSize >> iTextPos >> iImagePreview >> iShowDot >> iHTMLAllowed;

    KIconLoader::Size iconSize = (KIconLoader::Size)iIconSize;
    QIconView::ItemTextPos textPos = (QIconView::ItemTextPos)iTextPos;

    switch ( iconSize )
    {
    case KIconLoader::Large: m_paLargeIcons->setChecked( true ); break;
    case KIconLoader::Medium: m_paNormalIcons->setChecked( true ); break;
    case KIconLoader::Small: m_paSmallIcons->setChecked( true ); break;
    }

    if ( textPos == QIconView::Bottom )
	m_paBottomText->setChecked( true );
    else
	m_paRightText->setChecked( true );

    m_paImagePreview->setChecked( (bool) iImagePreview );
    m_paDotFiles->setChecked( (bool) iShowDot );
    // TODO apply HTML allowed
}

void KonqKfmIconView::slotReturnPressed( QIconViewItem *item )
{
    if ( !item )
	return;
    KFileItem *fileItem = ((KFileIVI*)item)->item();
    if ( !fileItem )
	return;
    if (m_pSettings->alwaysNewWin() && fileItem->mode() & S_IFDIR) {
	fileItem->run();
    } else {
	QString serviceType = QString::null;

	KURL u( fileItem->url() );

	if ( u.isLocalFile() )
	    serviceType = fileItem->mimetype();

        emit m_extension->openURLRequest( u, false, 0, 0, fileItem->mimetype() );
    }
}

void KonqKfmIconView::slotMouseButtonPressed(int _button, QIconViewItem* _item, const QPoint& _global)
{
    if(_item) {
	switch(_button) {
	case RightButton:
	    ((KFileIVI*)_item)->setSelected( true );
	    emit m_extension->popupMenu( _global, m_pIconView->selectedFileItems() );
	    break;
	case MidButton:
	    // New view
	    ((KFileIVI*)_item)->item()->run();
	    break;
	}
    }
}

void KonqKfmIconView::slotViewportRightClicked( QIconViewItem *i )
{
    if ( i )
	return;
    KURL bgUrl( m_dirLister->url() );

    // This is a directory. Always.
    mode_t mode = S_IFDIR;

    KFileItem item( mode, bgUrl );
    KFileItemList items;
    items.append( &item );
    emit m_extension->popupMenu( QCursor::pos(), items );
}

void KonqKfmIconView::slotStarted( const QString & /*url*/ )
{
    m_pIconView->selectAll( false );
    if ( m_bLoading )
	emit started( 0 /* no iojob */ );
    m_lstPendingMimeIconItems.clear();
}

void KonqKfmIconView::slotCanceled()
{
    if ( m_bLoading )
    {
        emit canceled( QString::null );
	m_bLoading = false;
    }
}

void KonqKfmIconView::slotCompleted()
{
    if ( m_bLoading )
    {
	emit completed();
	m_bLoading = false;
    }
    m_pIconView->setContentsPos( m_iXOffset, m_iYOffset );
    //m_paKOfficeMode->setEnabled( m_dirLister->kofficeDocsFound() );

    slotOnViewport();

    QTimer::singleShot( 0, this, SLOT( slotProcessMimeIcons() ) );
}

void KonqKfmIconView::slotNewItem( KFileItem * _fileitem )
{
    if ( !S_ISDIR( _fileitem->mode() ) )
    {
	m_lDirSize += _fileitem->size();
	m_lFileCount++;
    }
    else
	m_lDirCount++;

//  kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotNewItem(...)");
    KFileIVI* item = new KFileIVI( m_pIconView, _fileitem,
				   m_pIconView->size(), m_pProps->m_bImagePreview );
    item->setRenameEnabled( false );

    QObject::connect( item, SIGNAL( dropMe( KFileIVI *, QDropEvent * ) ),
		      m_pIconView, SLOT( slotDropItem( KFileIVI *, QDropEvent * ) ) );

    QString key;

    switch ( m_eSortCriterion )
    {
    case NameCaseSensitive: key = item->text(); break;
    case NameCaseInsensitive: key = item->text().lower(); break;
    case Size: key = makeSizeKey( item ); break;
    }

    item->setKey( key );

    if ( m_ulTotalFiles > 0 )
      emit m_extension->loadingProgress( ( m_pIconView->count() * 100 ) / m_ulTotalFiles );

    m_lstPendingMimeIconItems.append( item );
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
    if ( !S_ISDIR( _fileitem->mode() ) )
    {
	m_lDirSize -= _fileitem->size();
	m_lFileCount--;
    }
    else
	m_lDirCount--;

    //kdebug( KDEBUG_INFO, 1202, "KonqKfmIconView::slotDeleteItem(...)");
    // we need to find out the iconcontainer item containing the fileitem
    QIconViewItem *it = m_pIconView->firstItem();
    while ( it )
    {
	if ( ((KFileIVI*)it)->item() == _fileitem ) // compare the pointers
	{
	    m_pIconView->takeItem( it );
	    break;
	}
	it = it->nextItem();
    }
}

void KonqKfmIconView::slotClear()
{
    m_pIconView->clear();
    m_lstPendingMimeIconItems.clear();
}

void KonqKfmIconView::slotTotalFiles( int, unsigned long files )
{
    m_ulTotalFiles = files;
}

void KonqKfmIconView::slotDisplayFileSelectionInfo()
{
    long fileSizeSum = 0;
    long fileCount = 0;
    long dirCount = 0;

    KFileItemList lst = m_pIconView->selectedFileItems();
    KFileItemListIterator it( lst );

    for (; it.current(); ++it )
	if ( S_ISDIR( it.current()->mode() ) )
	    dirCount++;
	else
	{
	    fileSizeSum += it.current()->size();
	    fileCount++;
	}

    if ( lst.count() > 0 )
        emit m_extension->setStatusBarText( i18n( "%1 %2 - %3 %4 (%5 Total) - %6 %7" )
            .arg( lst.count() )
            .arg( ( lst.count() == 1 ) ? i18n( "Item" ) : i18n( "Items" ) )
            .arg( fileCount )
            .arg( ( fileCount == 1 ) ? i18n( "File" ) : i18n( "Files" ) )
            .arg( KIOJob::convertSize( fileSizeSum ) )
            .arg( dirCount )
            .arg( ( dirCount == 1 ) ? i18n( "Directory" ) : i18n( "Directories" ) ) );
    else
	slotOnViewport();
}

void KonqKfmIconView::slotProcessMimeIcons()
{
    if ( m_lstPendingMimeIconItems.count() == 0 ) {
	if ( m_bNeedAlign )
	    m_pIconView->arrangeItemsInGrid();
	return;
    }

    KFileIVI *item = m_lstPendingMimeIconItems.first();

    QPixmap *currentIcon = item->pixmap();

    KMimeType::Ptr dummy = item->item()->determineMimeType();

    QPixmap newIcon = item->item()->pixmap( m_pIconView->size(), m_pProps->m_bImagePreview );

    bool recalc = !m_pProps->m_bImagePreview;
    if ( currentIcon->serialNumber() != newIcon.serialNumber() )
    {
	item->QIconViewItem::setPixmap( newIcon, recalc, true );
	if ( m_pProps->m_bImagePreview )
	    m_bNeedAlign = true;
    }

    m_lstPendingMimeIconItems.removeFirst();
    QTimer::singleShot( 0, this, SLOT( slotProcessMimeIcons() ) );
}

bool KonqKfmIconView::openURL( const KURL &_url )
{
    if ( !m_dirLister )
    {
	// Create the directory lister
	m_dirLister = new KDirLister( true );

	QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ),
			  this, SLOT( slotStarted( const QString & ) ) );
	QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
	QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
	QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
	QObject::connect( m_dirLister, SIGNAL( newItem( KFileItem * ) ),
			  this, SLOT( slotNewItem( KFileItem * ) ) );
	QObject::connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
			  this, SLOT( slotDeleteItem( KFileItem * ) ) );
    }

    m_bLoading = true;
    m_lDirSize = 0;
    m_lFileCount = 0;
    m_lDirCount = 0;

    m_url = _url;

    KURL u( _url );
    // Start the directory lister !
    m_dirLister->openURL( u, m_pProps->m_bShowDot );
    // Note : we don't store the url. KDirLister does it for us.

    m_pIconView->setURL( _url.url() );

    KIOJob *job = KIOJob::find( m_dirLister->jobId() );
    if ( job )
    {
	connect( job, SIGNAL( sigTotalFiles( int, unsigned long ) ),
		 this, SLOT( slotTotalFiles( int, unsigned long ) ) );
    }

    m_ulTotalFiles = 0;
    m_bNeedAlign = false;

    // do it after starting the dir lister to avoid changing bgcolor of the
    // old view
    if ( m_pProps->enterDir( u ) )
    {
	m_pIconView->viewport()->setBackgroundColor( m_pProps->m_bgColor );
	m_pIconView->viewport()->setBackgroundPixmap( m_pProps->m_bgPixmap );
    }

#ifdef __GNUC__
#warning FIXME (Simon)
#endif
//  setCaptionFromURL( _url );
    m_pIconView->show(); // ?
    return true;
}

void KonqKfmIconView::slotOnItem( QIconViewItem *item )
{
  emit m_extension->setStatusBarText( ((KFileIVI *)item)->item()->getStatusBarInfo() );
}

void KonqKfmIconView::slotOnViewport()
{
    QIconViewItem *it = m_pIconView->firstItem();
    for (; it; it = it->nextItem() )
	if ( it->isSelected() )
	{
	    slotDisplayFileSelectionInfo();
	    return;
	}

    emit m_extension->setStatusBarText( i18n( "%1 %2 - %3 %4 (%5 Total) - %6 %7" )
        .arg( m_pIconView->count() )
        .arg( ( m_pIconView->count() == 1 ) ? i18n( "Item" ) : i18n( "Items" ) )
        .arg( m_lFileCount )
        .arg( ( m_lFileCount == 1 ) ? i18n( "File" ) : i18n( "Files" ) )
        .arg( KIOJob::convertSize( m_lDirSize ) )
        .arg( m_lDirCount )
        .arg( ( m_lDirCount == 1 ) ? i18n( "Directory" ) : i18n( "Directories" ) ) );
}

void KonqKfmIconView::setupSortKeys()
{

    switch ( m_eSortCriterion )
    {
    case NameCaseSensitive:
	for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
	    it->setKey( it->text() );
	break;
    case NameCaseInsensitive:
	for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
	    it->setKey( it->text().lower() );
	break;
    case Size:
	for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
	    it->setKey( makeSizeKey( (KFileIVI *)it ) );
	break;
    }
}

QString KonqKfmIconView::makeSizeKey( KFileIVI *item )
{
    return QString::number( item->item()->size() ).rightJustify( 20, '0' );
}

#include "konq_iconview.moc"
