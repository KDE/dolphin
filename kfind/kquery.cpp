#include <stdlib.h>

#include <qfile.h>

#include <kfileitem.h>
#include <kfilemetainfo.h>

#include "kquery.moc"

KQuery::KQuery(QObject *parent, const char * name)
  : QObject(parent, name),
    m_minsize(-1), m_maxsize(-1),
    m_timeFrom(0), m_timeTo(0),
    job(0)
{
  m_regexps.setAutoDelete(true);
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
  QString matchingLine;
	int matchingLineNumber;

  KFileItem * file = 0;
  QRegExp *filename_match;

  KIO::UDSEntryListConstIterator it = list.begin();
  KIO::UDSEntryListConstIterator end = list.end();
  for (; it != end; ++it)
  {
    delete file;
    file = new KFileItem(*it, m_url, true, true);

    // we don't want this
    if ( file->name() == "." || file->name() == ".." )
      continue;

    
    bool matched=false;
    
    for ( filename_match = m_regexps.first(); !matched && filename_match; filename_match = m_regexps.next() )
      {
	
	matched |=  filename_match->isEmpty()  ||
	  (filename_match->exactMatch( file->url().fileName( true ) ) );
      }
    if (!matched)
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

    // username / group match
    if ( (m_username != "") && (m_username != file->user()) )
       continue;
    if ( (m_groupname != "") && (m_groupname != file->group()) )
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
	if ( (file->permissions() & 0111) != 0111 || file->isDir() )
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

    // match datas in metainfo...
    if (!m_metainfo.isEmpty())
      {
	matchingLineNumber=0;
		bool foundmeta=false;

	QString filename = file->url().path();
	if(filename.startsWith("/dev/"))
	  continue;
	
	KFileMetaInfo metadatas(filename);
	KFileMetaInfoItem metaitem;
	QStringList metakeys;
	QString strmetakeycontent;

	if(metadatas.isEmpty())
		continue;

	if(m_metainfokey.isEmpty())
  {
     metakeys=metadatas.supportedKeys();
    for ( QStringList::Iterator it = metakeys.begin(); it != metakeys.end(); ++it ) {
        metaitem=metadatas.item(*it);
        strmetakeycontent=metaitem.string();
        if(strmetakeycontent.find(m_metainfo)!=-1)
        {
          foundmeta=true;
          break;
        }
    }
   }
   else
   {
      metaitem=metadatas.item(m_metainfokey);
      strmetakeycontent=metaitem.string();
      if(strmetakeycontent.find(m_metainfo)!=-1)
      {
        foundmeta=true;
        break;
      }
   }
   if(!foundmeta)
   	continue;
	}

    // match contents...
    if (!m_context.isEmpty())
      {
	bool found = false;

	// FIXME: doesn't work with non local files
	QString filename = file->url().path();
	if(filename.startsWith("/dev/"))
	  continue;
	QFile qf(filename);
	qf.open(IO_ReadOnly);
	QTextStream stream(&qf);
	stream.setEncoding(QTextStream::Locale);
	matchingLineNumber=0;
	while ( ! stream.atEnd() )
	  {
	    QString str = stream.readLine();
	  	  matchingLineNumber++;

	    if (str.isNull()) break;
       if (m_regexpForContent)
       {
	       if (m_regexp.search(str)>=0)
	      {
					matchingLine=QString::number(matchingLineNumber)+": "+str;
					found = true;
					break;
	      }
       }
       else
       {
	    if (str.find(m_context, 0, m_casesensitive) != -1)
	      {
					matchingLine=QString::number(matchingLineNumber)+": "+str;
				found = true;
				break;
	      }
       };
//            kapp->processEvents();
	  }


	if (!found)
	  continue;
      }
    emit addFile(file,matchingLine);
  }

  delete file;
}

void KQuery::setContext(const QString & context, bool casesensitive, bool useRegexp)
{
  m_context = context;
  m_casesensitive = casesensitive;
  m_regexpForContent=useRegexp;
  m_regexp.setWildcard(!m_regexpForContent);
  m_regexp.setCaseSensitive(casesensitive);
  if (m_regexpForContent)
     m_regexp.setPattern(m_context);
}

void KQuery::setMetaInfo(const QString &metainfo, const QString &metainfokey)
{
  m_metainfo=metainfo;
  m_metainfokey=metainfokey;
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

void KQuery::setUsername(QString username)
{
   m_username = username;
}

void KQuery::setGroupname(QString groupname)
{
   m_groupname = groupname;
}


void KQuery::setRegExp(const QString &regexp, bool caseSensitive)
{
  QRegExp sep(";");
  QStringList strList=QStringList::split( sep, regexp, false);
  m_regexps.clear();
  for ( QStringList::Iterator it = strList.begin(); it != strList.end(); ++it )
     m_regexps.append(new QRegExp((*it),caseSensitive,true));
}

void KQuery::setRecursive(bool recursive)
{
  m_recursive = recursive;
}

void KQuery::setPath(const KURL &url)
{
  m_url = url;
}

