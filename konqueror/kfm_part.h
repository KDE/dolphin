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

#ifndef __kfm_part_h__
#define __kfm_part_h__

class KfmView;
class KfmPart;

#include "kfm.h"
#include "kfm_abstract_gui.h"

#include <opPart.h>
#include <opMainWindow.h>
#include <opFrame.h>
#include <openparts_ui.h>
#include <opMenu.h>
#include <opToolBar.h>

#include <qwidget.h>

class KfmPart : public QWidget,
		public KfmAbstractGui,
		virtual public OPPartIf,
		virtual public KFM::Part_skel
{
  Q_OBJECT
public:
  KfmPart( QWidget *_parent = 0L );
  virtual ~KfmPart();
  
  virtual void init();
  virtual void cleanUp();

  virtual void bookmarkSelected( CORBA::Long id );
  
  /////////////////////////
  // Overloaded functions from @ref KfmAbstractGUI
  /////////////////////////  

  void setStatusBarText( const char *_text );
  void setLocationBarURL( const char *_url );
  void setUpURL( const char *_url );
  
  void addHistory( const char *_url, int _xoffset, int _yoffset );

  void createGUI( const char *_url );
  
  bool hasUpURL();
  bool hasBackHistory();
  bool hasForwardHistory();

protected:
  virtual void resizeEvent( QResizeEvent * );
  
  virtual bool event( const char* _event, const CORBA::Any& _value );
  virtual bool mappingCreateMenubar( OpenPartsUI::MenuBar_ptr _menubar );

  OpenPartsUI::Menu_var m_vMenuEdit;

  KfmView* m_pView;
};

#endif
