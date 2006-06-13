//
//  Copyright (C) 1998-2005 Matthias Hoelzer
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
//  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
//

#include <QLabel>
#include <q3listbox.h>
#include <kvbox.h>

#include "klistboxdialog.h"
#include "klistboxdialog.moc"

#include "klocale.h"

KListBoxDialog::KListBoxDialog(const QString &text, QWidget *parent)
    : KDialog( parent )
{
  setModal(true);
  setButtons( Ok | Cancel );
  enableButtonSeparator(true);

  KVBox *page = new KVBox(this);
  setMainWidget(page);

  label = new QLabel(text, page);
  label->setAlignment(Qt::AlignCenter);

  table = new Q3ListBox(page);
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
