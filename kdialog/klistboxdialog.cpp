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

#include <qlabel.h>
#include <qlistbox.h>
#include <qvbox.h>

#include "klistboxdialog.h"
#include "klistboxdialog.moc"

#include "klocale.h"

KListBoxDialog::KListBoxDialog(QString text, QWidget *parent)
    : KDialogBase( parent, 0, true, QString::null, Ok|Cancel, Ok, true )
{
  QVBox *page = makeVBoxMainWidget();

  label = new QLabel(text, page);
  label->setAlignment(AlignCenter);

  table = new QListBox(page);
  table->setFocus();
}

void KListBoxDialog::insertItem(const QString& item)
{
  table->insertItem(item);
  table->setCurrentItem(0);
}

void KListBoxDialog::setCurrentItem(const QString& item)
{
  for ( int i=0; i < (int) table->count(); i++ ) {
    if ( table->text(i) == item ) {
      table->setCurrentItem(i);
      break;
    }
  }
}

int KListBoxDialog::currentItem()
{
  return table->currentItem();
}
