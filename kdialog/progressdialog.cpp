//
//  Copyright (C) 2004-2005 Stephan Binner <binner@kde.org>
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

#include <dcopclient.h>
#include "progressdialog.h"
#include "kdebug.h"
#include "widgets.h"

ProgressDialog::ProgressDialog(QWidget* parent, const QString& caption, const QString& text, int totalSteps)
    : DCOPObject( "ProgressDialog" ), KProgressDialog(parent, 0, caption, text, false)
{
    setAutoClose( false );
    setTotalSteps( totalSteps );
    showCancelButton( false );
    Widgets::handleXGeometry(this);
}

void ProgressDialog::setTotalSteps( int totalSteps )
{
    progressBar()->setTotalSteps( totalSteps );
    if ( progress()>=totalSteps )
      showCancelButton( false );
}

int ProgressDialog::totalSteps() const
{
    return progressBar()->totalSteps();
}
    
void ProgressDialog::setProgress( int progress )
{
    progressBar()->setProgress( progress );
    if (progress>=totalSteps() )
      showCancelButton( false );
}      
      
int ProgressDialog::progress() const
{
    return progressBar()->progress();
}

void ProgressDialog::setLabel(const QString& label)
{
    KProgressDialog::setLabel( label );
}

void ProgressDialog::showCancelButton( bool show )
{
    setAllowCancel( false );
    KProgressDialog::showCancelButton( show );
}

bool ProgressDialog::wasCancelled() const
{
    return KProgressDialog::wasCancelled();
}   

void ProgressDialog::setAutoClose( bool close )
{
    KProgressDialog::setAutoClose( close );
}      
      
bool ProgressDialog::autoClose() const
{
    return KProgressDialog::autoClose();
}

void ProgressDialog::close()
{
    slotButtonClicked(KDialog::Close);
}

#include "progressdialog.moc"
