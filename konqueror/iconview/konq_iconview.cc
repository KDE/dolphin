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
#include <konqfileitem.h>
#include <kio/job.h>
#include <klibloader.h>
#include <klineeditdlg.h>
#include <kmimetype.h>
#include <konqiconviewwidget.h>
#include <konqsettings.h>
#include <kpropsdlg.h>
#include <krun.h>
#include <kstdaction.h>
#include <kurl.h>
#include <kparts/mainwindow.h>
#include <kparts/partmanager.h>
#include <kparts/factory.h>
#include <kiconloader.h>

#include <qmessagebox.h>
#include <qfile.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <klocale.h>
#include <qregexp.h>
#include <qvaluelist.h>

template class QList<KFileIVI>;
template class QValueList<int>;

class KonqIconViewFactory : public KParts::Factory
{
public:
    KonqIconViewFactory()
    {
      s_defaultViewProps = 0;
      s_instance = 0;
    }
    virtual ~KonqIconViewFactory()
    {
      if ( s_instance )
        delete s_instance;

      if ( s_defaultViewProps )
        delete s_defaultViewProps;

      s_instance = 0;
      s_defaultViewProps = 0;
    }

    virtual KParts::Part* createPart( QWidget *parentWidget, const char *, QObject *parent, const char *name, const char*, const QStringList &/*&args*/ )
    {
	KonqKfmIconView *obj = new KonqKfmIconView( parentWidget, parent, name );
	emit objectCreated( obj );
	return obj;
    }

    static KInstance *instance()
    {
      if ( !s_instance )
        s_instance = new KInstance( "konqiconview" );
      return s_instance;
    }

   static KonqPropsView *defaultViewProps()
   {
     if ( !s_defaultViewProps )
       s_defaultViewProps = KonqPropsView::defaultProps( instance() );

     return s_defaultViewProps;
   }

private:
  static KInstance *s_instance;
  static KonqPropsView *s_defaultViewProps;
};

KInstance *KonqIconViewFactory::s_instance = 0;
KonqPropsView *KonqIconViewFactory::s_defaultViewProps = 0;

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
    m_iconView->m_pProps->saveLocal( m_iconView->url() );
}

void IconViewBrowserExtension::savePropertiesAsDefault()
{
    m_iconView->m_pProps->saveAsDefault( KonqIconViewFactory::instance() );
}

void IconViewBrowserExtension::properties()
{
    KFileItem * item = m_iconView->iconViewWidget()->selectedFileItems().first();
    (void) new PropertiesDialog( item );
}

void IconViewBrowserExtension::editMimeType()
{
    KFileItem * item = m_iconView->iconViewWidget()->selectedFileItems().first();
    KonqOperations::editMimeType( item->mimetype() );
}


KonqKfmIconView::KonqKfmIconView( QWidget *parentWidget, QObject *parent, const char *name )
    : KParts::ReadOnlyPart( parent, name )
{
    kdDebug(1202) << "+KonqKfmIconView" << endl;

    setInstance( KonqIconViewFactory::instance() );

    m_extension = new IconViewBrowserExtension( this );

    setXMLFile( "konq_iconview.rc" );

    // Create a properties instance for this view
    // (copying the default values)
    //    m_pProps = new KonqPropsView( * KonqPropsView::defaultProps( KonqIconViewFactory::instance() ) );
    m_pProps = new KonqPropsView( * KonqIconViewFactory::defaultViewProps() );

    m_pIconView = new KonqIconViewWidget( parentWidget, "qiconview" );

    // When our viewport is adjusted (resized or scrolled) we need
    // to get the mime types for any newly visible icons. (Rikkus)
    connect(
      m_pIconView,  SIGNAL(viewportAdjusted()),
      this,         SLOT(slotProcessMimeIcons()));

    // pass signals to the extension
    connect( m_pIconView, SIGNAL( enableAction( const char *, bool ) ),
             m_extension, SIGNAL( enableAction( const char *, bool ) ) );

    setWidget( m_pIconView );

    //m_ulTotalFiles = 0;

    // Don't repaint on configuration changes during construction
    m_bInit = true;

    m_paDotFiles = new KToggleAction( i18n( "Show &Dot Files" ), 0, this, SLOT( slotShowDot() ), actionCollection(), "show_dot" );
    m_paImagePreview = new KToggleAction( i18n( "&Image Preview" ), 0, actionCollection(), "image_preview" );

    connect( m_paImagePreview, SIGNAL( toggled( bool ) ), this, SLOT( slotImagePreview( bool ) ) );

    //    m_pamSort = new KActionMenu( i18n( "Sort..." ), actionCollection(), "sort" );

    KToggleAction *aSortByNameCS = new KRadioAction( i18n( "by Name (Case Sensitive)" ), 0, actionCollection(), "sort_nc" );
    KToggleAction *aSortByNameCI = new KRadioAction( i18n( "by Name (Case Insensitive)" ), 0, actionCollection(), "sort_nci" );
    KToggleAction *aSortBySize = new KRadioAction( i18n( "by Size" ), 0, actionCollection(), "sort_size" );

    aSortByNameCS->setExclusiveGroup( "sorting" );
    aSortByNameCI->setExclusiveGroup( "sorting" );
    aSortBySize->setExclusiveGroup( "sorting" );

    aSortByNameCS->setChecked( true );
    aSortByNameCI->setChecked( false );
    aSortBySize->setChecked( false );

    connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseSensitive( bool ) ) );
    connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseInsensitive( bool ) ) );
    connect( aSortBySize, SIGNAL( toggled( bool ) ), this, SLOT( slotSortBySize( bool ) ) );

    m_paSortDirsFirst = new KToggleAction( i18n( "Directories first" ), 0, actionCollection(), "sort_directoriesfirst" );
    KToggleAction *aSortDescending = new KToggleAction( i18n( "Descending" ), 0, actionCollection(), "sort_descend" );

    m_paSortDirsFirst->setChecked( true );

    connect( aSortDescending, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDescending() ) );
    connect( m_paSortDirsFirst, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDirsFirst() ) );
    /*
    m_pamSort->insert( aSortByNameCS );
    m_pamSort->insert( aSortByNameCI );
    m_pamSort->insert( aSortBySize );

    m_pamSort->popupMenu()->insertSeparator();

    m_pamSort->insert( aSortDescending );
    */
    m_paSelect = new KAction( i18n( "&Select..." ), CTRL+Key_Plus, this, SLOT( slotSelect() ), actionCollection(), "select" );
    m_paUnselect = new KAction( i18n( "&Unselect..." ), CTRL+Key_Minus, this, SLOT( slotUnselect() ), actionCollection(), "unselect" );
    m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), this, "selectall" );
    m_paUnselectAll = new KAction( i18n( "U&nselect All" ), CTRL+Key_U, this, SLOT( slotUnselectAll() ), actionCollection(), "unselectall" );
    m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), CTRL+Key_Asterisk, this, SLOT( slotInvertSelection() ), actionCollection(), "invertselection" );

    m_paDefaultIcons = new KToggleAction( i18n( "&Default Size" ), 0, actionCollection(), "modedefault" );
    m_paLargeIcons = new KToggleAction( i18n( "&Large" ), 0, actionCollection(), "modelarge" );
    m_paMediumIcons = new KToggleAction( i18n( "&Medium" ), 0, actionCollection(), "modemedium" );
    m_paSmallIcons = new KToggleAction( i18n( "&Small" ), 0, actionCollection(), "modesmall" );
    m_paNoIcons = new KToggleAction( i18n( "&Disabled" ), 0, actionCollection(), "modenone" );
    //m_paKOfficeMode = new KToggleAction( i18n( "&KOffice mode" ), 0, this );

    m_paDefaultIcons->setExclusiveGroup( "ViewMode" );
    m_paLargeIcons->setExclusiveGroup( "ViewMode" );
    m_paMediumIcons->setExclusiveGroup( "ViewMode" );
    m_paSmallIcons->setExclusiveGroup( "ViewMode" );
    m_paNoIcons->setExclusiveGroup( "ViewMode" );
    //m_paKOfficeMode->setExclusiveGroup( "ViewMode" );

    m_paDefaultIcons->setChecked( true );
    m_paLargeIcons->setChecked( false );
    m_paMediumIcons->setChecked( false );
    m_paSmallIcons->setChecked( false );
    m_paNoIcons->setChecked( false );
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

    connect( m_paDefaultIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewDefault( bool ) ) );
    connect( m_paLargeIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewLarge( bool ) ) );
    connect( m_paMediumIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewMedium( bool ) ) );
    connect( m_paSmallIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewSmall( bool ) ) );
    //connect( m_paKOfficeMode, SIGNAL( toggled( bool ) ), this, SLOT( slotKofficeMode( bool ) ) );
    connect( m_paNoIcons, SIGNAL( toggled( bool ) ), this, SLOT( slotViewNone( bool ) ) );

    connect( m_paBottomText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextBottom( bool ) ) );
    connect( m_paRightText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextRight( bool ) ) );

    //
    /*
    actions()->append( BrowserView::ViewAction( m_paImagePreview, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paDotFiles, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_pamSort, BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( new KActionSeparator( this ), BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( paBackgroundColor, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( paBackgroundImage, BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( new KActionSeparator( this ), BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( m_paBottomText, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paRightText, BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( new KActionSeparator( this ), BrowserView::MenuView ) );

    actions()->append( BrowserView::ViewAction( m_paLargeIcons, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paMediumIcons, BrowserView::MenuView ) );
    actions()->append( BrowserView::ViewAction( m_paSmallIcons, BrowserView::MenuView ) );
    //actions()->append( BrowserView::ViewAction( m_paKOfficeMode, BrowserView::MenuView ) );


    actions()->append( BrowserView::ViewAction( m_paSelect, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paUnselect, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paSelectAll, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paUnselectAll, BrowserView::MenuEdit ) );
    actions()->append( BrowserView::ViewAction( m_paInvertSelection, BrowserView::MenuEdit ) );
    */

    QObject::connect( m_pIconView, SIGNAL( executed( QIconViewItem * ) ),
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

    // Extract 3 icon sizes from the icon theme. Use 16,32,48 as default.
    int i;
    m_iIconSize[0] = 16;
    m_iIconSize[1] = 32;
    m_iIconSize[2] = 48;
    KIconTheme *root = KGlobal::instance()->iconLoader()->theme();
    QValueList<int> avSizes = root->querySizes(KIcon::Desktop);
    QValueList<int>::Iterator it;
    for (i=0, it=avSizes.begin(); (it!=avSizes.end()) && (i<3); it++, i++)
    {
	m_iIconSize[i] = *it;
    }

    // Now we may react to configuration changes
    m_bInit = false;

    m_dirLister = 0L;
    m_bLoading = false;
    m_bNeedAlign = false;

    m_pIconView->setResizeMode( QIconView::Adjust );
    m_pIconView->setIcons( 0 ); // TODO : part of KonqPropsView

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
    kdDebug(1202) << "-KonqKfmIconView" << endl;
    if ( m_dirLister ) delete m_dirLister;
    delete m_pProps;
    //no need for that, KParts deletes our widget already ;-)
    //    delete m_pIconView;
}

void KonqKfmIconView::slotImagePreview( bool toggle )
{
    m_pProps->m_bImagePreview = toggle;
    if ( !m_pProps->m_bImagePreview ) {
	if ( m_pIconView->itemTextPos() == QIconView::Bottom )
	    m_pIconView->setGridX( 70 );
	else
	    m_pIconView->setGridX( 120 );
    }
    m_pIconView->setImagePreviewAllowed ( m_pProps->m_bImagePreview );
    if ( m_pProps->m_bImagePreview ) {
	QIconViewItem *i = m_pIconView->firstItem();
	for ( ; i; i = i->nextItem() ) {
	    m_pIconView->setGridX( QMAX( m_pIconView->gridX(), i->width() ) );
	}
    }
    m_pIconView->arrangeItemsInGrid( TRUE );
}

void KonqKfmIconView::slotShowDot()
{
    kdDebug(1202) << "KonqKfmIconView::slotShowDot()" << endl;
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

void KonqKfmIconView::slotSortDirsFirst()
{
  m_pIconView->setSortDirectoriesFirst( m_paSortDirsFirst->isChecked() );

  setupSortKeys();

  m_pIconView->sort( m_pIconView->sortDirection() );
}

void KonqKfmIconView::guiActivateEvent( KParts::GUIActivateEvent *event )
{
  ReadOnlyPart::guiActivateEvent( event );
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
	m_pIconView->setIcons( m_iIconSize[2] );
	m_pIconView->arrangeItemsInGrid( true );
    }
}

void KonqKfmIconView::slotViewMedium( bool b )
{
    if ( b )
    {
	m_pIconView->setIcons( m_iIconSize[1] );
	m_pIconView->arrangeItemsInGrid( true );
    }
}

void KonqKfmIconView::slotViewNone( bool /*b*/ )
{
  //TODO: Disable Icons
}

void KonqKfmIconView::slotViewSmall( bool b )
{
    if ( b )
    {
	m_pIconView->setIcons( m_iIconSize[0] );
	m_pIconView->arrangeItemsInGrid( true );
    }
}

void KonqKfmIconView::slotViewDefault( bool b)
{
    if ( b )
    {
	m_pIconView->setIcons( 0 );
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
	m_pProps->saveLocal( url() );
	m_pIconView->updateContents();
    }
}

void KonqKfmIconView::slotBackgroundImage()
{
    KonqBgndDialog dlg( url(), KonqIconViewFactory::instance() );
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
    m_lstPendingMimeIconItems.clear();
    return true;
}

void KonqKfmIconView::saveState( QDataStream &stream )
{
    stream << (Q_INT32)m_pIconView->iconSize()
	   << (Q_INT32)m_pIconView->itemTextPos()
	   << (Q_INT32)m_pProps->m_bImagePreview
	   << (Q_INT32)m_pProps->m_bShowDot
	   << (Q_INT32)m_pProps->m_bHTMLAllowed;
}

void KonqKfmIconView::restoreState( QDataStream &stream )
{
    Q_INT32 iIconSize, iTextPos, iImagePreview, iShowDot, iHTMLAllowed;

    stream >> iIconSize >> iTextPos >> iImagePreview >> iShowDot >> iHTMLAllowed;

    QIconView::ItemTextPos textPos = (QIconView::ItemTextPos)iTextPos;

    if (iIconSize == m_iIconSize[0])
	m_paSmallIcons->setChecked( true );
    else if (iIconSize == m_iIconSize[1])
	m_paMediumIcons->setChecked( true );
    else if (iIconSize == m_iIconSize[2])
	m_paLargeIcons->setChecked( true );
    else
	m_paDefaultIcons->setChecked( true );

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
    KonqFileItem *fileItem = (static_cast<KFileIVI*>(item))->item();
    if ( !fileItem )
	return;
    if (KonqFMSettings::settings()->alwaysNewWin() && fileItem->mode() & S_IFDIR) {
	fileItem->run();
    } else {
        // We want to emit openURLRequest, but not right now, because
        // the iconview is going to emit other signals.
        // Let's not destroy it while it isn't finished emitting.
        openURLRequestFileItem = fileItem;
        QTimer::singleShot( 0, this, SLOT(slotOpenURLRequest()) );
    }
}

void KonqKfmIconView::slotOpenURLRequest()
{
    KParts::URLArgs args;
    args.serviceType = openURLRequestFileItem->mimetype();
    emit m_extension->openURLRequest( openURLRequestFileItem->url(), args );
}

void KonqKfmIconView::slotMouseButtonPressed(int _button, QIconViewItem* _item, const QPoint& _global)
{
    if(_item) {
	switch(_button) {
	case RightButton:
	    (static_cast<KFileIVI*>(_item))->setSelected( true );
	    emit m_extension->popupMenu( _global, m_pIconView->selectedFileItems() );
	    break;
	case MidButton:
	    // New view
	    (static_cast<KFileIVI*>(_item))->item()->run();
	    break;
	}
    }
}

void KonqKfmIconView::slotViewportRightClicked( QIconViewItem *i )
{
    if ( i )
	return;
    if ( ! m_dirLister->rootItem() )
        return; // too early, '.' not yet listed

    KFileItemList items;
    items.append( m_dirLister->rootItem() );
    emit m_extension->popupMenu( QCursor::pos(), items );
}

void KonqKfmIconView::slotStarted( const QString & /*url*/ )
{
    m_pIconView->selectAll( false );
    if ( m_bLoading )
	emit started( m_dirLister->job() );
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
    // Root item ? Store in konqiconviewwidget
    if ( m_dirLister->rootItem() )
      m_pIconView->setRootItem( m_dirLister->rootItem() );

    m_pIconView->setContentsPos( m_extension->urlArgs().xOffset, m_extension->urlArgs().yOffset );
    if ( m_bLoading )
    {
	emit completed();
	m_bLoading = false;
    }
    //m_paKOfficeMode->setEnabled( m_dirLister->kofficeDocsFound() );

    slotOnViewport();

    QTimer::singleShot( 0, this, SLOT( slotProcessMimeIcons() ) );
}

void KonqKfmIconView::slotNewItems( const KonqFileItemList& entries )
{
  KonqFileItemListIterator it(entries);
  for (; it.current(); ++it) {

    KonqFileItem * _fileitem = it.current();

    if ( !S_ISDIR( _fileitem->mode() ) )
    {
	m_lDirSize += _fileitem->size();
	m_lFileCount++;
    }
    else
	m_lDirCount++;

    //kdDebug(1202) << "KonqKfmIconView::slotNewItem(...)" << _fileitem->url().url() << endl;
    KFileIVI* item = new KFileIVI( m_pIconView, _fileitem,
				   m_pIconView->iconSize(), m_pProps->m_bImagePreview );
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

    // old method
    //if ( m_ulTotalFiles > 0 )
    //  emit m_extension->loadingProgress( ( m_pIconView->count() * 100 ) / m_ulTotalFiles );

    m_lstPendingMimeIconItems.append( item );
  }
}

void KonqKfmIconView::slotDeleteItem( KonqFileItem * _fileitem )
{
    if ( !S_ISDIR( _fileitem->mode() ) )
    {
	m_lDirSize -= _fileitem->size();
	m_lFileCount--;
    }
    else
	m_lDirCount--;

    //kdDebug(1202) << "KonqKfmIconView::slotDeleteItem(...)" << endl;
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

/*
void KonqKfmIconView::slotTotalFiles( int, unsigned long files )
{
    m_ulTotalFiles = files;
}
*/

static QString displayString(int items, int files, long size, int dirs)
{
    QString text;
    if (items == 1)
	text = i18n("One Item");
    else
	text = i18n("%1 Items").arg(items);
    text += " - ";
    if (files == 1)
	text += i18n("One File");
    else
	text += i18n("%1 Files").arg(files);
    text += " ";
    text += i18n("(%1 Total)").arg(KIO::convertSize( size ) );
    text += " - ";
    if (dirs == 1)
	text += i18n("One Directory");
    else
	text += i18n("%1 Directories").arg(dirs);
    return text;
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

    if ( lst.count() > 0 ) {
        emit setStatusBarText( displayString(lst.count(),
					     fileCount,
					     fileSizeSum,
					     dirCount));
    } else
	slotOnViewport();

    emit m_extension->selectionInfo( lst );
}

void KonqKfmIconView::slotProcessMimeIcons()
{
    if ( m_lstPendingMimeIconItems.count() == 0 ) {

	if ( m_bNeedAlign )
	    m_pIconView->arrangeItemsInGrid();
	return;
    }

    // Find an icon that's visible.
    //
    // We only find mimetypes for icons that are visible. When more
    // of our viewport is exposed, we'll get a signal and then get
    // the mimetypes for the newly visible icons. (Rikkus)
    KFileIVI * item = 0L;

    QListIterator<KFileIVI> it(m_lstPendingMimeIconItems);

    QRect visibleContentsRect
    (
      m_pIconView->viewportToContents(QPoint(0, 0)),
      m_pIconView->viewportToContents
      (
        QPoint(m_pIconView->visibleWidth(), m_pIconView->visibleHeight())
      )
    );

    for (; it.current(); ++it)
      if (visibleContentsRect.intersects(it.current()->rect())) {
        item = it.current();
        break;
      }

    // No more visible items.
    if (0 == item)
      return;

    QPixmap *currentIcon = item->pixmap();

    KMimeType::Ptr dummy = item->item()->determineMimeType();

    QPixmap newIcon = item->item()->pixmap( m_pIconView->iconSize(), m_pProps->m_bImagePreview );

    if ( currentIcon->serialNumber() != newIcon.serialNumber() )
    {
	item->QIconViewItem::setPixmap( newIcon );
	if ( item->width() > m_pIconView->gridX() )
	    m_pIconView->setGridX( item->width() );
	if ( m_pProps->m_bImagePreview )
	    m_bNeedAlign = true;
    }

    m_lstPendingMimeIconItems.remove(item);
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
	QObject::connect( m_dirLister, SIGNAL( newItems( const KonqFileItemList& ) ),
			  this, SLOT( slotNewItems( const KonqFileItemList& ) ) );
	QObject::connect( m_dirLister, SIGNAL( deleteItem( KonqFileItem * ) ),
			  this, SLOT( slotDeleteItem( KonqFileItem * ) ) );
    }

    m_bLoading = true;
    m_lDirSize = 0;
    m_lFileCount = 0;
    m_lDirCount = 0;

    // Store url in the icon view
    m_pIconView->setURL( _url );

    // When our viewport is adjusted (resized or scrolled) we need
    // to get the mime types for any newly visible icons. (Rikkus)
    connect(
      m_pIconView,  SIGNAL(viewportAdjusted()),
      this,         SLOT(slotProcessMimeIcons()));

    // and in the part :-)
    m_url = _url;

    // Start the directory lister !
    m_dirLister->openURL( url(), m_pProps->m_bShowDot );
    // Note : we don't store the url. KDirLister does it for us.

    /*
      // should be possible to it without this now
    KIO::Job *job = m_dirLister->job();
    if ( job )
    {
	connect( job, SIGNAL( totalSize( KIO::Job *, unsigned long ) ),
	         this, SLOT( slotTotalFiles( KIO::Job *, unsigned long ) ) );
    }
    */

    //m_ulTotalFiles = 0;
    m_bNeedAlign = false;

    // do it after starting the dir lister to avoid changing bgcolor of the
    // old view
    if ( m_pProps->enterDir( _url, KonqIconViewFactory::defaultViewProps() ) )
    {
	m_pIconView->viewport()->setBackgroundColor( m_pProps->m_bgColor );
	m_pIconView->viewport()->setBackgroundPixmap( m_pProps->m_bgPixmap );
    }

    emit setWindowCaption( _url.decodedURL() );

    m_pIconView->show(); // ?
    if ( m_pIconView->itemTextPos() == QIconView::Bottom )
	m_pIconView->setGridX( 70 );
    else
	m_pIconView->setGridX( 120 );
    return true;
}

void KonqKfmIconView::slotOnItem( QIconViewItem *item )
{
  emit setStatusBarText( ((KFileIVI *)item)->item()->getStatusBarInfo() );
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

    emit setStatusBarText(
				       displayString(m_pIconView->count(),
						     m_lFileCount,
						     m_lDirSize,
						     m_lDirCount));
}

uint KonqKfmIconView::itemCount() const
{
  return m_pIconView->count();
}

uint KonqKfmIconView::dirSize() const
{
  return m_lDirSize;
}

uint KonqKfmIconView::dirCount() const
{
  return m_lDirCount;
}

uint KonqKfmIconView::fileCount() const
{
  return m_lFileCount;
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
