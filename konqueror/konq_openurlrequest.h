#ifndef __konqopenurlrequest_h
#define __konqopenurlrequest_h

#include <qstring.h>

#include <kparts/browserextension.h>

struct KonqOpenURLRequest {

  KonqOpenURLRequest() :
    followMode(false), newTab(false), newTabInFront(false)
    {}

  KonqOpenURLRequest( const QString & url ) :
    typedURL(url), followMode(false), newTab(false), newTabInFront(false)
    {}

  QString typedURL; // empty if URL wasn't typed manually
  bool followMode; // true if following another view - avoids loops
  QString nameFilter; // like *.cpp, extracted from the URL
  bool newTab; // open url in new tab
  bool newTabInFront; // new tab in front or back
  KParts::URLArgs args;
};

#endif
