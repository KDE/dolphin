/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 1999 - 2002 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

// Default values for konqueror/kdesktop settings
// This file is used by konqueror kdesktop, and kcmkonq,
// to share the same defaults

// appearance tab
#define DEFAULT_UNDERLINELINKS true
#define DEFAULT_WORDWRAPTEXT true // kfm-like, sorry Reggie :-)
#define DEFAULT_TEXTHEIGHT 2
#define DEFAULT_TEXTWIDTH 0       // 0 = automatic (font depending)
#define DEFAULT_TEXTWIDTH_MULTICOLUMN 600  // maxwidth, as the iconview has dynamic column width
#define DEFAULT_FILESIZEINBYTES false

#define DEFAULT_RENAMEICONDIRECTLY false

// transparency of blended mimetype icons in textpreview
#define DEFAULT_TEXTPREVIEW_ICONTRANSPARENCY 70

// show hidden files on desktop default
#define DEFAULT_SHOW_HIDDEN_ROOT_ICONS false
#define DEFAULT_VERT_ALIGN true

// Default terminal for "Open Terminal" in konqueror
#define DEFAULT_TERMINAL "konsole"

// Confirmations for deletions
#define DEFAULT_CONFIRMTRASH true
#define DEFAULT_CONFIRMDELETE true
#define DEFAULT_CONFIRMSHRED true
