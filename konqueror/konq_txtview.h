#ifndef __konq_txtview_h__
#define __konq_txtview_h__

#include "konq_baseview.h"

#include <qmultilineedit.h>

class KonqTxtView : public QMultiLineEdit,
                    public KonqBaseView,
		    virtual public Konqueror::TxtView_skel
{
  Q_OBJECT
  
public:
  KonqTxtView();
  virtual ~KonqTxtView();
  
  virtual bool mappingOpenURL( Konqueror::EventOpenURL eventURL );
  virtual bool mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu );

  virtual void stop();
  virtual char *viewName() { return CORBA::string_dup( "KonquerorTxtView" ); }

  virtual Konqueror::View::HistoryEntry *saveState();
  virtual void restoreState( const Konqueror::View::HistoryEntry &history );

protected slots:
  void slotFinished( int );
  void slotRedirection( int, const char * );
  void slotData( int, const char *, int );
  void slotError( int, int, const char * );

protected:
  virtual void mousePressEvent( QMouseEvent *e );  
    
private:
  int m_jobId;
};

#endif
