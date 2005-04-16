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
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

#ifndef __konq_profiledlg_h__
#define __konq_profiledlg_h__

#include <kdialogbase.h>

#include <qlistview.h>
#include <qmap.h>

class KonqViewManager;
class QListViewItem;
class QGridLayout;
class QCheckBox;
class QLineEdit;
class KPushButton;
class KListView;

typedef QMap<QString, QString> KonqProfileMap;

class KonqProfileItem : public QListViewItem
{
public:
  KonqProfileItem( KListView *, const QString & );
  ~KonqProfileItem() {}

  QString m_profileName;
};

class KonqProfileDlg : public KDialogBase
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

protected slots:
  virtual void slotUser1(); // User1 is "Rename Profile" button
  virtual void slotUser2(); // User2 is "Delete Profile" button
  virtual void slotUser3(); // User3 is Save button
  void slotTextChanged( const QString & );
  void slotSelectionChanged( QListViewItem * item );

  void slotItemRenamed( QListViewItem * );

private:
  void loadAllProfiles(const QString & = QString::null);
  KonqViewManager *m_pViewManager;

  KonqProfileMap m_mapEntries;

  QLineEdit *m_pProfileNameLineEdit;

  QCheckBox *m_cbSaveURLs;
  QCheckBox *m_cbSaveSize;

  KListView *m_pListView;
};

#endif
