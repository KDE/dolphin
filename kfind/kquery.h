#ifndef KQUERY_H
#define KQUERY_H

#include <time.h>

#include <qobject.h>
#include <qregexp.h>

#include <kio/job.h>
#include <kurl.h>

class KFileItem;

class KQuery : public QObject
{
  Q_OBJECT

 public:
  KQuery(QObject *parent = 0, const char * name = 0);
  ~KQuery();

  void setSizeRange( int min, int max );
  void setTimeRange( time_t from, time_t to );
  void setRegExp( const QString &regexp, bool caseSensitive );
  void setRecursive( bool recursive );
  void setPath(const KURL & url );
  void setFileType( int filetype );
  void setMimeType( const QString & mimetype );
  void setContext( const QString & context, bool casesensitive, bool useRegexp );

  void start();
  void kill();

 protected slots:
  void slotListEntries(KIO::Job *, const KIO::UDSEntryList &);
  void slotResult(KIO::Job *);
  void slotCanceled(KIO::Job *);

 signals:
  void addFile(const KFileItem *filename, const QString& matchingLine);
  //void addFile(const KFileItem *filename);
  void result(int);

 private:
  int m_filetype;
  int m_minsize;
  int m_maxsize;
  KURL m_url;
  time_t m_timeFrom;
  time_t m_timeTo;
  QRegExp m_regexp;
  bool m_recursive;
  QString m_mimetype;
  QString m_context;
  bool m_casesensitive;
  bool m_regexpForContent;
  QPtrList<QRegExp> m_regexps;
  KIO::ListJob *job;
};

#endif
