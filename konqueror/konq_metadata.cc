
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

  QValueList<KonqMetaDataProvider::SubProvider> m_subProviders;
};

KonqMetaDataProvider::KonqMetaDataProvider( KInstance *instance, QObject *parent, const char *name )
  : QObject( parent, name )
{
  d = new KonqMetaDataProviderPrivate;
  d->m_instance = instance ? instance : KGlobal::instance();
}

KonqMetaDataProvider::~KonqMetaDataProvider()
{
  clear();
  delete d;
}

bool KonqMetaDataProvider::openDir( const KURL &url )
{
  clear();
  d->m_doc = QDomDocument();
  d->m_docLoaded = false;

  // temporary ;-)
  if ( !url.isLocalFile() )
    return false;

  d->m_metaDataURL = url;
  d->m_metaDataURL.addPath( d->m_metaDataSuffix );

  d->m_url = url;

  if ( !QFile::exists( d->m_metaDataURL.path() ) )
    return true;

  QFile f( d->m_metaDataURL.path() );
  if ( !f.open( IO_ReadOnly ) )
    return true;

  d->m_doc = QDomDocument( &f );
  d->m_docLoaded = true;

  return true;
}

QDomElement KonqMetaDataProvider::metaData( const KURL &url, const QString &serviceType, const QString &key )
{
  if ( url != d->m_url && serviceType == "inode/directory" )
  {
    QValueList<SubProvider>::ConstIterator it = d->m_subProviders.begin();
    QValueList<SubProvider>::ConstIterator end = d->m_subProviders.end();
    for (; it != end; ++it )
      if ( (*it).m_url == url )
        return (*it).m_provider->metaData( url, QString::null, key );

    KonqMetaDataProvider *provider = new KonqMetaDataProvider( d->m_instance, this, "submetaprov" );

    if ( !provider->openDir( url ) )
    {
      delete provider;
      return QDomElement();
    }

    QDomElement res = provider->metaData( url, QString::null, key );

    if ( res.isNull() )
    {
      delete provider;
      return res;
    }

    SubProvider subProvider;
    subProvider.m_provider = provider;
    subProvider.m_url = url;
    d->m_subProviders.append( subProvider );
    return res;
  }

  if ( !d->m_docLoaded )
  {
    d->m_doc = QDomDocument( "kdemetadata" );
    QDomElement root = d->m_doc.createElement( "kdemetadata" );
    d->m_doc.appendChild( root );
    root.setAttribute( "version", "1.0" );
    QDomElement urlElement = d->m_doc.createElement( "url" );
    root.appendChild( urlElement );
    urlElement.appendChild( d->m_doc.createTextNode( url.url() ) );
    d->m_docLoaded = true;
  }

  QDomElement root = d->m_doc.documentElement();

  if ( url == d->m_url )
  {
    QDomElement e = root.namedItem( key ).toElement();

    if ( e.isNull() )
    {
      e = d->m_doc.createElement( key );
      root.appendChild( e );
    }

    return e;
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
      QDomElement e = child.namedItem( key ).toElement();

      if ( e.isNull() )
      {
        e = d->m_doc.createElement( key );
	child.appendChild( e );
      }

      return e;
    }
  }

  QDomElement e = d->m_doc.createElement( "item" );
  root.appendChild( e );
  QDomElement urlElement = d->m_doc.createElement( "url" );
  e.appendChild( urlElement );
  urlElement.appendChild( d->m_doc.createTextNode( url.url() ) );

  return e;
}

bool KonqMetaDataProvider::commit()
{
  QValueList<SubProvider>::ConstIterator it = d->m_subProviders.begin();
  QValueList<SubProvider>::ConstIterator end = d->m_subProviders.end();
  for (; it != end; ++it )
    if ( !(*it).m_provider->commit() )
      return false;

  if ( !d->m_docLoaded )
    return false;

  QFile f( d->m_metaDataURL.path() );
  if ( !f.open( IO_WriteOnly ) )
    return false;

  QTextStream str( &f );
  str << d->m_doc;
  f.close();
  return true;
}

void KonqMetaDataProvider::clear()
{
  while ( d->m_subProviders.count() > 0 )
  {
    SubProvider p = *d->m_subProviders.begin();
    delete p.m_provider;
    d->m_subProviders.remove( d->m_subProviders.begin() );
  }
}

#include "konq_metadata.moc"
