/***********************************************************************
 *
 *  kftabdlg.h
 *
 ***********************************************************************/

#ifndef KFTABDLG_H
#define KFTABDLG_H

#include <ktabctl.h>  

class QButtonGroup;
class QPushButton;
class QRadioButton;
class KfDirDialog;
class QLabel;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QString;
class QDate;
class QSize;

class KfindTabDialog: public KTabCtl
{
  Q_OBJECT

public:
  KfindTabDialog(QWidget * parent = 0,const char * name = 0,const char*searchPath=0);
  //    ~KfindTabDialog();
  QString createQuery();      
  QSize sizeHint();
  void setDefaults();

private slots:
  // Slots for first page
  void getDirectory();
  
  // Slots for second page
  void enableEdit(int);
  void disableAllEdit();
  void enableCheckedEdit();
  void isCheckedValid(); 
  
  // Slots for third page
  void checkSize();          

signals:

protected:

private:
  void resizeEvent( QResizeEvent * );
  QString date2String(QDate);
  QDate string2Date(QString);         

  bool modifiedFiles;
  bool betweenDates;
  bool prevMonth;
  bool prevDay;
  
  bool enableSearch;            

  QWidget *pages[3];
  
  // for firts page
  QLabel      *namedL;
  QComboBox   *nameBox;
  QLabel      *lookinL;
  QComboBox   *dirBox;
  QCheckBox   *subdirsCb;
  QPushButton *browseB;

  KfDirDialog *dirselector;

  // for second page
  QButtonGroup *bg[2];
  QRadioButton *rb1[2],*rb2[3];
  QLineEdit *le[4];
  QLabel *andL;
  QLabel *monthL;
  QLabel *dayL;
  
  // for third page
  QLabel *typeL;
  QComboBox *typeBox;
  QLabel *textL;
  QLineEdit * textEdit;
  QLabel *sizeL;
  QComboBox *sizeBox;
  QLineEdit *sizeEdit;
  QLabel *kbL;
};

#endif

 
