#ifndef __konq_baseview_h__
#define __konq_baseview_h__

#include "konqueror.h"

#include <opPart.h>
#include <qwidget.h>

class KonqBaseView : virtual public OPPartIf,
		     virtual public Konqueror::View_skel
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

protected:
  QString m_strURL;
};

#endif
