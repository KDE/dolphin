/***********************************************************************
 *
 *  kftabdlg.cpp
 *
 **********************************************************************/
#include <string.h>
#include <stdlib.h>

#include <qapplication.h>
#include <qtabdialog.h>
#include <qmultilinedit.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <stdio.h>
#include <qstring.h>
#include <qfont.h>
#include <qtooltip.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qdir.h>
#include <qregexp.h>
#include <qdatetime.h>
#include <qmessagebox.h>
#include <qlist.h>
#include <qsize.h>
#include <qkeycode.h>
#include <qdict.h>
       
#include <kdebug.h>
#include <klocale.h>
#include <kconfig.h>

#include "kfdird.h"      
#include "kftypes.h"
#include "kftabdlg.h"


#define FIND_PROGRAM "find"
#define SPECIAL_TYPES 7

extern QList<KfFileType> *types;

typedef struct GUIItem {
  QWidget *w;
  bool enabled;
};
QList<GUIItem> guiItem;

void appendGUIItem(QWidget *w) {
  GUIItem *gi = new GUIItem;

  gi->w = w;
  guiItem.append(gi);
}

KfindTabDialog::KfindTabDialog( QWidget *parent, const char *name, const char *searchPath )
    : QTabDialog( parent, name )
  {
    _searchPath = searchPath;
    
    // ************ Page One ************

    pages[0] = new QWidget( this, "page1" );
    
    nameBox    = new QComboBox(TRUE, pages[0], "combo1");
    namedL     = new QLabel(nameBox, i18n("&Named:"), pages[0], "named");
    dirBox     = new QComboBox(TRUE, pages[0], "combo2");
    lookinL    = new QLabel(dirBox, i18n("&Look in:"), pages[0], "named");
    subdirsCb  = new QCheckBox(i18n("Include &subfolders"), pages[0]);
    browseB    = new QPushButton(i18n("&Browse ..."), pages[0]);

    // Setup
    
    nameBox->setInsertionPolicy (QComboBox::AtTop);
    dirBox ->setInsertionPolicy (QComboBox::AtTop);
    nameBox->setMaxCount(15);
    dirBox ->setMaxCount(15);
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
    
    appendGUIItem(browseB);
    appendGUIItem(dirBox);
    appendGUIItem(subdirsCb);
    appendGUIItem(nameBox);

    // Signals
    
    connect( browseB,  SIGNAL(clicked()),
             this, SLOT(getDirectory()) );   
    
    loadHistory();
    
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
    le[3] ->setText("1");

    rb1[0]->setChecked (TRUE);

    bg[0]->insert( rb1[0] );
    bg[0]->insert( rb1[1] );
    bg[0]->setExclusive(TRUE);
    bg[1]->insert( rb2[0] );
    bg[1]->insert( rb2[1] );
    bg[1]->insert( rb2[2] );

    le[0]->setMaxLength(10);
    le[1]->setMaxLength(10);
    le[2]->setMaxLength(3);
    le[3]->setMaxLength(3);
  
    le[0]->setEnabled(modifiedFiles = FALSE);
    le[1]->setEnabled(betweenDates  = FALSE);
    le[2]->setEnabled(prevMonth     = FALSE);
    le[3]->setEnabled(prevDay       = FALSE);

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

    appendGUIItem(rb1[0]);
    appendGUIItem(rb1[1]);
    appendGUIItem(rb2[0]);
    appendGUIItem(rb2[1]);
    appendGUIItem(rb2[2]);
    appendGUIItem(le[0]);
    appendGUIItem(le[1]);
    appendGUIItem(le[2]);
    appendGUIItem(le[3]);

    connect( bg[1],  SIGNAL(clicked(int)),
             this, SLOT(enableEdit(int)) );
    connect( rb1[0],  SIGNAL(clicked()),
             this,  SLOT(disableAllEdit()) );
    connect( rb1[1],  SIGNAL(clicked()),
             this,  SLOT(enableCheckedEdit()) );

    connect( le[0],  SIGNAL(returnPressed()),
             this,  SLOT(isCheckedValid()) );
    connect( le[1],  SIGNAL(returnPressed()),
             this,  SLOT(isCheckedValid()) );
    connect( le[2],  SIGNAL(returnPressed()),
             this,  SLOT(isCheckedValid()) );
    connect( le[3],  SIGNAL(returnPressed()),
             this,  SLOT(isCheckedValid()) );      
    
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

    sizeEdit->setFixedWidth(50);
    sizeEdit->setText("1");
    slotSizeBoxChanged(0);
    
    // Signals

    connect(sizeBox, SIGNAL(highlighted(int)),
	    this, SLOT(slotSizeBoxChanged(int)));
    
    // Layout
    
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

    appendGUIItem(textEdit);
    appendGUIItem(typeBox);
    appendGUIItem(sizeBox);
    appendGUIItem(sizeEdit);
    appendGUIItem(caseCb);

    addTab( pages[2], i18n(" Advanced ") );  
  }

KfindTabDialog::~KfindTabDialog()
  {
    delete pages[0];
    delete pages[1];
    delete pages[2];
  };

void KfindTabDialog::loadHistory() {
  // load pattern history
  KConfig *conf = kapp->getConfig();
  QStrList sl;
  conf->setGroup("History");
  if(conf->readListEntry("Patterns", sl))
    nameBox->insertStrList(&sl);
  else
    nameBox->insertItem("*");

  if(conf->readListEntry("Directories", sl))
    {
      dirBox ->insertItem( _searchPath );
      dirBox->insertStrList(&sl);
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

void KfindTabDialog::saveHistory() {
  // save pattern history
  QStrList sl;
  QDict<char> dict(17, FALSE);
  sl.append(nameBox->currentText().ascii());
  for(int i = 0; i < nameBox->count(); i++) {   
    if(!dict.find(nameBox->text(i))) {
      dict.insert(nameBox->text(i), "dummy");
      sl.append(nameBox->text(i).ascii());
    }
  }
  KConfig *conf = kapp->getConfig();
  conf->setGroup("History");
  conf->writeEntry("Patterns", sl);

  dict.clear();
  sl.clear();
  sl.append(dirBox->currentText().ascii());
  for(int i = 0; i < dirBox->count(); i++) {
    if(!dict.find(dirBox->text(i))) {
      dict.insert(dirBox->text(i), "dummy");
      sl.append(dirBox->text(i).ascii());
    }
  }
  conf->writeEntry("Directories", sl);
}

void KfindTabDialog::slotSizeBoxChanged(int index) {
  sizeEdit->setEnabled((bool)(index != 0));
}

void KfindTabDialog::keyPressEvent(QKeyEvent *e) {
  if(e->key() == Key_Escape)
    return;
  QTabDialog::keyPressEvent(e);
}

QSize KfindTabDialog::sizeHint()
  {
    QSize size(320,195);
    
    return (size);   
  };

void KfindTabDialog::setDefaults()
  {
    le[0] ->setText(date2String(QDate(1980,1,1)));
    le[1] ->setText(date2String(QDate::currentDate()));
    le[2] ->setText("1");
    le[3] ->setText("1");

    typeBox ->setCurrentItem(0);
    sizeBox ->setCurrentItem(0);
    sizeEdit->setText("1");
  }

void KfindTabDialog::enableEdit(int i)
  {
    if (!rb1[1]->isChecked())
      {
          rb1[0]->setChecked(FALSE);
          rb1[1]->setChecked(TRUE);
      }

    disableAllEdit();

    modifiedFiles = TRUE;

    if (i==0)
        {
          le[i]  ->setEnabled(TRUE);
          le[i+1]->setEnabled(TRUE);
          betweenDates = TRUE;
        }
      else
        {
          i++;
          le[i]->setEnabled(TRUE);
          if (i==2)
              prevMonth = TRUE;
            else
             prevDay = TRUE;
        }
  }

void KfindTabDialog::disableAllEdit()
  {
    for (int i=0;i<4;i++)
      le[i]->setEnabled(FALSE);

    modifiedFiles = FALSE;
    betweenDates  = FALSE;
    prevMonth     = FALSE;
    prevDay       = FALSE;
  }


void KfindTabDialog::enableCheckedEdit()
  {
    for (int i=0;i<3;i++)
      if (rb2[i]->isChecked())
        enableEdit(i);
  }

void KfindTabDialog::isCheckedValid()
  {
    int match, len;
    QRegExp date("[0-9][0-9]?[/][0-9][0-9]?[/][0-9][0-9][0-9][0-9]");
    QRegExp r("[0-9]+");

    if (betweenDates == TRUE)
      {
        QDate hi, hi2;
        bool rightDates = TRUE;

        match = date.match(le[0]->text(), 0,&len);
        if ( !(match != -1 && len == (int)le[0]->text().length() ) )
	  rightDates=FALSE;

        if ( string2Date(le[0]->text(), &hi).isNull() ) 
          rightDates=FALSE;

        match = date.match(le[1]->text(), 0,&len);
        if ( !(match != -1 && len == (int)le[1]->text().length() ) )
	  rightDates=FALSE;

       if ( string2Date(le[1]->text(), &hi).isNull() ) 
         rightDates=FALSE;

        if (rightDates)
	  if (string2Date(le[0]->text(), &hi)>string2Date(le[1]->text(), &hi2))
             rightDates  = FALSE;

        if (!rightDates)
            {
              QMessageBox mb(this,"message box");
              mb.setText( i18n("The date is not valid!!"));
              mb.show();
              enableSearch = FALSE;
            };
      };

    if (prevMonth == TRUE)
      {
        match = r.match(le[2]->text(), 0,&len);
        if ( !(match != -1 && len == (int)le[2]->text().length() ) )
          {
            QMessageBox mb(this,"message box");
            mb.setText( i18n("The month(s) value isn't valid!!"));
            mb.show();
            enableSearch = FALSE;
          };
      };               
    if (prevDay == TRUE)
      {
        match = r.match(le[3]->text(), 0,&len);
        if (! (match != -1 && len == (int)le[3]->text().length() ) )
          {
            QMessageBox mb(this,"message box");
            mb.setText( i18n("The day(s) value isn't valid!!"));
            mb.show();
            enableSearch = FALSE;
          };
      };

  };

void KfindTabDialog::checkSize()
  {
    int match,len;
    QRegExp r("[0-9]+");

    match = r.match(sizeEdit->text(), 0,&len);
    if ( !(match != -1 && len == (int)sizeEdit->text().length() ) )
      {
        QMessageBox mb(this,"message box");
        mb.setText( i18n("The value in size isn't valid number!!"));
        mb.show();
        enableSearch = FALSE;
      };
  };

QString KfindTabDialog::createQuery()
  {
    QString str,pom;
    int month;
    char *type;

    enableSearch = TRUE;
    isCheckedValid();
    checkSize();

    str = FIND_PROGRAM;
    str += " ";

    if (enableSearch)
      {
	//      str += dirBox->text(dirBox->currentItem());
	//	printf("Tak co je v tom boxu: %s\n",dirBox->currentText());
	str += dirBox->currentText();

	QString str1;
	str1 += " \"(\" -name \"";
	//	str1 += nameBox->text(nameBox->currentItem());

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

	default: {
	    str1 = "";
	    KfFileType *typ;

             typ = types->first();
             for (int i=SPECIAL_TYPES; i<typeBox->currentItem(); i++ )
               typ = types->next();
             //      printf("Take filetype: %s\n",typ->getComment("").ascii());

             QStrList& pats = typ->getPattern();
             bool firstpattern = FALSE;
             str += " \"(\" ";
             for (QString pattern=pats.first(); pattern!=0L; 
		  pattern=pats.next())
               {
                 if (!firstpattern)
                   {
                     str += " -name ";
                     firstpattern=TRUE;
                   }
                 else
                   str += " -o -name ";

                 if ( pattern.find("*",0)==0 )
                   {
                     str += nameBox->text(nameBox->currentItem());
                     str += "\"" + pattern + "\"";
                   }
                 else
                   {
                     str += "\"" + pattern + "\"";
                     str += nameBox->text(nameBox->currentItem());
                   };
               };                                             
             str += " \")\"";

             //      printf("Query : %s\n",str.ascii());
          }
	}

	str += str1;

        if (!subdirsCb->isChecked())
            str.append(" -maxdepth 1 ");

        if (modifiedFiles)
          {
            if (betweenDates == TRUE)
              {
              QDate q1, q2;
                str.append(pom.sprintf(" -daystart -mtime -%d -mtime +%d",
               (string2Date(le[0]->text(),&q1)).daysTo(QDate::currentDate()),
		(string2Date(le[1]->text(),&q2)).daysTo(QDate::currentDate()) ));
              };

            if (prevMonth == TRUE)
              {
                sscanf(le[2]->text().ascii(),"%d",&month);
                str.append(pom = QString(" -daystart -mtime -%1 ")
                           .arg((int)(month*30.416667)));
              };

            if (prevDay == TRUE)
                str.append(pom = QString(" -daystart -mtime -%1").arg(le[3]->text()));
          };

        if (sizeBox->currentItem() !=  0)
          {
            switch(sizeBox->currentItem())
              {
	        case 1: {type=(char *)(sizeEdit->text().toInt()==0?"":"+");break;}
	        case 2: {type=(char *)(sizeEdit->text().toInt()==0?"":"-"); break;}
	        default: {type=(char *)(sizeEdit->text().toInt()==0?"":" ");} 
              };
            str.append(pom = QString(" -size  %1%2k ").arg(type).arg(sizeEdit->text()));
          };
      };

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
  };        

 
QString KfindTabDialog::date2String(QDate date)
  {
    QString str;

    str.sprintf("%.2d/%.2d/%4d",date.day(),date.month(),date.year());
    return(str);
  };

QDate &KfindTabDialog::string2Date(QString str, QDate *qd)
{   
    int year,month,day;
            
    sscanf(str.ascii(),"%2d/%2d/%4d",&day,&month,&year);
    qd->setYMD(year, month, day);
    return *qd; 

}

void  KfindTabDialog::getDirectory()
  {
    QString result;

    dirselector = new KfDirDialog(dirBox->text(dirBox->currentItem()),
                                  this,"dirselector",TRUE);
    CHECK_PTR(dirselector);

    if ( dirselector->exec() == QDialog::Accepted )
              result = dirselector->selectedDir();
    delete dirselector;

    if (!result.isNull())
      {
        //printf("Dir: %s\n",result.ascii());
        dirBox->insertItem(result,0);
        dirBox->setCurrentItem(0);
      };
  };

void KfindTabDialog::beginSearch() {
  saveHistory();
  for(GUIItem *g = guiItem.first(); g; g = guiItem.next()) {
    g->enabled = g->w->isEnabled();
    g->w->setEnabled(FALSE);    
  }
}

void KfindTabDialog::endSearch() {
  for(GUIItem *g = guiItem.first(); g; g = guiItem.next())
    g->w->setEnabled(g->enabled);
  loadHistory();
}
