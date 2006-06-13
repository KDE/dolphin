/**
 *  Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   
 *  Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       
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
 *
 *
 *  Please see the README
 *
 */

/**
 * @file UserInfo-chface: Dialog for choosing a new face for your user.
 * @author Braden MacDonald
 */

#ifndef CHFACEDLG_H
#define CHFACEDLG_H

#include <QObject>
#include <QPixmap>

#include <kdialog.h>
#include <k3iconview.h> // declaration below

enum FacePerm { adminOnly = 1, adminFirst = 2, userFirst = 3, userOnly = 4}; 

class ChFaceDlg : public KDialog
{
  Q_OBJECT
public:


  ChFaceDlg(const QString& picsdirs, QWidget *parent=0, const char *name=0, bool modal=true);


  QPixmap getFaceImage() const 
  {
    if(m_FacesWidget->currentItem())
      return *(m_FacesWidget->currentItem()->pixmap());
    else
      return QPixmap();
  }

private Q_SLOTS:
  void slotFaceWidgetSelectionChanged( Q3IconViewItem *item )
  	{ enableButton( Ok, !item->pixmap()->isNull() ); }

  void slotGetCustomImage();
  //void slotSaveCustomImage();

private:
  void addCustomPixmap( QString imPath, bool saveCopy );

  K3IconView *m_FacesWidget;
};

#endif // CHFACEDLG_H
