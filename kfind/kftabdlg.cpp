/***********************************************************************
 *
 *  kftabdlg.cpp
 *
 **********************************************************************/
#include <string.h>
#include <stdlib.h>

#include <qapp.h>
#include <qtabdlg.h>
#include <qmlined.h>
#include <qbttngrp.h>
#include <qradiobt.h>
#include <qlabel.h>
#include <qcombo.h>
#include <qlayout.h>
#include <stdio.h>
#include <qstring.h>
#include <qfont.h>
#include <qtooltip.h>
#include <qlined.h>
#include <qchkbox.h>
#include <qpushbt.h>
#include <qfiledlg.h>
#include <qdir.h>
#include <qregexp.h>
#include <qdatetm.h>
#include <qmsgbox.h>
#include <qlist.h>
#include <qsize.h>
       
#include "kfdird.h"      
#include "kftypes.h"
#include "kftabdlg.h"

#include <klocale.h>
#define trans klocale

extern QList<KfFileType> *types;

KfindTabDialog::KfindTabDialog( QWidget *parent, const char *name, const char *searchPath )
    : QTabDialog( parent, name )
  {
    //Page One of KfTAbDialog
    pages[0] = new QWidget( this, "page1" );

    nameBox    = new QComboBox(TRUE           ,pages[0],"combo1");
    namedL     = new QLabel(nameBox,trans->translate("&Named:"),
			    pages[0],"named");
    dirBox     = new QComboBox(FALSE          ,pages[0],"combo2");
    lookinL    = new QLabel(dirBox,trans->translate("&Look in:"),
			    pages[0],"named");
    subdirsCb  = new QCheckBox(                pages[0]);
    browseB    = new QPushButton(trans->translate("&Browse ..."),pages[0]);

    nameBox->insertItem( "*" );
    dirBox ->insertItem( searchPath );
    dirBox ->insertItem( "/" );
    dirBox ->insertItem( "/usr" );
    dirBox ->insertItem( "/lib" );
    dirBox ->insertItem( "/home" );
    dirBox ->insertItem( "/etc" );
    dirBox ->insertItem( "/var" );
    dirBox ->insertItem( "/mnt" );

    subdirsCb->setText( trans->translate("Include &subfolders") );

    int wTmpNamed = (namedL->sizeHint()).width();
    int wTmpLook  = (lookinL->sizeHint()).width();
    int wTmp = (wTmpNamed > wTmpLook) ? wTmpNamed:wTmpLook;
    if ((nameBox->style())==WindowsStyle)
      {
        namedL   ->setFixedSize(wTmp+10,25);
        lookinL  ->setFixedSize(wTmp+10,25);
      }
    else
      {
        namedL   ->setFixedSize(wTmp+10,30);
        lookinL  ->setFixedSize(wTmp+10,30);
      };                                        

    subdirsCb->setFixedSize(subdirsCb->sizeHint());
    browseB  ->setFixedSize(browseB->sizeHint());

    namedL ->setAlignment(namedL->alignment()|ShowPrefix);
    lookinL->setAlignment(namedL->alignment()|ShowPrefix);
    nameBox->setInsertionPolicy (QComboBox::AtTop);


    subdirsCb->setChecked ( TRUE );
    browseB ->setEnabled(TRUE);

    connect( browseB,  SIGNAL(clicked()),
             this, SLOT(getDirectory()) );   

    addTab( pages[0], trans->translate(" Name& Location ") );
                                                      
    //Page Two of KfTAbDialog
    pages[1] = new QWidget( this, "page2" );

    rb1[0] = new QRadioButton(       pages[1] );
    rb1[1] = new QRadioButton(       pages[1] );
    bg[0]  = new QButtonGroup();
    bg[1]  = new QButtonGroup();
    rb2[0] = new QRadioButton(       pages[1] );
    rb2[1] = new QRadioButton(       pages[1] );
    rb2[2] = new QRadioButton(       pages[1] );
    andL   = new QLabel (trans->translate("and"),      pages[1],"and");
    monthL = new QLabel (trans->translate("month(s)"), pages[1],"months");
    dayL   = new QLabel (trans->translate("day(s)"),   pages[1],"days");
    le[0]  = new QLineEdit(          pages[1], "lineEdit1" );
    le[1]  = new QLineEdit(          pages[1], "lineEdit2" );
    le[2]  = new QLineEdit(          pages[1], "lineEdit3" );
    le[3]  = new QLineEdit(          pages[1], "lineEdit4" );

    rb1[0]->setText( trans->translate("&All files") );
    rb1[1]->setText( trans->translate("Find all files created or &modified:"));
    rb2[0]->setText( trans->translate("&between") );
    rb2[1]->setText( trans->translate("during the previou&s ") );
    rb2[2]->setText( trans->translate("&during the previous ") );
    le[0] ->setText(date2String(QDate(1980,1,1)));
    le[1] ->setText(date2String(QDate::currentDate()));
    le[2] ->setText("1");
    le[3] ->setText("1");

    rb1[0]->setChecked (TRUE);

    rb1[0]->setFixedSize(rb1[0]->sizeHint().width(),25) ;
    rb1[1]->setFixedSize(rb1[1]->sizeHint().width(),25) ;
    rb2[0]->setFixedSize(rb2[0]->sizeHint().width(),25) ;
    rb2[1]->setFixedSize(rb2[1]->sizeHint().width(),25) ;
    rb2[2]->setFixedSize(rb2[2]->sizeHint().width(),25) ;

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
    
    addTab( pages[1], trans->translate(" Date Modified ") );

    //Page Tree of KfTAbDialog
    pages[2] = new QWidget( this, "page3" );

    typeBox =new QComboBox(FALSE,pages[2],"typeBox");
    typeL   =new QLabel(typeBox,trans->translate("Of &type:"),
			pages[2],"type");
    textL   =new QLabel(trans->translate("&Containing Text:"),pages[2],"text");
    textEdit=new QLineEdit(                 pages[2], "textEdit" );
    sizeBox =new QComboBox(FALSE           ,pages[2],"sizeBox");
    sizeL   =new QLabel(sizeBox,trans->translate("&Size is:"),
			pages[2],"size");
    sizeEdit=new QLineEdit(                 pages[2], "sizeEdit" );
    kbL     =new QLabel("KB"               ,pages[2],"kb");


    typeL->setAlignment(namedL->alignment()|ShowPrefix);
    textL->setAlignment(namedL->alignment()|ShowPrefix);
    sizeL->setAlignment(namedL->alignment()|ShowPrefix);

    typeL->setFixedSize(100,25);
    textL->setFixedSize(100,25);
    sizeL->setFixedSize(100,25);
    kbL->setFixedSize(20,25);

    sizeEdit->setMaxLength(5);

    textL->setEnabled(FALSE);
    textEdit ->setEnabled(FALSE);
    sizeEdit ->setEnabled(TRUE);

    KfFileType *typ;

    typeBox->insertItem(trans->translate("All Files and Folders"));
    for ( typ = types->first(); typ != 0L; typ = types->next() )
      if (typ->getComment("")!="")
	typeBox->insertItem(typ->getComment(""));
      else
	typeBox->insertItem(typ->getName());

    sizeBox ->insertItem( trans->translate("(none)") );
    sizeBox ->insertItem( trans->translate("At Least") );
    sizeBox ->insertItem( trans->translate("At Most") );
    sizeBox ->setFixedSize(sizeBox->sizeHint());
    sizeEdit->setText("1");

    connect( sizeEdit,  SIGNAL(returnPressed()),
             this    ,  SLOT(checkSize()) );      

    addTab( pages[2], trans->translate(" Advanced ") );  
    setOkButton(0L);
  }

KfindTabDialog::~KfindTabDialog()
  {
    delete pages[0];
    delete pages[1];
    delete pages[2];
  };

void KfindTabDialog::resizeEvent( QResizeEvent *ev )
  {
    int w = width();
    int   wTmp;
    QRect rTmp;

    QTabDialog::resizeEvent(ev);

    //Page One of KfTAbDialog
    namedL ->move(10,20);

    if ((nameBox->style())==WindowsStyle)
        lookinL->move(10,55);
      else
        lookinL->move(10,60);

    wTmp = 10+namedL->width();

    if ((nameBox->style())==WindowsStyle)
        nameBox->setGeometry(wTmp,namedL->y(),w-20-wTmp,25);
      else
        nameBox->setGeometry(wTmp,namedL->y(),w-20-wTmp,30);

    rTmp = browseB->geometry();
    wTmp = nameBox->x()+nameBox->width()-rTmp.width();
    browseB ->move(wTmp,lookinL->y());

    wTmp = 10+lookinL->width();
    dirBox->setGeometry(wTmp,lookinL->y(),browseB->x()-15-wTmp,25);

    subdirsCb ->move(10+lookinL->width(),lookinL->y()+35);

    //Page Two of KfTAbDialog
    rb1[0]->move( 5, 5);
    rb1[1]->move( 5, 30);
    rb2[0]->move( 25, 55);
    rb2[1]->move( 25, 80);
    rb2[2]->move( 25, 105);

    le[0]->setGeometry( 100, 60, 80, 20 );
    le[1]->setGeometry( 220, 60, 80, 20 );
    le[2]->setGeometry( 90+rb2[1]->width(), 85, 40, 20 );
    le[3]->setGeometry( 90+rb2[2]->width(), 110, 40, 20 );

    andL->move(190,55);
    monthL->move(le[2]->x()+le[2]->width()+15,80);
    dayL->move(le[3]->x()+le[3]->width()+15,105);
                                                  

     //Page Tree of KfTAbDialog
    typeL->move(10,20);
    wTmp = 10+typeL->width();
    typeBox ->setGeometry(wTmp,typeL->y(),w-20-wTmp,25);
    textL   ->move(10,55);
    textEdit->setGeometry(wTmp, textL->y(),w-20-wTmp, 25 );
    sizeL   ->move(10,90);
    sizeBox ->setGeometry(10+sizeL->width(),sizeL->y(),80,25);
    sizeEdit->setGeometry( 10+sizeBox->x()+sizeBox->width(), sizeL->y(),60,25);
    kbL     ->move(10+sizeEdit->x()+sizeEdit->width(),sizeL->y());
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
        bool rightDates = TRUE;

        match = date.match(le[0]->text(), 0,&len);
        if ( !(match != -1 && len == (int)strlen(le[0]->text())) )
	  rightDates=FALSE;

        if ( string2Date(le[0]->text()).isNull() ) 
          rightDates=FALSE;

        match = date.match(le[1]->text(), 0,&len);
        if ( !(match != -1 && len == (int)strlen(le[1]->text())) )
	  rightDates=FALSE;

       if ( string2Date(le[1]->text()).isNull() ) 
         rightDates=FALSE;

        if (rightDates)
	  if (string2Date(le[0]->text())>string2Date(le[1]->text()))
	    rightDates  = FALSE;

        if (!rightDates)
            {
              QMessageBox mb(this,"message box");
              mb.setText( trans->translate("The date is not valid!!"));
              mb.show();
              enableSearch = FALSE;
            };
      };

    if (prevMonth == TRUE)
      {
        match = r.match(le[2]->text(), 0,&len);
        if ( !(match != -1 && len == (int)strlen(le[2]->text())) )
          {
            QMessageBox mb(this,"message box");
            mb.setText( trans->translate("The month(s) value isn't valid!!"));
            mb.show();
            enableSearch = FALSE;
          };
      };               
    if (prevDay == TRUE)
      {
        match = r.match(le[3]->text(), 0,&len);
        if (! (match != -1 && len == (int)strlen(le[3]->text())) )
          {
            QMessageBox mb(this,"message box");
            mb.setText( trans->translate("The day(s) value isn't valid!!"));
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
    if ( !(match != -1 && len == (int)strlen(sizeEdit->text())) )
      {
        QMessageBox mb(this,"message box");
        mb.setText( trans->translate("The value in size isn't valid number!!"));
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

    if (enableSearch)
      {
        str = dirBox->text(dirBox->currentItem());

        nameBox->insertItem( nameBox->currentText(),0 );

        if ( (typeBox->currentItem())!=0 )
          {
             KfFileType *typ;

             typ = types->first();
             for (int i=1; i<typeBox->currentItem(); i++ )
               typ = types->next();
             //      printf("Take filetype: %s\n",typ->getComment("").data());

             QStrList& pats = typ->getPattern();
             bool firstpattern = FALSE;
             str += " ( ";
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
                     str += pattern.data();
                   }
                 else
                   {
                     str += pattern.data();
                     str += nameBox->text(nameBox->currentItem());
                   };
               };                                             
             str += " )";

             //      printf("Query : %s\n",str.data());
          }
        else
          {
            str += " ( -name ";
            str += nameBox->text(nameBox->currentItem());
	    str += " )";
          };

        if (!subdirsCb->isChecked())
            str.append(" -maxdepth 1 ");

        if (modifiedFiles)
          {
            if (betweenDates == TRUE)
              {
                str.append(pom.sprintf(" -daystart -mtime -%d -mtime +%d",
		 (string2Date(le[0]->text())).daysTo(QDate::currentDate()),
		 (string2Date(le[1]->text())).daysTo(QDate::currentDate()) ));
              };

            if (prevMonth == TRUE)
              {
                sscanf(le[2]->text(),"%d",&month);
                str.append(pom.sprintf(" -daystart -mtime -%d ",
                           (int)(month*30.416667)));
              };

            if (prevDay == TRUE)
                str.append(pom.sprintf(" -daystart -mtime -%s",le[3]->text()));
          };

        if (sizeBox->currentItem() !=  0)
          {
            switch(sizeBox->currentItem())
              {
	        case 1: {type=(atoi(sizeEdit->text())==0?"":"+");break;} 
	        case 2: {type=(atoi(sizeEdit->text())==0?"":"-");break;} 
	        default: {type=(atoi(sizeEdit->text())==0?"":" ");} 
              };
            str.append(pom.sprintf(" -size  %s%sk ",type,sizeEdit->text()));
          };
      };

    return(str);
  };        

 
QString KfindTabDialog::date2String(QDate date)
  {
    QString str;

    str.sprintf("%.2d/%.2d/%4d",date.day(),date.month(),date.year());
    return(str);
  };

QDate KfindTabDialog::string2Date(QString str)
{   
    int year,month,day;
    QDate tmpD;
        
    sscanf(str,"%2d/%2d/%4d",&day,&month,&year);
    if (QDate::isValid(year,month,day))
        return QDate(year,month,day);
      else
        return (tmpD);   
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
        //printf("Dir: %s\n",result.data());
        dirBox->insertItem(result,0);
        dirBox->setCurrentItem(0);
      };
  };

