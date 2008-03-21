/*
   Copyright (C) 2008 by Sebastian Trueg <trueg at kde.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _NEW_TAG_DIALOG_H_
#define _NEW_TAG_DIALOG_H_

#include <KDialog>
#include "ui_newtagdialog.h"

namespace Nepomuk {
    class Tag;
}

class NewTagDialog : public KDialog, public Ui_NewTagDialog
{
    Q_OBJECT

public:
    ~NewTagDialog();

    static Nepomuk::Tag createTag( QWidget* parent = 0 );

private Q_SLOTS:
    void slotLabelChanged( const QString& text );

private:
    NewTagDialog( QWidget* parent = 0 );
};

#endif
