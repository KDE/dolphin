/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

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

// behaviour tab
#define DEFAULT_SINGLECLICK true
#define DEFAULT_AUTOSELECT -1
#define DEFAULT_CHANGECURSOR false
#define DEFAULT_UNDERLINELINKS true

// browser/tree window color defaults -- Bernd
#define HTML_DEFAULT_BG_COLOR Qt::white
#define HTML_DEFAULT_LNK_COLOR Qt::blue
#define HTML_DEFAULT_TXT_COLOR Qt::black
#define HTML_DEFAULT_VLNK_COLOR Qt::magenta

// icon spacing defaults
#define DEFAULT_ICON_SPACING 5

// root window icon text transparency default -- stefan@space.twc.de
//#define DEFAULT_TRANSPARENT_ICON_TEXT true

// show hidden files on desktop default
#define DEFAULT_SHOW_HIDDEN_ROOT_ICONS false

// lets be modern .. -- Bernd
#define DEFAULT_VIEW_FONT "helvetica"
#define DEFAULT_VIEW_FIXED_FONT "courier"
#define DEFAULT_VIEW_FONT_SIZE 12

// the default size of the main window
#define MAINWINDOW_HEIGHT 360
#define MAINWINDOW_WIDTH 540

// Default terminal for Open Terminal and for 'run in terminal'
#define DEFAULT_TERMINAL "konsole"

// Default editor for viewing HTML source
#define DEFAULT_EDITOR "kedit"

// Default UserAgent string (e.g. Konqueror/2.0)
#define DEFAULT_USERAGENT_STRING QString("Konqueror/")+KDE_VERSION_STRING
