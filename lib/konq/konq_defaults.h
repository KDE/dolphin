/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
   Copyright (C) 1999 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

// Default values for konqueror/kdesktop settings
// This file is used by konqueror and kdesktop, and will be used by kcmkonq,
// to share the same defaults

#if 0 // OLD KFM STUFF - we might want to do this also in konq ?
#define ID_STRING_OPEN 1 /* Open */
#define ID_STRING_CD   2 /* Cd */
#define ID_STRING_NEW_VIEW 3 /* New View */
#define ID_STRING_COPY  4 /* Copy */
#define ID_STRING_DELETE 5 /* Delete */
#define ID_STRING_MOVE_TO_TRASH 6 /* Move to Trash */
#define ID_STRING_PASTE 7 /* Paste */
#define ID_STRING_OPEN_WITH 8 /* Open With */
#define ID_STRING_CUT 9 /* Cut */
#define ID_STRING_MOVE 10 /* Move */
#define ID_STRING_PROP 11 /* Properties */
#define ID_STRING_LINK 12 /* Link */
#define ID_STRING_TRASH 13 /* Empty Trash Bin*/
#define ID_STRING_ADD_TO_BOOMARKS 14
#define ID_STRING_SAVE_URL_PROPS 15 /* sven */
#define ID_STRING_SHOW_MENUBAR 16 /* sven */
#define ID_STRING_UP 17 /* sven */
#define ID_STRING_BACK 18 /* sven */
#define ID_STRING_FORWARD 19 /* sven */
#endif

// appearance tab
#define DEFAULT_UNDERLINELINKS true
#define DEFAULT_WORDWRAPTEXT true // kfm-like, sorry Reggie :-)
#define DEFAULT_FILESIZEINBYTES false

// root window icon text transparency default -- stefan@space.twc.de
//#define DEFAULT_TRANSPARENT_ICON_TEXT true

// transparency of blended mimetype icons in textpreview
#define DEFAULT_TEXTPREVIEW_ICONTRANSPARENCY 70

// show hidden files on desktop default
#define DEFAULT_SHOW_HIDDEN_ROOT_ICONS false
#define DEFAULT_VERT_ALIGN true

// Default terminal for Open Terminal and for 'run in terminal'
#define DEFAULT_TERMINAL "konsole"

// Confirmations for deletions
#define DEFAULT_CONFIRMTRASH true
#define DEFAULT_CONFIRMDELETE true
#define DEFAULT_CONFIRMSHRED true

#define DEFAULT_DESKTOP_IMAGEPREVIEW false
#define DEFAULT_DESKTOP_TEXTPREVIEW false

