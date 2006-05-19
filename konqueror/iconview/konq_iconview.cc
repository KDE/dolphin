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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_iconview.h"
#include "konq_propsview.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <QFile>

#include <kactioncollection.h>
#include <kactionmenu.h>
#include <kapplication.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobalsettings.h>
#include <kinputdialog.h>
#include <kivdirectoryoverlay.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <konq_settings.h>
#include <kpropertiesdialog.h>
#include <kseparatoraction.h>
#include <kstaticdeleter.h>
#include <kstdaction.h>
#include <kparts/factory.h>
#include <ktrader.h>
#include <ktoggleaction.h>

#include <QRegExp>
#include <QDateTime>

#include <config.h>

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

    virtual KParts::Part* createPartObject( QWidget *parentWidget,
                                      QObject *parent, const char*, const QStringList &args )
   {
      if( args.count() < 1 )
         kWarning() << "KonqKfmIconView: Missing Parameter" << endl;

      KonqKfmIconView *obj = new KonqKfmIconView( parentWidget, parent, args.first() );
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


K_EXPORT_COMPONENT_FACTORY( konq_iconview, KonqIconViewFactory )


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

void IconViewBrowserExtension::trash()
{
   KonqOperations::del(m_iconView->iconViewWidget(),
                       KonqOperations::TRASH,
                       m_iconView->iconViewWidget()->selectedUrls( KonqIconViewWidget::MostLocalUrls ));
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
  //kDebug(1202) << "IconViewBrowserExtension::setNameFilter " << nameFilter << endl;
  m_iconView->m_nameFilter = nameFilter;
}

KonqKfmIconView::KonqKfmIconView( QWidget *parentWidget, QObject *parent, const QString& mode  )
    : KonqDirPart( parent )
    , m_bNeedSetCurrentItem( false )
    , m_pEnsureVisible( 0 )
    , m_paOutstandingOverlaysTimer( 0 )
    , m_pTimeoutRefreshTimer( 0 )
    , m_itemDict( 43 )
{
    kDebug(1202) << "+KonqKfmIconView" << endl;

    setBrowserExtension( new IconViewBrowserExtension( this ) );

    // Create a properties instance for this view
    m_pProps = new KonqPropsView( KonqIconViewFactory::instance(), KonqIconViewFactory::defaultViewProps() );

    m_pIconView = new KonqIconViewWidget( parentWidget );
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

    m_paDotFiles = new KToggleAction( i18n( "Show &Hidden Files" ), actionCollection(), "show_dot" );
    connect(m_paDotFiles, SIGNAL(triggered(bool) ), SLOT( slotShowDot() ));
//    m_paDotFiles->setCheckedState(i18n("Hide &Hidden Files"));
    m_paDotFiles->setToolTip( i18n( "Toggle displaying of hidden dot files" ) );

    m_paDirectoryOverlays = new KToggleAction( i18n( "&Folder Icons Reflect Contents" ), actionCollection(), "show_directory_overlays" );
    connect(m_paDirectoryOverlays, SIGNAL(triggered(bool) ), SLOT( slotShowDirectoryOverlays() ));

    m_pamPreview = new KActionMenu( i18n( "&Preview" ), actionCollection(), "iconview_preview" );

    m_paEnablePreviews = new KToggleAction( i18n("Enable Previews"), actionCollection(), "iconview_preview_all" );
    m_paEnablePreviews->setCheckedState( i18n("Disable Previews") );
    connect( m_paEnablePreviews, SIGNAL( toggled( bool ) ), this, SLOT( slotPreview( bool ) ) );
    m_paEnablePreviews->setIcon(KIcon("thumbnail"));
    m_pamPreview->insert( m_paEnablePreviews );
    m_pamPreview->insert( new KSeparatorAction(actionCollection()) );

    KTrader::OfferList plugins = KTrader::self()->query( "ThumbCreator" );
    QMap< QString, KToggleAction* > previewActions;
    for ( KTrader::OfferList::ConstIterator it = plugins.begin(); it != plugins.end(); ++it )
    {
        if ( KToggleAction*& preview = previewActions[ ( *it )->name() ] )
            // Bypassing KAction restriction - this action will not be found via KActionCollection when doing a name search
            preview->QAction::setObjectName( QByteArray( preview->name() ) + ',' + ( *it )->desktopEntryName().toLatin1() );
        else
        {
            preview = new KToggleAction( (*it)->name(), actionCollection(), (*it)->desktopEntryName().toLatin1() );
            connect( preview, SIGNAL( toggled( bool ) ), this, SLOT( slotPreview( bool ) ) );
            m_pamPreview->insert( preview );
            m_paPreviewPlugins.append( preview );
        }
    }
    KToggleAction *soundPreview = new KToggleAction( i18n("Sound Files"), actionCollection(), "audio/" );
    connect( soundPreview, SIGNAL( toggled( bool ) ), this, SLOT( slotPreview( bool ) ) );
    m_pamPreview->insert( soundPreview );
    m_paPreviewPlugins.append( soundPreview );

    //    m_pamSort = new KActionMenu( i18n( "Sort..." ), actionCollection(), "sort" );

    KToggleAction *aSortByNameCS = new KToggleAction( i18n( "By Name (Case Sensitive)" ), actionCollection(), "sort_nc" );
    KToggleAction *aSortByNameCI = new KToggleAction( i18n( "By Name (Case Insensitive)" ), actionCollection(), "sort_nci" );
    KToggleAction *aSortBySize = new KToggleAction( i18n( "By Size" ), actionCollection(), "sort_size" );
    KToggleAction *aSortByType = new KToggleAction( i18n( "By Type" ), actionCollection(), "sort_type" );
    KToggleAction *aSortByDate = new KToggleAction( i18n( "By Date" ), actionCollection(), "sort_date" );

    QActionGroup* sorting = new QActionGroup(this);
    sorting->setExclusive(true);
    aSortByNameCS->setActionGroup( sorting );
    aSortByNameCI->setActionGroup( sorting );
    aSortBySize->setActionGroup( sorting );
    aSortByType->setActionGroup( sorting );
    aSortByDate->setActionGroup( sorting );

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
    KToggleAction *sort_action = dynamic_cast<KToggleAction *>(actionCollection()->action(sortcrit.toLatin1().constData()));
    if(sort_action!=NULL) sort_action->trigger();

    m_paSortDirsFirst = new KToggleAction( i18n( "Folders First" ), actionCollection(), "sort_directoriesfirst" );
    KToggleAction *aSortDescending = new KToggleAction( i18n( "Descending" ), actionCollection(), "sort_descend" );

    m_paSortDirsFirst->setChecked( KonqIconViewFactory::defaultViewProps()->isDirsFirst() );

    connect( aSortDescending, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDescending() ) );
    connect( m_paSortDirsFirst, SIGNAL( toggled( bool ) ), this, SLOT( slotSortDirsFirst() ) );

    //enable stored settings
    slotSortDirsFirst();
    if (KonqIconViewFactory::defaultViewProps()->isDescending())
    {
     aSortDescending->setChecked(true);
     m_pIconView->setSorting(true,true);//enable sort ascending in Q3IconView
     slotSortDescending();//invert sorting (now descending) and actually resort items
    }

    /*
    m_pamSort->insert( aSortByNameCS );
    m_pamSort->insert( aSortByNameCI );
    m_pamSort->insert( aSortBySize );

    m_pamSort->popupMenu()->insertSeparator();

    m_pamSort->insert( aSortDescending );
    */
    m_paSelect = new KAction( i18n( "Se&lect..." ), actionCollection(), "select" );
    connect(m_paSelect, SIGNAL(triggered(bool) ), SLOT( slotSelect() ));
    m_paSelect->setShortcut(Qt::CTRL+Qt::Key_Plus);
    m_paUnselect = new KAction( i18n( "Unselect..." ), actionCollection(), "unselect" );
    connect(m_paUnselect, SIGNAL(triggered(bool) ), SLOT( slotUnselect() ));
    m_paUnselect->setShortcut(Qt::CTRL+Qt::Key_Minus);
    m_paSelectAll = KStdAction::selectAll( this, SLOT( slotSelectAll() ), actionCollection(), "selectall" );
    m_paUnselectAll = new KAction( i18n( "Unselect All" ), actionCollection(), "unselectall" );
    connect(m_paUnselectAll, SIGNAL(triggered(bool) ), SLOT( slotUnselectAll() ));
    m_paUnselectAll->setShortcut(Qt::CTRL+Qt::Key_U);
    m_paInvertSelection = new KAction( i18n( "&Invert Selection" ), actionCollection(), "invertselection" );
    connect(m_paInvertSelection, SIGNAL(triggered(bool) ), SLOT( slotInvertSelection() ));
    m_paInvertSelection->setShortcut(Qt::CTRL+Qt::Key_Asterisk);

    m_paSelect->setToolTip( i18n( "Allows selecting of file or folder items based on a given mask" ) );
    m_paUnselect->setToolTip( i18n( "Allows unselecting of file or folder items based on a given mask" ) );
    m_paSelectAll->setToolTip( i18n( "Selects all items" ) );
    m_paUnselectAll->setToolTip( i18n( "Unselects all selected items" ) );
    m_paInvertSelection->setToolTip( i18n( "Inverts the current selection of items" ) );

    //m_paBottomText = new KToggleAction( i18n( "Text at &Bottom" ), 0, actionCollection(), "textbottom" );
    //m_paRightText = new KToggleAction( i18n( "Text at &Right" ), 0, actionCollection(), "textright" );
    //m_paBottomText->setActionGroup( "TextPos" );
    //m_paRightText->setActionGroup( "TextPos" );
    //connect( m_paBottomText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextBottom( bool ) ) );
    //connect( m_paRightText, SIGNAL( toggled( bool ) ), this, SLOT( slotTextRight( bool ) ) );

    connect( m_pIconView, SIGNAL( executed( Q3IconViewItem * ) ),
             this, SLOT( slotReturnPressed( Q3IconViewItem * ) ) );
    connect( m_pIconView, SIGNAL( returnPressed( Q3IconViewItem * ) ),
             this, SLOT( slotReturnPressed( Q3IconViewItem * ) ) );

    connect( m_pIconView, SIGNAL( onItem( Q3IconViewItem * ) ),
             this, SLOT( slotOnItem( Q3IconViewItem * ) ) );

    connect( m_pIconView, SIGNAL( onViewport() ),
             this, SLOT( slotOnViewport() ) );

    connect( m_pIconView, SIGNAL( mouseButtonPressed(int, Q3IconViewItem*, const QPoint&)),
             this, SLOT( slotMouseButtonPressed(int, Q3IconViewItem*, const QPoint&)) );
    connect( m_pIconView, SIGNAL( mouseButtonClicked(int, Q3IconViewItem*, const QPoint&)),
             this, SLOT( slotMouseButtonClicked(int, Q3IconViewItem*, const QPoint&)) );
    connect( m_pIconView, SIGNAL( contextMenuRequested(Q3IconViewItem*, const QPoint&)),
             this, SLOT( slotContextMenuRequested(Q3IconViewItem*, const QPoint&)) );

    // Signals needed to implement the spring loading folders behavior
    connect( m_pIconView, SIGNAL( held( Q3IconViewItem * ) ),
             this, SLOT( slotDragHeld( Q3IconViewItem * ) ) );
    connect( m_pIconView, SIGNAL( dragEntered( bool ) ),
             this, SLOT( slotDragEntered( bool ) ) );
    connect( m_pIconView, SIGNAL( dragLeft() ),
             this, SLOT( slotDragLeft() ) );
    connect( m_pIconView, SIGNAL( dragMove( bool ) ),
             this, SLOT( slotDragMove( bool ) ) );
    connect( m_pIconView, SIGNAL( dragFinished() ),
             this, SLOT( slotDragFinished() ) );

    // Create the directory lister
    m_dirLister = new KDirLister( true );
    setDirLister( m_dirLister );
    m_dirLister->setMainWindow(m_pIconView->topLevelWidget());

    connect( m_dirLister, SIGNAL( started( const KUrl & ) ),
             this, SLOT( slotStarted() ) );
    connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
    connect( m_dirLister, SIGNAL( canceled( const KUrl& ) ), this, SLOT( slotCanceled( const KUrl& ) ) );
    connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
    connect( m_dirLister, SIGNAL( newItems( const KFileItemList& ) ),
             this, SLOT( slotNewItems( const KFileItemList& ) ) );
    connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
             this, SLOT( slotDeleteItem( KFileItem * ) ) );
    connect( m_dirLister, SIGNAL( refreshItems( const KFileItemList& ) ),
             this, SLOT( slotRefreshItems( const KFileItemList& ) ) );
    connect( m_dirLister, SIGNAL( redirection( const KUrl & ) ),
             this, SLOT( slotRedirection( const KUrl & ) ) );
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
    m_bDirPropertiesChanged = true;
    m_bPreviewRunningBeforeCloseURL = false;
    m_pIconView->setResizeMode( Q3IconView::Adjust );

    connect( m_pIconView, SIGNAL( selectionChanged() ),
             this, SLOT( slotSelectionChanged() ) );

    // Respect kcmkonq's configuration for word-wrap icon text.
    // If we want something else, we have to adapt the configuration or remove it...
    m_pIconView->setIconTextHeight(KonqFMSettings::settings()->iconTextHeight());

    // Finally, determine initial grid size again, with those parameters
    //    m_pIconView->calculateGridX();

    setViewMode( mode );
}

KonqKfmIconView::~KonqKfmIconView()
{
    // Before anything else, stop the image preview. It might use our fileitems,
    // and it will only be destroyed togetierh with our widget
    m_pIconView->stopImagePreview();

    kDebug(1202) << "-KonqKfmIconView" << endl;
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
    QString name = sender()->objectName(); // e.g. clipartthumbnail (or audio/, special case)
    if (name == "iconview_preview_all")
    {
        m_pProps->setShowingPreview( toggle );
        m_pIconView->setPreviewSettings( m_pProps->previewSettings() );
        if ( !toggle )
        {
            kDebug() << "KonqKfmIconView::slotPreview stopping all previews for " << name << endl;
            m_pIconView->disableSoundPreviews();

            bool previewRunning = m_pIconView->isPreviewRunning();
            if ( previewRunning )
                m_pIconView->stopImagePreview();
            m_pIconView->setIcons( m_pIconView->iconSize(), QStringList() += "*" );
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
        QStringList types = name.split( ',', QString::SkipEmptyParts );
        for ( QStringList::ConstIterator it = types.begin(); it != types.end(); ++it )
        {
            m_pProps->setShowingPreview( *it, toggle );
            m_pIconView->setPreviewSettings( m_pProps->previewSettings() );
            if ( !toggle )
            {
                kDebug() << "KonqKfmIconView::slotPreview stopping image preview for " << *it << endl;
                if ( *it == "audio/" )
                    m_pIconView->disableSoundPreviews();
                else
                {
                    KService::Ptr serv = KService::serviceByDesktopName( *it );
                    Q_ASSERT( serv );
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

    for ( Q3IconViewItem *item = m_pIconView->firstItem(); item; item = item->nextItem() )
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
    QString pattern = KInputDialog::getText( QString(),
        i18n( "Select files:" ), "*", &ok, m_pIconView );
    if ( ok )
    {
        QRegExp re( pattern, Qt::CaseSensitive, QRegExp::Wildcard );

        m_pIconView->blockSignals( true );

        Q3IconViewItem *it = m_pIconView->firstItem();
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
    QString pattern = KInputDialog::getText( QString(),
        i18n( "Unselect files:" ), "*", &ok, m_pIconView );
    if ( ok )
    {
        QRegExp re( pattern, Qt::CaseSensitive, QRegExp::Wildcard );

        m_pIconView->blockSignals( true );

        Q3IconViewItem *it = m_pIconView->firstItem();
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
       effSize = IconSize(K3Icon::Desktop);

    int oldEffSize = m_pIconView->iconSize();
    if (oldEffSize == 0)
       oldEffSize = IconSize(K3Icon::Desktop);

    // Make sure all actions are initialized.
    KonqDirPart::newIconSize( size );

    if ( effSize == oldEffSize )
        return;

    // Stop a preview job that might be running
    m_pIconView->stopImagePreview();

    // Set icons size, arrage items in grid and repaint the whole view
    m_pIconView->setIcons( size );

    // If previews are enabled start a new job
    if ( m_pProps->isShowingPreview() )
        m_pIconView->startImagePreview( m_pProps->previewSettings(), true );
}

bool KonqKfmIconView::doCloseURL()
{
    m_dirLister->stop();

    m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();

    m_bPreviewRunningBeforeCloseURL = m_pIconView->isPreviewRunning();
    m_pIconView->stopImagePreview();
    return true;
}

void KonqKfmIconView::slotReturnPressed( Q3IconViewItem *item )
{
    if ( !item )
        return;

    item->setSelected( false, true );
    m_pIconView->visualActivate(item);

    KFileItem *fileItem = (static_cast<KFileIVI*>(item))->item();
    if ( !fileItem )
        return;
    lmbClicked( fileItem );
}

void KonqKfmIconView::slotDragHeld( Q3IconViewItem *item )
{
    kDebug() << "KonqKfmIconView::slotDragHeld()" << endl;

    // This feature is not usable if the user wants one window per folder
    if ( KonqFMSettings::settings()->alwaysNewWin() )
        return;

    if ( !item )
        return;

    KFileItem *fileItem = (static_cast<KFileIVI*>(item))->item();

    SpringLoadingManager::self().springLoadTrigger(this, fileItem, item);
}

void KonqKfmIconView::slotDragEntered( bool )
{
    if ( SpringLoadingManager::exists() )
        SpringLoadingManager::self().dragEntered(this);
}

void KonqKfmIconView::slotDragLeft()
{
    kDebug() << "KonqKfmIconView::slotDragLeft()" << endl;

    if ( SpringLoadingManager::exists() )
        SpringLoadingManager::self().dragLeft(this);
}

void KonqKfmIconView::slotDragMove( bool accepted )
{
    if ( !accepted )
        emit setStatusBarText( i18n( "You cannot drop any items in a directory in which you do not have write permission" ) );
}

void KonqKfmIconView::slotDragFinished()
{
    kDebug() << "KonqKfmIconView::slotDragFinished()" << endl;

    if ( SpringLoadingManager::exists() )
        SpringLoadingManager::self().dragFinished(this);
}


void KonqKfmIconView::slotContextMenuRequested(Q3IconViewItem* _item, const QPoint& _global)
{
    const KFileItemList items = m_pIconView->selectedFileItems();
    if ( items.isEmpty() )
        return;

    KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems;

    KFileIVI* i = static_cast<KFileIVI*>(_item);
    if (i)
        i->setSelected( true, true /* don't touch other items */ );

    KFileItem * rootItem = m_dirLister->rootItem();
    if ( rootItem ) {
        KUrl parentDirURL = rootItem->url();
        // Check if parentDirURL applies to the selected items (usually yes, but not with search results)
        KFileItemList::const_iterator kit = items.begin();
        const KFileItemList::const_iterator kend = items.end();
        for ( ; kit != kend; ++kit )
            if ( (*kit)->url().directory( ) != rootItem->url().path() )
                parentDirURL = KUrl();
        // If rootItem is the parent of the selected items, then we can use isWritable() on it.
        if ( !parentDirURL.isEmpty() && !rootItem->isWritable() )
            popupFlags |= KParts::BrowserExtension::NoDeletion;
    }

    emit m_extension->popupMenu( 0L, _global, items, KParts::URLArgs(), popupFlags);
}

void KonqKfmIconView::slotMouseButtonPressed(int _button, Q3IconViewItem* _item, const QPoint&)
{
    if ( _button == Qt::RightButton && !_item )
    {
        // Right click on viewport
        KFileItem * item = m_dirLister->rootItem();
        bool delRootItem = false;
        if ( ! item )
        {
            if ( m_bLoading )
            {
                kDebug(1202) << "slotViewportRightClicked : still loading and no root item -> dismissed" << endl;
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

void KonqKfmIconView::slotMouseButtonClicked(int _button, Q3IconViewItem* _item, const QPoint& )
{
    if( _button == Qt::MidButton )
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

void KonqKfmIconView::slotCanceled( const KUrl& url )
{
    // Check if this canceled() signal is about the URL we're listing.
    // It could be about the URL we were listing, and openURL() aborted it.
    if ( m_bLoading && url.equals( m_pIconView->url(), KUrl::CompareWithoutTrailingSlash ) )
    {
        emit canceled( QString() );
        m_bLoading = false;
    }

    // Stop the "refresh if busy too long" timer because a viewport
    // update is coming.
    if ( m_pTimeoutRefreshTimer && m_pTimeoutRefreshTimer->isActive() )
        m_pTimeoutRefreshTimer->stop();

    // See slotCompleted(). If a listing gets canceled, it doesn't emit
    // the completed() signal, so handle that case.
    if ( !m_pIconView->viewport()->updatesEnabled() )
    {
        m_pIconView->viewport()->setUpdatesEnabled( true );
        m_pIconView->viewport()->repaint();
    }
    if ( m_pEnsureVisible ){
        m_pIconView->ensureItemVisible( m_pEnsureVisible );
        m_pEnsureVisible = 0;
    }
}

void KonqKfmIconView::slotCompleted()
{
    // Stop the "refresh if busy too long" timer because a viewport
    // update is coming.
    if ( m_pTimeoutRefreshTimer && m_pTimeoutRefreshTimer->isActive() )
        m_pTimeoutRefreshTimer->stop();

    // If updates to the viewport are still blocked (so slotNewItems() has
    // not been called), a viewport repaint is forced.
    if ( !m_pIconView->viewport()->updatesEnabled() )
    {
        m_pIconView->viewport()->setUpdatesEnabled( true );
        m_pIconView->viewport()->repaint();
    }

    // Root item ? Store root item in konQ3IconViewwidget (whether 0L or not)
    m_pIconView->setRootItem( m_dirLister->rootItem() );

    // only after initial listing, not after updates
    // If we don't set a current item, the iconview has none (one more keypress needed)
    // but it appears on focusin... Q3IconView bug, Reggie acknowledged it LONG ago (07-2000).
    if ( m_bNeedSetCurrentItem )
    {
        m_pIconView->setCurrentItem( m_pIconView->firstItem() );
        m_bNeedSetCurrentItem = false;
    }

    if ( m_bUpdateContentsPosAfterListing ) {
         m_pIconView->setContentsPos( extension()->urlArgs().xOffset,
                                      extension()->urlArgs().yOffset );
    }

    if ( m_pEnsureVisible ){
        m_pIconView->ensureItemVisible( m_pEnsureVisible );
        m_pEnsureVisible = 0;
    }

    m_bUpdateContentsPosAfterListing = false;

    if ( !m_pIconView->firstItem() )
	resetCount();

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
    // Stop the autorefresh timer since data to display has arrived and will
    // be drawn in moments
    if ( m_pTimeoutRefreshTimer && m_pTimeoutRefreshTimer->isActive() )
        m_pTimeoutRefreshTimer->stop();
    // We need to disable graphics updates on the iconview when
    // inserting items, or else a blank paint operation will be
    // performed on the top-left corner for each inserted item!
    m_pIconView->setUpdatesEnabled( false );
    KFileItemList::const_iterator kit = entries.begin();
    const KFileItemList::const_iterator kend = entries.end();
    for (; kit != kend; ++kit )
    {
        //kDebug(1202) << "KonqKfmIconView::slotNewItem(...)" << _fileitem->url().url() << endl;
        KFileIVI* item = new KFileIVI( m_pIconView, *kit, m_pIconView->iconSize() );
        item->setRenameEnabled( false );

        KFileItem* fileItem = item->item();
        if ( !m_itemsToSelect.isEmpty() ) {
           int tsit = m_itemsToSelect.indexOf( fileItem->name() );
           if ( tsit >= 0 ) {
              m_itemsToSelect.removeAt( tsit );
              m_pIconView->setSelected( item, true, true );
              if ( m_bNeedSetCurrentItem ){
                 m_pIconView->setCurrentItem( item );
                 if( !m_pEnsureVisible )
                    m_pEnsureVisible = item;
                 m_bNeedSetCurrentItem = false;
              }
           }
        }

        if ( fileItem->isDir() && m_pProps->isShowingDirectoryOverlays() ) {
            showDirectoryOverlay(item);
        }

        QString key;

        switch ( m_eSortCriterion )
        {
            case NameCaseSensitive: key = item->text(); break;
            case NameCaseInsensitive: key = item->text().toLower(); break;
            case Size: key = makeSizeKey( item ); break;
            case Type: key = item->item()->mimetype()+ "\008" +item->text().toLower(); break; // ### slows down listing :-(
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

        //kDebug() << "KonqKfmIconView::slotNewItems " << (*kit)->url().url() << " " << (*kit)->mimeTypePtr()->name() << " mimetypeknown:" << (*kit)->isMimeTypeKnown() << endl;
        if ( !(*kit)->isMimeTypeKnown() )
            m_mimeTypeResolver->m_lstPendingMimeIconItems.append( item );

        m_itemDict.insert( *kit, item );
    }
    // After filtering out updates-on-insertions we can re-enable updates
    m_pIconView->setUpdatesEnabled( true );
    // Locking the viewport has filtered out blanking and now, since we
    // have some items to draw, we can restore updating.
    if ( !m_pIconView->viewport()->updatesEnabled() )
        m_pIconView->viewport()->setUpdatesEnabled( true );
    KonqDirPart::newItems( entries );
}

void KonqKfmIconView::slotDeleteItem( KFileItem * _fileitem )
{
    if ( _fileitem == m_dirLister->rootItem() )
    {
        m_pIconView->stopImagePreview();
        m_pIconView->setRootItem( 0L );
        return;
    }

    //kDebug(1202) << "KonqKfmIconView::slotDeleteItem(...)" << endl;
    // we need to find out the iconcontainer item containing the fileitem
    KFileIVI * ivi = m_itemDict[ _fileitem ];
    // It can be that we know nothing about this item, e.g. because it's filtered out
    // (by default: dot files). KDirLister still tells us about it when it's modified, since
    // it doesn't know if we showed it before, and maybe its mimetype changed so we
    // might have to hide it now.
    if (ivi)
    {
        m_pIconView->stopImagePreview();
        KonqDirPart::deleteItem( _fileitem );

        m_pIconView->takeItem( ivi );
        m_mimeTypeResolver->m_lstPendingMimeIconItems.removeAll( ivi );
        m_itemDict.remove( _fileitem );
        if (m_paOutstandingOverlays.first() == ivi) // Being processed?
           m_paOutstandingOverlaysTimer->start(20); // Restart processing...

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
              m_paOutstandingOverlaysTimer->setSingleShot(true);
              connect(m_paOutstandingOverlaysTimer, SIGNAL(timeout()),
                      SLOT(slotDirectoryOverlayStart()));
           }
           m_paOutstandingOverlaysTimer->start(20);
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
        m_paOutstandingOverlaysTimer->start(0); // Don't call directly to prevent deep recursion.
}

// see also KDesktop::slotRefreshItems
void KonqKfmIconView::slotRefreshItems( const KFileItemList& entries )
{
    bool bNeedRepaint = false;
    bool bNeedPreviewJob = false;
    KFileItemList::const_iterator kit = entries.begin();
    const KFileItemList::const_iterator kend = entries.end();
    for (; kit != kend; ++kit )
    {
        KFileIVI * ivi = m_itemDict[ *kit ];
        Q_ASSERT(ivi);
        kDebug() << "KonqKfmIconView::slotRefreshItems '" << (*kit)->name() << "' ivi=" << ivi << endl;
        if (ivi)
        {
            QSize oldSize = ivi->pixmap()->size();
            if ( ivi->isThumbnail() ) {
                bNeedPreviewJob = true;
                ivi->invalidateThumbnail();
            }
            else
                ivi->refreshIcon( true );
            ivi->setText( (*kit)->text() );
            if ( (*kit)->isMimeTypeKnown() )
                ivi->setMouseOverAnimation( (*kit)->iconName() );
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

    // We're now going to update the view with new contents. To avoid
    // meaningless paint operations (such as a clear() just before drawing
    // fresh contents) we disable updating the viewport until we'll
    // receive some data or a timeout timer expires.
    m_pIconView->viewport()->setUpdatesEnabled( false );
    if ( !m_pTimeoutRefreshTimer )
    {
        m_pTimeoutRefreshTimer = new QTimer( this );
        m_pTimeoutRefreshTimer->setSingleShot( true );
        connect( m_pTimeoutRefreshTimer, SIGNAL( timeout() ),
                 this, SLOT( slotRefreshViewport() ) );
    }
    m_pTimeoutRefreshTimer->start( 700 );

    // Clear contents but don't clear graphics as updates are disabled.
    m_pIconView->clear();
    // If directory properties are changed, apply pending changes
    // changes are: view background or color, iconsize, enabled previews
    if ( m_bDirPropertiesChanged )
    {
        m_pProps->applyColors( m_pIconView->viewport() );
        newIconSize( m_pProps->iconSize() );
        m_pIconView->setPreviewSettings( m_pProps->previewSettings() );
    }

    m_mimeTypeResolver->m_lstPendingMimeIconItems.clear();
    m_itemDict.clear();
    // Bug in Q3IconView IMHO - it should emit selectionChanged()
    // (bug reported, but code seems to be that way on purpose)
    m_pIconView->slotSelectionChanged();
    slotSelectionChanged();
}

void KonqKfmIconView::slotRedirection( const KUrl & url )
{
    const QString prettyURL = url.pathOrURL();
    emit m_extension->setLocationBarURL( prettyURL );
    emit setWindowCaption( prettyURL );
    m_pIconView->setURL( url );
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
  // kDebug() << "KonqKfmIconView::determineIcon " << item->item()->name() << endl;
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
    kDebug(1202) << "KonqKfmIconView::slotRenderingFinished()" << endl;
    if ( m_bNeedEmitCompleted )
    {
        kDebug(1202) << "KonqKfmIconView completed() after rendering" << endl;
        emit completed();
        m_bNeedEmitCompleted = false;
    }
    if ( m_bNeedAlign )
    {
        m_bNeedAlign = false;
        kDebug(1202) << "arrangeItemsInGrid" << endl;
        m_pIconView->arrangeItemsInGrid();
    }
}

void KonqKfmIconView::slotRefreshViewport()
{
    kDebug(1202) << "KonqKfmIconView::slotRefreshViewport()" << endl;
    QWidget * vp = m_pIconView->viewport();
    bool prevState = vp->updatesEnabled();
    vp->setUpdatesEnabled( true );
    vp->repaint();
    vp->setUpdatesEnabled( prevState );
}

bool KonqKfmIconView::doOpenURL( const KUrl & url )
{
    // Store url in the icon view
    m_pIconView->setURL( url );

    m_bLoading = true;
    m_bNeedSetCurrentItem = true;

    // Check for new properties in the new dir
    // enterDir returns true the first time, and any time something might
    // have changed.
    m_bDirPropertiesChanged = m_pProps->enterDir( url );

    m_dirLister->setNameFilter( m_nameFilter );

    m_dirLister->setMimeFilter( mimeFilter() );

    // This *must* happen before m_dirLister->openURL because it emits
    // clear() and Q3IconView::clear() calls setContentsPos(0,0)!
    KParts::URLArgs args = m_extension->urlArgs();
    if ( args.reload )
    {
        args.xOffset = m_pIconView->contentsX();
        args.yOffset = m_pIconView->contentsY();
        m_extension->setURLArgs( args );

        m_filesToSelect.clear();
        const KFileItemList selItems = selectedFileItems();
        KFileItemList::const_iterator kit = selItems.begin();
        const KFileItemList::const_iterator kend = selItems.end();
        for (; kit != kend; ++kit )
            m_filesToSelect += (*kit)->name();
    }

    m_itemsToSelect = m_filesToSelect;

    m_dirLister->setShowingDotFiles( m_pProps->isShowingDotFiles() );

    m_bNeedAlign = false;
    m_bUpdateContentsPosAfterListing = true;

    m_paOutstandingOverlays.clear();

    // Start the directory lister !
    m_dirLister->openURL( url, false, args.reload );

    // View properties (icon size, background, ..) will be applied into slotClear()
    // if m_bDirPropertiesChanged is set. If so, here we update preview actions.
    if ( m_bDirPropertiesChanged )
    {
      m_paDotFiles->setChecked( m_pProps->isShowingDotFiles() );
      m_paDirectoryOverlays->setChecked( m_pProps->isShowingDirectoryOverlays() );
      m_paEnablePreviews->setChecked( m_pProps->isShowingPreview() );
      for ( m_paPreviewPlugins.first(); m_paPreviewPlugins.current(); m_paPreviewPlugins.next() )
      {
          QStringList types = QString(m_paPreviewPlugins.current()->objectName()).split(',', QString::SkipEmptyParts );
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
    }

    const QString prettyURL = url.pathOrURL();
    emit setWindowCaption( prettyURL );

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

void KonqKfmIconView::slotOnItem( Q3IconViewItem *item )
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
    // note: this should be moved to KonQ3IconViewWidget. It would make the code
    // more readable :)

    m_mode = mode;
    if (mode=="MultiColumnView")
    {
        m_pIconView->setArrangement(Q3IconView::TopToBottom);
        m_pIconView->setItemTextPos(Q3IconView::Right);
    }
    else
    {
        m_pIconView->setArrangement(Q3IconView::LeftToRight);
        m_pIconView->setItemTextPos(Q3IconView::Bottom);
    }

    if ( m_bPreviewRunningBeforeCloseURL )
    {
        m_bPreviewRunningBeforeCloseURL = false;
        // continue (param: false) a preview job interrupted by doCloseURL
        m_pIconView->startImagePreview( m_pProps->previewSettings(), false );
    }
}

void KonqKfmIconView::setupSortKeys()
{
    switch ( m_eSortCriterion )
    {
    case NameCaseSensitive:
        m_pIconView->setCaseInsensitiveSort( false );
        for ( Q3IconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( it->text() );
        break;
    case NameCaseInsensitive:
        m_pIconView->setCaseInsensitiveSort( true );
        for ( Q3IconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( it->text().toLower() );
        break;
    case Size:
        for ( Q3IconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( makeSizeKey( (KFileIVI *)it ) );
        break;
    case Type:
        // Sort by Type + Name (#17014)
        for ( Q3IconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
            it->setKey( static_cast<KFileIVI *>( it )->item()->mimetype() + "\008" + it->text().toLower() );
        break;
    case Date:
    {
        //Sorts by time of modification (#52750)
        QDateTime dayt;
        for ( Q3IconViewItem *it = m_pIconView->firstItem(); it; it = it->nextItem() )
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
    return KIO::number( item->item()->size() ).rightJustified( 20, '0' );
}

void KonqKfmIconView::disableIcons( const KUrl::List & lst )
{
    m_pIconView->disableIcons( lst );
}


SpringLoadingManager *SpringLoadingManager::s_self = 0L;
static KStaticDeleter<SpringLoadingManager> s_springManagerDeleter;

SpringLoadingManager::SpringLoadingManager()
    : m_startPart(0L)
{
    connect( &m_endTimer, SIGNAL( timeout() ),
             this, SLOT( finished() ) );

}

SpringLoadingManager &SpringLoadingManager::self()
{
    if ( !s_self )
    {
        s_springManagerDeleter.setObject(s_self, new SpringLoadingManager());
    }

    return *s_self;
}

bool SpringLoadingManager::exists()
{
    return s_self!=0L;
}


void SpringLoadingManager::springLoadTrigger(KonqKfmIconView *view,
                                             KFileItem *file,
                                             Q3IconViewItem *item)
{
    if ( !file || !file->isDir() )
        return;

    // We start a new spring loading chain
    if ( m_startPart==0L )
    {
        m_startURL = view->url();
        m_startPart = view;
    }

    // Only the last part of the chain is allowed to trigger a spring load
    // event (if a spring loading chain is in progress)
    if ( view!=m_startPart )
        return;


    item->setSelected( false, true );
    view->iconViewWidget()->visualActivate(item);

    KUrl url = file->url();

    KParts::URLArgs args;
    file->determineMimeType();
    if ( file->isMimeTypeKnown() )
        args.serviceType = file->mimetype();
    args.trustedSource = true;

    // Open the folder URL, we don't want to modify the browser
    // history, hence the use of openURL and setLocationBarURL
    view->openURL(url);
    const QString prettyURL = url.pathOrURL();
    emit view->extension()->setLocationBarURL( prettyURL );
}

void SpringLoadingManager::dragLeft(KonqKfmIconView */*view*/)
{
    // We leave a view maybe the user tries to cancel the current spring loading
    if ( !m_startURL.isEmpty() )
    {
        m_endTimer.setSingleShot(true);
        m_endTimer.start(1000);
    }
}

void SpringLoadingManager::dragEntered(KonqKfmIconView *view)
{
    // We enter a view involved in the spring loading chain
    if ( !m_startURL.isEmpty() && m_startPart==view )
    {
        m_endTimer.stop();
    }
}

void SpringLoadingManager::dragFinished(KonqKfmIconView */*view*/)
{
    if ( !m_startURL.isEmpty() )
    {
        finished();
    }
}


void SpringLoadingManager::finished()
{
    kDebug() << "SpringLoadManager::finished()" << endl;

    KUrl url = m_startURL;
    m_startURL = KUrl();

    KParts::ReadOnlyPart *part = m_startPart;
    m_startPart = 0L;

    KonqKfmIconView *view = static_cast<KonqKfmIconView*>(part);
    view->openURL(url);
    const QString prettyURL = url.pathOrURL();
    emit view->extension()->setLocationBarURL( prettyURL );

    deleteLater();
    s_self = 0L;
    s_springManagerDeleter.setObject(s_self, static_cast<SpringLoadingManager*>(0L));
}



#include "konq_iconview.moc"
