#ifndef __konqopenurlrequest_h
#define __konqopenurlrequest_h

#include <qstring.h>

struct KonqOpenURLRequest {

  KonqOpenURLRequest() {}

  KonqOpenURLRequest( const QString & url ) :
    typedURL(url), followMode(false)
    {}

  QString typedURL; // empty if URL wasn't typed manually
  bool followMode; // true if following another view - avoids loops
};

#endif
