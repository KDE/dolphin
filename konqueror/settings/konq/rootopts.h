//
//
// "Desktop Icons Options" Tab for KDesktop configuration
//
// (c) Martin R. Jones 1996
//
// Port to KControl, split from "Misc" Tab, Port to KControl2
// (c) David Faure 1998
// Desktop menus, paths
// (c) David Faure 2000

#ifndef __ROOT_OPTIONS_H
#define __ROOT_OPTIONS_H


#include <qstring.h>

#include <kcmodule.h>

class KConfig;
class QCheckBox;
class QComboBox;
class QLineEdit;
namespace KIO { class Job; }

//-----------------------------------------------------------------------------
// The "Desktop Icons Options" Tab contains :
// Show Hidden Files on Desktop
// Which menus for which mouse clicks on the desktop
// The paths for Desktop, Trash, Templates and Autostart

class KRootOptions : public KCModule
{
        Q_OBJECT
public:
        KRootOptions(KConfig *config, QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();

private slots:

	void changed();


private:
        KConfig *g_pConfig;

        // Checkboxes
        QCheckBox *showHiddenBox;
        QCheckBox *VertAlignBox;

        // Combo for the menus
        void fillMenuCombo( QComboBox * combo );
        //QComboBox *leftComboBox;
        QComboBox *middleComboBox;
        QComboBox *rightComboBox;
        typedef enum { NOTHING = 0, WINDOWLISTMENU, DESKTOPMENU, APPMENU } menuChoice;

        // Desktop Paths
        QLineEdit *leDesktop;
        QLineEdit *leTrash;
        QLineEdit *leTemplates;
        QLineEdit *leAutostart;
        void moveDir( QString src, QString dest );
private slots:
        void slotResult( KIO::Job * job );
};

#endif // __ROOT_OPTIONS_H
