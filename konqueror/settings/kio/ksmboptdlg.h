// ksmboptdlg.h - SMB configuration by Nicolas Brodu <brodu@kde.org>
// code adapted from other option dialogs

#ifndef __KSMBOPTDLG_H
#define __KSMBOPTDLG_H

#include <qwidget.h>
#include <qlabel.h>
#include <qlineedit.h>
#include <qhbuttongroup.h>
#include <qradiobutton.h>
#include <qptrlist.h>
#include <qlistbox.h>

#include <kcmodule.h>


class KSMBOptions : public KCModule
{
Q_OBJECT
public:
	KSMBOptions(QWidget *parent = 0);
	~KSMBOptions();

	virtual void load();
	virtual void save();
	virtual void defaults();
	QString quickHelp() const;

private slots:
	void textChanged(const QString&);
	void addClicked();
	void deleteClicked();
	void listboxHighlighted( const QString& );

	void changed();


private:
	// information concerning this dialog
	QLabel *labelInfo1;
	QLabel *labelInfo2;
	QLabel *labelInfoSambaMode;

	// Network related options
	QGroupBox *groupNet;          // Network section group
	QLabel *labelBrowseServer;    // Browse server label
	QLineEdit *editBrowseServer;  // Browse server field
	QLabel *labelBroadcast;       // Broadcast address label
	QLineEdit *editBroadcast;     // Broadcast address field
	QLabel *labelWINS;            // WINS address label
	QLineEdit *editWINS;          // WINS address field
	QLabel *labelOtherOptions, *labelConfFile; // For smb.conf location

	// User related options
	QGroupBox *groupUser;         // User settings group
	QLabel* onserverLA;
	QLineEdit* onserverED;
	QLabel* shareLA;
	QLineEdit* shareED;
	QLabel* loginasLA;
	QLineEdit* loginasED;
	QLabel* passwordLA;
	QLineEdit* passwordED;
	QPushButton* addPB;
	QPushButton* deletePB;
	QLabel* bindingsLA;
	QListBox* bindingsLB;
	int highlighted_item;
	
	// Password policy
	QButtonGroup *groupPassword;
	QRadioButton *radioPolicyAdd;
	QRadioButton *radioPolicyNoStore;
	
	// Bindings management
	static QString build_string(const QString& server, const QString& share, const QString& login, const QString& password);
	class Item {
	public:
		QString server;
		QString share;
		QString login;
		QString password;
		QString text;
		Item(const QString &e, const QString& h, const QString& l, const QString& p);
	};
	friend class Item;
	QPtrList<Item> bindings;
};

#endif // __KSMBOPTDLG_H
