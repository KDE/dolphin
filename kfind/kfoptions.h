/***********************************************************************
 *
 *  KfOptions.h
 *
 **********************************************************************/

#ifndef KFWITGETS_H
#define KFWIDGETS_H

#include <klocale.h>

class QButtonGroup;
class QPushButton;
class QRadioButton;
class QLabel;
class QComboBox;
class QCheckBox;
class QLineEdit;
class QString;
class QDate;
class QTabDialog;
class QListBox;

#include <qtabdlg.h>

class KfOptions: public QTabDialog
{
  Q_OBJECT
 
public:
  KfOptions( QWidget *parent=0, const char *name=0 );
  //    ~KfOptions();
  
private slots:
  void applyChanges();
  //Slots for first page
  void selectFile();
  void setFileSelecting();
  
  //Slots for second page
  void fillFiletypeDetail(int);

  //Slots for third page
  void fillArchiverDetail(int);

private:
  /// Inserts all pages in the dialog.
  void insertPages();

  /// Store pointers to dialog pages
  QWidget *pages[3]; 
  
  // First page of tab preferences dialog
  QLabel *formatL, *fileL, *kfindfileL;
  QPushButton *browseB;
  QComboBox *formatBox;
  QLineEdit *fileE;
  QRadioButton *kfindfileB, *selectfileB;
  QButtonGroup *bg;
  
  void initFileSelecting(); //initialize first page of dialog

  // Second page of tab preferences dialog
  QListBox *filetypesLBox, *paternsLBox;
  QLabel *typeL, *iconL, *paternsL, *defappsL;
  QLineEdit *typeE, *iconE, *paternsE, *defappsE, *commentE;
  QPushButton *addTypeB, *removeTypeB;

  void fillFiletypesLBox();

  // Third page of tab preferences dialog
  QListBox *archiversLBox, *paternsLBox2;
  QLabel *createL, *addL, *paternsL2;
  QLineEdit *createE, *addE;
  QPushButton *addArchiverB, *removeArchiverB;

  void fillArchiverLBox();
};

#endif
