//
//
// "Desktop Icons Options" Tab for KFM configuration
//
// (c) Martin R. Jones 1996
//
// Port to KControl, split from "Misc" Tab.
// (c) David Faure 1998

#ifndef __ROOT_OPTIONS_H
#define __ROOT_OPTIONS_H

#include <kcontrol.h>

class KConfig;
class QCheckBox;

extern KConfig *g_pConfig;

//-----------------------------------------------------------------------------
// The "Desktop Icons Options" Tab contains :
// only Show Hidden Files on Desktop

class KRootOptions : public KConfigWidget
{
        Q_OBJECT
public:
        KRootOptions( QWidget *parent = 0L, const char *name = 0L );
        virtual void loadSettings();
        virtual void saveSettings();
        virtual void applySettings();
        virtual void defaultSettings();
 
private:
        QCheckBox *showHiddenBox;
};

#endif // __ROOT_OPTIONS_H
