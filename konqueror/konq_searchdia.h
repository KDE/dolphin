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

#ifndef __konq_searchdia_h__
#define __konq_searchdia_h__

#include <qdialog.h>
#include <qregexp.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qvbox.h>

class KonqSearchDialog : public QDialog
{
  Q_OBJECT
public:
  KonqSearchDialog( QWidget *parent = 0L );
  
signals:
  void findFirst( const QRegExp &expr );
  void findNext();
  void clear();

protected slots:
  void slotFind();
  void slotClear();

protected:
  virtual void resizeEvent( QResizeEvent * );

private:
  bool m_bFirstSearch;
  
  QPushButton *m_pFindButton;
  QPushButton *m_pClearButton;
  QPushButton *m_pCloseButton;
  
  QLineEdit *m_pLineEdit;
  
  QVBox *m_pVBox;
};

#endif
