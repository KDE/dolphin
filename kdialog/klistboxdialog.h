//
//  Copyright (C) 1998 Matthias Hoelzer
//  email:  hoelzer@physik.uni-wuerzburg.de
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the7 implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//


#ifndef _KLISTBOXDIALOG_H_
#define _KLISTBOXDIALOG_H_

#include <kdialogbase.h>

class KListBoxDialog : public KDialogBase
{
  Q_OBJECT

public:

  KListBoxDialog(QString text, QWidget *parent=0);
  ~KListBoxDialog() {};

  QListBox &getTable() { return *table; };

  void insertItem( const QString& text );
  void setCurrentItem ( const QString& text );
  int currentItem();

protected:

  QListBox *table;
  QLabel *label;

};


#endif
