//
//  Copyright (C) 2004 Stephan Binner <binner@kde.org>
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

#include <dcopclient.h>
#include "progressdialog.h"
#include "kdebug.h"

ProgressDialog::ProgressDialog(QWidget* parent, const QString& caption, const QString& text, int totalSteps)
    : DCOPObject( "ProgressDialog" ), KProgressDialog(parent, 0, caption, text, false)
{
    setAutoClose( false );
    setTotalSteps( totalSteps );
    showCancelButton( false );
}

void ProgressDialog::setTotalSteps( int totalSteps )
{
    progressBar()->setTotalSteps( totalSteps );
    showCancelButton( false );
}

int ProgressDialog::totalSteps()
{
    return progressBar()->totalSteps();
}
    
void ProgressDialog::setProgress( int value )
{
    progressBar()->setProgress( value );
    showCancelButton( false );
}      
      
int ProgressDialog::progress()
{
    return progressBar()->progress();
}

void ProgressDialog::setLabel(const QString& label)
{
    KProgressDialog::setLabel( label );
}

void ProgressDialog::close()
{
    slotClose();
}

#include "progressdialog.moc"
