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

#ifndef __konq_txtview_h__
#define __konq_txtview_h__

#include "konq_baseview.h"

#include <qmultilineedit.h>

class KonqMainView;

class KonqTxtView : public QMultiLineEdit,
                    public KonqBaseView,
		    virtual public Konqueror::TxtView_skel
{
  Q_OBJECT
  
public:
  KonqTxtView( KonqMainView *mainView = 0L );
  virtual ~KonqTxtView();
  
  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );
  virtual bool mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu );
  virtual bool mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu );

  virtual CORBA::Long xOffset();
  virtual CORBA::Long yOffset();

  virtual void stop();
  virtual char *viewName() { return CORBA::string_dup( "KonquerorTxtView" ); }
  
  virtual void slotSelectAll();
  virtual void slotEdit();

  virtual void print();

protected slots:
  void slotFinished( int );
  void slotRedirection( int, const char * );
  void slotData( int, const char *, int );
  void slotError( int, int, const char * );

protected:
  virtual void mousePressEvent( QMouseEvent *e );

private:
  int m_jobId;
  KonqMainView *m_pMainView;
  int m_iXOffset;
  int m_iYOffset;
};

#endif
