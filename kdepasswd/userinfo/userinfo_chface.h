/***************************************************************************
 *   Copyright 2003 Braden MacDonald <bradenm_k@shaw.ca>                   *
 *   Copyright 2003 Ravikiran Rajagopal <ravi@ee.eng.ohio-state.edu>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License (version 2) as   *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 ***************************************************************************/
/**
 * @file UserInfo-chface: Dialog for choosing a new face for your user.
 * @author Braden MacDonald
 */
#ifndef USERINFO_CHFACE_H
#define USERINFO_CHFACE_H

#include <kdialogbase.h>
#include <kiconview.h>

class KUserInfoChFaceDlg : public KDialogBase
{
  Q_OBJECT
public:
  KUserInfoChFaceDlg(const QString& picsdirs, QWidget *parent=0, const char *name=0, bool modal=true);

  QPixmap getFaceImage() const {
    if(m_FacesWidget->currentItem())
      return *(m_FacesWidget->currentItem()->pixmap());
    else
      return QPixmap();
  }

private slots:
  void slotFaceWidgetSelectionChanged( QIconViewItem *item ) { enableButtonOK( !item->pixmap()->isNull() ); }
  void slotGetCustomImage();
  //void slotSaveCustomImage();

private:
  void addCustomPixmap( QString imPath, bool saveCopy );
  KIconView *m_FacesWidget;
};

#endif
