#ifndef DEFAULTS_H
#define DEFAULTS_H "$Id$"

// Please do not change this value!!  It has been found that it causes
// many bugs since many sites insist at analyzing the userAgent string
// to look for the "Mozilla/4.0" before serving up pages!!!  If you
// dislike this, you can change yours from the control-panel under
// "Browsing/Web/UserAgent." (DA)
#define DEFAULT_USERAGENT_STRING QString("Mozilla/4.0 (compatible; Konqueror/")+KDE_VERSION_STRING+QString("; X11)")

#endif
