//
//
// "Misc Options" Tab for KFM configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998

#ifndef __KFM_MISC_OPTIONS_H
#define __KFM_MISC_OPTIONS_H

#include <qstrlist.h>
#include <qcheckbox.h>
#include <qlineedit.h>

#include <kconfig.h>
#include <kcontrol.h>

extern KConfig *g_pConfig;

//-----------------------------------------------------------------------------
// The "Misc Options" Tab contains :

// Allow per-url settings         (Sven)
// Tree view follows navigation   (David)
// Preferred terminal             (David)
// ... there is room for others :))

class KMiscOptions : public KConfigWidget
{
        Q_OBJECT
public:
        KMiscOptions( QWidget *parent = 0L, const char *name = 0L );
        virtual void loadSettings();
        virtual void saveSettings();
        virtual void applySettings();
        virtual void defaultSettings();
 
private:
        QCheckBox *urlpropsbox;
        QCheckBox *treefollowbox;
        QLineEdit *leTerminal;
        QLineEdit *leEditor;
};

#endif // __KFM_MISC_OPTIONS_H
