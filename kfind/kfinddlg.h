/***********************************************************************
 *
 *  KfindDlg.h
 *
 ***********************************************************************/

#ifndef KFINDDLG_H
#define KFINDDLG_H

#include <kdialogbase.h>

class QString;

class KQuery;
class KURL;
class KFileItem;
class KfindTabWidget;
class KfindWindow;
class KStatusBar;
class KAboutApplication;

class KfindDlg: public KDialogBase
{
Q_OBJECT

public:
  KfindDlg(const KURL & url, QWidget * parent = 0, const char * name = 0);
  ~KfindDlg();
  void copySelection();

  void setStatusMsg(const QString &);
  void setProgressMsg(const QString &);

public slots:
  void startSearch();
  void stopSearch();
  void newSearch();
  void addFile(const KFileItem* item, const QString& matchingLine);
  void setFocus();
  void slotResult(int);
//  void slotSearchDone();
  void  about ();

signals:
  void haveResults(bool);
  void resultSelected(bool);

private:
  void closeEvent(QCloseEvent *);
  KfindTabWidget *tabWidget;
  KfindWindow * win;
  KAboutApplication * aboutWin;

  bool isResultReported;
  KQuery *query;
  KStatusBar *mStatusBar;
};

#endif


