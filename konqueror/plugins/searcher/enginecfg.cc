/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Internet Keywords support (C) 1999 Yves Arrouye <yves@realnames.com>
    Current maintainer Yves Arrouye <yves@realnames.com>

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/ 

#include "enginecfg.h"
#include "main.h"

#include <kconfig.h>
#include <kinstance.h>
#include <kurl.h>

EngineCfg *EngineCfg::s_pSelf = 0L;

#define SEARCH_SUFFIX	" " "Search"

#define IKW_KEY		"Internet Keywords"
#define IKW_SUFFIX	" " IKW_KEY
#define IKW_REALNAMES	"RealNames"

EngineCfg::EngineCfg() {
    KConfig *config = KonqSearcherFactory::instance()->config();

    config->setGroup(IKW_KEY);

    m_bInternetKeywordsEnabled = config->readBoolEntry("NavEnabled", true);

    QString selNavEngine = config->readEntry("NavSelectedEngine", IKW_REALNAMES);
    QString selNavSearch = config->readEntry("NavSearchFallback");
    
    QStringList engines = config->readListEntry("NavEngines");

    QStringList::ConstIterator gIt = engines.begin();
    QStringList::ConstIterator gEnd = engines.end();
    for (; gIt != gEnd; ++gIt) {
	QString grpName = *gIt + IKW_SUFFIX;
	if (config->hasGroup(grpName)) {
	    config->setGroup(grpName);
    
	    NavEntry e;
	    e.m_strName = *gIt;
	    e.m_strQuery = config->readEntry("Query");
	    e.m_strQueryWithSearch = config->readEntry("QueryWithSearch");
    
	    m_lstInternetKeywordsEngines.append(e);

	    if (e.m_strName == selNavEngine) {
		m_currInternetKeywordsNavEngine = e;
	    }
	}
    }

    NavEntry rn = navEntryByName(IKW_REALNAMES);
    if (rn.m_strName == QString::null) {
	rn.m_strName = IKW_REALNAMES;
	rn.m_strQuery = "http://navigation.realnames.com/resolver.dll?realname=\\1&charset=\\2&providerid=132";
	rn.m_strQueryWithSearch = "http://navigation.realnames.com/resolver.dll?action=navigation&realname=\\1&charset=\\2&providerid=132&fallbackuri=\\|";

	if (rn.m_strName == selNavEngine) {
	    m_currInternetKeywordsNavEngine = rn;
	}
	m_lstInternetKeywordsEngines.append(rn);
    }

    config->setGroup("General");

    m_bVerbose = config->readBoolEntry("Verbose");

    engines = config->readListEntry("SearchEngines");
  
    gIt = engines.begin();
    gEnd = engines.end();
    for (; gIt != gEnd; ++gIt) {
	QString grpName = *gIt + SEARCH_SUFFIX;
	if (config->hasGroup(grpName)) {
	    config->setGroup(grpName);
	} else {
	    config->setGroup(*gIt);
	}
    
	SearchEntry e;
	e.m_strName = *gIt;
	e.m_lstKeys = config->readListEntry("Keys");
	e.m_strQuery = config->readEntry("Query");
    
	m_lstSearchEngines.append(e);

	if (e.m_strName == selNavSearch) {
	    m_currInternetKeywordsSearchEngine = e;
	}
    }
}

void EngineCfg::saveSearchEngine(SearchEntry e) {
    QValueList<SearchEntry>::Iterator it = m_lstSearchEngines.begin();
    QValueList<SearchEntry>::Iterator end = m_lstSearchEngines.end();
    
    for (; it != end; ++it) {
	if ((*it).m_strName == e.m_strName) {
	    m_lstSearchEngines.remove(it);
	    break;
	}
    }

    m_lstSearchEngines.append(e);
  
    saveConfig();
}

void EngineCfg::removeSearchEngine(const QString &name) {
  QValueList<SearchEntry>::Iterator it = m_lstSearchEngines.begin();
  QValueList<SearchEntry>::Iterator end = m_lstSearchEngines.end();

  for (; it != end; ++it) {
      if ((*it).m_strName == name) {
	  m_lstSearchEngines.remove(it);
	  break;
      }
  }

  saveConfig();
}  

QString EngineCfg::searchQuery(const QString &key) const {
  QValueList<SearchEntry>::ConstIterator it = m_lstSearchEngines.begin();
  QValueList<SearchEntry>::ConstIterator end = m_lstSearchEngines.end();

  for (; it != end; ++it) {
      if ((*it).m_lstKeys.contains(key)) {
	  return (*it).m_strQuery;
      }
  }

  return QString::null;
}

EngineCfg::SearchEntry EngineCfg::searchEntryByName(const QString &name) const {
    QValueList<SearchEntry>::ConstIterator it = m_lstSearchEngines.begin();
    QValueList<SearchEntry>::ConstIterator end = m_lstSearchEngines.end();
    
    for (; it != end; ++it) {
	if ((*it).m_strName == name) {
	    return *it;
	}
    }

    return SearchEntry();
}

QString EngineCfg::navQuery() const {
    if (m_bInternetKeywordsEnabled) {
	QString search = m_currInternetKeywordsSearchEngine.m_strQuery;
	if (search != QString::null) {
	    int pct = m_currInternetKeywordsNavEngine.m_strQueryWithSearch.find("\\|");
	    if (pct >= 0) {
		int npct = search.find("\\1");
		if (npct >= 0) {
		    search = search.replace(npct, 2, "%1");
		}
		KURL::encode(search);
		QString res = m_currInternetKeywordsNavEngine.m_strQueryWithSearch;
		return res.replace(pct, 2, search);
	    }
	}

	return m_currInternetKeywordsNavEngine.m_strQuery;
    }

    return QString::null;
}

EngineCfg::NavEntry EngineCfg::navEntryByName(const QString &name) const {
    QValueList<NavEntry>::ConstIterator it = m_lstInternetKeywordsEngines.begin();
    QValueList<NavEntry>::ConstIterator end = m_lstInternetKeywordsEngines.end();

    for (; it != end; ++it) {
	if ((*it).m_strName == name) {
	    return *it;
	}
    }

    return NavEntry();
}

EngineCfg* EngineCfg::self() {
    if (!s_pSelf) {
	s_pSelf = new EngineCfg;
    }

    return s_pSelf;
}

void EngineCfg::saveConfig() const {
    KConfig *config = KonqSearcherFactory::instance()->config();

    QStringList engines;

    QValueList<SearchEntry>::ConstIterator it = m_lstSearchEngines.begin();
    QValueList<SearchEntry>::ConstIterator end = m_lstSearchEngines.end();

    for (; it != end; ++it) {
	engines.append((*it).m_strName);
	config->setGroup((*it).m_strName + SEARCH_SUFFIX);
	config->writeEntry("Keys", (*it).m_lstKeys);
	config->writeEntry("Query", (*it).m_strQuery);
    }
  
    config->setGroup("General");

    config->writeEntry("SearchEngines", engines);
    if (m_bVerbose) {
	config->writeEntry("Verbose", m_bVerbose);
    }

    engines.clear();

    QValueList<NavEntry>::ConstIterator nit = m_lstInternetKeywordsEngines.begin();
    QValueList<NavEntry>::ConstIterator nend = m_lstInternetKeywordsEngines.end();

    for (; nit != nend; ++nit) {
	engines.append((*nit).m_strName);
	config->setGroup((*nit).m_strName + IKW_SUFFIX);
	config->writeEntry("Query", (*nit).m_strQuery);
	if ((*nit).m_strQueryWithSearch != QString::null) {
	    config->writeEntry("QueryWithSearch", (*nit).m_strQueryWithSearch);
	}
    }
  
    config->setGroup(IKW_KEY);

    config->writeEntry("NavEngines", engines);
    config->writeEntry("NavEnabled", m_bInternetKeywordsEnabled);
    config->writeEntry("NavSelectedEngine", m_currInternetKeywordsNavEngine.m_strName);
    config->writeEntry("NavSearchFallback", m_currInternetKeywordsSearchEngine.m_strName);

    config->sync();
}

