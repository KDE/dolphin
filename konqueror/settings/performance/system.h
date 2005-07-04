/*
 *  Copyright (c) 2004 Lubos Lunak <l.lunak@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Steet, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _KCM_PERF_SYSTEM_H
#define _KCM_PERF_SYSTEM_H

#include <kcmodule.h>

#include "system_ui.h"

namespace KCMPerformance
{

class SystemWidget
    : public System_ui
    {
    Q_OBJECT
    public:
        SystemWidget( QWidget* parent_P = NULL );
        void load();
        void save();
        void defaults();
    signals:
        void changed();
    };

}  // namespace 

#endif
