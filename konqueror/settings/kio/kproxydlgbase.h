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

#ifndef KPROXY_DIALOG_BASE_H
#define KPROXY_DIALOG_BASE_H

#include <qstring.h>
#include <qstringlist.h>

#include <kdialogbase.h>
#include <kprotocolmanager.h>


class KProxyData
{
public:
  KProxyData();
  KProxyData::KProxyData( const KProxyData &data );
  ~KProxyData();

  void reset ();
  bool operator==( const KProxyData &data );
  KProxyData& operator=( const KProxyData &data );

public:
  QString ftpProxy;
  QString httpProxy;
  QString httpsProxy;
  QString scriptProxy;

  bool useReverseProxy;
  QStringList noProxyFor;
  KProtocolManager::ProxyType type;

private:
  void init();
};


class KProxyDialogBase : public KDialogBase
{
public:
  KProxyDialogBase( QWidget* parent = 0, const char* name = 0,
                    bool modal = false, const QString &caption = QString::null);

  virtual ~KProxyDialogBase();

  virtual const KProxyData data() const=0;

  virtual void setProxyData (const KProxyData&)=0;

protected:
  bool m_bHasValidData;
};
#endif
