//
//
// "Desktop Icons Options" Tab for KDesktop configuration
//
// (c) Martin R. Jones 1996
//
// Port to KControl, split from "Misc" Tab.
// (c) David Faure 1998

#ifndef __ROOT_OPTIONS_H
#define __ROOT_OPTIONS_H


#include <qstring.h>

#include <kcmodule.h>

class KConfig;
class QCheckBox;
class QComboBox;

//-----------------------------------------------------------------------------
// The "Desktop Icons Options" Tab contains :
// Show Hidden Files on Desktop
// Which menus for which mouse clicks on the desktop

class KRootOptions : public KCModule
{
        Q_OBJECT
public:
        KRootOptions(KConfig *config, QString group, QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();

private slots:

	void changed();

        
private:

        KConfig *g_pConfig;
	QString groupname;

        void fillMenuCombo( QComboBox * combo );
        QCheckBox *showHiddenBox;
        QComboBox *leftComboBox;
        QComboBox *middleComboBox;
        QComboBox *rightComboBox;
        typedef enum { NOTHING = 0, WINDOWLISTMENU, DESKTOPMENU, APPMENU } menuChoice;
};

#endif // __ROOT_OPTIONS_H
