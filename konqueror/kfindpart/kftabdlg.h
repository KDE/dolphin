/***********************************************************************
 *
 *  kftabdlg.h
 *
 ***********************************************************************/

#ifndef KFTABDLG_H
#define KFTABDLG_H

#include <qtabwidget.h>
#include <qcombobox.h>
#include <qvalidator.h> // for KDigitValidator

#include <kurl.h>
#include <kmimetype.h>

class QButtonGroup;
class QPushButton;
class QRadioButton;
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
  KfindTabWidget(QWidget * parent = 0, const char *name=0);
  virtual ~KfindTabWidget();
  void initMimeTypes();
  void setQuery(class KQuery * query);
  void setDefaults();

  void beginSearch();
  void endSearch();
  void loadHistory();
  void saveHistory();

  void setURL( const KURL & url );

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

  QString date2String(const QDate &);
  QDate &string2Date(const QString &, QDate * );

  QWidget *pages[3];

  // for first page
  QComboBox   *nameBox;
  QComboBox   *dirBox;
  QCheckBox   *subdirsCb;
  QPushButton *browseB;

  KfDirDialog *dirselector;

  // for second page
  QButtonGroup *bg[2];
  QRadioButton *rb1[2], *rb2[3];
  QLineEdit *le[4];

  // for third page
  QComboBox *typeBox;
  QLineEdit * textEdit;
  QComboBox *sizeBox;
  QLineEdit *sizeEdit;
  QCheckBox *caseCb;

  KURL m_url;

  KMimeType::List m_types;
};

class KDigitValidator : public QValidator
{
  Q_OBJECT

public:
  KDigitValidator(QWidget * parent, const char *name = 0 );
  ~KDigitValidator();

  QValidator::State validate(QString & input, int &) const;

 private:
  QRegExp *r;
};

#endif
