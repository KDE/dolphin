#ifndef DEFAULTS_H
#define DEFAULTS_H "$Id$"

// browser/tree window color defaults -- Bernd
#define HTML_DEFAULT_BG_COLOR white
#define HTML_DEFAULT_LNK_COLOR red
#define HTML_DEFAULT_TXT_COLOR black
#define HTML_DEFAULT_VLNK_COLOR magenta

// root window grid spacing defaults -- Bernd
#define DEFAULT_GRID_WIDTH 70
#define DEFAULT_GRID_HEIGHT 70
#define DEFAULT_GRID_MAX 150
#define DEFAULT_GRID_MIN 50

// root window icon text transparency default -- stefan@space.twc.de
#define DEFAULT_ROOT_ICONS_STYLE 0
// show hidden files on desktop default
#define DEFAULT_SHOW_HIDDEN_ROOT_ICONS false
//CT root icons foreground/background defaults
#define DEFAULT_ICON_FG white
#define DEFAULT_ICON_BG black
//CT

// lets be modern .. -- Bernd
//#define DEFAULT_VIEW_FONT "helvetica"
//#define DEFAULT_VIEW_FIXED_FONT "courier"

// the default size of the kfm browswer windows
// these are optimized sizes displaying a maximum number
// of icons. -- Bernd
#define KFMGUI_HEIGHT 360
#define KFMGUI_WIDTH 540

// Default terminal for Open Terminal and for 'run in terminal'
#define DEFAULT_TERMINAL "konsole"

// Default editor for "View Document/Frame Source"
#define DEFAULT_EDITOR "kedit"

// Default UserAgent string (e.g. Konqueror/1.1)
#define DEFAULT_USERAGENT_STRING QString("Konqueror/")+KDE_VERSION_STRING

#endif
