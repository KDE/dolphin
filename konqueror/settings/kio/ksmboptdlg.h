// ksmboptdlg.h - SMB configuration by Nicolas Brodu <brodu@kde.org>
// code adapted from other option dialogs
#ifndef __KSMBOPTDLG_H
#define __KSMBOPTDLG_H

#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>

#include <kcontrol.h>
class KConfig;
 
extern KConfig *g_pConfig;

class KSMBOptions : public KConfigWidget
{
Q_OBJECT
public:
	KSMBOptions(QWidget *parent = 0L, const char *name = 0L);
	~KSMBOptions();

	virtual void loadSettings();
	virtual void saveSettings();
	virtual void applySettings();
	virtual void defaultSettings();

private:
	// information concerning this dialog
	QLabel *labelInfo1;
	QLabel *labelInfo2;

	// Network related options
	QLabel *labelNet;             // Network section label
	QLabel *labelBrowseServer;    // Browse server label
	QLineEdit *editBrowseServer;  // Browse server field
	QLabel *labelBroadcast;       // Broadcast address label
	QLineEdit *editBroadcast;     // Broadcast address field

	// User related options
	QLabel *labelUser;            // User settings label
	QLabel *labelDefaultUser;     // Default user label
	QLineEdit *editDefaultUser;   // Default user field
	QLabel *labelPassword;        // Remember password label (+ warning !)
	QButtonGroup *groupPassword;  // Remember password button group
	// Remember password policy
	QRadioButton *radioPolicyAsk;
	QRadioButton *radioPolicyNever;
	QRadioButton *radioPolicyAlways;
};

#endif // __KSMBOPTDLG_H
