/*
 * windows.h
 *
 * Copyright (c) 1997 Patrick Dowler dowler@morgul.fsh.uvic.ca
 *
 * Requires the Qt widget libraries, available at no cost at
 * http://www.troll.no/
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

#ifndef __KKWMDESKTOPCONFIG_H__
#define __KKWMDESKTOPCONFIG_H__

#include <qlcdnumber.h> 
#include <qlabel.h>
#include <qdialog.h>
#include <qmessagebox.h>
#include <qcheckbox.h> 
#include <qpushbutton.h>

#include <kcmodule.h>

class KIntNumInput;
class QVButtonGroup;
class QSlider;

class KWinDesktopConfig : public KCModule
{
  Q_OBJECT

public:

  KWinDesktopConfig( QWidget *parent=0, const char* name=0 );
  ~KWinDesktopConfig( );

  void load();
  void save();
  void defaults();
  QString quickHelp() const;

public  slots:

  void setEBorders();
  void slotChanged();

  
private:

  bool getElectricBorders( void );
  int getElectricBordersDelay();
  bool getElectricBordersMovePointer( void );

  void setElectricBorders( bool );
  void setElectricBordersDelay( int );
  void setElectricBordersMovePointer( bool );

  int getBorderSnapZone();
  void setBorderSnapZone( int );
  
  int getWindowSnapZone();
  void setWindowSnapZone( int );

  QVButtonGroup *ElectricBox;
  QCheckBox *enable, *movepointer;
  KIntNumInput *delays;
  QLabel *sec;

  QVButtonGroup *MagicBox;
  KIntNumInput *BrdrSnap, *WndwSnap;
  QCheckBox *OverlapSnap;
};

#endif

