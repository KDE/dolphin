#ifndef __KSPLITTER_H__
#define __KSPLITTER_H__

#include <qsplitter.h>

//Temporary solution until we have something like a KSplitter in the libs or better TT change theirs. 
//Michael

class KSplitter : public QSplitter
{
Q_OBJECT
  public:
    KSplitter( Orientation o, QWidget * parent=0, const char * name=0 )
  : QSplitter(o, parent, name ) {}
   ~KSplitter() {}

    //make these two public
   int idAfter( QWidget* w ){ return QSplitter::idAfter( w ); }

};


#endif
