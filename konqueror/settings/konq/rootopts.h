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
#include <kurl.h>
#include <kcmodule.h>

class QPushButton;
class KConfig;
class QCheckBox;
class QComboBox;
class KURLRequester;
class KListView;
namespace KIO { class Job; }

//-----------------------------------------------------------------------------
// The "Desktop Icons Options" Tab contains :
// Show Hidden Files on Desktop
// Which menus for which mouse clicks on the desktop

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
	friend class KRootOptDevicesItem;

private slots:

        void enableChanged();
	void changed();
	void enableDevicesBoxChanged();
	void comboBoxChanged();
	void editButtonPressed();

private:
        KConfig *g_pConfig;

        // Checkboxes
        QCheckBox *iconsEnabledBox;
        QCheckBox *showHiddenBox;
        QCheckBox *menuBarBox;
	QCheckBox *vrootBox;
	
	QCheckBox *enableDevicesBox;
	KListView *devicesListView;
	void fillDevicesListView();
	void saveDevicesListView();
		
        KListView *previewListView;
        // Combo for the menus
        void fillMenuCombo( QComboBox * combo );
        QComboBox *leftComboBox;
        QComboBox *middleComboBox;
        QComboBox *rightComboBox;
        QPushButton *leftEditButton;
        QPushButton *middleEditButton;
        QPushButton *rightEditButton;
        
        typedef enum { NOTHING = 0, WINDOWLISTMENU, DESKTOPMENU, APPMENU } menuChoice;
        bool m_wheelSwitchesWorkspace;
};

//-----------------------------------------------------------------------------
// The "Path" Tab contains :
// The paths for Desktop, Trash and Autostart

class DesktopPathConfig : public KCModule
{
        Q_OBJECT
public:
        DesktopPathConfig(QWidget *parent = 0L, const char *name = 0L );
        virtual void load();
        virtual void save();
        virtual void defaults();
        virtual QString quickHelp() const;

private slots:
	void changed();


private:
        // Desktop Paths
        KURLRequester *urDesktop;
        KURLRequester *urTrash;
        KURLRequester *urAutostart;
        KURLRequester *urDocument;

        bool moveDir( const KURL & src, const KURL & dest, const QString & type );
        bool m_ok;

private slots:
        void slotResult( KIO::Job * job );
};

#endif // __ROOT_OPTIONS_H
