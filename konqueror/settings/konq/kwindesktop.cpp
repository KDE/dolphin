/*
 * $Id$
 *
 * Copyright (c) 1997 Bernd Johannes Wuebben wuebben@math.cornell.edu
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

#include <qlayout.h>
#include <qvbuttongroup.h>
#include <qcheckbox.h>
#include <qslider.h>
#include <qwhatsthis.h>

#include <kapp.h>
#include <kglobal.h>
#include <klocale.h>
#include <kconfig.h>
#include <knuminput.h>

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "kwindesktop.h"

/*
 * Some inter-widget spacing constants
 */
#define SPACE_XO 20
#define SPACE_YO 20
#define SPACE_XI 10
#define SPACE_YI 10

#define max(a,b) ((a > b) ? a:b)

// kwm config keywords
#define KWM_ELECTRIC_BORDER                  "ElectricBorder"
#define KWM_ELECTRIC_BORDER_DELAY            "ElectricBorderNumberOfPushes"
#define KWM_ELECTRIC_BORDER_MOVE_POINTER     "ElectricBorderPointerWarp"

//CT 15mar 98 - magics
#define KWM_BRDR_SNAP_ZONE                   "BorderSnapZone"
#define KWM_BRDR_SNAP_ZONE_DEFAULT           10
#define KWM_WNDW_SNAP_ZONE                   "WindowSnapZone"
#define KWM_WNDW_SNAP_ZONE_DEFAULT           10

#define MAX_BRDR_SNAP                          100
#define MAX_WNDW_SNAP                          100
#define MAX_EDGE_RES                         1000


//CT 21Oct1998 - emptied
KWinDesktopConfig::~KWinDesktopConfig ()
{
}

//CT 21Oct1998 - rewritten for using layouts
//aleXXX 02Nov2000 - rewritten for the Qt 2 layouts
KWinDesktopConfig::KWinDesktopConfig (QWidget * parent, const char *name)
  : KCModule (parent, name)
{
  QBoxLayout *lay = new QVBoxLayout(this, 5);

//CT disabled <i>sine die</i>
//  ElectricBox = new QVButtonGroup(i18n("Active desktop borders"),
//                 this);
//  ElectricBox->setMargin(15);
//
//  enable= new QCheckBox(i18n("Enable &active desktop borders"), ElectricBox);
//  QWhatsThis::add( enable, i18n("If this option is enabled, moving the mouse to a screen border"
//    " will change your desktop. This is e.g. useful if you want to drag windows from one desktop"
//    " to the other.") );
//
//  movepointer = new QCheckBox(i18n("&Move pointer towards center after switch"),
//                              ElectricBox);
//  QWhatsThis::add( movepointer, i18n("If this option is enabled, after switching desktops using"
//    " the active borders feature the mouse pointer will be moved to the middle of the screen. This"
//    " way you don't repeatedly change desktops because the mouse pointer is still on the border"
//    " of the screen.") );
//
//  delays = new KIntNumInput(10, ElectricBox);
//  delays->setRange(0, MAX_EDGE_RES/10, 10, true);
//  delays->setLabel(i18n("&Desktop switch delay:"));
//  QWhatsThis::add( delays, i18n("Here you can set a delay for switching desktops using the active"
//    " borders feature. Desktops will be switched after the mouse has been touching a screen border"
//    " for the specified number of seconds.") );
//
//  connect( enable, SIGNAL(clicked()), this, SLOT(setEBorders()));
//
//  lay->addWidget(ElectricBox);
//
//  // Electric borders are not in kwin yet => disable controls
//  enable->setEnabled(false);
//  movepointer->setEnabled(false);
//  delays->setEnabled(false);


  //CT 15mar98 - add EdgeResistance, BorderAttractor, WindowsAttractor config
  MagicBox = new QVButtonGroup(i18n("Magic borders"), this);
  MagicBox->setMargin(15);

  BrdrSnap = new KIntNumInput(10, MagicBox);
  BrdrSnap->setRange( 0, MAX_BRDR_SNAP);
  BrdrSnap->setLabel(i18n("&Border snap zone:"));
  BrdrSnap->setSuffix(i18n(" pixels"));
  BrdrSnap->setSteps(1,1);
  QWhatsThis::add( BrdrSnap, i18n("Here you can set the snap zone for screen borders, i.e."
    " the 'strength' of the magnetic field which will make windows snap to the border when"
    " moved near it.") );

  WndwSnap = new KIntNumInput(10, MagicBox);
  WndwSnap->setRange( 0, MAX_WNDW_SNAP);
  WndwSnap->setLabel(i18n("&Window snap zone:"));
  WndwSnap->setSuffix( i18n(" pixels"));
  BrdrSnap->setSteps(1,1);
  QWhatsThis::add( WndwSnap, i18n("Here you can set the snap zone for windows, i.e."
    " the 'strength' of the magnetic field which will make windows snap to eachother when"
    " they're moved near another window.") );

  OverlapSnap=new QCheckBox(i18n("Snap windows only when &overlapping"),MagicBox);
  QWhatsThis::add( OverlapSnap, i18n("Here you can set that windows will be only"
    " snapped if you try to overlap them, i.e. they won't be snapped if the windows"
    " comes only near another window or border.") );

  lay->addWidget(MagicBox);
  lay->addStretch();
  load();

  connect( BrdrSnap, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect( WndwSnap, SIGNAL(valueChanged(int)), this, SLOT(slotChanged()));
  connect( OverlapSnap, SIGNAL(clicked()), this, SLOT(slotChanged()));
}


void KWinDesktopConfig::slotChanged()
{
  emit changed(true);
}

void KWinDesktopConfig::setEBorders()
{
    delays->setEnabled(enable->isChecked());
    movepointer->setEnabled(enable->isChecked());
}

bool KWinDesktopConfig::getElectricBorders()
{
    return  enable->isChecked();
}

int KWinDesktopConfig::getElectricBordersDelay()
{
    return delays->value();
}

bool KWinDesktopConfig::getElectricBordersMovePointer()
{
    return movepointer->isChecked();
}

void KWinDesktopConfig::setElectricBordersMovePointer(bool move){

  if(move){
    movepointer->setEnabled(true);
    movepointer->setChecked(true);
  }
  else{
    movepointer->setEnabled(false);
    movepointer->setChecked(false);
  }

  movepointer->setEnabled(enable->isChecked());

}

void KWinDesktopConfig::setElectricBorders(bool b){
    enable->setChecked(b);
    setEBorders();
}

void KWinDesktopConfig::setElectricBordersDelay(int delay)
{
    delays->setValue(delay);
}


int KWinDesktopConfig::getBorderSnapZone() {
  return BrdrSnap->value();
}

void KWinDesktopConfig::setBorderSnapZone(int pxls) {
  BrdrSnap->setValue(pxls);
}

int KWinDesktopConfig::getWindowSnapZone() {
  return WndwSnap->value();
}

void KWinDesktopConfig::setWindowSnapZone(int pxls) {
  WndwSnap->setValue(pxls);
}

void KWinDesktopConfig::load( void )
{
  int v;

  KConfig *config = new KConfig("kwinrc");
  config->setGroup( "Windows" );

/* Electric borders are not in kwin yet (?)
  v = config->readNumEntry(KWM_ELECTRIC_BORDER);
  setElectricBorders(v != -1);

  v = config->readNumEntry(KWM_ELECTRIC_BORDER_DELAY);
  setElectricBordersDelay(v);

  //CT 17mar98 re-allign this reading with the one in kwm  ("on"/"off")
  // matthias: this is obsolete now. Should be fixed in 1.1 with NoWarp, MiddleWarp, FullWarp
  key = config->readEntry(KWM_ELECTRIC_BORDER_MOVE_POINTER);
  if (key == "MiddleWarp")
    setElectricBordersMovePointer(TRUE);
*/
  //CT 15mar98 - magics
  v = config->readNumEntry(KWM_BRDR_SNAP_ZONE, KWM_BRDR_SNAP_ZONE_DEFAULT);
  if (v > MAX_BRDR_SNAP) setBorderSnapZone(MAX_BRDR_SNAP);
  else if (v < 0) setBorderSnapZone (0);
  else setBorderSnapZone(v);

  v = config->readNumEntry(KWM_WNDW_SNAP_ZONE, KWM_WNDW_SNAP_ZONE_DEFAULT);
  if (v > MAX_WNDW_SNAP) setWindowSnapZone(MAX_WNDW_SNAP);
  else if (v < 0) setWindowSnapZone (0);
  else setWindowSnapZone(v);
  //CT ---

  OverlapSnap->setChecked(config->readBoolEntry("SnapOnlyWhenOverlapping",false));

  emit changed(false);
  delete config;
}

void KWinDesktopConfig::save( void )
{
  KConfig *config = new KConfig("kwinrc", false, false);
  config->setGroup( "Windows" );

/* Electric borders are not in kwin yet
  int v = getElectricBordersDelay()>10?80*getElectricBordersDelay():800;
  if (getElectricBorders())
    config->writeEntry(KWM_ELECTRIC_BORDER,v);
  else
    config->writeEntry(KWM_ELECTRIC_BORDER,-1);


  config->writeEntry(KWM_ELECTRIC_BORDER_DELAY,getElectricBordersDelay());

  bv = getElectricBordersMovePointer();
  config->writeEntry(KWM_ELECTRIC_BORDER_MOVE_POINTER,bv?"MiddleWarp":"NoWarp");
*/

  //CT 15mar98 - magics
  config->writeEntry(KWM_BRDR_SNAP_ZONE,getBorderSnapZone());

  config->writeEntry(KWM_WNDW_SNAP_ZONE,getWindowSnapZone());

  config->writeEntry("SnapOnlyWhenOverlapping",OverlapSnap->isChecked());

  config->sync();
  delete config;
}

void KWinDesktopConfig::defaults( void )
{
  setWindowSnapZone(KWM_WNDW_SNAP_ZONE_DEFAULT);
  setBorderSnapZone(KWM_BRDR_SNAP_ZONE_DEFAULT);
}

QString KWinDesktopConfig::quickHelp() const
{
  return i18n("<h1>Borders</h1> Here you can configure two nice features the KDE window manager KWin"
    " offers: <ul><li><em>Active Desktop Borders</em> enable you to switch desktops by moving the mouse pointer"
    " to a screen border.</li><li><em>Magic borders</em> provide sort of a 'magnetic field' which will"
    " make windows snap to other windows or the screen border when moved near to them.</li></ul>"
    " Please note, that changes here only take effect when you are using KWin as your window manager."
    " If you do use a different window manager, check its documentation for how to enable such features.");
}

#include "kwindesktop.moc"
