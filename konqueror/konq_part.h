/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __konq_part_h__
#define __konq_part_h__

#include <part.h>

class KonqPart : public Part
{
  Q_OBJECT
public:
  KonqPart( Part *parent = 0, const char *name = 0 );
  ~KonqPart();
  
  void setOpenInitialURL( bool open ) { m_bOpenInitialURL = open; }
  
  virtual View *createView( QWidget *parent = 0, const char *name = 0 );
  virtual Shell *createShell();
  
  virtual void paintEverything( QPainter &painter, const QRect &rect,
                                bool transparent = false, View *view = 0 );
				
protected:
  virtual QString configFile() const;
  
private:
  bool m_bOpenInitialURL;  
};

#endif
