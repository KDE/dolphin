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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KPROXY_DIALOG_BASE_H
#define KPROXY_DIALOG_BASE_H

#include <QMap>
#include <QString>
#include <QStringList>

#include <kdialogbase.h>
#include <kprotocolmanager.h>

class KProxyData
{
public:
  KProxyData();
  KProxyData( const KProxyData &data );

  void reset ();
  KProxyData& operator=( const KProxyData &data );

  bool useReverseProxy;
  bool showEnvVarValue;
  QStringList noProxyFor;
  KProtocolManager::ProxyType type;
  QMap<QString, QString> proxyList;

private:
  void init();
};


class KProxyDialogBase : public KDialogBase
{
public:
  KProxyDialogBase( QWidget* parent = 0, const char* name = 0,
                    bool modal = false, const QString &caption = QString());

  virtual ~KProxyDialogBase() {};

  virtual const KProxyData data() const=0;

  virtual void setProxyData (const KProxyData&)=0;

protected:
  void setHighLight (QWidget* widget = 0, bool highlight = false);
  bool m_bHasValidData;
};
#endif
