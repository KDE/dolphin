//-----------------------------------------------------------------------------
//
// UserAgent Options
// (c) Kalle Dalheimer 1997
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H

#include <qdialog.h>
#include <qstring.h>
#include <qwidget.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlistbox.h>

#include <kcontrol.h>
#include <kconfig.h>

extern KConfig *g_pConfig;

class UserAgentOptions : public KConfigWidget
{
  Q_OBJECT

public:
  UserAgentOptions ( QWidget * parent = 0L, const char * name = 0L) ;
  ~UserAgentOptions();

  virtual void loadSettings();
  virtual void saveSettings();
  virtual void applySettings();
  virtual void defaultSettings();
  
private slots:
  void textChanged(const QString&);
//  void returnPressed();
  void addClicked();
  void deleteClicked();
  void listboxHighlighted( const QString& );
  
private:
  QLabel* onserverLA;
  QLineEdit* onserverED;
  QLabel* loginasLA;
  QLineEdit* loginasED;

  QPushButton* addPB;
  QPushButton* deletePB;
  
  QLabel* bindingsLA;
  QListBox* bindingsLB;

  QPushButton* okPB;
  QPushButton* cancelPB;
  QPushButton* helpPB;

  QStringList settingsList;

  int highlighted_item;
};


#endif
