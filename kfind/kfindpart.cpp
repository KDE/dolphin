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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kfind.h"
#include "kfindpart.h"
#include "kquery.h"

#include <kparts/genericfactory.h>
#include <kdebug.h>
#include <klocale.h>

#include <qdir.h>
#include <kinstance.h>

class KonqDirPart;

typedef KParts::GenericFactory<KFindPart> KFindFactory;
K_EXPORT_COMPONENT_FACTORY( libkfindpart, KFindFactory );

KFindPart::KFindPart( QWidget * parentWidget, const char *widgetName, 
	              QObject *parent, const char *name ,
		      const QStringList & /*args*/ )
    : KonqDirPart (parent, name )/*KParts::ReadOnlyPart*/
{
    setInstance( KFindFactory::instance() );

    setBrowserExtension( new KFindPartBrowserExtension(this) );

    kdDebug() << "KFindPart::KFindPart " << this << endl;
    m_kfindWidget = new Kfind( parentWidget, widgetName );
    m_kfindWidget->setMaximumHeight(m_kfindWidget->minimumSizeHint().height());
    const KFileItem *item = ((KonqDirPart*)parent)->currentItem();
    kdDebug() << "Kfind: currentItem:  " << ( item ? item->url().path().local8Bit() : QString("null") ) << endl;
    QDir d;
  	if( item && d.exists( item->url().path() ))
	  	m_kfindWidget->setURL( item->url() );

    setWidget( m_kfindWidget );

    connect( m_kfindWidget, SIGNAL(started()),
             this, SLOT(slotStarted()) );
    connect( m_kfindWidget, SIGNAL(destroyMe()),
             this, SLOT(slotDestroyMe()) );

    //setXMLFile( "kfind.rc" );
    query = new KQuery(this);
    connect(query, SIGNAL(addFile(const KFileItem *, const QString&)),
            SLOT(addFile(const KFileItem *, const QString&)));
    connect(query, SIGNAL(result(int)),
            SLOT(slotResult(int)));

    m_kfindWidget->setQuery(query);
    m_bShowsResult = false;

    m_lstFileItems.setAutoDelete( true );
}

KFindPart::~KFindPart()
{
}

KAboutData *KFindPart::createAboutData()
{
    return new KAboutData( "kfindpart", I18N_NOOP( "Find Component" ), "1.0" );
}

bool KFindPart::openURL( const KURL &url )
{
    m_kfindWidget->setURL( url );
    return true;
}

void KFindPart::slotStarted()
{
    kdDebug() << "KFindPart::slotStarted" << endl;
    m_bShowsResult = true;
    m_lstFileItems.clear(); // clear our internal list
    emit started();
    emit clear();
}

void KFindPart::addFile(const KFileItem *item, const QString& matchingLine)
{
    m_lstFileItems.append( item );

    KFileItemList lstNewItems;
    lstNewItems.append(item);
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

  m_kfindWidget->searchFinished();
}

void KFindPart::slotDestroyMe()
{
  m_kfindWidget->stopSearch();
  emit clear(); // this is necessary to clear the delayed-mimetypes items list
  m_lstFileItems.clear(); // clear our internal list
  emit findClosed();
}

void KFindPart::saveKFindState( QDataStream *stream )
{
  m_kfindWidget->saveState( stream );
  //Now we'll save the search result
  KFileItem *fileitem=m_lstFileItems.first();
  *stream << m_lstFileItems.count();
  while(fileitem!=NULL)
  {
        *stream << fileitem->url();
        fileitem=m_lstFileItems.next();
  }
}

void KFindPart::restoreKFindState( QDataStream *stream )
{
  int nbitems;
  int i;
  KURL itemUrl;

  m_kfindWidget->restoreState( stream );

  *stream >> nbitems;
  slotStarted();
  for(i=0;i<nbitems;i++)
  {
    *stream >> itemUrl;
    addFile(new KFileItem(itemUrl,"",0),"");
  }
    emit finished();
}

KFindPartBrowserExtension::KFindPartBrowserExtension( KFindPart *findPart )
 : KParts::BrowserExtension( findPart )
{
  m_findPart = findPart;
  m_bSaveViewPropertiesLocally = false;
}

#include "kfindpart.moc"
