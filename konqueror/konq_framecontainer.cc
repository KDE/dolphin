/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

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
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "konq_framecontainer.h"
#include <kdebug.h>
#include <kglobalsettings.h>
#include <kconfig.h>
#include <math.h> // pow()

//###################################################################

void KonqFrameContainerBase::printFrameInfo(const QString& spaces)
{
  kDebug(1202) << spaces << "KonqFrameContainerBase " << this << ", this shouldn't happen!" << endl;
}

//###################################################################

KonqFrameContainer::KonqFrameContainer( Qt::Orientation o,
                                        QWidget* parent,
                                        KonqFrameContainerBase* parentContainer )
  : QSplitter( o, parent ), m_bAboutToBeDeleted(false)
{
  m_pParentContainer = parentContainer;
  m_pFirstChild = 0L;
  m_pSecondChild = 0L;
  m_pActiveChild = 0L;
  setOpaqueResize( KGlobalSettings::opaqueResize() );
  connect(this, SIGNAL(splitterMoved(int, int)), this, SIGNAL(setRubberbandCalled()));
//### CHECKME
}

KonqFrameContainer::~KonqFrameContainer()
{
    //kDebug(1202) << "KonqFrameContainer::~KonqFrameContainer() " << this << " - " << className() << endl;
	delete m_pFirstChild;
	delete m_pSecondChild;
}

void KonqFrameContainer::listViews( ChildViewList *viewList )
{
   if( m_pFirstChild )
      m_pFirstChild->listViews( viewList );

   if( m_pSecondChild )
      m_pSecondChild->listViews( viewList );
}

void KonqFrameContainer::saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id, int depth )
{
  int idSecond = id + (int)pow( 2.0, depth );

  //write children sizes
  config->writeEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ), sizes() );

  //write children
  QStringList strlst;
  if( firstChild() )
    strlst.append( QString::fromLatin1( firstChild()->frameType() ) + QString::number(idSecond - 1) );
  if( secondChild() )
    strlst.append( QString::fromLatin1( secondChild()->frameType() ) + QString::number( idSecond ) );

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  //write orientation
  QString o;
  if( orientation() == Qt::Horizontal )
    o = QString::fromLatin1("Horizontal");
  else if( orientation() == Qt::Vertical )
    o = QString::fromLatin1("Vertical");
  config->writeEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ), o );

  //write docContainer
  if (this == docContainer) config->writeEntry( QString::fromLatin1( "docContainer" ).prepend( prefix ), true );

  if (m_pSecondChild == m_pActiveChild) config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), 1 );
  else config->writeEntry( QString::fromLatin1( "activeChildIndex" ).prepend( prefix ), 0 );

  //write child configs
  if( firstChild() ) {
    QString newPrefix = QString::fromLatin1( firstChild()->frameType() ) + QString::number(idSecond - 1);
    newPrefix.append( QLatin1Char( '_' ) );
    firstChild()->saveConfig( config, newPrefix, saveURLs, docContainer, id, depth + 1 );
  }

  if( secondChild() ) {
    QString newPrefix = QString::fromLatin1( secondChild()->frameType() ) + QString::number( idSecond );
    newPrefix.append( QLatin1Char( '_' ) );
    secondChild()->saveConfig( config, newPrefix, saveURLs, docContainer, idSecond, depth + 1 );
  }
}

void KonqFrameContainer::copyHistory( KonqFrameBase *other )
{
    Q_ASSERT( other->frameType() == "Container" );
    if ( firstChild() )
        firstChild()->copyHistory( static_cast<KonqFrameContainer *>( other )->firstChild() );
    if ( secondChild() )
        secondChild()->copyHistory( static_cast<KonqFrameContainer *>( other )->secondChild() );
}

KonqFrameBase* KonqFrameContainer::otherChild( KonqFrameBase* child )
{
   if( firstChild() == child )
      return secondChild();
   else if( secondChild() == child )
      return firstChild();
   return 0L;
}

void KonqFrameContainer::printFrameInfo( const QString& spaces )
{
        kDebug(1202) << spaces << "KonqFrameContainer " << this << " visible=" << QString("%1").arg(isVisible())
                      << " activeChild=" << m_pActiveChild << endl;
        if (!m_pActiveChild)
            kDebug(1202) << "WARNING: " << this << " has a null active child!" << endl;
        KonqFrameBase* child = firstChild();
        if (child != 0L)
            child->printFrameInfo(spaces + "  ");
        else
            kDebug(1202) << spaces << "  Null child" << endl;
        child = secondChild();
        if (child != 0L)
            child->printFrameInfo(spaces + "  ");
        else
            kDebug(1202) << spaces << "  Null child" << endl;
}

void KonqFrameContainer::reparentFrame( QWidget* parent, const QPoint & p )
{
    setParent( parent );
    move( p );
}

void KonqFrameContainer::swapChildren()
{
  KonqFrameBase *firstCh = m_pFirstChild;
  m_pFirstChild = m_pSecondChild;
  m_pSecondChild = firstCh;
}

void KonqFrameContainer::setTitle( const QString &title , QWidget* sender)
{
  //kDebug(1202) << "KonqFrameContainer::setTitle( " << title << " , " << sender << " )" << endl;
  if (m_pParentContainer && activeChild() && (sender == activeChild()->asQWidget()))
      m_pParentContainer->setTitle( title , this);
}

void KonqFrameContainer::setTabIcon( const KUrl &url, QWidget* sender )
{
  //kDebug(1202) << "KonqFrameContainer::setTabIcon( " << url << " , " << sender << " )" << endl;
  if (m_pParentContainer && activeChild() && (sender == activeChild()->asQWidget()))
      m_pParentContainer->setTabIcon( url, this );
}

void KonqFrameContainer::insertChildFrame( KonqFrameBase* frame, int /*index*/  )
{
  //kDebug(1202) << "KonqFrameContainer " << this << ": insertChildFrame " << frame << endl;

  if (frame)
  {
      if( !m_pFirstChild )
      {
          m_pFirstChild = frame;
          frame->setParentContainer(this);
          //kDebug(1202) << "Setting as first child" << endl;
      }
      else if( !m_pSecondChild )
      {
          m_pSecondChild = frame;
          frame->setParentContainer(this);
          //kDebug(1202) << "Setting as second child" << endl;
      }
      else
        kWarning(1202) << this << " already has two children..."
                        << m_pFirstChild << " and " << m_pSecondChild << endl;
  } else
    kWarning(1202) << "KonqFrameContainer " << this << ": insertChildFrame(0L) !" << endl;
}

void KonqFrameContainer::removeChildFrame( KonqFrameBase * frame )
{
  //kDebug(1202) << "KonqFrameContainer::RemoveChildFrame " << this << ". Child " << frame << " removed" << endl;

  if( m_pFirstChild == frame )
  {
    m_pFirstChild = m_pSecondChild;
    m_pSecondChild = 0L;
  }
  else if( m_pSecondChild == frame )
    m_pSecondChild = 0L;

  else
    kWarning(1202) << this << " Can't find this child:" << frame << endl;
}

void KonqFrameContainer::childEvent( QChildEvent *c )
{
  // Child events cause layout changes. These are unnecassery if we are going
  // to be deleted anyway.
  if (!m_bAboutToBeDeleted)
      QSplitter::childEvent(c);
}

#include "konq_framecontainer.moc"
