/**
 *  Copyright (c) 2001 David Faure <david@mandrakesoft.com>
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

#ifndef __BEHAVIOUR_H__
#define __BEHAVIOUR_H__

#include <kcmodule.h>

class QCheckBox;
class QLabel;
class QLineEdit;
class KConfig;
class QVButtonGroup;
class QRadioButton;

//-----------------------------------------------------------------------------


class KBehaviourOptions : public KCModule
{
  Q_OBJECT
public:
  KBehaviourOptions(KConfig *config, QString group, QWidget *parent=0, const char *name=0);

  virtual void load();
  virtual void save();
  virtual void defaults();

protected slots:

  void changed();
  void updateWinPixmap(bool);

private:

  KConfig *g_pConfig;
  KConfig *kfmclientConfig;
  QString groupname;

  QCheckBox *cbNewWin;
  QCheckBox *cbListProgress;

  QLabel *winPixmap;

  QLineEdit *homeURL;

  QVButtonGroup *bgOneProcess;
  QRadioButton  *rbOPNever,
                *rbOPLocal,
                *rbOPWeb,
                *rbOPAlways;
};

#endif		// __BEHAVIOUR_H__
