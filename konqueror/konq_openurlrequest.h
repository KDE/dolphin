#ifndef __konqopenurlrequest_h
#define __konqopenurlrequest_h

#include <qstringlist.h>

#include <kparts/browserextension.h>

struct KonqOpenURLRequest {

  KonqOpenURLRequest() :
      followMode(false), newTab(false), newTabInFront(false), openAfterCurrentPage( false )
    {}

  KonqOpenURLRequest( const QString & url ) :
    typedURL(url), followMode(false), newTab(false), newTabInFront(false), openAfterCurrentPage(false)
    {}

  QString debug() const {
#ifndef NDEBUG
      QStringList s;
      if ( !args.frameName.isEmpty() )
          s << "frameName=" + args.frameName;
      if ( !nameFilter.isEmpty() )
          s << "nameFilter=" + nameFilter;
      if ( !typedURL.isEmpty() )
          s << "typedURL=" + typedURL;
      if ( followMode )
          s << "followMode";
      if ( newTab )
          s << "newTab";
      if ( newTabInFront )
          s << "newTabInFront";
      if ( openAfterCurrentPage )
          s << "openAfterCurrentPage";
      return "[" + s.join(" ") + "]";
#else
      return QString::null;
#endif
  }

  QString typedURL; // empty if URL wasn't typed manually
  QString nameFilter; // like *.cpp, extracted from the URL
  bool followMode; // true if following another view - avoids loops
  bool newTab; // open url in new tab
  bool newTabInFront; // new tab in front or back
  bool openAfterCurrentPage;
  KParts::URLArgs args;
};

#endif
