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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include <kdebug.h>
#include <klocale.h>
#include <kmenubar.h>
#include "konq_view.h"
#include "konq_settingsxt.h"
#include "konq_frame.h"
#include "konq_guiclients.h"
#include "konq_viewmgr.h"
#include <kiconloader.h>

PopupMenuGUIClient::PopupMenuGUIClient( KonqMainWindow *mainWindow,
                                        const KTrader::OfferList &embeddingServices,
                                        bool showEmbeddingServices, bool doTabHandling )
{
    //giving a name to each guiclient: just for debugging
    // (needs delete instance() in the dtor if enabled for good)
    //setInstance( new KInstance( "PopupMenuGUIClient" ) );

    m_mainWindow = mainWindow;

    m_doc = QDomDocument( "kpartgui" );
    QDomElement root = m_doc.createElement( "kpartgui" );
    root.setAttribute( "name", "konqueror" );
    m_doc.appendChild( root );

    QDomElement menu = m_doc.createElement( "Menu" );
    root.appendChild( menu );
    menu.setAttribute( "name", "popupmenu" );

    if ( !mainWindow->menuBar()->isVisible() )
    {
        QDomElement showMenuBarElement = m_doc.createElement( "action" );
        showMenuBarElement.setAttribute( "name", "options_show_menubar" );
        menu.appendChild( showMenuBarElement );

        menu.appendChild( m_doc.createElement( "separator" ) );
    }

    if ( mainWindow->fullScreenMode() )
    {
        QDomElement stopFullScreenElement = m_doc.createElement( "action" );
        stopFullScreenElement.setAttribute( "name", "fullscreen" );
        menu.appendChild( stopFullScreenElement );

        menu.appendChild( m_doc.createElement( "separator" ) );
    }

    if ( showEmbeddingServices )
    {
        KTrader::OfferList::ConstIterator it = embeddingServices.begin();
        KTrader::OfferList::ConstIterator end = embeddingServices.end();

        if ( embeddingServices.count() == 1 )
        {
            KService::Ptr service = *embeddingServices.begin();
            addEmbeddingService( menu, 0, i18n( "Preview in %1" ,  service->name() ), service );
        }
        else if ( embeddingServices.count() > 1 )
        {
            int idx = 0;
            QDomElement subMenu = m_doc.createElement( "menu" );
            menu.appendChild( subMenu );
            QDomElement text = m_doc.createElement( "text" );
            subMenu.appendChild( text );
            text.appendChild( m_doc.createTextNode( i18n( "Preview In" ) ) );
            subMenu.setAttribute( "group", "preview" );
            subMenu.setAttribute( "name", "preview submenu" );

            bool inserted = false;

            for (; it != end; ++it, ++idx )
            {
                addEmbeddingService( subMenu, idx, (*it)->name(), *it );
                inserted = true;
            }

            if ( !inserted ) // oops, if empty then remove the menu :-]
                menu.removeChild( menu.namedItem( "menu" ) );
        }
    }

    if ( doTabHandling )
    {
        QDomElement openInSameWindow = m_doc.createElement( "action" );
        openInSameWindow.setAttribute( "name", "sameview" );
        openInSameWindow.setAttribute( "group", "tabhandling" );
        menu.appendChild( openInSameWindow );
        
	QDomElement openInWindow = m_doc.createElement( "action" );
        openInWindow.setAttribute( "name", "newview" );
        openInWindow.setAttribute( "group", "tabhandling" );
        menu.appendChild( openInWindow );

        QDomElement openInTabElement = m_doc.createElement( "action" );
        openInTabElement.setAttribute( "name", "openintab" );
        openInTabElement.setAttribute( "group", "tabhandling" );
        menu.appendChild( openInTabElement );

        QDomElement separatorElement = m_doc.createElement( "separator" );
        separatorElement.setAttribute( "group", "tabhandling" );
        menu.appendChild( separatorElement );
    }

    //kDebug() << k_funcinfo << m_doc.toString() << endl;

    setDOMDocument( m_doc );
}

PopupMenuGUIClient::~PopupMenuGUIClient()
{
}

KAction *PopupMenuGUIClient::action( const QDomElement &element ) const
{
  KAction *res = KXMLGUIClient::action( element );

  if ( !res )
    res = m_mainWindow->action( element );

  return res;
}

void PopupMenuGUIClient::addEmbeddingService( QDomElement &menu, int idx, const QString &name, const KService::Ptr &service )
{
  QDomElement action = m_doc.createElement( "action" );
  menu.appendChild( action );

  QByteArray actName;
  actName.setNum( idx );

  action.setAttribute( "name", QString::number( idx ) );

  action.setAttribute( "group", "preview" );

  (void)new KAction( name, service->pixmap( K3Icon::Small ), 0,
                     m_mainWindow, SLOT( slotOpenEmbedded() ), actionCollection(), actName );
}

ToggleViewGUIClient::ToggleViewGUIClient( KonqMainWindow *mainWindow )
: QObject( mainWindow )
{
  m_mainWindow = mainWindow;

  KTrader::OfferList offers = KTrader::self()->query( "Browser/View" );
  KTrader::OfferList::Iterator it = offers.begin();
  while ( it != offers.end() )
  {
    QVariant prop = (*it)->property( "X-KDE-BrowserView-Toggable" );
    QVariant orientation = (*it)->property( "X-KDE-BrowserView-ToggableView-Orientation" );

    if ( !prop.isValid() || !prop.toBool() ||
         !orientation.isValid() || orientation.toString().isEmpty() )
    {
      offers.erase( it );
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
    QString description = i18n( "Show %1" ,  (*cIt)->name() );
    QString name = (*cIt)->desktopEntryName();
    //kDebug(1202) << "ToggleViewGUIClient: name=" << name << endl;
    KToggleAction *action = new KToggleAction( description, mainWindow->actionCollection(), name.toLatin1() );
    action->setCheckedState( i18n( "Hide %1" ,  (*cIt)->name() ) );

    // HACK
    if ( (*cIt)->icon() != "unknown" )
      action->setIconName( (*cIt)->icon() );

    connect( action, SIGNAL( toggled( bool ) ),
             this, SLOT( slotToggleView( bool ) ) );

    m_actions.insert( name, action );

    QVariant orientation = (*cIt)->property( "X-KDE-BrowserView-ToggableView-Orientation" );
    bool horizontal = orientation.toString().toLower() == "horizontal";
    m_mapOrientation.insert( name, horizontal );
  }

  connect( m_mainWindow, SIGNAL( viewAdded( KonqView * ) ),
           this, SLOT( slotViewAdded( KonqView * ) ) );
  connect( m_mainWindow, SIGNAL( viewRemoved( KonqView * ) ),
           this, SLOT( slotViewRemoved( KonqView * ) ) );
}

ToggleViewGUIClient::~ToggleViewGUIClient()
{
}

QList<KAction*> ToggleViewGUIClient::actions() const
{
  return m_actions.values();
}

void ToggleViewGUIClient::slotToggleView( bool toggle )
{
  QString serviceName = QLatin1String( sender()->name() );

  bool horizontal = m_mapOrientation[ serviceName ];

  KonqViewManager *viewManager = m_mainWindow->viewManager();

  if ( toggle )
  {

    KonqView *childView = viewManager->splitWindow( horizontal ? Qt::Vertical : Qt::Horizontal,
                                                    QLatin1String( "Browser/View" ),
                                                    serviceName,
                                                    !horizontal /* vertical = make it first */);

    QList<int> newSplitterSizes;

    if ( horizontal )
      newSplitterSizes << 100 << 30;
    else
      newSplitterSizes << 30 << 100;

    if (!childView || !childView->frame())
        return;

    // Toggleviews don't need their statusbar
    childView->frame()->statusbar()->hide();

    KonqFrameContainerBase *newContainer = childView->frame()->parentContainer();

    if (newContainer->frameType()=="Container")
      static_cast<KonqFrameContainer*>(newContainer)->setSizes( newSplitterSizes );

#if 0 // already done by splitWindow
    if ( m_mainWindow->currentView() )
    {
        QString locBarURL = m_mainWindow->currentView()->url().prettyUrl(); // default one in case it doesn't set it
        childView->openURL( m_mainWindow->currentView()->url(), locBarURL );
    }
#endif

    // If not passive, set as active :)
    if (!childView->isPassiveMode())
      viewManager->setActivePart( childView->part() );

    kDebug() << "ToggleViewGUIClient::slotToggleView setToggleView(true) on " << childView << endl;
    childView->setToggleView( true );

    m_mainWindow->viewCountChanged();

  }
  else
  {
    QList<KonqView*> viewList;

    m_mainWindow->listViews( &viewList );

    foreach ( KonqView* view, viewList )
      if ( view->service() && view->service()->desktopEntryName() == serviceName )
        // takes care of choosing the new active view, and also calls slotViewRemoved
        viewManager->removeView( view );
  }

}


void ToggleViewGUIClient::saveConfig( bool add, const QString &serviceName )
{
  // This is used on konqueror's startup....... so it's never used, since
  // the K menu only contains calls to kfmclient......
  // Well, it could be useful again in the future.
  // Currently, the profiles save this info.
  QStringList toggableViewsShown = KonqSettings::toggableViewsShown();
  if (add)
  {
      if ( !toggableViewsShown.contains( serviceName ) )
          toggableViewsShown.append(serviceName);
  }
  else
      toggableViewsShown.removeAll(serviceName);
  KonqSettings::setToggableViewsShown( toggableViewsShown );
}

void ToggleViewGUIClient::slotViewAdded( KonqView *view )
{
  QString name = view->service()->desktopEntryName();

  KAction *action = m_actions.value( name );

  if ( action )
  {
    static_cast<KToggleAction *>( action )->setChecked( true );
    saveConfig( true, name );

    // KonqView::isToggleView() is not set yet.. so just check for the orientation

#if 0
    QVariant vert = view->service()->property( "X-KDE-BrowserView-ToggableView-Orientation");
    bool vertical = vert.toString().toLower() == "vertical";
    QVariant nohead = view->service()->property( "X-KDE-BrowserView-ToggableView-NoHeader");
    bool noheader = nohead.isValid() ? nohead.toBool() : false;
    // if it is a vertical toggle part, turn on the header.
    // this works even when konq loads the view from a profile.
    if ( vertical && (!noheader))
    {
        view->frame()->header()->setText(view->service()->name());
        view->frame()->header()->setAction(action);
    }
#endif
  }
}

void ToggleViewGUIClient::slotViewRemoved( KonqView *view )
{
  QString name = view->service()->desktopEntryName();

  KAction *action = m_actions.value( name );

  if ( action )
  {
    static_cast<KToggleAction *>( action )->setChecked( false );
    saveConfig( false, name );
  }
}

#include "konq_guiclients.moc"
