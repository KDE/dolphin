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

#ifndef __konq_partview_h__
#define __konq_partview_h__

#include "konq_baseview.h"
#include <qwidget.h>

class OPFrame;

class KonqPartView : public QWidget,
                     public KonqBaseView,
		     virtual public Konqueror::PartView_skel
{
  Q_OBJECT
public:
  KonqPartView();
  virtual ~KonqPartView();

  virtual void init();  
  virtual void cleanUp();

  virtual bool mappingFillMenuView( Browser::View::EventFillMenu viewMenu );

  virtual void detachPart();
      
  virtual void setPart( OpenParts::Part_ptr part );
  virtual OpenParts::Part_ptr part();
  
  virtual void stop() {}
  
  virtual CORBA::Long xOffset() { return (CORBA::Long)0; }
  virtual CORBA::Long yOffset() { return (CORBA::Long)0; }
  
  virtual char *viewName() { return CORBA::string_dup("KonquerorPartView"); }
  
protected:
  void resizeEvent( QResizeEvent * );

  OpenParts::Part_var m_vPart;
  OPFrame *m_pFrame;  
};		     

#endif
