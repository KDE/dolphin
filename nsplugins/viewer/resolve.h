/*
  Copyright (c) 2000 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
 
  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
 
  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 
*/                                                                            


#define RESOLVE_RETVAL(fname,error)           \
  kdDebug() << "NSPluginInstance::" << endl;  \
                                              \
  if (!_handle)                               \
    return error;                             \
                                              \
  if (!func_ ## fname)                        \
    func_ ## fname = _handle->symbol("NPP_"#fname); \
                                              \
  if (!func_ ## fname)                        \
  {                                           \
    kdDebug() << "Failed: NPP_" << endl;      \
    return error;                             \
  }                                           \
  kdDebug() << "Resolved NPP_" << endl;


#define RESOLVE(fname) RESOLVE_RETVAL(fname, NPERR_GENERIC_ERROR)
#define RESOLVE_VOID(fname) RESOLVE_RETVAL(fname, ;)


#define CHECK(fname,error)                    \
  kdDebug() << "Result of " << endl;          \
  return error;


