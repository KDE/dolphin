/* This file is part of the KDE projects
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

#include "konq_iconview.h"
#include "konq_propsview.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <qfile.h>

#include <kaction.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobalsettings.h>
#include <kinputdialog.h>
#include <konq_settings.h>
#include <kpropertiesdialog.h>
#include <kstdaction.h>
#include <kparts/factory.h>
#include <ktrader.h>
#include <klocale.h>
#include <kivdirectoryoverlay.h>
#include <kmessagebox.h>

#include <qregexp.h>
#include <qdatetime.h>

#include <config.h>

template class QPtrList<KFileIVI>;
//template class QValueList<int>;

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

    virtual KParts::Part* createPartObject( QWidget *parentWidget, const char *,
                                      QObject *parent, const char *name, const char*, const QStringList &args )
   {
      if( args.count() < 1 )
         kdWarning() << "KonqKfmIconView: Missing Parameter" << endl;

      KonqKfmIconView *obj = new KonqKfmIconView( parentWidget, parent, name,args.first() );
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
         s_defaultViewProps = new KonqPropsView( instance(), 0L );

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
    void *init_konq_iconview()
    {
        return new KonqIconViewFactory;
    }
}

IconViewBrowserExtension::IconViewBrowserExtension( KonqKfmIconView *iconView )
 : KonqDirPartBrowserExtension( iconView )
{
  m_iconView = iconView;
  m_bSaveViewPropertiesLocally = false;
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
    if ( m_iconView->iconViewWidget()->initConfig( false ) )
        m_iconView->iconViewWidget()->arrangeItemsInGrid(); // called if the font changed.
}

void IconViewBrowserExtension::properties()
{
    (void) new KPropertiesDialog( m_iconView->iconViewWidget()->selectedFileItems() );
}

void IconViewBrowserExtension::editMimeType()
{
    KFileItem * item = m_iconView->iconViewWidget()->selectedFileItems().first();
    KonqOperations::editMimeType( item->mimetype() );
}

void IconViewBrowserExtension::setSaveViewPropertiesLocally( bool value )
{
  m_iconView->m_pProps->setSaveViewPropertiesLocally( value );
}

void IconViewBrowserExtension::setNameFilter( const QString &nameFilter )
{
  //kdDebug(1202) << "IconViewBrowserExtension::setNameFilter " << nameFilter << endl;
  m_iconView->m_nameFilter = nameFilter;
}

KonqKfmIconView::KonqKfmIconView( QWidget *parentWidget, QObject *parent, const char *name, const QString& mode  )
    : KonqDirPart( parent, name ), m_paOutstandingOverlaysTimer( 0 ), m_itemDict( 43 )
{
    kdDebug(1202) << "+KonqKfmIconView" << endl;

    setBrowserExtension( new IconViewBrowserExtension( this ) );

    // Create a properties instance for this view
    m_pProps = new KonqPropsView( KonqIconViewFactory::instance(), KonqIconViewFactory::defaultViewProps() );

    m_pIconView = new KonqIconViewWidget( parentWidget, "qiconview" );
    m_pIconView->initConfig( true );

    connect( m_pIconView,  SIGNAL(imagePreviewFinished()),
             this, SLOT(slotRenderingFinished()));

    // connect up the icon inc/dec signals
    connect( m_pIconView,  SIGNAL(incIconSize()),
             this, SLOT(slotIncIconSize()));
    connect( m_pIconView,  SIGNAL(decIconSize()),
             this, SLOT(slotDecIconSize()));

    // pass signals to the extension
    connect( m_pIconView, SIGNAL( enableAction( const char *, bool ) ),
             m_extension, SIGNAL( enableAction( const char *, bool ) ) );

    // signals from konqdirpart (for BC reasons)
    connect( this, SIGNAL( findOpened( KonqDirPart * ) ), SLOT( slotKFindOpened() ) );
    connect( this, SIGNAL( findClosed( KonqDirPart * ) ), SLOT( slotKFindClosed() ) );

    setWidget( m_pIconView );
    m_mimeTypeResolver = new KMimeTypeResolver<KFileIVI,KonqKfmIconView>(this);

    setInstance( KonqIconViewFactory::instance() );

    setXMLFile( "konq_iconview.rc" );

    // Don't repaint on configuration changes during construction
    m_bInit = true;

    m_paDotFiles = new KToggleAction( i18n( "Show &Hidden Files" ), 0, this, SLOT( slotShowDot() ),
                                      actionCollection(), "show_dot" );
    m_paDotFiles->setStatusText( i18n( "Toggle displaying of hidden dot files" ) );

    m_paDirectoryOverlays = new KToggleAction( i18n( "&Folder Icons Reflect Contents" ), 0, this, SLOT( slotShowDirectoryOverlays() ),
                                      actionCollection(), "show_directory_overlays" );

    m_pamPreview = new KActionMenu( i18n( "&Preview" ), actionCollection(), "iconview_preview" );

    m_paEnablePreviews = new KToggleAction( i18n("Show Previews"), 0, actionCollection(), "iconview_preview_all" );
    connect( m_paEnablePreviews, SIGNAL( toggled( bool ) ), this, SLOT( slotPreview( bool ) ) );
    m_paEnablePreviews->setIcon("thumbnail");
    m_pamPreview->insert( m_paEnablePreviews );
    m_pamPreview->insert( new KActionSeparator(this) );

    KTrader::OfferList plugins = KTrader::self()->query( "ThumbCreator" );
    QMap< QString, KToggleAction* > previewActions;
    for ( KTrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it )
    {
        if ( KToggleAction*& preview = previewActions[ ( *it )->name() ] )
            preview->setName( QCString( preview->name() ) + ',' + ( *it )->desktopEntryName().latin1() );
        else
        {
            preview = new KToggleAction( (*it)->name(), 0, actionCollection(), (*it)->desktopEntryName().latin1() );
            connect( preview, SIGNAL( toggled( bool ) ), this, SLOT( slotPreview( bool ) ) );
            m_pamPreview->insert( preview );
            m_paPreviewPlugins.append( preview );
        }
    }
    KToggleAction *soundPreview = new KToggleAction( i18n("Sound Files"), 0, actionCollection(), "audio/" );
    connect( soundPreview, SIGNAL( toggled( bool ) ), this, SLOT( slotPreview( bool ) ) );
    m_pamPreview->insert( soundPreview );
    m_paPreviewPlugins.append( soundPreview );

    //    m_pamSort = new KActionMenu( i18n( "Sort..." ), actionCollection(), "sort" );

    KToggleAction *aSortByNameCS = new KRadioAction( i18n( "By Name (Case Sensitive)" ), 0, actionCollection(), "sort_nc" );
    KToggleAction *aSortByNameCI = new KRadioAction( i18n( "By Name (Case Insensitive)" ), 0, actionCollection(), "sort_nci" );
    KToggleAction *aSortBySize = new KRadioAction( i18n( "By Size" ), 0, actionCollection(), "sort_size" );
    KToggleAction *aSortByType = new KRadioAction( i18n( "By Type" ), 0, actionCollection(), "sort_type" );
    KToggleAction *aSortByDate = new KRadioAction( i18n( "By Date" ), 0, actionCollection(), "sort_date" );

    aSortByNameCS->setExclusiveGroup( "sorting" );
    aSortByNameCI->setExclusiveGroup( "sorting" );
    aSortBySize->setExclusiveGroup( "sorting" );
    aSortByType->setExclusiveGroup( "sorting" );
    aSortByDate->setExclusiveGroup( "sorting" );

    aSortByNameCS->setChecked( false );
    aSortByNameCI->setChecked( false );
    aSortBySize->setChecked( false );
    aSortByType->setChecked( false );
    aSortByDate->setChecked( false );

    connect( aSortByNameCS, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseSensitive( bool ) ) );
    connect( aSortByNameCI, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByNameCaseInsensitive( bool ) ) );
    connect( aSortBySize, SIGNAL( toggled( bool ) ), this, SLOT( slotSortBySize( bool ) ) );
    connect( aSortByType, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByType( bool ) ) );
    connect( aSortByDate, SIGNAL( toggled( bool ) ), this, SLOT( slotSortByDate( bool ) ) );

    //enable menu item representing the saved sorting criterion
    QString sortcrit = KonqIconViewFactory::defaultViewProps()->sortCriterion();
    KRadioAction *sort_action = dynamic_cast<KRadioAction *>(actionCollection()->action(sortcrit.latin1()));
    if(sort_action!=NULL) sort_action->activate();

    m_paSortDirsFirst = new KToggleAction( i18n( "Folders First" ), 0, actionCollection(), "sort_directoriesfirst" );
    KToggleAction *aSortDescending = new KToggleAction( i18n( "Descending" ), 0, actionCollection(), "sort_descend" );

    m_paSortDirsFirst->setChecked( KonqIconViewFactory::defaultViewProps()->isDirsFirst() );

    connect( aSortDescending, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDescending() ) );
    connect( m_paSortDirsFirst, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDirsFirst() ) );

    //enable stored settings
    slotSortDirsFirst();
    if (KonqIconViewFactory::defaultViewProps()->isDescending())
    {
     aSortDescending->setChecked(true);
     m_pIconView->setSorting(true,true);//enable sort ascending in QIconview
     slotSortDescending();//invert sorting (now descending) and actually resort items
    }

    /*
    m_pamSort->insert( aSortByNameCS );
    m_pamSort->insert( aSortByNameCI );
    m_pamSort->insert( aSortBySize );

    m_pamSort->popupMenu()->insertSeparator();

    m_pamSort->insert( aSortDescending );
    */
    m_paSelect = new KAction( i18n( "Se&lect..." ), CTRL+Key_Plus, this, SLOT( slotSelect() ),
                              actionCollection(), "select" );
    m_paUnselect = new KAction( i18n( "Unselect..." ), CTRL+Key_Minus, this, SLOT( slotUnselect() ),
                                actionCollection(), "unselect" );
    m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), actionCollection(), "selectall" );
    m_paUnselectAll = new KAction( i18n( "Unselect All" ), CTRL+Key_U, this, SLOT( slotUnselectAll() ),
                                   actionCollection(), "unselectall" );
    m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), CTRL+Key_Asterisk,
                                       this, SLOT( slotInvertSelection() ),
                                       actionCollection(), "invertselection" );

    m_paSelect->setStatusText( i18n( "Allows selecting of file or folder items based on a given mask" ) );
    m_paUnselect->setStatusText( i18n( "Allows unselecting of file or folder items based on a given mask" ) );
    m_paSelectAll->setStatusText( i18n( "Selects all items" ) );
    m_paUnselectAll->setStatusText( i18n( "Unselects all selected items" ) );
    m_paInvertSelection->setStatusText( i18n( "Inverts the current selection of items" ) );

    //m_paBottomText = new KToggleAction( i18n( "Text at &Bottom" ), 0, actionCollection(), "textbottom" );
    //m_paRightText = new KToggleAction( i18n( "Text at &Right" ), 0, actionCollection(), "textright" );
    //m_paBottomText->setExclusiveGroup( "TextPos" );
    //m_paRightText->setExclusiveGroup( "TextPos" );
    //connect( m_paBottomText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextBottom( bool ) ) );
    //connect( m_paRightText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextRight( bool ) ) );

    connect( m_pIconView, SIGNAL( executed( QIconViewItem * ) ),
             this, SLOT( slotReturnPressed( QIconViewItem * ) ) );
    connect( m_pIconView, SIGNAL( returnPressed( QIconViewItem * ) ),
             this, SLOT( slotReturnPressed( QIconViewItem * ) ) );

    connect( m_pIconView, SIGNAL( onItem( QIconViewItem * ) ),
             this, SLOT( slotOnItem( QIconViewItem * ) ) );

    connect( m_pIconView, SIGNAL( onViewport() ),
             this, SLOT( slotOnViewport() ) );

    connect( m_pIconView, SIGNAL( mouseButtonPressed(int, QIconViewItem*, const QPoint&)),
             this, SLOT( slotMouseButtonPressed(int, QIconViewItem*, const QPoint&)) );
    connect( m_pIconView, SIGNAL( mouseButtonClicked(int, QIconViewItem*, const QPoint&)),
             this, SLOT( slotMouseButtonClicked(int, QIconViewItem*, const QPoint&)) );
    connect( m_pIconView, SIGNAL( contextMenuRequested(QIconViewItem*, const QPoint&)),
             this, SLOT( slotContextMenuRequested(QIconViewItem*, const QPoint&)) );

    // Create the directory lister
    m_dirLister = new KDirLister( true );
    m_dirLister->setMainWindow(m_pIconView->topLevelWidget());

    connect( m_dirLister, SIGNAL( started( const KURL & ) ),
             this, SLOT( slotStarted() ) );
    connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
    connect( m_dirLister, SIGNAL( canceled( const KURL& ) ), this, SLOT( slotCanceled( const KURL& ) ) );
    connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
    connect( m_dirLister, SIGNAL( newItems( const KFileItemList& ) ),
             this, SLOT( slotNewItems( const KFileItemList& ) ) );
    connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
             this, SLOT( slotDeleteItem( KFileItem * ) ) );
    connect( m_dirLister, SIGNAL( refreshItems( const KFileItemList& ) ),
             this, SLOT( slotRefreshItems( const KFileItemList& ) ) );
    connect( m_dirLister, SIGNAL( redirection( const KURL & ) ),
             this, SLOT( slotRedirection( const KURL & ) ) );
    connect( m_dirLister, SIGNAL( itemsFilteredByMime(const KFileItemList& ) ),
             SIGNAL( itemsFilteredByMime(const KFileItemList& ) ) );
    connect( m_dirLister, SIGNAL( infoMessage( const QString& ) ),
             extension(), SIGNAL( infoMessage( const QString& ) ) );
    connect( m_dirLister, SIGNAL( percent( int ) ),
             extension(), SIGNAL( loadingProgress( int ) ) );
    connect( m_dirLister, SIGNAL( speed( int ) ),
             extension(), SIGNAL( speedProgress( int ) ) );

    // Now we may react to configuration changes
    m_bInit = false;

    m_bLoading = true;
    m_bNeedAlign = false;
    m_bNeedEmitCompleted = false;
    m_bUpdateContentsPosAfterListing = false;
    m_pIconView->setResizeMode( QIconView::Adjust );

    connect( m_pIconView, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    // Respect kcmkonq's configuration for word-wrap icon text.
    // If we want something else, we have to adapt the configuration or remove it...
    m_pIconView->setWordWrapIconText(KonqFMSettings::settings()->wordWrapText());

    // Finally, determine initial grid size again, with those parameters
    //    m_pIconView->calculateGridX();

    setViewMode( mode );
}

KonqKfmIconView::~KonqKfmIconView()
{
    // Before anything else, stop the image preview. It might use our fileitems,
    // and it will only be destroyed togetierh with our widget
    m_pIconView->stopImagePreview();

    kdDebug(1202) << "-KonqKfmIconView" << endl;
    m_dirLister->disconnect( this );
    delete m_dirLister;
    delete m_mimeTypeResolver;
    delete m_pProps;
    //no need for that, KParts deletes our widget already ;-)
    //    delete m_pIconView;
}

const KFileItem * KonqKfmIconView::currentItem()
{
    return m_pIconView->currentItem() ? static_cast<KFileIVI *>(m_pIconView->currentItem())->item() : 0L;
}

void KonqKfmIconView::slotPreview( bool toggle )
{
    QCString name = sender()->name(); // e.g. clipartthumbnail (or audio/, special case)
    if (name == "iconview_preview_all")
    {
        m_pProps->setShowingPreview( toggle );
        m_pIconView->setPreviewSettings( m_pProps->previewSettings() );
        if ( !toggle )
        {
            kdDebug() << "KonqKfmIconView::slotPreview stopping all previews for " << name << endl;
            m_pIconView->disableSoundPreviews();

            bool previewRunning = m_pIconView->isPreviewRunning();
            if ( previewRunning )
                m_pIconView->stopImagePreview();
            m_pIconView->setIcons( m_pIconView->iconSize(), "*" );
        }
        else
        {
            m_pIconView->startImagePreview( m_pProps->previewSettings(), true );
        }
        for ( m_paPreviewPlugins.first(); m_paPreviewPlugins.current(); m_paPreviewPlugins.next() )
            m_paPreviewPlugins.current()->setEnabled( toggle );
    }
    else
    {
        QStringList types = QStringList::split( ',', name );
        for ( QStringList::ConstIterator it = types.begin(); it != types.end(); ++it )
        {
            m_pProps->setShowingPreview( *it, toggle );
            m_pIconView->setPreviewSettings( m_pProps->previewSettings() );
            if ( !toggle )
            {
                kdDebug() << "KonqKfmIconView::slotPreview stopping image preview for " << *it << endl;
                if ( *it == "audio/" )
                    m_pIconView->disableSoundPreviews();
                else
                {
                    KService::Ptr serv = KService::serviceByDesktopName( *it );
                    Q_ASSERT( serv != 0L );
                    if ( serv ) {
                        bool previewRunning = m_pIconView->isPreviewRunning();
                        if ( previewRunning )
                            m_pIconView->stopImagePreview();
                        QStringList mimeTypes = serv->property("MimeTypes").toStringList();
                        m_pIconView->setIcons( m_pIconView->iconSize(), mimeTypes );
                        if ( previewRunning )
                            m_pIconView->startImagePreview( m_pProps->previewSettings(), false );
                    }
                }
            }
            else
            {
                m_pIconView->startImagePreview( m_pProps->previewSettings(), true );
            }
        }
    }
}

void KonqKfmIconView::slotShowDot()
{
    m_pProps->setShowingDotFiles( !m_pProps->isShowingDotFiles() );
    m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );
    m_dirLister->emitChanges();
    //we don't want the non-dot files to remain where they are
    m_bNeedAlign = true;
    slotCompleted();
}

void KonqKfmIconView::slotShowDirectoryOverlays()
{
    bool show = !m_pProps->isShowingDirectoryOverlays();

    m_pProps->setShowingDirectoryOverlays( show );

    for ( QIconViewItem *item = m_pIconView->firstItem(); item; item = item->nextItem() )
    {
        KFileIVI* kItem = static_cast<KFileIVI*>(item);
        if ( !kItem->item()->isDir() ) continue;

        if (show) {
            showDirectoryOverlay(kItem);
        } else {
            kItem -> setShowDirectoryOverlay(false);
        }
    }

    m_pIconView->updateContents();
}

void KonqKfmIconView::slotSelect()
{
    bool ok;
    QString pattern = KInputDialog::getText( QString::null,
        i18n( "Select files:" ), "*", &ok, m_pIconView );
    if ( ok )
    {
        QRegExp re( pattern, true, true );

        m_pIconView->blockSignals( true );

        QIconViewItem *it = m_pIconView->firstItem();
        while ( it )
        {
            if ( re.exactMatch( it->text() ) )
                it->setSelected( true, true );
            it = it->nextItem();
        }

        m_pIconView->blockSignals( false );

        // do this once, not for each item
        m_pIconView->slotSelectionChanged();
        slotSelectionChanged();
    }
}

void KonqKfmIconView::slotUnselect()
{
    bool ok;
    QString pattern = KInputDialog::getText( QString::null,
        i18n( "Unselect files:" ), "*", &ok, m_pIconView );
    if ( ok )
    {
        QRegExp re( pattern, true, true );

        m_pIconView->blockSignals( true );

        QIconViewItem *it = m_pIconView->firstItem();
        while ( it )
        {
            if ( re.exactMatch( it->text() ) )
                it->setSelected( false, true );
            it = it->nextItem();
        }

        m_pIconView->blockSignals( false );

        // do this once, not for each item
        m_pIconView->slotSelectionChanged();
        slotSelectionChanged();
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

    KonqIconViewFactory::defaultViewProps()->setSortCriterion("sort_nc");
    setupSorting( NameCaseSensitive );
}

void KonqKfmIconView::slotSortByNameCaseInsensitive( bool toggle )
{
    if ( !toggle )
        return;

    KonqIconViewFactory::defaultViewProps()->setSortCriterion("sort_nci");
    setupSorting( NameCaseInsensitive );
}

void KonqKfmIconView::slotSortBySize( bool toggle )
{
    if ( !toggle )
        return;

    KonqIconViewFactory::defaultViewProps()->setSortCriterion("sort_size");
    setupSorting( Size );
}

void KonqKfmIconView::slotSortByType( bool toggle )
{
  if ( !toggle )
    return;

  KonqIconViewFactory::defaultViewProps()->setSortCriterion("sort_type");
  setupSorting( Type );
}

void KonqKfmIconView::slotSortByDate( bool toggle )
{
  if( !toggle)
    return;

  KonqIconViewFactory::defaultViewProps()->setSortCriterion("sort_date");
  setupSorting( Date );
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

    setupSortKeys(); // keys have to change, for directories

    m_pIconView->sort( m_pIconView->sortDirection() );

    KonqIconViewFactory::defaultViewProps()->setDescending( !m_pIconView->sortDirection() );
}

void KonqKfmIconView::slotSortDirsFirst()
{
    m_pIconView->setSortDirectoriesFirst( m_paSortDirsFirst->isChecked() );

    setupSortKeys();

    m_pIconView->sort( m_pIconView->sortDirection() );

    KonqIconViewFactory::defaultViewProps()->setDirsFirst( m_paSortDirsFirst->isChecked() );
}

void KonqKfmIconView::newIconSize( int size )
{
    //Either of the sizes can be 0 to indicate the default (Desktop) size icons.
    //check for that when checking whether the size changed
    int effSize = size;
    if (effSize == 0)
       effSize = IconSize(KIcon::Desktop);

    int oldEffSize = m_pIconView->iconSize();
    if (oldEffSize == 0)
       oldEffSize = IconSize(KIcon::Desktop);

    // Make sure all actions are initialized.
    KonqDirPart::newIconSize( size );

    if ( effSize == oldEffSize )
        return;

    m_pIconView->setIcons( size );
    if ( m_pProps->isShowingPreview() )
        m_pIconView->startImagePreview( m_pProps->previewSettings(), true );
    // arrangeItemsInGrid already done in KonqIconViewWidget::setIcons
    //m_pIconView->arrangeItemsInGrid();
}

bool KonqKfmIconView::doCloseURL()
{
    m_dirLister->stop();

    m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
    m_pIconView->stopImagePreview();
    return true;
}

void KonqKfmIconView::slotReturnPressed( QIconViewItem *item )
{
    if ( !item )
        return;

    item->setSelected( false, true );
    m_pIconView->visualActivate(item);

    KFileItem *fileItem = (static_cast<KFileIVI*>(item))->item();
    if ( !fileItem )
        return;
    KURL url = fileItem->url();
    url.cleanPath();
    bool isIntoTrash =  url.isLocalFile() && url.path(1).startsWith(KGlobalSettings::trashPath());
    if ( !isIntoTrash || (isIntoTrash && fileItem->isDir()) )
    {
        lmbClicked( fileItem );
    }
    else
    {
        KMessageBox::information(0L, i18n("You must take the file out of the trash before being able to use it."));
    }
}

void KonqKfmIconView::slotContextMenuRequested(QIconViewItem* _item, const QPoint& _global)
{
    if (  m_pIconView->selectedFileItems().count() == 0 )
        return;
    KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems;
    KFileIVI* i = static_cast<KFileIVI*>(_item);
    if (i)
        i->setSelected( true, true /* don't touch other items */ );
    emit m_extension->popupMenu( 0L, _global, m_pIconView->selectedFileItems(), KParts::URLArgs(), popupFlags);
}

void KonqKfmIconView::slotMouseButtonPressed(int _button, QIconViewItem* _item, const QPoint&)
{
    if ( _button == RightButton )
        if(!_item)
        {
            // Right click on viewport
            KFileItem * item = m_dirLister->rootItem();
            bool delRootItem = false;
            if ( ! item )
            {
                if ( m_bLoading )
                {
                    kdDebug(1202) << "slotViewportRightClicked : still loading and no root item -> dismissed" << endl;
                    return; // too early, '.' not yet listed
                }
                else
                {
                    // We didn't get a root item (e.g. over FTP)
                    // We have to create a dummy item. I tried using KonqOperations::statURL,
                    // but this was leading to a huge delay between the RMB and the popup. Bad.
                    // But KonqPopupMenu now takes care of stating before opening properties.
                    item = new KFileItem( S_IFDIR, (mode_t)-1, url() );
                    delRootItem = true;
                }
            }

            KFileItemList items;
            items.append( item );

            KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::ShowNavigationItems | KParts::BrowserExtension::ShowUp;

            emit m_extension->popupMenu( 0L, QCursor::pos(), items, KParts::URLArgs(), popupFlags );

            if ( delRootItem )
                delete item; // we just created it
        }
}

void KonqKfmIconView::slotMouseButtonClicked(int _button, QIconViewItem* _item, const QPoint& )
{
    if( _button == MidButton )
        mmbClicked( _item ? static_cast<KFileIVI*>(_item)->item() : 0L );
}

void KonqKfmIconView::slotStarted()
{
    // Only emit started if this comes after openURL, i.e. it's not for an update.
    // We don't want to start a spinning wheel during updates.
    if ( m_bLoading )
        emit started( 0 );

    // An update may come in while we are still processing icons...
    // So don't clear the list.
    //m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
}

void KonqKfmIconView::slotCanceled()
{
    // Called by kfindpart. Not by kdirlister.
    slotCanceled( m_pIconView->url() );
}

void KonqKfmIconView::slotCanceled( const KURL& url )
{
    // Check if this canceled() signal is about the URL we're listing.
    // It could be about the URL we were listing, and openURL() aborted it.
    if ( m_bLoading && url.equals( m_pIconView->url(), true ) )
    {
        emit canceled( QString::null );
        m_bLoading = false;
    }
}

void KonqKfmIconView::slotCompleted()
{
    // Root item ? Store root item in konqiconviewwidget (whether 0L or not)
    m_pIconView->setRootItem( m_dirLister->rootItem() );

    if ( m_bLoading ) // only after initial listing, not after updates
        // If we don't set a current item, the iconview has none (one more keypress needed)
        // but it appears on focusin... qiconview bug, Reggie acknowledged it LONG ago (07-2000).
        m_pIconView->setCurrentItem( m_pIconView->firstItem() );

    if ( m_bUpdateContentsPosAfterListing ) {
         m_pIconView->setContentsPos( extension()->urlArgs().xOffset,
                                      extension()->urlArgs().yOffset );
    }

    m_bUpdateContentsPosAfterListing = false;

    slotOnViewport();

    // slotRenderingFinished will do it
    m_bNeedEmitCompleted = true;

    if (m_pProps->isShowingPreview())
        m_mimeTypeResolver->start( 0 ); // We need the mimetypes asap
    else
    {
        slotRenderingFinished(); // emit completed, we don't want the wheel...
        // to keep turning while we find mimetypes in the background
        m_mimeTypeResolver->start( 10 );
    }

    m_bLoading = false;

    // Disable cut icons if any
    slotClipboardDataChanged();
}

void KonqKfmIconView::slotNewItems( const KFileItemList& entries )
{
    for (KFileItemListIterator it(entries); it.current(); ++it)
    {
        //kdDebug(1202) << "KonqKfmIconView::slotNewItem(...)" << _fileitem->url().url() << endl;
        KFileIVI* item = new KFileIVI( m_pIconView, *it, m_pIconView->iconSize() );
        item->setRenameEnabled( false );

        KFileItem* fileItem = item->item();

        if ( fileItem->isDir() && m_pProps->isShowingDirectoryOverlays() ) {
            showDirectoryOverlay(item);
        }

        QString key;

        switch ( m_eSortCriterion )
        {
            case NameCaseSensitive: key = item->text(); break;
            case NameCaseInsensitive: key = item->text().lower(); break;
            case Size: key = makeSizeKey( item ); break;
            case Type: key = item->item()->mimetype()+ "\008" +item->text().lower(); break; // ### slows down listing :-(
            case Date:
            {
                QDateTime dayt;
                dayt.setTime_t(item->item()->time(KIO::UDS_MODIFICATION_TIME ));
                key = dayt.toString("yyyyMMddhhmmss");
                break;
            }
            default: Q_ASSERT(0);
        }

        item->setKey( key );

        //kdDebug() << "KonqKfmIconView::slotNewItems " << (*it)->url().url() << " " << (*it)->mimeTypePtr()->name() << " mimetypeknown:" << (*it)->isMimeTypeKnown() << endl;
        if ( !(*it)->isMimeTypeKnown() )
            m_mimeTypeResolver->m_lstPendingMimeIconItems.append( item );

        m_itemDict.insert( *it, item );
    }
    KonqDirPart::newItems( entries );
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
    m_pIconView->stopImagePreview();

    if ( _fileitem == m_dirLister->rootItem() )
    {
        m_pIconView->setRootItem( 0L );
        return;
    }

    KonqDirPart::deleteItem( _fileitem );

    //kdDebug(1202) << "KonqKfmIconView::slotDeleteItem(...)" << endl;
    // we need to find out the iconcontainer item containing the fileitem
    KFileIVI * ivi = m_itemDict[ _fileitem ];
    // It can be that we know nothing about this item, e.g. because it's filtered out
    // (by default: dot files). KDirLister still tells us about it when it's modified, since
    // it doesn't know if we showed it before, and maybe its mimetype changed so we
    // might have to hide it now.
    if (ivi)
    {
        m_pIconView->takeItem( ivi );
        m_mimeTypeResolver->m_lstPendingMimeIconItems.remove( ivi );
        m_itemDict.remove( _fileitem );
        if (m_paOutstandingOverlays.first() == ivi) // Being processed?
           m_paOutstandingOverlaysTimer->start(20, true); // Restart processing...

        m_paOutstandingOverlays.remove(ivi);
        delete ivi;
    }
}

void KonqKfmIconView::showDirectoryOverlay(KFileIVI* item)
{
    KFileItem* fileItem = item->item();

    if ( KGlobalSettings::showFilePreview( fileItem->url() ) ) {
        m_paOutstandingOverlays.append(item);
        if (m_paOutstandingOverlays.count() == 1)
        {
           if (!m_paOutstandingOverlaysTimer)
           {
              m_paOutstandingOverlaysTimer = new QTimer(this);
              connect(m_paOutstandingOverlaysTimer, SIGNAL(timeout()),
                      SLOT(slotDirectoryOverlayStart()));
           }
           m_paOutstandingOverlaysTimer->start(20, true);
        }
    }
}

void KonqKfmIconView::slotDirectoryOverlayStart()
{
    do
    {
       KFileIVI* item = m_paOutstandingOverlays.first();
       if (!item)
          return; // Nothing to do

       KIVDirectoryOverlay* overlay = item->setShowDirectoryOverlay( true );

       if (overlay)
       {
          connect( overlay, SIGNAL( finished() ), this, SLOT( slotDirectoryOverlayFinished() ) );
          overlay->start(); // Watch out, may emit finished() immediately!!
          return; // Let it run....
       }
       m_paOutstandingOverlays.removeFirst();
    } while (true);
}

void KonqKfmIconView::slotDirectoryOverlayFinished()
{
    m_paOutstandingOverlays.removeFirst();

    if (m_paOutstandingOverlays.count() > 0)
        m_paOutstandingOverlaysTimer->start(0, true); // Don't call directly to prevent deep recursion.
}

// see also KDesktop::slotRefreshItems
void KonqKfmIconView::slotRefreshItems( const KFileItemList& entries )
{
    bool bNeedRepaint = false;
    bool bNeedPreviewJob = false;
    KFileItemListIterator rit(entries);
    for (; rit.current(); ++rit)
    {
        KFileIVI * ivi = m_itemDict[ rit.current() ];
        Q_ASSERT(ivi);
        kdDebug() << "KonqKfmIconView::slotRefreshItems '" << rit.current()->name() << "' ivi=" << ivi << endl;
        if (ivi)
        {
            QSize oldSize = ivi->pixmap()->size();
            if ( ivi->isThumbnail() ) {
                bNeedPreviewJob = true;
                ivi->invalidateThumbnail();
            }
            else
                ivi->refreshIcon( true );
            ivi->setText( rit.current()->text() );
            if ( rit.current()->isMimeTypeKnown() )
                ivi->setMouseOverAnimation( rit.current()->iconName() );
            if ( !bNeedRepaint && oldSize != ivi->pixmap()->size() )
                bNeedRepaint = true;
        }
    }

    if ( bNeedPreviewJob && m_pProps->isShowingPreview() )
    {
        m_pIconView->startImagePreview( m_pProps->previewSettings(), false );
    }
    else
    {
        // In case we replace a big icon with a small one, need to repaint.
        if ( bNeedRepaint )
            m_pIconView->updateContents();
    }
}

void KonqKfmIconView::slotClear()
{
    resetCount();
    m_pIconView->clear();
    m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
    m_itemDict.clear();
    // Bug in QIconview IMHO - it should emit selectionChanged()
    // (bug reported, but code seems to be that way on purpose)
    m_pIconView->slotSelectionChanged();
    slotSelectionChanged();
}

void KonqKfmIconView::slotRedirection( const KURL & url )
{
    emit m_extension->setLocationBarURL( url.prettyURL() );
    emit setWindowCaption( url.prettyURL() );
    m_url = url;
}

void KonqKfmIconView::slotSelectionChanged()
{
    // Display statusbar info, and emit selectionInfo
    KFileItemList lst = m_pIconView->selectedFileItems();
    emitCounts( lst, true );

    bool itemSelected = lst.count()>0;
    m_paUnselect->setEnabled( itemSelected );
    m_paUnselectAll->setEnabled( itemSelected );
    m_paInvertSelection->setEnabled( itemSelected );
}

void KonqKfmIconView::determineIcon( KFileIVI * item )
{
  // kdDebug() << "KonqKfmIconView::determineIcon " << item->item()->name() << endl;
  //int oldSerial = item->pixmap()->serialNumber();

  (void) item->item()->determineMimeType();

  item->setIcon( iconSize(), item->state(), true, true );
  item->setMouseOverAnimation( item->item()->iconName() );
}

void KonqKfmIconView::mimeTypeDeterminationFinished()
{
    if ( m_pProps->isShowingPreview() )
    {
        // TODO if ( m_url.isLocalFile() || m_bAutoPreviewRemote )
        {
            // We can do this only when the mimetypes are fully determined,
            // since we only do image preview... on images :-)
            m_pIconView->startImagePreview( m_pProps->previewSettings(), false );
            return;
        }
    }
    slotRenderingFinished();
}

void KonqKfmIconView::slotRenderingFinished()
{
    kdDebug(1202) << "KonqKfmIconView::slotRenderingFinished()" << endl;
    if ( m_bNeedEmitCompleted )
    {
        kdDebug(1202) << "KonqKfmIconView completed() after rendering" << endl;
        emit completed();
        m_bNeedEmitCompleted = false;
    }
    if ( m_bNeedAlign )
    {
        m_bNeedAlign = false;
	kdDebug(1202) << "arrangeItemsInGrid" << endl;
        m_pIconView->arrangeItemsInGrid();
    }
}

bool KonqKfmIconView::doOpenURL( const KURL & url )
{
    // Store url in the icon view
    m_pIconView->setURL( url );

    m_bLoading = true;

    // Check for new properties in the new dir
    // newProps returns true the first time, and any time something might
    // have changed.
    bool newProps = m_pProps->enterDir( url );

    m_dirLister->setNameFilter( m_nameFilter );

    m_dirLister->setMimeFilter( mimeFilter() );

    // This *must* happen before m_dirLister->openURL because it emits
    // clear() and QIconView::clear() calls setContentsPos(0,0)!
    if ( m_extension->urlArgs().reload )
    {
        KParts::URLArgs args = m_extension->urlArgs();
        args.xOffset = m_pIconView->contentsX();
        args.yOffset = m_pIconView->contentsY();
        m_extension->setURLArgs( args );
    }

    m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );

    m_bNeedAlign = false;
    m_bUpdateContentsPosAfterListing = true;

    m_paOutstandingOverlays.clear();

    // Start the directory lister !
    m_dirLister->openURL( url, false, m_extension->urlArgs().reload );

    // Apply properties and reflect them on the actions
    // do it after starting the dir lister to avoid changing the properties
    // of the old view
    if ( newProps )
    {
      newIconSize( m_pProps->iconSize() );

      m_paDotFiles->setChecked( m_pProps->isShowingDotFiles() );
      m_paDirectoryOverlays->setChecked( m_pProps->isShowingDirectoryOverlays() );
      m_paEnablePreviews->setChecked( m_pProps->isShowingPreview() );
      for ( m_paPreviewPlugins.first(); m_paPreviewPlugins.current(); m_paPreviewPlugins.next() )
      {
          QStringList types = QStringList::split( ',', m_paPreviewPlugins.current()->name() );
          bool enabled = false;
          for ( QStringList::ConstIterator it = types.begin(); it != types.end(); ++it )
              if ( m_pProps->isShowingPreview( *it ) )
              {
                  enabled = true;
                  break;
              }
          m_paPreviewPlugins.current()->setChecked( enabled );
          m_paPreviewPlugins.current()->setEnabled( m_pProps->isShowingPreview() );
      }

      m_pIconView->setPreviewSettings(m_pProps->previewSettings());

      // It has to be "viewport()" - this is what KonqDirPart's slots act upon,
      // and otherwise we get a color/pixmap in the square between the scrollbars.
      m_pProps->applyColors( m_pIconView->viewport() );
    }

    emit setWindowCaption( url.prettyURL() );

    return true;
}

void KonqKfmIconView::slotKFindOpened()
{
    m_dirLister->setAutoUpdate( false );
}

void KonqKfmIconView::slotKFindClosed()
{
    m_dirLister->setAutoUpdate( true );
}

void KonqKfmIconView::slotOnItem( QIconViewItem *item )
{
    emit setStatusBarText( static_cast<KFileIVI *>(item)->item()->getStatusBarInfo() );
    emitMouseOver( static_cast<KFileIVI*>(item)->item());
}

void KonqKfmIconView::slotOnViewport()
{
    KFileItemList lst = m_pIconView->selectedFileItems();
    emitCounts( lst, false );
    emitMouseOver( 0 );
}

void KonqKfmIconView::setViewMode( const QString &mode )
{
    if ( mode == m_mode )
        return;
    // note: this should be moved to KonqIconViewWidget. It would make the code
    // more readable :)

    m_mode = mode;
    if (mode=="MultiColumnView")
    {
        m_pIconView->setArrangement(QIconView::TopToBottom);
        m_pIconView->setItemTextPos(QIconView::Right);
    }
    else
    {
        m_pIconView->setArrangement(QIconView::LeftToRight);
        m_pIconView->setItemTextPos(QIconView::Bottom);
    }
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
    case Type:
        // Sort by Type + Name (#17014)
        for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( static_cast<KFileIVI *>( it )->item()->mimetype() + "\008" + it->text().lower() );
        break;
    case Date:
    {
        //Sorts by time of modification (#52750)
        QDateTime dayt;
        for ( QIconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
        {
            dayt.setTime_t(static_cast<KFileIVI *>( it )->item()->time(KIO::UDS_MODIFICATION_TIME));
            it->setKey(dayt.toString("yyyyMMddhhmmss"));
        }
        break;
    }
    }
}

QString KonqKfmIconView::makeSizeKey( KFileIVI *item )
{
    return KIO::number( item->item()->size() ).rightJustify( 20, '0' );
}

void KonqKfmIconView::disableIcons( const KURL::List & lst )
{
    m_pIconView->disableIcons( lst );
}

#include "konq_iconview.moc"
