/*
    kshorturifilterhelper.cpp

    This file is part of the KDE project
    Copyright (C) 2002 Lubos Lunak <llunak@suse.cz>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2
    as published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/* Helper for localdomainurifilter for finding out if a host exist */

#ifndef NULL
#define NULL 0
#endif

#include <netdb.h>
#include <stdio.h>

int main( int argc, char* argv[] )
{
    struct hostent* ent;

    if( argc != 2 )
      return 2;

    ent = gethostbyname( argv[ 1 ] );
    if (ent)
      fputs( ent->h_name, stdout );

    return (ent != NULL || h_errno == NO_ADDRESS) ? 0 : 1;
}
