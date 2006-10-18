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

#ifndef PROGRESSDIALOGIFACE_H
#define PROGRESSDIALOGIFACE_H

#include <dcopobject.h>

class ProgressDialogIface : virtual public DCOPObject
{
    K_DCOP
    k_dcop:

    virtual void setMaximum( int ) =0;
    virtual int maximum() const =0;
    
    virtual void setValue( int ) =0;
    virtual int value() const =0;
    
    virtual void setLabel( const QString& ) =0;
    
    virtual void showCancelButton ( bool ) =0;
    virtual bool wasCancelled() const =0;
    virtual void ignoreCancel() =0;
    
    virtual void setAutoClose( bool ) =0;
    virtual bool autoClose() const =0;    
    virtual void close() =0;
};

#endif // PROGRESSDIALOGIFACE_H
