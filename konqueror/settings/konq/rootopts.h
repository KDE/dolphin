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
class QListView;
namespace KIO { class Job; }

//-----------------------------------------------------------------------------
// The "Desktop Icons Options" Tab contains :
// Show Hidden Files on Desktop
// Which menus for which mouse clicks on the desktop
// The paths for Desktop, Trash and Autostart

class KRootOptions : public KCModule
{
        Q_OBJECT
public:
        KRootOptions(KConfig *config, QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();
        virtual QString quickHelp() const;
        friend class KRootOptPreviewItem;

private slots:

	void changed();


private:
        KConfig *g_pConfig;

        // Checkboxes
        QCheckBox *showHiddenBox;
        QCheckBox *VertAlignBox;
        QCheckBox *menuBarBox;
	QCheckBox *vrootBox;
        QListView *previewListView;

        // Combo for the menus
        void fillMenuCombo( QComboBox * combo );
        QComboBox *leftComboBox;
        QComboBox *middleComboBox;
        QComboBox *rightComboBox;
        typedef enum { NOTHING = 0, WINDOWLISTMENU, DESKTOPMENU, APPMENU } menuChoice;

        // Desktop Paths
        QLineEdit *leDesktop;
        QLineEdit *leTrash;
        QLineEdit *leAutostart;
        QLineEdit *leDocument;

        bool moveDir( const KURL & src, const KURL & dest, const QString & type );
        bool m_ok;

private slots:
        void slotResult( KIO::Job * job );
};

#endif // __ROOT_OPTIONS_H
