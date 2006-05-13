#ifndef KQUERY_H
#define KQUERY_H

#include <time.h>

#include <QObject>
#include <QRegExp>
#include <QQueue>
#include <QList>
#include <QDir>
#include <QStringList>

#include <kio/job.h>
#include <kurl.h>
#include <kprocess.h>

class KFileItem;

class KQuery : public QObject
{
  Q_OBJECT

 public:
  KQuery(QObject *parent = 0, const char * name = 0);
  ~KQuery();

  void setSizeRange( int mode, KIO::filesize_t value1, KIO::filesize_t value2);
  void setTimeRange( time_t from, time_t to );
  void setRegExp( const QString &regexp, bool caseSensitive );
  void setRecursive( bool recursive );
  void setPath(const KUrl & url );
  void setFileType( int filetype );
  void setMimeType( const QStringList & mimetype );
  void setContext( const QString & context, bool casesensitive,
  bool search_binary, bool useRegexp );
  void setUsername( QString username );
  void setGroupname( QString groupname );
  void setMetaInfo(const QString &metainfo, const QString &metainfokey);
  void setUseFileIndex(bool);

  void start();
  void kill();
  const KUrl& url()              {return m_url;};

 private:
  /* Check if file meets the find's requirements*/
  inline void processQuery(KFileItem*);

 public Q_SLOTS:
  /* List of files found using slocate */
  void slotListEntries(QStringList);
 protected Q_SLOTS:
  /* List of files found using KIO */
  void slotListEntries(KIO::Job *, const KIO::UDSEntryList &);
  void slotResult(KJob *);
  void slotCanceled(KIO::Job *);
  void slotreceivedSdtout(KProcess*,char*,int);
  void slotreceivedSdterr(KProcess*,char*,int);
  void slotendProcessLocate(KProcess*);

 Q_SIGNALS:
  void addFile(const KFileItem *filename, const QString& matchingLine);
  void result(int);

 private:
  void checkEntries();

  int m_filetype;
  int m_sizemode;
  KIO::filesize_t m_sizeboundary1;
  KIO::filesize_t m_sizeboundary2;
  KUrl m_url;
  time_t m_timeFrom;
  time_t m_timeTo;
  QRegExp m_regexp;// regexp for file content
  bool m_recursive;
  QStringList m_mimetype;
  QString m_context;
  QString m_username;
  QString m_groupname;
  QString m_metainfo;
  QString m_metainfokey;
  bool m_casesensitive;
  bool m_search_binary;
  bool m_regexpForContent;
  bool m_useLocate;
  char* bufferLocate;
  int bufferLocateLength;
  QStringList locateList;
  KProcess *processLocate;
  QList<QRegExp*> m_regexps;// regexps for file name
//  QValueList<bool> m_regexpsContainsGlobs;  // what should this be good for ? Alex
  KIO::ListJob *job;
  bool m_insideCheckEntries;
  QQueue<KFileItem *> m_fileItems;
  QRegExp* metaKeyRx;
  int m_result;
  QStringList ignore_mimetypes;
  QStringList ooo_mimetypes;     // OpenOffice.org mimetypes
  QStringList koffice_mimetypes;
};

#endif
