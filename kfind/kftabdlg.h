/***********************************************************************
 *
 *  kftabdlg.h
 *
 ***********************************************************************/

#ifndef KFTABDLG_H
#define KFTABDLG_H

#include <qtabwidget.h>  
#include <qcombobox.h>
#include <qvalidator.h>  

class QButtonGroup;
class QPushButton;
class QRadioButton;
class QLabel;
class QCheckBox;
class QLineEdit;
class QString;
class QDate;
class QRegExp;

class KfDirDialog;

class KfindTabWidget: public QTabWidget
{
  Q_OBJECT

public:
  KfindTabWidget(QWidget * parent = 0, const char *name=0, const char *searchPath=0);
  virtual ~KfindTabWidget();
  QString createQuery();      
  void setDefaults();

  void beginSearch();
  void endSearch();
  void loadHistory();
  void saveHistory();

public slots:
  void setFocus() { nameBox->setFocus(); }

private slots:
  void getDirectory();
  void fixLayout();
  void slotSizeBoxChanged(int);

signals:

protected:

private:
  bool isDateValid(); 

  QString date2String(QDate);
  QDate &string2Date(QString, QDate *);

  QWidget *pages[3];
  
  // for first page
  QLabel      *namedL;
  QComboBox   *nameBox;
  QLabel      *lookinL;
  QComboBox   *dirBox;
  QCheckBox   *subdirsCb;
  QPushButton *browseB;

  KfDirDialog *dirselector;

  // for second page
  QButtonGroup *bg[2];
  QRadioButton *rb1[2], *rb2[3];
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
  QCheckBox *caseCb;

  QString _searchPath;
};

class KDigitValidator: public QValidator
{
  Q_OBJECT

public:
  KDigitValidator(QWidget * parent, const char *name = 0 );
  ~KDigitValidator();
  
  QValidator::State validate(QString & input, int &) const;
  
 private:
  QRegExp *r;
};

class KfComboBox: public QComboBox
{
  Q_OBJECT

public:
  KfComboBox(QWidget * parent, const char *name = 0 );
  ~KfComboBox();
  
  void keyPressEvent(QKeyEvent *e);

signals:
  void returnPressed();

};

#endif
