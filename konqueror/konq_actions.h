
#ifndef __konq_actions_h__
#define __konq_actions_h__ $Id$

#include <kaction.h>

class KonqComboAction : public KSelectAction
{
  Q_OBJECT
public:
    KonqComboAction( const QString& text, int accel = 0, QObject* parent = 0, const char* name = 0 );
    KonqComboAction( const QString& text, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqComboAction( const QString& text, const QIconSet& pix, int accel = 0,
	     QObject* parent = 0, const char* name = 0 );
    KonqComboAction( const QString& text, const QIconSet& pix, int accel,
	     QObject* receiver, const char* slot, QObject* parent, const char* name = 0 );
    KonqComboAction( QObject* parent = 0, const char* name = 0 );

    virtual int plug( QWidget *w );

    virtual void unplug( QWidget *w );

    //Konqueror Extension
    void changeItem( const QString &text, int index = -1 );
  
};

#endif
