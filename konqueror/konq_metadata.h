#ifndef __konq_metadata_h__
#define __konq_metadata_h__

#include <qobject.h>

#include <kurl.h>

class KInstance;
class KonqMetaDataProviderPrivate;

class KonqMetaDataProvider : public QObject
{
  Q_OBJECT
public:
  KonqMetaDataProvider( KInstance *instance = 0, QObject *parent = 0, const char *name = 0 );
  virtual ~KonqMetaDataProvider();

  virtual bool openDir( const KURL &url );

  virtual bool metaData( const KURL &url, const QString &serviceType, const QString &key, QString &val );

private:
  KonqMetaDataProviderPrivate *d;
};

#endif
