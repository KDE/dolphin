#ifndef __konq_main_h
#define __konq_main_h

#include <kapp.h>

// This is used to know if we are being closed by session management
// or by the user. See KonqMainWindow::~KonqMainWindow.
// Credits to Matthias Ettrich for the idea.
class KonquerorApplication : public KApplication
{
public:
  KonquerorApplication() : KApplication(),
      closed_by_sm( false ) {}

  bool closedByUser() const { return !closed_by_sm; }
  void commitData(QSessionManager& sm) {
    closed_by_sm = true;
    KApplication::commitData( sm );
    closed_by_sm = false;
  }
 
private:
  bool closed_by_sm;
 
};

#endif
