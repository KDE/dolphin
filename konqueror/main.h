#ifndef __main_h__
#define __main_h__

#include "kbookmark.h"
#include "kfmgui.h"

class KfmBookmarkManager : public KBookmarkManager
{
public:
  virtual void editBookmarks( const char *_url );
};

#endif
