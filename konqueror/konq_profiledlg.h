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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#ifndef __konq_profiledlg_h__
#define __konq_profiledlg_h__

#include <kdialog.h>
#include <qmap.h>

class KonqViewManager;
class QListView;
class QListViewItem;
class QGridLayout;
class QCheckBox;
class QLineEdit;
class QPushButton;

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
  static QMap<QString,QString> readAllProfiles();

protected slots:
  void slotSave();
  void slotDelete();
  void slotRename();
  void slotTextChanged( const QString & );
  void slotSelectionChanged( QListViewItem * item );

private:
  KonqViewManager *m_pViewManager;

  QMap<QString,QString> m_mapEntries;

  QGridLayout *m_pGrid;

  QLineEdit *m_pProfileNameLineEdit;

  QPushButton *m_pDeleteProfileButton;
  QPushButton *m_pRenameProfileButton;
  QPushButton *m_pSaveButton;
  QPushButton *m_pCloseButton;
  QCheckBox *m_cbSaveURLs;
  QCheckBox *m_cbSaveSize;

  QListView *m_pListView;
};

#endif
