#ifndef __kfm_abstract_gui_h__
#define __kfm_abstract_gui_h__

#include "kfmview.h"

#include <qcolor.h>
#include <qpixmap.h>

class KfmAbstractGui
{
public:
  enum MouseMode { DoubleClick, SingleClick };

  virtual const QColor& bgColor() = 0L;
  virtual const QColor& textColor() = 0L;
  virtual const QColor& linkColor() = 0L;
  virtual const QColor& vLinkColor() = 0L;
  
  virtual const char* stdFontName() = 0L;
  virtual const char* fixedFontName() = 0L;
  virtual int fontSize() = 0L;

  virtual const QPixmap& bgPixmap() = 0L;
  
  virtual MouseMode mouseMode() = 0L;

  virtual bool underlineLink() = 0L;
  virtual bool changeCursor() = 0L;
  virtual bool isShowingDotFiles() = 0L;

  virtual void setStatusBarText( const char *_text ) = 0L;
  virtual void setLocationBarURL( const char *_url ) = 0L;
  virtual void setUpURL( const char *_url ) = 0L;

  virtual void addHistory( const char *_url, int _xoffset, int _yoffset ) = 0L;

  virtual KfmView::ViewMode viewMode() = 0L;

  virtual void createGUI( const char *_url ) = 0L;

  virtual bool hasUpURL() = 0L;
  virtual bool hasBackHistory() = 0L;
  virtual bool hasForwardHistory() = 0L;
};

#endif
