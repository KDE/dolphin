#ifndef __konqopenurlrequest_h
#define __konqopenurlrequest_h

#include <qstring.h>

#include <kparts/browserextension.h>

struct KonqOpenURLRequest {

  KonqOpenURLRequest() :
    followMode(false)
    {}

  KonqOpenURLRequest( const QString & url ) :
    typedURL(url), followMode(false)
    {}

  QString typedURL; // empty if URL wasn't typed manually
  bool followMode; // true if following another view - avoids loops
  QString nameFilter; // like *.cpp, extracted from the URL
  KParts::URLArgs args;
};

#endif
