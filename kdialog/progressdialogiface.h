#ifndef PROGRESSDIALOGIFACE_H
#define PROGRESSDIALOGIFACE_H

#include <dcopobject.h>

class ProgressDialogIface : virtual public DCOPObject
{
    K_DCOP
    k_dcop:

    virtual void setTotalSteps( int ) =0;
    virtual int totalSteps() =0;
    
    virtual void setValue( int ) =0;
    virtual int value() =0;
    
    virtual void setLabel( const QString& ) =0;
    
    virtual void close() =0;
};

#endif // PROGRESSDIALOGIFACE_H
