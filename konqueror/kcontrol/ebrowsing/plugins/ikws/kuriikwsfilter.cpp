/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>
    Copyright (C) 2002, 2003 Dawit Alemayehu <adawit@kde.org>

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
    Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <unistd.h>

#include <kdebug.h>
#include <klocale.h>
#include <kurl.h>
#include <kinstance.h>
#include <kglobal.h>

#include "ikwsopts.h"
#include "kuriikwsfiltereng.h"
#include "kuriikwsfilter.h"

/**
 * IMPORTANT: If you change anything here, please run the regression test
 * kdelibs/kio/tests/kurifiltertest
 */

typedef KGenericFactory<KAutoWebSearch> KAutoWebSearchFactory;
K_EXPORT_COMPONENT_FACTORY (libkuriikwsfilter, KAutoWebSearchFactory("kcmkurifilt"))

KAutoWebSearch::KAutoWebSearch(QObject *parent, const char *name, const QStringList&)
               :KURIFilterPlugin(parent, name ? name : "kuriikwsfilter", 1.0),
                DCOPObject("KURIIKWSFilterIface")
{
}

KAutoWebSearch::~KAutoWebSearch()
{
}

void KAutoWebSearch::configure()
{
  if ( KURISearchFilterEngine::self()->verbose() )
    kdDebug() << "KAutoWebSearch::configure: Config reload requested..." << endl;

  KURISearchFilterEngine::self()->loadConfig();
}

bool KAutoWebSearch::filterURI( KURIFilterData &data ) const
{
  if (KURISearchFilterEngine::self()->verbose())
    kdDebug() << "KAutoWebSearch::filterURI: '" <<  data.uri().url() << "'" << endl;

  KURL u = data.uri();
  if ( u.pass().isEmpty() )
  {
    QString result = KURISearchFilterEngine::self()->autoWebSearchQuery( data.typedString() );
    if( !result.isEmpty() )
    {
      if ( KURISearchFilterEngine::self()->verbose() )
        kdDebug () << "Filtered URL: " << result << endl;

      setFilteredURI( data, KURL( result ) );
      setURIType( data, KURIFilterData::NET_PROTOCOL );
      return true;
    }
  }
  return false;
}

#include "kuriikwsfilter.moc"
