
#include "konq_metadata.h"

#include <qdom.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kinstance.h>
#include <kglobal.h>
#include <kdebug.h>

class KonqMetaDataProviderPrivate
{
public:
  KonqMetaDataProviderPrivate()
  {
    m_metaDataSuffix = QString::fromLatin1( ".kdemetadata" );
  }
  ~KonqMetaDataProviderPrivate()
  {
  }

  QString m_metaDataSuffix;

  KURL m_url;
  KURL m_metaDataURL;
  QDomDocument m_doc;
  bool m_docLoaded;
  KInstance *m_instance;
};

KonqMetaDataProvider::KonqMetaDataProvider( KInstance *instance, QObject *parent, const char *name )
  : QObject( parent, name )
{
  d = new KonqMetaDataProviderPrivate;
  d->m_instance = instance ? instance : KGlobal::instance();
}

KonqMetaDataProvider::~KonqMetaDataProvider()
{
  delete d;
}

bool KonqMetaDataProvider::openDir( const KURL &url )
{
  d->m_doc = QDomDocument();
  d->m_docLoaded = false;

  // temporary ;-)
  if ( !url.isLocalFile() )
    return false;

  d->m_metaDataURL = url;
  d->m_metaDataURL.addPath( d->m_metaDataSuffix );

  d->m_url = url;

  if ( !QFile::exists( d->m_metaDataURL.path() ) )
    return false;

  QFile f( d->m_metaDataURL.path() );
  if ( !f.open( IO_ReadOnly ) )
    return false;

  d->m_doc = QDomDocument( &f );
  d->m_docLoaded = true;

  return true;
}

bool KonqMetaDataProvider::metaData( const KURL &url, const QString &serviceType, const QString &key, QString &val )
{
  if ( url != d->m_url && serviceType == "inode/directory" )
  {
    KonqMetaDataProvider *subProvider = new KonqMetaDataProvider( d->m_instance, this, "submetaprov" );

    if ( !subProvider->openDir( url ) )
    {
      delete subProvider;
      return false;
    }

    if ( !subProvider->metaData( url, QString::null, key, val ) )
    {
      delete subProvider;
      return false;
    }

    delete subProvider;
    return true;
  }

  if ( !d->m_docLoaded )
    return false;

  QDomElement root = d->m_doc.documentElement();

  if ( url == d->m_url )
  {
    val = root.namedItem( key ).toElement().text();
    return true;
  }

  QDomElement child = root.firstChild().toElement();
  for (; !child.isNull(); child = child.nextSibling().toElement() )
  {
    if ( child.tagName() != "item" )
      continue;

    QDomElement urlElement = child.namedItem( "url" ).toElement();
    if ( urlElement.isNull() )
      continue;

    if ( url == urlElement.text() )
    {
      val = child.namedItem( key ).toElement().text();
      return true;
    }
  }

  return false;
}

void KonqMetaDataProvider::saveMetaData( const KURL &url, const QString &serviceType, const QString &key, const QString &val )
{
  if ( url != d->m_url && serviceType == "inode/directory" )
  {
    KonqMetaDataProvider *subProvider = new KonqMetaDataProvider( d->m_instance, this, "submetaprov" );
    
    subProvider->openDir( url );
    subProvider->saveMetaData( url, QString::null, key, val );
    
    delete subProvider;
    return;
  }
  
  if ( !d->m_docLoaded )
  {
    d->m_doc = QDomDocument( "kdemetadata" );
    d->m_doc.appendChild( d->m_doc.createElement( "kdemetadata" ) );
    d->m_docLoaded = true;
  }
  
  QDomElement root = d->m_doc.documentElement();
  
  if ( url == d->m_url )
  {
    QDomElement newElement = d->m_doc.createElement( key );
  
    QDomElement e = root.namedItem( key ).toElement();
    if ( !e.isNull() )
      root.replaceChild( newElement, e );
    else
      root.appendChild( newElement );
    
    //HACK
    newElement.appendChild( d->m_doc.createCDATASection( val ) );
  }
  else
  {
    bool foundItem = false;
    
    QDomElement child = root.firstChild().toElement();
    for (; !child.isNull(); child = child.nextSibling().toElement() )
    {
      if ( child.tagName() != "item" )
        continue;

      QDomElement urlElement = child.namedItem( "url" ).toElement();
      if ( urlElement.isNull() )
        continue;
 
      if ( url == urlElement.text() )
      {
        QDomElement newElement = d->m_doc.createElement( key );
	
	QDomElement e = child.namedItem( key ).toElement();
	if ( !e.isNull() )
	  child.replaceChild( newElement, e );
	else
	  child.appendChild( newElement );

	//HACK
	newElement.appendChild( d->m_doc.createCDATASection( val ) );
	foundItem = true;
        break;
      }
    }
    
    if ( !foundItem )
    {
      QDomElement newChild = d->m_doc.createElement( "item" );
      root.appendChild( newChild );
      QDomElement urlElement = d->m_doc.createElement( "url" );
      newChild.appendChild( urlElement );
      urlElement.appendChild( d->m_doc.createTextNode( url.url() ) );
      QDomElement newKey = d->m_doc.createElement( key );
      newChild.appendChild( newKey );
      //HACK
      newKey.appendChild( d->m_doc.createCDATASection( val ) );
    }
  }
    
  QFile f( d->m_metaDataURL.path() );
  if ( !f.open( IO_WriteOnly ) )
    return;
    
  QTextStream str( &f );
  str << d->m_doc;
  f.close();
}

