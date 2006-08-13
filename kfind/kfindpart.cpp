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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "kfind.h"
#include "kfindpart.h"
#include "kquery.h"

#include <kparts/genericfactory.h>
#include <kdebug.h>
#include <klocale.h>
#include <kfileitem.h>
#include <kdirlister.h>
#include <kinstance.h>

#include <QDir>

class KonqDirPart;

typedef KParts::GenericFactory<KFindPart> KFindFactory;
K_EXPORT_COMPONENT_FACTORY( libkfindpart, KFindFactory )

KFindPart::KFindPart( QWidget * parentWidget, QObject *parent, const QStringList & /*args*/ )
    : KonqDirPart (parent)/*KParts::ReadOnlyPart*/
{
    setInstance( KFindFactory::instance() );

    setBrowserExtension( new KonqDirPartBrowserExtension( this ) );

    kDebug() << "KFindPart::KFindPart " << this << endl;
    m_kfindWidget = new Kfind( parentWidget );
    m_kfindWidget->setMaximumHeight(m_kfindWidget->minimumSizeHint().height());
    const KFileItem *item = ((KonqDirPart*)parent)->currentItem();
    kDebug() << "Kfind: currentItem:  " << ( item ? item->url().path().toLocal8Bit() : QString("null") ) << endl;
    QDir d;
    if( item && d.exists( item->url().path() ))
        m_kfindWidget->setURL( item->url() );

    setWidget( m_kfindWidget );

    connect( m_kfindWidget, SIGNAL(started()),
             this, SLOT(slotStarted()) );
    connect( m_kfindWidget, SIGNAL(destroyMe()),
             this, SLOT(slotDestroyMe()) );
    connect(m_kfindWidget->dirlister,SIGNAL(deleteItem(KFileItem*)), this, SLOT(removeFile(KFileItem*)));
    connect(m_kfindWidget->dirlister,SIGNAL(newItems(const KFileItemList&)), this, SLOT(newFiles(const KFileItemList&)));
    //setXMLFile( "kfind.rc" );
    query = new KQuery(this);
    connect(query, SIGNAL(addFile(const KFileItem *, const QString&)),
            SLOT(addFile(const KFileItem *, const QString&)));
    connect(query, SIGNAL(result(int)),
            SLOT(slotResult(int)));

    m_kfindWidget->setQuery(query);
    m_bShowsResult = false;
}

KFindPart::~KFindPart()
{
    qDeleteAll(m_lstFileItems);
    m_lstFileItems.clear();
}

KAboutData *KFindPart::createAboutData()
{
    return new KAboutData( "kfindpart", I18N_NOOP( "Find Component" ), "1.0" );
}

bool KFindPart::doOpenURL( const KUrl &url )
{
    m_kfindWidget->setURL( url );
    return true;
}

void KFindPart::slotStarted()
{
    kDebug() << "KFindPart::slotStarted" << endl;
    m_bShowsResult = true;
    m_lstFileItems.clear(); // clear our internal list
    emit started();
    emit clear();
}

void KFindPart::addFile(const KFileItem *item, const QString& /*matchingLine*/)
{
    // item is deleted by caller
    // we need to clone it
    KFileItem *clonedItem = new KFileItem(*item);
    m_lstFileItems.append( clonedItem );

    KFileItemList lstNewItems;
    lstNewItems.append(clonedItem);
    emit newItems(lstNewItems);
   
  /*
  win->insertItem(item);

  if (!isResultReported)
  {
    emit haveResults(true);
    isResultReported = true;
  }

  */
}

/* An item has been removed, so update konqueror's view */
void KFindPart::removeFile(KFileItem *item)
{
  KFileItem *iter;
  KFileItemList listiter;

  emit started();
  emit clear();

  m_lstFileItems.removeAll( item );  //not working ?

  foreach(iter, m_lstFileItems) {
    if(iter->url()!=item->url())
      listiter.append(iter);
  }
  
  emit newItems(listiter);
  emit finished();
}

void KFindPart::newFiles(const KFileItemList&)
{
  if(m_bShowsResult)
    return;
  emit started();
  emit clear();
  emit newItems(m_lstFileItems);
  emit finished();
}

void KFindPart::slotResult(int errorCode)
{
  if (errorCode == 0)
    emit finished();
    //setStatusMsg(i18n("Ready."));
  else if (errorCode == KIO::ERR_USER_CANCELED)
    emit canceled();
    //setStatusMsg(i18n("Aborted."));
  else
    emit canceled(); // TODO ?
    //setStatusMsg(i18n("Error."));
  m_bShowsResult=false;
  m_kfindWidget->searchFinished();
}

void KFindPart::slotDestroyMe()
{
  m_kfindWidget->stopSearch();
  emit clear(); // this is necessary to clear the delayed-mimetypes items list
  m_lstFileItems.clear(); // clear our internal list
  emit findClosed();
}

void KFindPart::saveState( QDataStream& stream )
{
  KonqDirPart::saveState(stream); 

  m_kfindWidget->saveState( &stream );
  //Now we'll save the search result
  stream << m_lstFileItems.count();
  foreach(KFileItem *fileitem, m_lstFileItems)
  {
        stream << *fileitem;
  }
}

void KFindPart::restoreState( QDataStream& stream )
{
  KonqDirPart::restoreState(stream); 
  int nbitems;
  KUrl itemUrl;

  m_kfindWidget->restoreState( &stream );

  stream >> nbitems;
  slotStarted();
  for(int i=0;i<nbitems;i++)
  {
    KFileItem* item = new KFileItem( KFileItem::Unknown, KFileItem::Unknown, KUrl() );
    stream >> *item;
    m_lstFileItems.append(item);
  }
  if (nbitems)
    emit newItems(m_lstFileItems);

  emit finished();
}

#include "kfindpart.moc"
