/*
    localdomainurifilter.h

    This file is part of the KDE project
    Copyright (C) 2002 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _LOCALDOMAINURIFILTER_H_
#define _LOCALDOMAINURIFILTER_H_

#include <time.h>

#include <dcopobject.h>
#include <kgenericfactory.h>
#include <kurifilter.h>
#include <qregexp.h>

class KInstance;
class KProcess;

/*
 This filter takes care of hostnames in the local search domain.
 If you're in domain domain.org which has a host intranet.domain.org
 and the typed URI is just intranet, check if there's a host
 intranet.domain.org and if yes, it's a network URI.
*/

class LocalDomainURIFilter : public KURIFilterPlugin, public DCOPObject
{
  K_DCOP
  Q_OBJECT

  public:
    LocalDomainURIFilter( QObject* parent, const QStringList& args );
    virtual bool filterURI( KURIFilterData &data ) const;

  k_dcop:
    virtual void configure();

  private:
    bool isLocalDomainHost( QString& cmd ) const;
    mutable QString last_host;
    mutable bool last_result;
    mutable time_t last_time;
    mutable QString m_fullname;
    QRegExp m_hostPortPattern;

  private Q_SLOTS:
    void receiveOutput( KProcess *, char *, int );
};

#endif
