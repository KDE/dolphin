/*  This file is part of the KDE project

    Copyright (C) 2002-2003 Konqueror Developers
                  2002-2003 Douglas Hanley <douglash@caltech.edu>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include "konq_tabs.h"

#include <qptrlist.h>
#include <qpopupmenu.h>

#include <kdebug.h>
#include <kiconloader.h>
#include <klocale.h>

#include "konq_frame.h"
#include "konq_view.h"
#include "konq_viewmgr.h"

#include <konq_pixmapprovider.h>
#include <kstdaccel.h>

//###################################################################

KonqFrameTabs::KonqFrameTabs(QWidget* parent, KonqFrameContainerBase* parentContainer, KonqViewManager* viewManager, const char * name)
  : KTabWidget(parent, name)
{
  //kdDebug(1202) << "KonqFrameTabs::KonqFrameTabs()" << endl;

  m_pParentContainer = parentContainer;
  m_pChildFrameList = new QPtrList<KonqFrameBase>;
  m_pChildFrameList->setAutoDelete(false);
  m_pActiveChild = 0L;
  m_pViewManager = viewManager;

  connect( this, SIGNAL( currentChanged ( QWidget * ) ), this, SLOT( slotCurrentChanged( QWidget* ) ) );

  m_pPopupMenu = new QPopupMenu( this );
  m_pPopupMenu->insertItem( SmallIcon( "tab_new" ), i18n("&New Tab"), m_pViewManager->mainWindow(), SLOT( slotAddTab() ), QKeySequence("Ctrl+Shift+N") );
  m_pPopupMenu->insertItem( SmallIcon( "tab_duplicate" ), i18n("&Duplicate Tab"), m_pViewManager->mainWindow(), SLOT( slotDuplicateTabPopup() ), QKeySequence("Ctrl+Shift+D") );
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "tab_breakoff" ), i18n("D&etach Tab"), m_pViewManager->mainWindow(), SLOT( slotBreakOffTabPopup() ), QKeySequence("Ctrl+Shift+B") );
  m_pPopupMenu->insertItem( SmallIcon( "tab_remove" ), i18n("&Close Tab"), m_pViewManager->mainWindow(), SLOT( slotRemoveTabPopup() ), QKeySequence("Ctrl+W") );
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "reload" ), i18n( "&Reload" ), m_pViewManager->mainWindow(), SLOT( slotReload() ), KStdAccel::key(KStdAccel::Reload) );
  m_pPopupMenu->insertItem( SmallIcon( "reload_all_tab" ), i18n( "&Reload All Tabs" ), m_pViewManager->mainWindow(), SLOT( slotReloadAllTabs() ));
  m_pPopupMenu->insertSeparator();
  m_pPopupMenu->insertItem( SmallIcon( "tab_remove" ), i18n("Close &Other Tabs"), m_pViewManager->mainWindow(), SLOT( slotRemoveOtherTabsPopup() ) );
  connect( this, SIGNAL( contextMenu( QWidget *, const QPoint & )), SLOT(slotContextMenu( QWidget *, const QPoint & )));

  KConfig *config = KGlobal::config();
  KConfigGroupSaver cs( config, QString::fromLatin1("FMSettings") );
  setHoverCloseButton( config->readBoolEntry( "HoverCloseButton", true ) );
  connect( this, SIGNAL( closeRequest( QWidget * )), SLOT(slotCloseRequest( QWidget * )));
}

KonqFrameTabs::~KonqFrameTabs()
{
  //kdDebug(1202) << "KonqFrameTabs::~KonqFrameTabs() " << this << " - " << className() << endl;
  m_pChildFrameList->setAutoDelete(true);
  delete m_pChildFrameList;
}

void KonqFrameTabs::listViews( ChildViewList *viewList ) {
  for( QPtrListIterator<KonqFrameBase> it( *m_pChildFrameList ); *it; ++it )
    it.current()->listViews(viewList);
}

void KonqFrameTabs::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id, int depth )
{
  //write children
  QStringList strlst;
  int i = 0;
  QString newPrefix;
  for (KonqFrameBase* it = m_pChildFrameList->first(); it; it = m_pChildFrameList->next())
    {
      newPrefix = QString::fromLatin1( it->frameType() ) + "T" + QString::number(i);
      strlst.append( newPrefix );
      newPrefix.append( '_' );
      it->saveConfig( config, newPrefix, saveURLs, docContainer, id, depth);
      i++;
    }

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), currentPageIndex() );
}

void KonqFrameTabs::copyHistory( KonqFrameBase *other )
{
  if( other->frameType() != "Tabs" ) {
    kdDebug(1202) << "Frame types are not the same" << endl;
    return;
  }

  for (uint i = 0; i < m_pChildFrameList->count(); i++ )
    {
      m_pChildFrameList->at(i)->copyHistory( static_cast<KonqFrameTabs *>( other )->m_pChildFrameList->at(i) );
    }
}

void KonqFrameTabs::printFrameInfo( const QString& spaces )
{
  kdDebug(1202) << spaces << "KonqFrameTabs " << this << " visible=" << QString("%1").arg(isVisible())
                << " activeChild=" << m_pActiveChild << endl;
  if (!m_pActiveChild) kdDebug(1202) << "WARNING: " << this << " has a null active child!" << endl;
  KonqFrameBase* child;
  int childFrameCount = m_pChildFrameList->count();
  for (int i = 0 ; i < childFrameCount ; i++) {
    child = m_pChildFrameList->at(i);
    if (child != 0L) child->printFrameInfo(spaces + "  ");
    else kdDebug(1202) << spaces << "  Null child" << endl;
  }
}

void KonqFrameTabs::reparentFrame( QWidget* parent, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, p, showIt );
}

void KonqFrameTabs::setTitle( const QString &title , QWidget* sender)
{
  // kdDebug(1202) << "KonqFrameTabs::setTitle( " << title << " , " << sender << " )" << endl;
  QString newTitle = title;
  newTitle.replace('&', "&&");

  removeTabToolTip( sender );
  if (newTitle.length() > 30)
    {
      setTabToolTip( sender, newTitle );
      newTitle = newTitle.left(27) + "...";
    }
  if (tabLabel( sender ) != newTitle)
    changeTab( sender , newTitle );
}

void KonqFrameTabs::setTabIcon( const QString &url, QWidget* sender )
{
  //kdDebug(1202) << "KonqFrameTabs::setTabIcon( " << url << " , " << sender << " )" << endl;
  QIconSet iconSet = QIconSet( KonqPixmapProvider::self()->pixmapFor( url ) );
  if (tabIconSet( sender ).pixmap().serialNumber() != iconSet.pixmap().serialNumber())
    setTabIconSet( sender, iconSet );
}

void KonqFrameTabs::activateChild()
{
  if (m_pActiveChild)
    {
      showPage( m_pActiveChild->widget() );
      m_pActiveChild->activateChild();
    }
}

void KonqFrameTabs::insertChildFrame( KonqFrameBase* frame, int index )
{
  //kdDebug(1202) << "KonqFrameTabs " << this << ": insertChildFrame " << frame << endl;

  if (frame)
    {
      //kdDebug(1202) << "Adding frame" << endl;
      insertTab(frame->widget(),"", index);
      frame->setParentContainer(this);
      if (index == -1) m_pChildFrameList->append(frame);
      else m_pChildFrameList->insert(index, frame);
      KonqView* activeChildView = frame->activeChildView();
      if (activeChildView != 0L) {
        activeChildView->setCaption( activeChildView->caption() );
        activeChildView->setTabIcon( activeChildView->url().url() );
      }
    }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": insertChildFrame(0L) !" << endl;
}

void KonqFrameTabs::removeChildFrame( KonqFrameBase * frame )
{
  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;
  if (frame) {
    removePage(frame->widget());
    m_pChildFrameList->remove(frame);
  }
  else
    kdWarning(1202) << "KonqFrameTabs " << this << ": removeChildFrame(0L) !" << endl;

  //kdDebug(1202) << "KonqFrameTabs::RemoveChildFrame finished" << endl;
}

void KonqFrameTabs::slotCurrentChanged( QWidget* newPage )
{
  KonqFrameBase* currentFrame = dynamic_cast<KonqFrameBase*>(newPage);

  if (!m_pViewManager->isLoadingProfile()) {
    m_pActiveChild = currentFrame;
    currentFrame->activateChild();
  }
}

void KonqFrameTabs::moveTabLeft(int index)
{
  if ( index == 0 )
    return;
  KonqFrameBase* currentFrame = m_pChildFrameList->at(index );
  kdDebug()<<" currentFrame :"<<currentFrame<<" index :"<<index<<endl;
  removePage(currentFrame->widget());
  m_pChildFrameList->remove(currentFrame);

  insertChildFrame( currentFrame, index-1 );
  setCurrentPage( index-1 );
}

void KonqFrameTabs::moveTabRight(int index)
{
  if ( index == count()-1 )
    return;
  KonqFrameBase* currentFrame = m_pChildFrameList->at(index );
  kdDebug()<<" currentFrame :"<<currentFrame<<" index :"<<index<<endl;
  removePage(currentFrame->widget());
  m_pChildFrameList->remove(currentFrame);

  insertChildFrame( currentFrame, index+1 );
  setCurrentPage( index+1 );
}

void KonqFrameTabs::slotContextMenu( QWidget *w, const QPoint &p )
{
  m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
  m_pPopupMenu->exec( p );
}

void KonqFrameTabs::slotCloseRequest( QWidget *w )
{
  m_pViewManager->mainWindow()->setWorkingTab( dynamic_cast<KonqFrameBase*>(w) );
  m_pViewManager->removeTab();
}

#include "konq_tabs.moc"
