/*  This file is part of the KDE project

    Copyright (C) 2000 Alexander Neundorf <neundorf@kde.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef __SMBRODLG_H
#define __SMBRODLG_H 

#include <qwidget.h>
#include <qlineedit.h>
#include <qcheckbox.h>

#include <kcmodule.h>


class SMBRoOptions : public KCModule
{
   Q_OBJECT
   public:
      SMBRoOptions(QWidget *parent = 0L, const char *name = 0L);
      ~SMBRoOptions();

      virtual void load();
      virtual void save();
      virtual void defaults();
      QString quickHelp() const;

   private slots:
      void changed();

   private:
      QLineEdit *m_userLe;
      QLineEdit *m_passwordLe;
      QLineEdit *m_workgroupLe;
      QCheckBox *m_showHiddenShares;
};

#endif
