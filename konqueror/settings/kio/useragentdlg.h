//-----------------------------------------------------------------------------
//
// UserAgent Options
// (c) Kalle Dalheimer 1997
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H "$Id$"

#include <qdialog.h>
#include <qstring.h>
#include <qwidget.h>
#include <qlineedit.h>
#include <qlabel.h>
#include <qpushbutton.h>
#include <qlistbox.h>

#include <kcmodule.h>


class UserAgentOptions : public KCModule
{
  Q_OBJECT

public:
  UserAgentOptions ( QWidget * parent = 0L, const char * name = 0L) ;
  ~UserAgentOptions();

  virtual void load();
  virtual void save();
  virtual void defaults();

  QString quickHelp() const;
private slots:
  void textChanged(const QString&);
//  void returnPressed();
  void addClicked();
  void deleteClicked();
  void listboxHighlighted( const QString& );
  
  void changed();


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
