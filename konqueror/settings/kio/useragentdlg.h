//-----------------------------------------------------------------------------
//
// UserAgent Options
// (c) Kalle Dalheimer 1997
//
// Port to KControl
// (c) David Faure <faure@kde.org> 1998
//
// (C) Dirk Mueller <mueller@kde.org> 2000
//
// (C) Dawit Alemayehu <adawit@kde.org> 2000

#ifndef _USERAGENTDLG_H
#define _USERAGENTDLG_H "$Id$"

#include <qmap.h>
#include <qwidget.h>

#include <kcmodule.h>

class QPushButton;
class QComboBox;
class QLineEdit;
class QListView;
class QLabel;

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
  void activated(const QString&);
  void addClicked();
  void deleteClicked();
  void resetClicked();
  void bindingsSelected();

  void changed();


private:
  bool onlySelectionChange;

  QLabel* onserverLA;
  QLineEdit* onserverED;
  QLabel* loginasLA;
  QComboBox* loginasED;
  QLabel* loginidLA;
  QLineEdit* loginidED;

  QPushButton* addPB;
  QPushButton* deletePB;
  QPushButton* resetPB;

  QLabel* bindingsLA;
  QListView* bindingsLV;

  QPushButton* okPB;
  QPushButton* cancelPB;
  QPushButton* helpPB;

  QMap<QString, QString> aliasMap;
};
#endif
