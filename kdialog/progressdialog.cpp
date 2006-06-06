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

#include "progressdialog.h"
#include "kdebug.h"
#include "widgets.h"
#include <kprogressdialog.h>
#include "kdialogadaptor.h" 

ProgressDialog::ProgressDialog(QWidget* parent, const QString& caption, const QString& text, int totalSteps)
    : KProgressDialog(parent, caption, text, false)
{
    (void)new ProgressDialogAdaptor( this );
    QDBus::sessionBus().registerObject( QLatin1String("/ProgressDialog"), this );
    QDBus::sessionBus().busService()->requestName( "org.kde.kdialog", /*flags=*/0 );
    setAutoClose( false );
    progressBar()->setMaximum( totalSteps );
    showCancelButton( false );
    Widgets::handleXGeometry(this);
}

void ProgressDialog::setMaximum( int totalSteps )
{
    progressBar()->setMaximum( totalSteps );
    if ( progressBar()->value()>=totalSteps )
      showCancelButton( false );
}

int ProgressDialog::maximum() const
{
    return progressBar()->maximum();
}
    
void ProgressDialog::setValue( int progress )
{
    progressBar()->setValue( progress );
    if (progress>=maximum() )
      showCancelButton( false );
}      
      
int ProgressDialog::value() const
{
    return progressBar()->value();
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
