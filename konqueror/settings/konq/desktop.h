/**
 *  Copyright (c) 2000 Matthias Elter <elter@kde.org>
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

#ifndef __desktop_h__
#define __desktop_h__

#include <kcmodule.h>
#include <qslider.h>

class QSpinBox;
class QLabel;
class KLineEdit;

class KDesktopConfig : public KCModule
{
  Q_OBJECT
  
 public:
  KDesktopConfig(QWidget *parent = 0L, const char *name = 0L);

  void load();
  void save();
  void defaults();
  
  int buttons();
  QString quickHelp() const;

 protected slots:
  void slotValueChanged(int);
  void slotTextChanged(const QString&);
    
 private:
  QSlider    *_numSlider;
  QLabel     *_numLabel;
  QLabel     *_nameLabel[16];
  KLineEdit  *_nameInput[16];
};

#endif
