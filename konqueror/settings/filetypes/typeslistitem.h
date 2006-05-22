/* This file is part of the KDE project
   Copyright (C) 2003 Waldo Bastian <bastian@kde.org>
   Copyright (C) 2003 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _TYPESLISTITEM_H
#define _TYPESLISTITEM_H

#include <q3listview.h>

#include <kmimetype.h>

class TypesListItem : public Q3ListViewItem
{
public:
  /**
   * Create a filetype group
   */
  TypesListItem(Q3ListView *parent, const QString & major );

  /**
   * Create a filetype item inside a group
   */
  TypesListItem(TypesListItem *parent, KMimeType::Ptr mimetype, bool newItem=false);

  /**
   * Create a filetype item not inside a group (used by keditfiletype)
   */
  TypesListItem(Q3ListView *parent, KMimeType::Ptr mimetype, bool newItem=false);

  ~TypesListItem();

  QString name() const { return m_major + "/" + m_minor; }
  QString majorType() const { return m_major; }
  QString minorType() const { return m_minor; }
  void setMinor(QString m) { m_minor = m; }
  QString comment() const { return m_comment; }
  void setComment(QString c) { m_comment = c; }
  /**
   * Returns true if "this" is a group
   */
  bool isMeta() const { return metaType; }
  /**
   * Returns true if the type is essential, i.e. can't be deleted
   * (see KMimeType::checkEssentialMimeTypes)
   */
  bool isEssential() const;
  QString icon() const { return m_icon; }
  void setIcon(const QString& i);
  QStringList patterns() const { return m_patterns; }
  void setPatterns(const QStringList &p) { m_patterns = p; }
  QStringList appServices() const;
  void setAppServices(const QStringList &dsl) { m_appServices = dsl; }
  QStringList embedServices() const;
  void setEmbedServices(const QStringList &dsl) { m_embedServices = dsl; }
  int autoEmbed() const { return m_autoEmbed; }
  void setAutoEmbed( int a ) { m_autoEmbed = a; }
  const KMimeType::Ptr& mimeType() const { return m_mimetype; }
  bool canUseGroupSetting() const;

  void getAskSave(bool &);
  void setAskSave(bool);

  // Whether the service s lists this mimetype explicitly
  KMimeType::Ptr findImplicitAssociation(const QString &desktop);

  bool isMimeTypeDirty() const; // whether the mimetype .desktop file needs saving
  bool isDirty() const;
  void sync();
  void setup();
  void refresh(); // update m_mimetype from ksycoca when Apply is pressed

  static bool defaultEmbeddingSetting(  const QString& major );
  static void reset();

private:
  void getServiceOffers( QStringList & appServices, QStringList & embedServices ) const;
  void saveServices( KConfig & profile, QStringList services, const QString & servicetype2 );
  void initMeta( const QString & major );
  void init(KMimeType::Ptr mimetype);
  static int readAutoEmbed( KMimeType::Ptr mimetype );

  KMimeType::Ptr m_mimetype;
  unsigned int groupCount:16; // shared between saveServices and sync
  unsigned int m_autoEmbed:2; // 0 yes, 1 no, 2 use group setting
  unsigned int metaType:1;
  unsigned int m_bNewItem:1;
  unsigned int m_bFullInit:1;
  unsigned int m_askSave:2; // 0 yes, 1 no, 2 default
  QString m_major, m_minor, m_comment, m_icon;
  QStringList m_patterns;
  QStringList m_appServices;
  QStringList m_embedServices;
  static QMap< QString, QStringList >* s_changedServices;
};

#endif
