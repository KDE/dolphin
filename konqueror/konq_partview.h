#ifndef __konq_partview_h__
#define __konq_partview_h__

#include "konq_baseview.h"

#include <opFrame.h>

class KonqPartView : public QWidget,
                     public KonqBaseView,
		     virtual public Konqueror::PartView_skel
{
  Q_OBJECT
public:
  KonqPartView();
  virtual ~KonqPartView();

  virtual void init();  
  virtual void cleanUp();

  virtual bool mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu );

  virtual void detachPart();
      
  virtual void setPart( OpenParts::Part_ptr part );
  virtual OpenParts::Part_ptr part();
  
  virtual void stop() {}
  
  virtual char *viewName() { return CORBA::string_dup("KonquerorPartView"); }
  
protected:
  void resizeEvent( QResizeEvent * );

  OpenParts::Part_var m_vPart;
  OPFrame *m_pFrame;  
};		     

#endif
