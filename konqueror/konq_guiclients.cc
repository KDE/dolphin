/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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

#include <kaction.h>
#include <kconfig.h>
#include <kdebug.h>
#include <klocale.h>
#include <kmenubar.h>
#include <konq_childview.h>
#include <konq_factory.h>
#include <konq_frame.h>
#include <konq_guiclients.h>
#include <konq_mainview.h>
#include <konq_viewmgr.h>

static const char *viewModeGUI = ""
"<!DOCTYPE viewmodexml>"
"<viewmodexml name=\"viewmode\">"
"<MenuBar>"
" <Menu name=\"view\">"
"  <Menu name=\"viewmodes\">"
"  </Menu>"
" </Menu>"
"</MenuBar>"
"</viewmodexml>";

ViewModeGUIClient::ViewModeGUIClient( KonqMainView *mainView )
 : QObject( mainView )
{
  m_mainView = mainView;
  m_doc.setContent( QString::fromLatin1( viewModeGUI ) );
  m_menuElement = m_doc.documentElement().namedItem( "MenuBar" ).namedItem( "Menu" ).namedItem( "Menu" ).toElement();
  m_actions = 0L;
  setDocument( m_doc );
}

KAction *ViewModeGUIClient::action( const QDomElement &element ) const
{
  if ( !m_actions )
    return 0L;

  return m_actions->action( element.attribute( "name" ) );
}

void ViewModeGUIClient::update( const KTrader::OfferList &services )
{
  if ( m_actions )
    delete m_actions;

  m_actions = new KActionCollection( this );

  QDomNode n = m_menuElement.firstChild();
  while ( !n.isNull() )
  {
    m_menuElement.removeChild( n );
    n = m_menuElement.firstChild();
  }

  if ( services.count() <= 1 )
    return;

  QDomElement textElement = m_doc.createElement( "text" );
  textElement.appendChild( m_doc.createTextNode( i18n( "View Mode..." ) ) );
  m_menuElement.appendChild( textElement );

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
      QVariant prop = (*it)->property( "X-KDE-BrowserView-Toggable" );
      if ( !prop.isValid() || !prop.toBool() ) // No toggable views in view mode
      {
          KRadioAction *action = new KRadioAction( (*it)->comment(), 0, m_actions, (*it)->name() );

          QDomElement e = m_doc.createElement( "Action" );
          m_menuElement.appendChild( e );
          e.setAttribute( "name", (*it)->name() );

          if ( (*it)->name() == m_mainView->currentChildView()->service()->name() )
              action->setChecked( true );

          action->setExclusiveGroup( "KonqMainView_ViewModes" );

          connect( action, SIGNAL( toggled( bool ) ),
                   m_mainView, SLOT( slotViewModeToggle( bool ) ) );
      }
  }
}

static const char *openWithGUI = ""
"<!DOCTYPE kpartgui>"
"<kpartgui name=\"openwith\">"
"<MenuBar>"
" <Menu name=\"edit\">"
" </Menu>"
"</MenuBar>"
"</kpartgui>";

OpenWithGUIClient::OpenWithGUIClient( KonqMainView *mainView )
 : QObject( mainView )
{
  m_mainView = mainView;
  m_doc.setContent( QString::fromLatin1( openWithGUI ) );
  m_menuElement = m_doc.documentElement().namedItem( "MenuBar" ).namedItem( "Menu" ).toElement();
  m_actions = 0L;
  setDocument( m_doc );
}

KAction *OpenWithGUIClient::action( const QDomElement &element ) const
{
  if ( !m_actions )
    return 0L;

  return m_actions->action( element.attribute( "name" ) );
}

void OpenWithGUIClient::update( const KTrader::OfferList &services )
{
  static QString openWithText = i18n( "Open With" ).append( ' ' );
  if ( m_actions )
    delete m_actions;

  m_actions = new KActionCollection( this );

  QDomNode n = m_menuElement.firstChild();
  while ( !n.isNull() )
  {
    m_menuElement.removeChild( n );
    n = m_menuElement.firstChild();
  }

  KTrader::OfferList::ConstIterator it = services.begin();
  KTrader::OfferList::ConstIterator end = services.end();
  for (; it != end; ++it )
  {
    KAction *action = new KAction( (*it)->comment().prepend( openWithText ), 0, m_actions, (*it)->name() );
    action->setIcon( (*it)->icon() );

    QDomElement e = m_doc.createElement( "Action" );
    m_menuElement.appendChild( e );
    e.setAttribute( "name", (*it)->name() );

    connect( action, SIGNAL( activated() ),
	     m_mainView, SLOT( slotOpenWith() ) );
  }

  m_menuElement.appendChild( m_doc.createElement( "Separator" ) );
}

PopupMenuGUIClient::PopupMenuGUIClient( KonqMainView *mainView, const KTrader::OfferList &embeddingServices )
{
  m_mainView = mainView;

  m_doc = QDomDocument( "kpartgui" );
  QDomElement root = m_doc.createElement( "kpartgui" );
  root.setAttribute( "name", "konqueror" );
  m_doc.appendChild( root );

  QDomElement menu = m_doc.createElement( "Menu" );
  root.appendChild( menu );
  menu.setAttribute( "name", "popupmenu" );

  if ( !mainView->menuBar()->isVisible() )
  {
    QDomElement showMenuBarElement = m_doc.createElement( "action" );
    showMenuBarElement.setAttribute( "name", "showmenubar" );
    menu.appendChild( showMenuBarElement );

    menu.appendChild( m_doc.createElement( "separator" ) );
  }

  if ( mainView->fullScreenMode() )
  {
    QDomElement stopFullScreenElement = m_doc.createElement( "action" );
    stopFullScreenElement.setAttribute( "name", "fullscreen" );
    menu.appendChild( stopFullScreenElement );

    menu.appendChild( m_doc.createElement( "separator" ) );
  }

  QString currentServiceName = mainView->currentChildView()->service()->name();

  KTrader::OfferList::ConstIterator it = embeddingServices.begin();
  KTrader::OfferList::ConstIterator end = embeddingServices.end();

  QVariant builtin;
  if ( embeddingServices.count() == 1 )
  {
    KService::Ptr service = *embeddingServices.begin();
    builtin = service->property( "X-KDE-BrowserView-HideFromMenus" );
    if ( ( !builtin.isValid() || !builtin.toBool() ) &&
	 service->name() != currentServiceName )
      addEmbeddingService( menu, 0, i18n( "Preview in %1" ).arg( service->comment() ), service );
  }
  else if ( embeddingServices.count() > 1 )
  {
    int idx = 0;
    QDomElement subMenu = m_doc.createElement( "menu" );
    menu.appendChild( subMenu );
    QDomElement text = m_doc.createElement( "text" );
    subMenu.appendChild( text );
    text.appendChild( m_doc.createTextNode( i18n( "Preview in" ) ) );
    subMenu.setAttribute( "group", "preview" );

    bool inserted = false;

    for (; it != end; ++it )
    {
      builtin = (*it)->property( "X-KDE-BrowserView-HideFromMenus" );
      if ( ( !builtin.isValid() || !builtin.toBool() ) &&
       (*it)->name() != currentServiceName )
      {
        addEmbeddingService( subMenu, idx++, (*it)->comment(), *it );
	inserted = true;
      }
    }

    if ( !inserted ) // oops, if empty then remove the menu :-]
      menu.removeChild( menu.namedItem( "menu" ) );
  }

  setDocument( m_doc );
}

PopupMenuGUIClient::~PopupMenuGUIClient()
{
}

KAction *PopupMenuGUIClient::action( const QDomElement &element ) const
{
  KAction *res = KXMLGUIClient::action( element );

  if ( !res )
    res = m_mainView->action( element );

  return res;
}

void PopupMenuGUIClient::addEmbeddingService( QDomElement &menu, int idx, const QString &name, const KService::Ptr &service )
{
  QDomElement action = m_doc.createElement( "action" );
  menu.appendChild( action );

  QCString actName;
  actName.setNum( idx );

  action.setAttribute( "name", QString::number( idx ) );

  action.setAttribute( "group", "preview" );

  (void)new KAction( name, service->pixmap( KIcon::Small ), 0,
		     m_mainView, SLOT( slotOpenEmbedded() ), actionCollection(), actName );
}

static const char *toggleViewGUI = ""
"<!DOCTYPE toggleviewxml>"
"<toggleviewxml name=\"toggleview\">"
"<MenuBar>"
" <Menu name=\"settings\">"
" </Menu>"
"</MenuBar>"
"</toggleviewxml>";

ToggleViewGUIClient::ToggleViewGUIClient( KonqMainView *mainView )
: QObject( mainView )
{
  m_mainView = mainView;
  m_doc.setContent( QString::fromLatin1( toggleViewGUI ) );
  QDomElement menuElement = m_doc.documentElement().namedItem( "MenuBar" ).namedItem( "Menu" ).toElement();
  setDocument( m_doc );

  KTrader::OfferList offers = KTrader::self()->query( "Browser/View" );
  KTrader::OfferList::Iterator it = offers.begin();
  while ( it != offers.end() )
  {
    QVariant prop = (*it)->property( "X-KDE-BrowserView-Toggable" );
    QVariant orientation = (*it)->property( "X-KDE-BrowserView-ToggableView-Orientation" );

    if ( !prop.isValid() || !prop.toBool() ||
	 !orientation.isValid() || orientation.toString().isEmpty() )
    {
      offers.remove( it );
      it = offers.begin();
    }
    else
      ++it;
  }

  m_empty = ( offers.count() == 0 );

  if ( m_empty )
    return;

  KTrader::OfferList::ConstIterator cIt = offers.begin();
  KTrader::OfferList::ConstIterator cEnd = offers.end();
  for (; cIt != cEnd; ++cIt )
  {
    QString description = i18n( "Show %1" ).arg( (*cIt)->comment() );
    QString name = (*cIt)->name();
    //kdDebug(1202) << "ToggleViewGUIClient: name=" << name << endl;
    KToggleAction *action = new KToggleAction( description, 0, actionCollection(), name.latin1() );

    // HACK
    if ( (*cIt)->icon() != "unknown" )
      action->setIcon( (*cIt)->icon() );

    connect( action, SIGNAL( toggled( bool ) ),
	     this, SLOT( slotToggleView( bool ) ) );

    QDomElement e = m_doc.createElement( "Action" );
    menuElement.appendChild( e );
    e.setAttribute( "name", name );
    QVariant orientation = (*cIt)->property( "X-KDE-BrowserView-ToggableView-Orientation" );
    bool horizontal = orientation.toString().lower() == "horizontal";
    m_mapOrientation.insert( name, horizontal );
  }

  connect( m_mainView, SIGNAL( viewAdded( KonqChildView * ) ),
	   this, SLOT( slotViewAdded( KonqChildView * ) ) );
  connect( m_mainView, SIGNAL( viewRemoved( KonqChildView * ) ),
	   this, SLOT( slotViewRemoved( KonqChildView * ) ) );
}

ToggleViewGUIClient::~ToggleViewGUIClient()
{
}

void ToggleViewGUIClient::slotToggleView( bool toggle )
{
  QString serviceName = QString::fromLatin1( sender()->name() );

  bool horizontal = m_mapOrientation[ serviceName ];

  KonqViewManager *viewManager = m_mainView->viewManager();

  KonqFrameContainer *mainContainer = viewManager->mainContainer();

  if ( toggle )
  {
    // This should be probably merged with KonqViewManager::splitWindow

    KonqFrameBase *splitFrame = mainContainer ? mainContainer->firstChild() : 0L;

    KonqFrameContainer *newContainer;

    KonqChildView *childView = viewManager->split( splitFrame, horizontal ? Qt::Vertical : Qt::Horizontal,
                                                   QString::fromLatin1( "Browser/View" ), serviceName, &newContainer );

    if ( !horizontal )
    {
      if (!splitFrame)
        kdWarning(1202) << "No split frame !" << endl;
      else
      {
        //kdDebug(1202) << "Swapping" << endl;
        newContainer->moveToLast( splitFrame->widget() );
        newContainer->swapChildren();
      }
    }

    QValueList<int> newSplitterSizes;

    if ( horizontal )
      newSplitterSizes << 100 << 30;
    else
      newSplitterSizes << 30 << 100;

    newContainer->setSizes( newSplitterSizes );

    if ( m_mainView->currentChildView() )
    {
        childView->setLocationBarURL( m_mainView->currentChildView()->url().url() ); // default one in case it doesn't set it
        childView->openURL( m_mainView->currentChildView()->url() );
    }

    // If not passive, set as active :)
    if (!childView->passiveMode())
      //viewManager->setActivePart( view );
      childView->view()->widget()->setFocus();

  }
  else
  {
    QList<KonqChildView> viewList;

    mainContainer->listViews( &viewList );

    QListIterator<KonqChildView> it( viewList );
    for (; it.current(); ++it )
      if ( it.current()->service()->name() == serviceName )
        // takes care of choosing the new active view
        viewManager->removeView( it.current() );
  }

  // The current approach is : save this setting as soon as it is changed
  // (This obeys to "no 'Save settings' menu item approach in the Style Guide")
  // I'm on the safe side, this way: whoever doesn't agree has to discuss
  // with the style guide authors, not with me ;-)     (David)
  KConfig *config = KonqFactory::instance()->config();
  KConfigGroupSaver cgs( config, "MainView Settings" );
  QStringList toggableViewsShown = config->readListEntry( "ToggableViewsShown" );
  if (toggle)
  {
      if ( !toggableViewsShown.contains( serviceName ) )
          toggableViewsShown.append(serviceName);
  }
  else
      toggableViewsShown.remove(serviceName);
  config->writeEntry( "ToggableViewsShown", toggableViewsShown );
}

void ToggleViewGUIClient::slotViewAdded( KonqChildView *view )
{
  QString name = view->service()->name();

  KAction *action = actionCollection()->action( name );

  if ( action )
    static_cast<KToggleAction *>( action )->setChecked( true );
}

void ToggleViewGUIClient::slotViewRemoved( KonqChildView *view )
{
  QString name = view->service()->name();

  KAction *action = actionCollection()->action( name );

  if ( action )
    static_cast<KToggleAction *>( action )->setChecked( false );
}

static const char *fullScreenGUI = ""
"<!DOCTYPE fullscreenxml>"
"<fullscreenxml name=\"fullScreen\">"
"<ToolBar name=\"mainToolBar\">"
" <Action name=\"fullscreen\"/>"
"</ToolBar>"
"</fullscreenxml>";

FullScreenGUIClient::FullScreenGUIClient( KonqMainView * mainView )
{
  QDomDocument doc;
  doc.setContent( QString::fromLatin1( fullScreenGUI ) );
  setDocument( doc );
  KAction * act = mainView->actionCollection()->action( "fullscreen" );
  actionCollection()->insert( act );
}

#include "konq_guiclients.moc"
