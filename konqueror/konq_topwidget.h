/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
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

#ifndef __konq_topwidget_h
#define __konq_topwidget_h

#include <qwidget.h>

/**
 * This invisible widget is created on startup and used for session management
 * (this widget is the application "topwidget", hence its name)
 * @see KApplication::setTopWidget
 * In kfm, it was the class Kfm, defined in kfmw.h
 *
 * In Kfm, it has been hacked to contain lots of global methods and variables
 * Please don't do the same mistakes with konqueror !
 * This should remain ONLY the top widget, nothing else.
 */
class KonqTopWidget : public QWidget
{
    Q_OBJECT
public:

  KonqTopWidget();

  ~KonqTopWidget();
    
  /**
   * @return the unique instance of the top widget.
   * Used to call save and shutdown.
   */
  static KonqTopWidget * getKonqTopWidget() { return pKonqTopWidget; }

  /**
   * @return true if the session is being closed 
   *  (@see KonqTopWidget::slotShutDown)
   */
  static bool isGoingDown() { return s_bGoingDown; }

public slots:
  void slotSave();
  void slotShutDown();
  
protected:
  static KonqTopWidget *pKonqTopWidget;
  static bool s_bGoingDown;
};

#endif
