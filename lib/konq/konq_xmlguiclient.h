/* This file is part of the KDE project
   Copyright (C) 2001 Holger Freyther <freyther@yahoo.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konqxmlguiclient_h
#define __konqxmlguiclient_h

#include <sys/types.h>

#include <QtXml/QDomDocument>
#include <kaction.h>
#include <kxmlguiclient.h>
#include <QStringList>
#include <libkonq_export.h>

/**
 * This class implements common methods to manipulate the DOMDocument of KXMLGUIClient
 *
 */
class LIBKONQ_EXPORT KonqXMLGUIClient : public KXMLGUIClient
{
public:
  KonqXMLGUIClient( );
  KonqXMLGUIClient( KXMLGUIClient *parent );
  virtual ~KonqXMLGUIClient( );
  /**
   * Reimplemented for internal purpose
   */
  QDomDocument domDocument( ) const;

  QDomElement domElement( ) const;

protected:
  void addAction( KAction *action, const QDomElement &menu = QDomElement() );
  void addAction( const char *name, const QDomElement &menu = QDomElement() );
  void addSeparator( const QDomElement &menu = QDomElement() );
  /// only add a separator if an action is added afterwards
  void addPendingSeparator();
  void addGroup( const QString &grp );
  void addMerge( const QString &name );

  // @return true if addAction was called at least once
  bool hasAction() const;
  void prepareXMLGUIStuff();

private:
  QDomElement m_menuElement;
  QDomDocument m_doc;
  void handlePendingSeparator();
  class Private;
  Private *d;
};
#endif

