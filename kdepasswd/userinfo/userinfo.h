/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/
/*
 * @file UserInfo: User Account Control Center module for KDE
 */

#ifndef USERINFO_H
#define USERINFO_H

class QPushButton;
class KAboutData;

#include <kcmodule.h>
#include <kglobal.h>

class KUserInfoConfig : public KCModule
{
  Q_OBJECT

public:
  KUserInfoConfig(QWidget *parent=0, const char *name=0, const QStringList &list=QStringList());

  const KAboutData* aboutData() const;

  void save();
  void load();
  void defaults();
  bool eventFilter(QObject*, QEvent*);
  int buttons();
  QString quickHelp() const;


private slots:
  void slotChangeRealName();
  void slotChangePassword();
  void slotFaceButtonClicked();

private:
  void changeFace(const QString &);
  void changeFace(const QPixmap &);
  void faceButtonDropEvent(QDropEvent*);

  QString m_userPicsDir;
  QString m_facesDir;

  int m_FaceSource; // 1 = Admin Only, 2 = Admin first, 3 = User first, or user only

  QPixmap m_FacePixmap;

  QString m_HeaderText;
  QString m_InfoRightText;

  QPushButton *m_pFaceButton;
  QLabel *m_pHeader;
  QLabel *m_pInfoRight;

  QString m_UserName;
  QString m_FullName;
  QString m_UserID;
  QString m_HomeDir;
  QString m_Shell;
};

#endif
