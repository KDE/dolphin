#ifndef PROGRESSDIALOGIFACE_H
#define PROGRESSDIALOGIFACE_H

#include <dcopobject.h>

class ProgressDialogIface : virtual public DCOPObject
{
    K_DCOP
    k_dcop:

    virtual void setTotalSteps( int ) =0;
    virtual int totalSteps() const =0;
    
    virtual void setProgress( int ) =0;
    virtual int progress() const =0;
    
    virtual void setLabel( const QString& ) =0;
    
    virtual void showCancelButton ( bool ) =0;
    virtual bool wasCancelled() const =0;
    
    virtual void setAutoClose( bool ) =0;
    virtual bool autoClose() const =0;    
    virtual void close() =0;
};

#endif // PROGRESSDIALOGIFACE_H
