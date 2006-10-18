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

#ifndef PROGRESSDIALOG_H
#define PROGRESSDIALOG_H

#include <kprogressdialog.h>
class ProgressDialog : public KProgressDialog
{
    Q_OBJECT

    public:
      ProgressDialog(QWidget* parent = 0, const QString& caption = QString(), const QString& text = QString(), int totalSteps = 100);
      
      void setMaximum( int );
      int maximum() const;
    
      void setValue( int );
      int value() const;
      
      void setLabel(const QString&);
    
      void showCancelButton(bool show);
      bool wasCancelled() const;
      void ignoreCancel();

      void setAutoClose( bool );
      bool autoClose() const;
                  
      void close();
};

#endif // PROGRESSDIALOG_H
