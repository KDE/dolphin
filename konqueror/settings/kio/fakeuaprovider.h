/*
 * Copyright (c) 2001 Dawit Alemayehu <adawit@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __FAKE_UAS_PROVIDER_H___
#define __FAKE_UAS_PROVIDER_H___

#include <ktrader.h>

class QString;
class QStringList;

class FakeUASProvider
{
public:
  enum StatusCode {
    SUCCEEDED=0,
    ALREADY_EXISTS,
    DUPLICATE_ENTRY
  };

  FakeUASProvider();
  ~FakeUASProvider(){};

  StatusCode createNewUAProvider( const QString& );
  QString aliasFor( const QString& );
  QStringList userAgentStringList();
  bool isListDirty() const { return m_bIsDirty; }
  void setListDirty( bool dirty ) { m_bIsDirty = dirty; }

protected:
  bool verifyDesktopFilename( QString& filename );
  void loadFromDesktopFiles();
  void parseDescription();

private:
  KTrader::OfferList m_providers;
  QStringList m_lstIdentity;
  QStringList m_lstAlias;
  bool m_bIsDirty;
};
#endif
