 /*
   kproxydlgbase.h - Base dialog box for proxy configuration

   Copyright (C) 2001- Dawit Alemayehu <adawit@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License (GPL) version 2 as published by the Free Software
   Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "kproxydlgbase.h"

KProxyData::KProxyData()
{
  init ();
}

KProxyData::KProxyData(const KProxyData &data)
{
  type = data.type;
  noProxyFor = data.noProxyFor;
  useReverseProxy = data.useReverseProxy;

  ftpProxy = data.ftpProxy;
  httpProxy = data.httpProxy;
  httpsProxy = data.httpsProxy;
  scriptProxy = data.scriptProxy;
}

KProxyData::~KProxyData()
{
}

KProxyData& KProxyData::operator=( const KProxyData &data )
{
  type = data.type;
  noProxyFor = data.noProxyFor;
  useReverseProxy = data.useReverseProxy;

  ftpProxy = data.ftpProxy;
  httpProxy = data.httpProxy;
  httpsProxy = data.httpsProxy;
  scriptProxy = data.scriptProxy;

  return (*this);
}

bool KProxyData::operator==(const KProxyData &data)
{
  return (useReverseProxy == data.useReverseProxy && type == data.type &&
          noProxyFor == data.noProxyFor && ftpProxy == data.ftpProxy &&
          httpProxy == data.httpProxy && httpsProxy == data.httpsProxy &&
          scriptProxy == data.scriptProxy);
}

void KProxyData::reset()
{
  init();
}

void KProxyData::init()
{
  noProxyFor.clear();
  useReverseProxy = false;

  ftpProxy = QString::null;
  httpProxy = QString::null;
  httpsProxy = QString::null;
  scriptProxy = QString::null;
}


KProxyDialogBase::KProxyDialogBase( QWidget* parent, const char* name,
                                    bool modal, const QString &caption )
                 :KDialogBase( parent, name, modal, caption, Ok|Cancel )
{
  m_bHasValidData = false;
}

KProxyDialogBase::~KProxyDialogBase()
{
}
