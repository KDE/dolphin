#ifndef KONQ_APPLICATION_H
#define KONQ_APPLICATION_H

#include <kapplication.h>

// This is used to know if we are being closed by session management
// or by the user. See KonqMainWindow::~KonqMainWindow.
// Credits to Matthias Ettrich for the idea.
class KonquerorApplication : public KApplication
{
  Q_OBJECT
public:
  KonquerorApplication();

  bool closedByUser() const { return !closed_by_sm; }
  void commitData(QSessionManager& sm) {
    closed_by_sm = true;
    KApplication::commitData( sm );
    closed_by_sm = false;
  }

public slots:
  void slotReparseConfiguration();

private:
  bool closed_by_sm;

};

#endif
