/***********************************************************************
 *
 *  kftabdlg.cpp
 *
 **********************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <qobjectlist.h>        
#include <qapplication.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qstring.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qdir.h>
#include <qregexp.h>
#include <qmessagebox.h>
#include <qlist.h>
#include <qsize.h>
#include <qvalidator.h>
       
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

#include "kfdird.h"      
#include "kftypes.h"
#include "kftabdlg.h"

#define FIND_PROGRAM "find"
#define SPECIAL_TYPES 7

extern QList<KfFileType> *types;

KfindTabWidget::KfindTabWidget(QWidget *parent, const char *name, 
			       const char *searchPath)
  : QTabWidget( parent, name )
{
    _searchPath = searchPath;
    
    // This validator will be used for all numeric edit fields
    KDigitValidator *digitV = new KDigitValidator(this);

    // ************ Page One ************

    pages[0] = new QWidget( this, "page1" );
    
    nameBox    = new KfComboBox(pages[0], "combo1");
    namedL     = new QLabel(nameBox, i18n("&Named:"), pages[0], "named");
    dirBox     = new KfComboBox(pages[0], "combo2");
    lookinL    = new QLabel(dirBox, i18n("&Look in:"), pages[0], "named");
    subdirsCb  = new QCheckBox(i18n("Include &subfolders"), pages[0]);
    browseB    = new QPushButton(i18n("&Browse ..."), pages[0]);

    // Setup
    
    nameBox->setInsertionPolicy (QComboBox::AtTop);
    dirBox ->setInsertionPolicy (QComboBox::AtTop);
    subdirsCb->setChecked ( TRUE );
    
    // Layout
    
    QGridLayout *grid = new QGridLayout( pages[0], 3, 3, 15, 10 );
    grid->addWidget( namedL, 0, 0 );
    grid->addMultiCellWidget( nameBox, 0, 0, 1, 2 );
    grid->addWidget( lookinL, 1, 0 );
    grid->addWidget( dirBox, 1, 1 );
    grid->addWidget( browseB, 1, 2);
    grid->addWidget( subdirsCb, 2, 1);
    grid->setColStretch(1,1);
    grid->activate();
    
    // Signals

    connect( browseB, SIGNAL(clicked()),
             this, SLOT(getDirectory()) );   
    connect( nameBox, SIGNAL(returnPressed()),
    	     parent, SLOT(startSearch()) );
    connect( dirBox, SIGNAL(returnPressed()),
    	     parent, SLOT(startSearch()) );
    
    addTab( pages[0], i18n(" Name& Location ") );
    
    // ************ Page Two

    pages[1] = new QWidget( this, "page2" );

    rb1[0] = new QRadioButton(i18n("&All files"), pages[1]);
    rb1[1] = new QRadioButton(i18n("Find all files created or &modified:"), pages[1]);
    bg[0]  = new QButtonGroup();
    bg[1]  = new QButtonGroup();
    rb2[0] = new QRadioButton(i18n("&between"), pages[1] );
    rb2[1] = new QRadioButton(i18n("during the previou&s"), pages[1] );
    rb2[2] = new QRadioButton(i18n("&during the previous"), pages[1] );
    andL   = new QLabel(i18n("and"), pages[1], "and");
    monthL = new QLabel(i18n("month(s)"), pages[1], "months");
    dayL   = new QLabel(i18n("day(s)"), pages[1], "days");
    le[0]  = new QLineEdit(pages[1], "lineEdit1" );
    le[1]  = new QLineEdit(pages[1], "lineEdit2" );
    le[2]  = new QLineEdit(pages[1], "lineEdit3" );
    le[3]  = new QLineEdit(pages[1], "lineEdit4" );

    // Setup 
    
    le[0] ->setText(date2String(QDate(1980,1,1)));
    le[1] ->setText(date2String(QDate::currentDate()));
    
    le[2] ->setText("1");
    le[2] ->setValidator(digitV);
    
    le[3] ->setText("1");
    le[3] ->setValidator(digitV);
    
    rb1[0]->setChecked (TRUE);

    bg[0]->insert( rb1[0] );
    bg[0]->insert( rb1[1] );

    bg[1]->insert( rb2[0] );
    bg[1]->insert( rb2[1] );
    bg[1]->insert( rb2[2] );

    le[0]->setMaxLength(10);
    le[1]->setMaxLength(10);
    le[2]->setMaxLength(3);
    le[3]->setMaxLength(3);
  
    // Layout 
    
    int tmp = le[0]->fontMetrics().width(" 00/00/0000 ");
    le[0]->setMinimumSize(tmp, le[0]->sizeHint().height());
    le[1]->setMinimumSize(tmp, le[1]->sizeHint().height());
    tmp = le[2]->fontMetrics().width(" 000 ");
    le[2]->setMinimumSize(tmp, le[2]->sizeHint().height());
    le[3]->setMinimumSize(tmp, le[3]->sizeHint().height());
    
    QGridLayout *grid1 = new QGridLayout( pages[1], 5,  6, 10, 4 );
    grid1->addMultiCellWidget(rb1[0], 0, 0, 0, 6 );
    grid1->addMultiCellWidget(rb1[1], 1, 1, 0, 6 );
    grid1->addColSpacing(0, 15);
    grid1->addWidget(rb2[0], 2, 1 );
    grid1->addWidget(le[0], 2, 2 );
    grid1->addWidget(andL, 2, 3 );
    grid1->addMultiCellWidget( le[1], 2, 2, 4, 5 );
    grid1->addMultiCellWidget( rb2[1], 3, 3, 1, 3 );
    grid1->addWidget(le[2], 3, 4 );
    grid1->addWidget(monthL, 3, 5 );
    grid1->addMultiCellWidget( rb2[2], 4, 4, 1, 3 );
    grid1->addWidget(le[3], 4, 4 );
    grid1->addWidget(dayL, 4, 5 );
    grid1->setColStretch(6, 1);
    grid1->activate();
    
    // Connect

    connect( bg[0],  SIGNAL(clicked(int)),
             this,   SLOT(fixLayout()) );
    connect( bg[1],  SIGNAL(clicked(int)),
             this,   SLOT(fixLayout()) );
    for(int i=0; i<4; i++)
      connect( le[i],  SIGNAL(returnPressed()),
	       parent, SLOT(startSearch()) );
    
    addTab( pages[1], i18n(" Date Modified ") );

    // ************ Page Three
    
    pages[2] = new QWidget( this, "page3" );

    typeBox =new QComboBox(FALSE, pages[2], "typeBox");
    typeL   =new QLabel(typeBox, i18n("Of &type:"), pages[2], "type");
    textEdit=new QLineEdit(pages[2], "textEdit" );
    textL   =new QLabel(textEdit, i18n("&Containing Text:"), pages[2], "text");
    sizeBox =new QComboBox(FALSE, pages[2], "sizeBox");
    sizeL   =new QLabel(sizeBox,i18n("&Size is:"), pages[2],"size");
    sizeEdit=new QLineEdit(pages[2], "sizeEdit" );
    kbL     =new QLabel(i18n("KB"), pages[2], "kb");
    caseCb  =new QCheckBox(i18n("Case S&ensitive"), pages[2]);
    
    // Setup 

    KfFileType *typ;

    typeBox->insertItem(i18n("All Files and Folders"));
    typeBox->insertItem(i18n("Files"));
    typeBox->insertItem(i18n("Folders"));
    typeBox->insertItem(i18n("Symbolic links"));
    typeBox->insertItem(i18n("Special files (sockets, device files...)"));
    typeBox->insertItem(i18n("Executable files"));
    typeBox->insertItem(i18n("SUID executable files"));

    for ( typ = types->first(); typ != 0L; typ = types->next() )
      if (typ->getComment("") != "")
	typeBox->insertItem(typ->getComment(""));
      else
	typeBox->insertItem(typ->getName());
    
    sizeBox ->insertItem( i18n("(none)") );
    sizeBox ->insertItem( i18n("At Least") );
    sizeBox ->insertItem( i18n("At Most") );

    sizeEdit->setText("1");
    sizeEdit->setMaxLength(5);
    sizeEdit->setValidator(digitV);
    
    // Connect
    connect( textEdit,  SIGNAL(returnPressed()),
    	     parent, SLOT(startSearch()) );
    connect( sizeBox, SIGNAL(highlighted(int)),
	     this, SLOT(slotSizeBoxChanged(int)));
    
    // Layout
    tmp = sizeEdit->fontMetrics().width(" 00000 ");
    sizeEdit->setMinimumSize(tmp, sizeEdit->sizeHint().height());
    
    QGridLayout *grid2 = new QGridLayout( pages[2], 3, 6, 15, 10 );
    grid2->addWidget( typeL, 0, 0 );
    grid2->addWidget( textL, 1, 0 );
    grid2->addWidget( sizeL, 2, 0 );
    grid2->addMultiCellWidget( typeBox, 0, 0, 1, 6 );
    grid2->addMultiCellWidget( textEdit, 1, 1, 1, 6 );
    grid2->addWidget( sizeBox, 2, 1 );
    grid2->addWidget( sizeEdit, 2, 2 );
    grid2->addWidget( kbL, 2, 3 );
    grid2->addColSpacing(4, 5);
    grid2->addWidget( caseCb, 2, 5 );
    grid2->setColStretch(6,1);
    grid2->activate();

    addTab( pages[2], i18n(" Advanced ") );  
    
    fixLayout();
    loadHistory();
}

KfindTabWidget::~KfindTabWidget()
{
  delete pages[0];
  delete pages[1];
  delete pages[2];
}

static void save_pattern(QComboBox *obj, const QString new_item,
			 const char *group, const char *entry) 
{ 
  int i;
  for(i=0; i<obj->count(); i++)
    if(new_item == obj->text(i))
      break;
  
  // If we could not finish the loop item already exists
  // Nothing to save
  if(i < obj->count())
    return;
  
  // New item. Add it to the combo and save
  obj->insertItem(new_item);
  
  // QComboBox allows insertion of items more than specified by
  // maxCount() (QT bug?). This API call will truncate list if needed.
  obj->setMaxCount(15);

  QStrList sl;
  for(i=0; i<obj->count(); i++)
    sl.append(obj->text(i).ascii());
  
  KConfig *conf = kapp->getConfig();
  conf->setGroup(group);
  conf->writeEntry(entry, sl, ',');
}

void KfindTabWidget::saveHistory() 
{
  save_pattern(nameBox, nameBox->currentText(), "History", "Patterns");
  save_pattern(dirBox, dirBox->currentText(), "History", "Directories");
}

void KfindTabWidget::loadHistory() 
{
  // Load pattern history
  KConfig *conf = kapp->getConfig();
  QStrList sl;
  conf->setGroup("History");
  if(conf->readListEntry("Patterns", sl, ','))
    nameBox->insertStrList(&sl);
  else
    nameBox->insertItem("*");
  
  if(conf->readListEntry("Directories", sl, ',')) {
    dirBox->insertStrList(&sl);
    // If the _searchPath already exists in the list we do not 
    // want to add it again
    int indx = sl.find(_searchPath);
    if(indx == -1)
      dirBox->insertItem(_searchPath);
    else
      dirBox->setCurrentItem(indx);
  }
  else {
    QDir m_dir("/lib");
    dirBox ->insertItem( _searchPath );
    dirBox ->insertItem( "/" );
    dirBox ->insertItem( "/usr" );
    if (m_dir.exists())
      dirBox ->insertItem( "/lib" );
    dirBox ->insertItem( "/home" );
    dirBox ->insertItem( "/etc" );
    dirBox ->insertItem( "/var" );
    dirBox ->insertItem( "/mnt" );
  }
}

void KfindTabWidget::slotSizeBoxChanged(int index) 
{
  sizeEdit->setEnabled((bool)(index != 0));
}

/*
  Disable/enables the widget by disabling/enabling all pages 
  and all children on the current page
*/
void KfindTabWidget::setEnabled(bool enabled) 
{
  QWidget *curPage = currentPage();
  // We need to change all but not the current page.
  // This will preserve current page to be current
  for(int i=0; i<3; i++) 
    if(pages[i] != curPage)
      setTabEnabled(pages[i], enabled);
  
  // Disable the current one now
  setTabEnabled(curPage, enabled);
  
  // Diable all children but QGridLayout itself
  const QObjectList *list = curPage->children();
  QObjectListIt it( *list );
  QObject * obj;
  while ( (obj=it.current()) != 0 ) {
    ++it;
    if(!obj->isA("QGridLayout"))
      ((QWidget*)obj)->setEnabled( enabled );
  }
  
  // If we enable the widget we want to restore 
  // disabled/enabled layout
  if(enabled)
    fixLayout();
}

void KfindTabWidget::setDefaults()
  {
    le[0] ->setText(date2String(QDate(1980,1,1)));
    le[1] ->setText(date2String(QDate::currentDate()));
    le[2] ->setText("1");
    le[3] ->setText("1");

    typeBox ->setCurrentItem(0);
    sizeBox ->setCurrentItem(0);
    sizeEdit->setText("1");
  }

/*
  Checks if dates are correct and popups a error box 
  if they are not.
*/
bool KfindTabWidget::isDateValid()
{
  if(!(rb1[1]->isChecked() &&
       rb2[0]->isChecked()))
    // "Between dates" check box is not checked, nothing to check
    return TRUE;
  
  // If we can not parse either of the dates or 
  // "from" date is bigger than "to" date return FALSE.
  QDate hi1, hi2;
  if ( string2Date(le[0]->text(), &hi1).isNull() ||
       string2Date(le[1]->text(), &hi2).isNull() || 
       hi1 > hi2) {
    QMessageBox mb(this,"message box");
    mb.setText( i18n("The date is not valid!!"));
    mb.show();
    return FALSE;
  }
  
  return TRUE;
}

QString KfindTabWidget::createQuery() {
  // If some of the dates are invalid, return NULL
  if(!isDateValid())
    return NULL;
  
  QString str,pom;
  int month;
  char *type;
  
  str = FIND_PROGRAM;
  str += " ";
  
  str += dirBox->currentText();
  
  QString str1;
  str1 += " \"(\" -name \"";
  
  if(nameBox->currentText().isEmpty())
    str1 += "*";
  else
    str1 += nameBox->currentText();
  str1 += "\" \")\"";
  str1 += " ";
  
  switch(typeBox->currentItem()) {
  case 0: // all files
    break;
    
  case 1: // files
    str1 += "-type f";
    break;
    
  case 2: // folders
    str1 += "-type d";
    break;
    
  case 3: // symlink
    str1 += "-type l";
    break;
    
  case 4: // special file
    str1 += "-type p -or -type s -or -type b or -type c";
    break;
    
  case 5: // executables
    str1 += "-perm +111 -type f";
    break;
    
  case 6: // suid binaries
    str1 += "-perm +6000 -type f";
    break;
    
  default: 
    str1 = "";
    KfFileType *typ;
    
    typ = types->first();
    for (int i=SPECIAL_TYPES; i<typeBox->currentItem(); i++ )
      typ = types->next();
    
    QStrList& pats = typ->getPattern();
    bool firstpattern = FALSE;
    str += " \"(\" ";
    for (QString pattern=pats.first(); pattern!=0L; 
	 pattern=pats.next()) {
      if (!firstpattern) {
	str += " -name ";
	firstpattern=TRUE;
      }
      else
	str += " -o -name ";
      
      if ( pattern.find("*",0) == 0 ) {
	str += nameBox->text(nameBox->currentItem());
	str += "\"" + pattern + "\"";
      }
      else {
	str += "\"" + pattern + "\"";
	str += nameBox->text(nameBox->currentItem());
      }
    }                                             
    str += " \")\"";
  }
    
  str += str1;
  
  if (!subdirsCb->isChecked())
    str.append(" -maxdepth 1 ");
  
  if (rb1[1]->isChecked()) { // Modified
    if (rb2[0]->isChecked()) { // Between dates
      QDate q1, q2;
      str.append(pom.sprintf(" -daystart -mtime -%d -mtime +%d",
			     (string2Date(le[0]->text(),&q1)).daysTo(QDate::currentDate()),
			     (string2Date(le[1]->text(),&q2)).daysTo(QDate::currentDate()) ));
    }
    else
      if (rb2[1]->isChecked()) { // Previous mounth
	sscanf(le[2]->text().ascii(),"%d",&month);
	str.append(pom = QString(" -daystart -mtime -%1 ")
		   .arg((int)(month*30.416667)));
      }
      else
	if (rb2[2]->isChecked()) // Previous day
	  str.append(pom = QString(" -daystart -mtime -%1").arg(le[3]->text()));
    
    
    if (sizeBox->currentItem() !=  0) {
      switch(sizeBox->currentItem()) {
      case 1: 
	type=(char *)(sizeEdit->text().toInt()==0 ? "" : "+");
	break;
      case 2: 
	type=(char *)(sizeEdit->text().toInt()==0? "" : "-");
	break;
      default: 
	type=(char *)(sizeEdit->text().toInt()==0? "" : " ");
      }
      str.append(pom = QString(" -size  %1%2k ").arg(type).arg(sizeEdit->text()));
    }
  }
  
  if(!textEdit->text().isEmpty()) {
    str += "|xargs egrep -l";
    if(caseCb->isChecked())
      str += " \"";
    else
      str += " -i \"";
    str += textEdit->text();
    str += "\"";
  }
  
  kdebug(KDEBUG_INFO, 1903, "QUERY=%s\n", str.ascii());    
  
  return(str);
}        

 
QString KfindTabWidget::date2String(QDate date) {
  QString str;
  
  str.sprintf("%.2d/%.2d/%4d",date.day(),date.month(),date.year());
  return(str);
}

QDate &KfindTabWidget::string2Date(QString str, QDate *qd) {   
  int year,month,day;
  
  // If we can not scan exactly 3 integers do not try to parse
  if(sscanf(str.ascii(),"%2d/%2d/%4d",&day,&month,&year) == 3)
    qd->setYMD(year, month, day);

  return *qd; 
}

void  KfindTabWidget::getDirectory() {
  QString result;
  
  dirselector = new KfDirDialog(dirBox->text(dirBox->currentItem()),
				this,"dirselector",TRUE);
  CHECK_PTR(dirselector);
  
  if ( dirselector->exec() == QDialog::Accepted )
    result = dirselector->selectedDir();
  delete dirselector;
  
  if (!result.isNull()) {
    //printf("Dir: %s\n",result.ascii());
    dirBox->insertItem(result,0);
    dirBox->setCurrentItem(0);
  }
}

void KfindTabWidget::beginSearch() 
{
  saveHistory();
  setEnabled( FALSE );
}

void KfindTabWidget::endSearch() 
{
  setEnabled( TRUE );
}

/*
  Disables/enables all edit fields depending on their 
  respective check buttons.
*/
void KfindTabWidget::fixLayout() 
{
  int i;
  // If "All files" is checked - disable all edits 
  // and second radio group on page two
  
  if(rb1[0]->isChecked())  { 
    for(i=0; i<4; i++)
      le[i]->setEnabled(FALSE);
    
    for(i=0; i<3; i++)
      rb2[i]->setEnabled(FALSE);
  }
  else {
    for(i=0; i<3; i++)
      rb2[i]->setEnabled(TRUE);
    
    le[0]->setEnabled(rb2[0]->isChecked());
    le[1]->setEnabled(rb2[0]->isChecked());
    le[2]->setEnabled(rb2[1]->isChecked());
    le[3]->setEnabled(rb2[2]->isChecked());
  }
  
  // Size box on page three
  sizeEdit->setEnabled(sizeBox->currentItem() != 0);
}

/**
   Digit validator. Allows only digits to be typed.
**/
KDigitValidator::KDigitValidator( QWidget * parent, const char *name )
  : QValidator( parent, name )
{
  r = new QRegExp("^[0-9]*$");
}

KDigitValidator::~KDigitValidator()
{}
 
QValidator::State KDigitValidator::validate( QString & input, int & ) const
{
  if (r->match(input) < 0) {
    // Beep on user if he enters non-digit
    QApplication::beep();
    return QValidator::Invalid;
  }
  else
    return QValidator::Acceptable;
}

/**
   Special editable ComboBox. Invokes search if return key is pressed 
   and _does not_ save newly typed item in its list.
**/
KfComboBox::KfComboBox(QWidget * parent, const char *name)
  : QComboBox( TRUE, parent, name )
{}

KfComboBox::~KfComboBox()
{}

void KfComboBox::keyPressEvent(QKeyEvent *e)
{
  if(e->key() == Key_Return)
    emit returnPressed();
  else
    QComboBox::keyPressEvent(e);
}
