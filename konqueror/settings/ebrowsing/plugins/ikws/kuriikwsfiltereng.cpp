
/*  This file is part of the KDE project

    Copyright (C) 2002, 2003 Dawit Alemayehu <adawit@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

    Advanced web shortcuts:
    Copyright (C) 2001 Andreas Hochsteger <e9625392@student.tuwien.ac.at>


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

#include <unistd.h>

#include <QRegExp>
#include <QTextCodec>

#include <kurl.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kstandarddirs.h>
#include <kprotocolinfo.h>
#include <ksimpleconfig.h>
#include <kstaticdeleter.h>

#include "kuriikwsfiltereng.h"
#include "searchprovider.h"
#include <kglobal.h>

#define PIDDBG kDebug(7023) << "(" << getpid() << ") "
#define PDVAR(n,v) PIDDBG << n << " = '" << v << "'\n"

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * kdelibs/kio/tests/kurifiltertest
 */

KURISearchFilterEngine *KURISearchFilterEngine::s_pSelf = 0;
static KStaticDeleter<KURISearchFilterEngine> kurisearchfilterengsd;

KURISearchFilterEngine::KURISearchFilterEngine()
{
  loadConfig();
}

QString KURISearchFilterEngine::webShortcutQuery( const QString& typedString ) const
{
  QString result;

  if (m_bWebShortcutsEnabled)
  {
    QString search = typedString;
    int pos = search.indexOf(m_cKeywordDelimiter);

    QString key;
    if ( pos > -1 )
      key = search.left(pos);
    else if ( m_cKeywordDelimiter == ' ' && !search.isEmpty() )
      key = search;

    if (!key.isEmpty() && !KProtocolInfo::isKnownProtocol( key ))
    {
      SearchProvider *provider = SearchProvider::findByKey(key);

      if (provider)
      {
        result = formatResult(provider->query(), provider->charset(),
                              QString(), search.mid(pos+1), true);
        delete provider;
      }
    }
  }

  return result;
}


QString KURISearchFilterEngine::autoWebSearchQuery( const QString& typedString ) const
{
  QString result;

  if (m_bWebShortcutsEnabled && !m_defaultSearchEngine.isEmpty())
  {
    // Make sure we ignore supported protocols, e.g. "smb:", "http:"
    int pos = typedString.indexOf(':');

    if (pos == -1 || !KProtocolInfo::isKnownProtocol(typedString.left(pos)))
    {
      SearchProvider *provider = SearchProvider::findByDesktopName(m_defaultSearchEngine);

      if (provider)
      {
        result = formatResult (provider->query(), provider->charset(),
                               QString(), typedString, true);
        delete provider;
      }
    }
  }

  return result;
}

QByteArray KURISearchFilterEngine::name() const
{
  return "kuriikwsfilter";
}

KURISearchFilterEngine* KURISearchFilterEngine::self()
{
  if (!s_pSelf)
    kurisearchfilterengsd.setObject( s_pSelf, new KURISearchFilterEngine );
  return s_pSelf;
}

QStringList KURISearchFilterEngine::modifySubstitutionMap(SubstMap& map,
                                                          const QString& query) const
{
  // Returns the number of query words
  QString userquery = query;

  // Do some pre-encoding, before we can start the work:
  {
    int start = 0;
    int pos = 0;
    QRegExp qsexpr("\\\"[^\\\"]*\\\"");

    // Temporary substitute spaces in quoted strings (" " -> "%20")
    // Needed to split user query into StringList correctly.
    while ((pos = qsexpr.indexIn(userquery, start)) >= 0)
    {
      int i = 0;
      int n = 0;
      QString s = userquery.mid (pos, qsexpr.matchedLength());
      while ((i = s.indexOf(" ")) != -1)
      {
        s = s.replace (i, 1, "%20");
        n++;
      }
      start = pos + qsexpr.matchedLength() + 2*n; // Move after last quote
      userquery = userquery.replace (pos, qsexpr.matchedLength(), s);
    }
  }

  // Split user query between spaces:
  QStringList l = userquery.simplified().split(" ", QString::SkipEmptyParts);

  // Back-substitute quoted strings (%20 -> " "):
  {
    int i = 0;
    while ((i = userquery.indexOf("%20")) != -1)
      userquery = userquery.replace(i, 3, " ");

    for ( QStringList::Iterator it = l.begin(); it != l.end(); ++it )
      *it = (*it).replace("%20", " ");
  }

  PIDDBG << "Generating substitution map:\n";
  // Generate substitution map from user query:
  for (int i=0; i<=l.count(); i++)
  {
    int j = 0;
    int pos = 0;
    QString v = "";
    QString nr = QString::number(i);

    // Add whole user query (\{0}) to substitution map:
    if (i==0)
      v = userquery;
    // Add partial user query items to substitution map:
    else
      v = l[i-1];

    // Back-substitute quoted strings (%20 -> " "):
    while ((j = v.indexOf("%20")) != -1)
      v = v.replace(j, 3, " ");

    // Insert partial queries (referenced by \1 ... \n) to map:
    map.replace(QString::number(i), v);
    PDVAR ("  map['" + nr + "']", map[nr]);

    // Insert named references (referenced by \name) to map:
    j = 0;
    if ((i>0) && (pos = v.indexOf("=")) > 0)
    {
      QString s = v.mid(pos + 1);
      QString k = v.left(pos);

      // Back-substitute references contained in references (e.g. '\refname' substitutes to 'thisquery=\0')
      while ((j = s.indexOf("%5C")) != -1) s = s.replace(j, 3, "\\");
      map.replace(k, s);
      PDVAR ("  map['" + k + "']", map[k]);
    }
  }

  return l;
}

static QString encodeString(const QString &s, int mib)
{
  Q_UNUSED( mib ); // removed in KDE4/Qt4.
  QStringList l = s.split(" ");
  for(QStringList::Iterator it = l.begin();
      it != l.end(); ++it)
  {
     *it =  QLatin1String( QUrl::toPercentEncoding( *it ) ); //KUrl::encode_string(*it);
  }
  return l.join("+");
}

QString KURISearchFilterEngine::substituteQuery(const QString& url, SubstMap &map, const QString& userquery, const int encodingMib) const
{
  QString newurl = url;
  QStringList ql = modifySubstitutionMap (map, userquery);
  int count = ql.count();

  // Check, if old style '\1' is found and replace it with \{@} (compatibility mode):
  {
    int pos = -1;
    if ((pos = newurl.indexOf("\\1")) >= 0)
    {
      PIDDBG << "WARNING: Using compatibility mode for newurl='" << newurl
             << "'. Please replace old style '\\1' with new style '\\{0}' "
                "in the query definition.\n";
      newurl = newurl.replace(pos, 2, "\\{@}");
    }
  }

  PIDDBG << "Substitute references:\n";
  // Substitute references (\{ref1,ref2,...}) with values from user query:
  {
    int pos = 0;
    QRegExp reflist("\\\\\\{[^\\}]+\\}");

    // Substitute reflists (\{ref1,ref2,...}):
    while ((pos = reflist.indexIn(newurl)) >= 0)
    {
      bool found = false;

      //bool rest = false;
      QString v = "";
      QString rlstring = newurl.mid(pos + 2, reflist.matchedLength() - 3);
      PDVAR ("  reference list", rlstring);

      // \{@} gets a special treatment later
      if (rlstring == "@")
      {
        v = "\\@";
        found = true;
      }

      // TODO: strip whitespaces around commas
      QStringList rl = rlstring.split(",", QString::SkipEmptyParts);
      int i = 0;

      while ((i<rl.count()) && !found)
      {
        QString rlitem = rl[i];
        QRegExp range("[0-9]*\\-[0-9]*");

        // Substitute a range of keywords
        if (range.indexIn(rlitem) >= 0)
        {
          int pos = rlitem.indexOf("-");
          int first = rlitem.left(pos).toInt();
          int last  = rlitem.right(rlitem.length()-pos-1).toInt();

          if (first == 0)
            first = 1;

          if (last  == 0)
            last = count;

          for (int i=first; i<=last; i++)
          {
            v += map[QString::number(i)] + " ";
            // Remove used value from ql (needed for \{@}):
            ql[i-1] = "";
          }

          v = v.trimmed();
          if (!v.isEmpty())
            found = true;

          PDVAR ("    range", QString::number(first) + "-" + QString::number(last) + " => '" + v + "'");
          v = encodeString(v, encodingMib);
        }
        else if ( rlitem.startsWith("\"") && rlitem.endsWith("\"") )
        {
          // Use default string from query definition:
          found = true;
          QString s = rlitem.mid(1, rlitem.length() - 2);
          v = encodeString(s, encodingMib);
          PDVAR ("    default", s);
        }
        else if (map.contains(rlitem))
        {
          // Use value from substitution map:
          found = true;
          PDVAR ("    map['" + rlitem + "']", map[rlitem]);
          v = encodeString(map[rlitem], encodingMib);

          // Remove used value from ql (needed for \{@}):
          QString c = rlitem.left(1);
          if (c=="0")
          {
            // It's a numeric reference to '0'
            for (QStringList::Iterator it = ql.begin(); it!=ql.end(); ++it)
              (*it) = "";
          }
          else if ((c>="0") && (c<="9"))
          {
            // It's a numeric reference > '0'
            int n = rlitem.toInt();
            ql[n-1] = "";
          }
          else
          {
            // It's a alphanumeric reference
            QStringList::Iterator it = ql.begin();
            while ((it != ql.end()) && ((rlitem + "=") != (*it).left(rlitem.length()+1)))
              ++it;
            if ((rlitem + "=") == (*it).left(rlitem.length()+1))
              (*it) = "";
          }

          // Encode '+', otherwise it would be interpreted as space in the resulting url:
          int vpos = 0;
          while ((vpos = v.indexOf('+')) != -1)
            v = v.replace (vpos, 1, "%2B");

        }
        else if (rlitem == "@")
        {
          v = "\\@";
          PDVAR ("    v", v);
        }

        i++;
      }

      newurl = newurl.replace(pos, reflist.matchedLength(), v);
    }

    // Special handling for \{@};
    {
      PDVAR ("  newurl", newurl);
      // Generate list of unmatched strings:
      QString v = "";
      for (int i=0; i<ql.count(); i++) {
        v += " " + ql[i];
      }
      v = v.simplified();
      PDVAR ("    rest", v);
      v = encodeString(v, encodingMib);

      // Substitute \{@} with list of unmatched query strings
      int vpos = 0;
      while ((vpos = newurl.indexOf("\\@")) != -1)
        newurl = newurl.replace (vpos, 2, v);
    }
  }

  return newurl;
}

QString KURISearchFilterEngine::formatResult( const QString& url,
                                              const QString& cset1,
                                              const QString& cset2,
                                              const QString& query,
                                              bool isMalformed ) const
{
  SubstMap map;
  return formatResult (url, cset1, cset2, query, isMalformed, map);
}

QString KURISearchFilterEngine::formatResult( const QString& url,
                                              const QString& cset1,
                                              const QString& cset2,
                                              const QString& query,
                                              bool /* isMalformed */,
                                              SubstMap& map ) const
{
  // Return nothing if userquery is empty and it contains
  // substitution strings...
  if (query.isEmpty() && url.indexOf(QRegExp(QRegExp::escape("\\{"))) > 0)
    return QString();

  // Debug info of map:
  if (!map.isEmpty())
  {
    PIDDBG << "Got non-empty substitution map:\n";
    for(SubstMap::Iterator it = map.begin(); it != map.end(); ++it)
      PDVAR ("    map['" + it.key() + "']", it.value());
  }

  // Create a codec for the desired encoding so that we can transcode the user's "url".
  QString cseta = cset1;
  if (cseta.isEmpty())
    cseta = "iso-8859-1";

  QTextCodec *csetacodec = QTextCodec::codecForName(cseta.toLatin1());
  if (!csetacodec)
  {
    cseta = "iso-8859-1";
    csetacodec = QTextCodec::codecForName(cseta.toLatin1());
  }

  // Decode user query:
  QString userquery = QUrl::fromPercentEncoding( query.toLatin1() );

  PDVAR ("user query", userquery);
  PDVAR ("query definition", url);

  // Add charset indicator for the query to substitution map:
  map.replace("ikw_charset", cseta);

  // Add charset indicator for the fallback query to substitution map:
  QString csetb = cset2;
  if (csetb.isEmpty())
    csetb = "iso-8859-1";
  map.replace("wsc_charset", csetb);

  QString newurl = substituteQuery (url, map, userquery, csetacodec->mibEnum());

  PDVAR ("substituted query", newurl);

  return newurl;
}

void KURISearchFilterEngine::loadConfig()
{
  // Migrate from the old format, this block should remain until
  // we can assume "every" user has upgraded to a KDE version that
  // contains the sycoca based search provider configuration (malte).
  {
    KSimpleConfig oldConfig(KGlobal::dirs()->saveLocation("config") + QString(name()) + "rc");
    oldConfig.setGroup("General");

    if (oldConfig.hasKey("SearchEngines"))
    {
      // User has an old config file in his local config dir
      PIDDBG << "Migrating config file to .desktop files..." << endl;
      QString fallback = oldConfig.readEntry("InternetKeywordsSearchFallback");
      QStringList engines = oldConfig.readEntry("SearchEngines", QStringList() );
      for (QStringList::ConstIterator it = engines.begin(); it != engines.end(); ++it)
      {
        if (!oldConfig.hasGroup(*it + " Search"))
            continue;

        oldConfig.setGroup(*it + " Search");
        QString query = oldConfig.readEntry("Query");
        QStringList keys = oldConfig.readEntry("Keys", QStringList() );
        QString charset = oldConfig.readEntry("Charset");
        oldConfig.deleteGroup(*it + " Search");

        QString name;
        for (QStringList::ConstIterator key = keys.begin(); key != keys.end(); ++key)
        {
            // take the longest key as name for the .desktop file
            if ((*key).length() > name.length())
                name = *key;
        }

        if (*it == fallback)
            fallback = name;
        SearchProvider *provider = SearchProvider::findByKey(name);

        if (provider)
        {
          // If this entry has a corresponding global entry
          // that comes with KDE's default configuration,
          // compare both and if thei're equal, don't
          // create a local copy
          if (provider->name() == *it && provider->query() == query &&
              provider->keys() == keys && (provider->charset() == charset ||
              (provider->charset().isEmpty() && charset.isEmpty())))
          {
              PIDDBG << *it << " is unchanged, skipping" << endl;
              continue;
          }

          delete provider;
        }

        KSimpleConfig desktop(KGlobal::dirs()->saveLocation("services", "searchproviders/") + name + ".desktop");
        desktop.setGroup("Desktop Entry");
        desktop.writeEntry("Type", "Service");
        desktop.writeEntry("ServiceTypes", "SearchProvider");
        desktop.writeEntry("Name", *it);
        desktop.writeEntry("Query", query);
        desktop.writeEntry("Keys", keys);
        desktop.writeEntry("Charset", charset);

        PIDDBG << "Created searchproviders/" << name << ".desktop for " << *it << endl;
      }

      oldConfig.deleteEntry("SearchEngines", false);
      oldConfig.setGroup("General");
      oldConfig.writeEntry("InternetKeywordsSearchFallback", fallback);

      PIDDBG << "...completed" << endl;
    }
  }

  PIDDBG << "Keywords Engine: Loading config..." << endl;

  // Load the config.
  KConfig config( name() + "rc", false, false );
  config.setGroup( "General" );

  m_cKeywordDelimiter = config.readEntry("KeywordDelimiter", int(':'));
  m_bWebShortcutsEnabled = config.readEntry("EnableWebShortcuts", QVariant(true)).toBool();
  m_defaultSearchEngine = config.readEntry("DefaultSearchEngine");
  m_bVerbose = config.readEntry("Verbose", QVariant(false)).toBool();

  // Use either a white space or a : as the keyword delimiter...
  if (strchr (" :",m_cKeywordDelimiter) == 0)
    m_cKeywordDelimiter = ':';

  PIDDBG << "Keyword Delimiter: " << m_cKeywordDelimiter << endl;
  PIDDBG << "Default Search Engine: " << m_defaultSearchEngine << endl;
  PIDDBG << "Web Shortcuts Enabled: " << m_bWebShortcutsEnabled << endl;
  PIDDBG << "Verbose: " << m_bVerbose << endl;
}
