/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>

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

#ifndef __konq_profiledlg_h__
#define __konq_profiledlg_h__

#include <kdialog.h>

#include <q3listview.h>
#include <QMap>
//Added by qt3to4:
#include <QGridLayout>

class KonqViewManager;
class Q3ListViewItem;
class QGridLayout;
class QCheckBox;
class QLineEdit;
class KPushButton;
class K3ListView;

typedef QMap<QString, QString> KonqProfileMap;

class KonqProfileItem : public Q3ListViewItem
{
public:
  KonqProfileItem( K3ListView *, const QString & );
  ~KonqProfileItem() {}

  QString m_profileName;
};

class KonqProfileDlg : public KDialog
{
  Q_OBJECT
public:
  KonqProfileDlg( KonqViewManager *manager, const QString &preselectProfile, QWidget *parent = 0L );
  ~KonqProfileDlg();

  /**
   * Find, read and return all available profiles
   * @return a map with < name, full path >
   */
  static KonqProfileMap readAllProfiles();

protected Q_SLOTS:
  virtual void slotUser1(); // User1 is "Rename Profile" button
  virtual void slotUser2(); // User2 is "Delete Profile" button
  virtual void slotUser3(); // User3 is Save button
  void slotTextChanged( const QString & );
  void slotSelectionChanged( Q3ListViewItem * item );

  void slotItemRenamed( Q3ListViewItem * );

private:
  void loadAllProfiles(const QString & = QString());
  KonqViewManager *m_pViewManager;

  KonqProfileMap m_mapEntries;

  QLineEdit *m_pProfileNameLineEdit;

  QCheckBox *m_cbSaveURLs;
  QCheckBox *m_cbSaveSize;

  K3ListView *m_pListView;
};

#endif
