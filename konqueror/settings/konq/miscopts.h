//
//
// "Misc Options" Tab for Konqueror configuration
//
// (c) Sven Radej 1998
// (c) David Faure 1998-2000

#ifndef __MISC_OPTIONS_H
#define __MISC_OPTIONS_H

#include <kcmodule.h>


//-----------------------------------------------------------------------------
// The "Misc Options" Tab contains :

// Preferred terminal             (David)
// Network operations in a single window (David)
// ... there is room for others :))


#include <qstring.h>
class KConfig;
class QCheckBox;
class QLineEdit;

class KMiscOptions : public KCModule
{
        Q_OBJECT
public:
        KMiscOptions(KConfig *config, QString group, QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();

private slots:

	void changed();

private:

        KConfig *g_pConfig;
	QString groupname;

        QLineEdit *leTerminal;
        QCheckBox *cbListProgress;
};

#endif
