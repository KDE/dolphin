//-----------------------------------------------------------------------------
//
// UserAgent Options
// (c) Kalle Dalheimer 1997
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998
//
// (C) Dirk Mueller <mueller@kde.org> 2000

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H "$Id$"

#include <qwidget.h>
#include <qlineedit.h>
#include <qlabel.h>

#include <kcmodule.h>

class QPushButton;
class QComboBox;
class QListView;

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
  void addClicked();
  void deleteClicked();
  void bindingsSelected();

  void changed();


private:
  QLabel* onserverLA;
  QLineEdit* onserverED;
  QLabel* loginasLA;
  QComboBox* loginasED;

  QPushButton* addPB;
  QPushButton* deletePB;

  QLabel* bindingsLA;
  QListView* bindingsLV;

  QPushButton* okPB;
  QPushButton* cancelPB;
  QPushButton* helpPB;

  QStringList settingsList;
};


#endif
