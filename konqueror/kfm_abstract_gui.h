#ifndef __kfm_abstract_gui_h__
#define __kfm_abstract_gui_h__

#include "kfmview.h"

class KfmGuiProps;

class KfmAbstractGui
{
public:

  enum MouseMode { DoubleClick, SingleClick };

  KfmGuiProps * props() { return m_Props; }

  virtual void setStatusBarText( const char *_text ) = 0L;
  virtual void setLocationBarURL( const char *_url ) = 0L;
  virtual void setUpURL( const char *_url ) = 0L;

  virtual void addHistory( const char *_url, int _xoffset, int _yoffset ) = 0L;

  virtual void createGUI( const char *_url ) = 0L;

  virtual bool hasUpURL() = 0L;
  virtual bool hasBackHistory() = 0L;
  virtual bool hasForwardHistory() = 0L;
  
protected:
  KfmGuiProps * m_Props;
      
};

#endif
