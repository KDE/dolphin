#include <stdlib.h>

#include <qstring.h>
#include <qfile.h>
#include <qtextstream.h>

#include <kapp.h>
#include <kio/job.h>
#include <kfileitem.h>

#include "kquery.h"
#include "kquery.moc"

KQuery::KQuery(QObject *parent, const char * name)
  : QObject(parent, name),
    m_minsize(-1), m_maxsize(-1),
    m_timeFrom(0), m_timeTo(0),
    job(0)
{
}

KQuery::~KQuery()
{
}

void KQuery::kill()
{
  if (job)
    job->kill(false);
}

void KQuery::start()
{
  if (m_recursive)
    job = KIO::listRecursive( m_url, false );
  else
    job = KIO::listDir( m_url, false );

  connect(job, SIGNAL(entries(KIO::Job *, const KIO::UDSEntryList &)),
	  SLOT(slotListEntries(KIO::Job *, const KIO::UDSEntryList &)));
  connect(job, SIGNAL(result(KIO::Job *)), SLOT(slotResult(KIO::Job *)));
  connect(job, SIGNAL(canceled(KIO::Job *)), SLOT(slotCanceled(KIO::Job *)));
}

void KQuery::slotResult( KIO::Job * _job )
{
  if (job != _job) return;
  job = 0;

  emit result(_job->error());
}

void KQuery::slotCanceled( KIO::Job * _job )
{
  if (job != _job) return;
  job = 0;

  emit result(KIO::ERR_USER_CANCELED);
}

void KQuery::slotListEntries( KIO::Job *, const KIO::UDSEntryList & list)
{
  KIO::UDSEntryListConstIterator it = list.begin();
  KIO::UDSEntryListConstIterator end = list.end();
  for (; it != end; ++it)
  {
    KFileItem * file = new KFileItem(*it, m_url, true, true);

    // we don't want this
    if ( file->name() == "." || file->name() == ".." )
      continue;

    if ( !m_regexp.isEmpty() &&
	 m_regexp.match( file->url().fileName( true ) ) == -1 )
      continue;

    // make sure the files are in the correct range
    if ( ( m_minsize >= 0 && file->size() < m_minsize ) ||
	 ( m_maxsize >= 0 && file->size() > m_maxsize ) )
      continue;

    // make sure it's in the correct date range
    // what about 0 times?
    if ( m_timeFrom && m_timeFrom > file->time(KIO::UDS_MODIFICATION_TIME) )
      continue;
    if ( m_timeTo && m_timeTo < file->time(KIO::UDS_MODIFICATION_TIME) )
      continue;

    // file type
    switch (m_filetype)
      {
      case 0:
	break;
      case 1: // plain file
	if ( !S_ISREG( file->mode() ) )
	  continue;
	break;
      case 2:
        if ( !file->isDir() )
	  continue;
	break;
      case 3:
	if ( !file->isLink() )
	  continue;
	break;
      case 4:
	if ( !S_ISCHR ( file->mode() ) && !S_ISBLK ( file->mode() ) &&
	     !S_ISFIFO( file->mode() ) && !S_ISSOCK( file->mode() ) )
	  continue;
	break;
      case 5: // binary
	if ( (file->permissions() & 0111) != 0111) // fixme -- and not dir?
	  continue;
	break;
      case 6: // suid
	if ( (file->permissions() & 04000) != 04000 ) // fixme
	  continue;
	break;
      default:
	if (!m_mimetype.isEmpty() && m_mimetype != file->mimetype())
	  continue;
      }

    // match contents...
    if (!m_context.isEmpty())
      {
	bool found = false;

	// FIXME: doesn't work with non local files
	QString filename = file->url().path();
	QFile qf(filename);
	qf.open(IO_ReadOnly);
	QTextStream stream(&qf);
	stream.setEncoding(QTextStream::Locale);
	while ( ! stream.atEnd() )
	  {
	    QString str = stream.readLine();
	    if (str.isNull()) break;
	    if (str.find(m_context, 0, m_casesensitive) != -1)
	      {
		found = true;
		break;
	      }

//            kapp->processEvents();
	  }

	if (!found)
	  continue;
      }


    emit addFile(file);
  }
}

void KQuery::setContext(const QString & context, bool casesensitive)
{
  m_context = context;
  m_casesensitive = casesensitive;
}

void KQuery::setMimeType(const QString &mimetype)
{
  m_mimetype = mimetype;
}

void KQuery::setFileType(int filetype)
{
  m_filetype = filetype;
}

void KQuery::setSizeRange(int min, int max)
{
  m_minsize = min;
  m_maxsize = max;
}

void KQuery::setTimeRange(time_t from, time_t to)
{
  m_timeFrom = from;
  m_timeTo = to;
}

void KQuery::setRegExp(const QRegExp &regexp)
{
  m_regexp = regexp;
}

void KQuery::setRecursive(bool recursive)
{
  m_recursive = recursive;
}

void KQuery::setPath(const KURL &url)
{
  m_url = url;
}

