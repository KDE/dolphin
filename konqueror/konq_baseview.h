#ifndef __konq_baseview_h__
#define __konq_baseview_h__

#include "konqueror.h"
#include "kbookmarkmenu.h"

#include <opPart.h>
#include <qwidget.h>
#include <qstring.h>

/**
 * The base class for all views in konqueror
 */
class KonqBaseView : virtual public OPPartIf,
		     virtual public Konqueror::View_skel,
                     public KBookmarkOwner
{
public:
  KonqBaseView();
  ~KonqBaseView();
  
  virtual void init();
  virtual void cleanUp();
  
  virtual bool event( const char *event, const CORBA::Any &value );
  virtual bool mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu );
  virtual bool mappingCreateViewToolBar( Konqueror::View::EventCreateViewToolBar viewToolBar );
  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );

  virtual char *url();

  virtual Konqueror::View::HistoryEntry *saveState();
  virtual void restoreState( const Konqueror::View::HistoryEntry &history );

  ////////////////////
  /// Overloaded functions of KBookmarkOwner
  ////////////////////
  /**
   * This function is called if the user selectes a bookmark. You must overload it.
   */
  virtual void openBookmarkURL( const char *_url );
  /**
   * @return the title of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentTitle();
  /**
   * @return the URL of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentURL();
  
protected:
  QString m_strURL;
  CORBA::String_var debug_ViewName;
};

#endif
