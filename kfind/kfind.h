/***********************************************************************
 *
 *  Kfind.h
 *
 ***********************************************************************/

#ifndef KFIND_H
#define KFIND_H

#include "kfmenu.h"
#include "kftabdlg.h"
#include "kfwin.h"

class QPushButton;

class Kfind: public QWidget
{
  Q_OBJECT

  public:
    Kfind( QWidget * parent = 0 ,const char * name = 0,const char*searchPath = 0);
  //    ~Kfind();

  private slots:
    void startSearch();
    void stopSearch();
    void newSearch();
    void resultSelected(const char *);
    void aboutFind();
    void prefs();

  signals:
    void  haveResults(bool);
    void  resultSelected(bool);

  protected:
    void resizeEvent( QResizeEvent * );
    void timerEvent( QTimerEvent * );
    void message(const char *str);

  private:
    KfindMenu *menu;
    KfindTabDialog *tabDialog;
    QPushButton * bt[3];
    KfindWindow * win;

    int timerID;
    int childPID;
    QString outFile;
    bool doProcess;
};

#endif

 
