/***********************************************************************
 *
 *  KfOptions.cpp
 *
 **********************************************************************/

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
#include <qlistbox.h> 
#include <qstrlist.h>

#include <stdlib.h>
#include <string.h>

#include <klocale.h>

#include "kftypes.h"
#include "kfarch.h"
#include "kfoptions.h"
#include "kfsave.h"

extern QList<KfFileType> *types;
extern KfSaveOptions *saving;
extern QList<KfArchiver> *archivers;

KfOptions::KfOptions( QWidget *parent, const char *name ):QTabDialog( parent, name) 
  {
    resize(400,330);
    insertPages();

    setOKButton( i18n("OK"));
    setCancelButton( i18n("Cancel"));
    setCaption(i18n("Preferences"));
    
    this->setMinimumSize(400,330);
    this->setMaximumSize(400,330);
 
    connect(this,SIGNAL(applyButtonPressed()),this,SLOT(applyChanges()));
 
  };

KfOptions::~KfOptions()
  {
    delete pages[0];
    delete pages[1];
  };

void KfOptions::insertPages()
  {
    setFocusPolicy(QWidget::StrongFocus);    

    // First page of tab preferences dialog
    pages[0]= new QWidget(this,"page1");

    formatL     = new QLabel(i18n("File format:"),
			     pages[0],"formatL");
    fileL       = new QLabel(i18n("Save results to file:"),
			     pages[0],"fileL");
    kfindfileL  = new QLabel(i18n(
			      "Save results to file ~/.kfind-results.html"),
			     pages[0],"kfindfileL");
    browseB     = new QPushButton(i18n("Browse"),
			     pages[0],"browseB");
    formatBox   = new QComboBox(                     pages[0],"formatBox");
    fileE       = new QLineEdit(                     pages[0],"fileE");
    kfindfileB  = new QRadioButton(                  pages[0]);
    selectfileB = new QRadioButton(                  pages[0]);
    bg          = new QButtonGroup();

    bg->insert(kfindfileB);
    bg->insert(selectfileB);
    bg->setExclusive(TRUE);

    formatBox->insertItem("HTML");
    formatBox->insertItem(i18n("Plain Text"));
    formatBox->setFixedSize(formatBox    ->sizeHint());

    formatL    ->setFixedSize(formatL    ->sizeHint());
    fileL      ->setFixedSize(fileL      ->sizeHint());
    kfindfileL ->setFixedSize(kfindfileL ->sizeHint());
    kfindfileB ->setFixedSize(kfindfileB ->sizeHint());
    selectfileB->setFixedSize(selectfileB->sizeHint());
    browseB    ->setFixedSize(browseB    ->sizeHint());
    fileE      ->setFixedSize(344-(10+10+fileL->width()+browseB->width()),25);

    kfindfileB ->move(10,20);
    selectfileB->move(10,50);
    kfindfileL ->move(30,20);
    fileL      ->move(30,50);
    fileE      ->move(fileL->x()+fileL->width()+10,fileL->y()-5);
    browseB    ->move(fileE->x()+fileE->width()+10,fileL->y()-5);
    formatL    ->move(30,fileL->y()+fileL->height()+20);
    formatBox  ->move(formatL->x()+formatL->width()+20,formatL->y()-5);

    initFileSelecting();

    connect( kfindfileB  ,SIGNAL(clicked()),
             this, SLOT(setFileSelecting()) );
    connect( selectfileB ,SIGNAL(clicked()),
             this, SLOT(setFileSelecting()) );
    connect( browseB     ,SIGNAL(clicked()),
             this, SLOT(selectFile()) );

    addTab(pages[0],i18n("Saving"));


    // Third page of tab preferences dialog
    pages[1]= new QWidget(this,"page3");

    archiversLBox   = new QListBox(                pages[1],"archivesLBox");
    createL         = new QLabel(i18n("Create Archive:"),
				 pages[1],"createL");
    addL            = new QLabel(i18n("Add to Archive:"),
				 pages[1],"addL");
    createE         = new QLineEdit(               pages[1],"createE");
    addE            = new QLineEdit(               pages[1],"addE");
    paternsL2       = new QLabel(i18n("Patterns:"),
				 pages[1],"paternsL2");
    paternsLBox2    = new QListBox(                pages[1],"paternsLBox2");
    addArchiverB    = new QPushButton(i18n("Add New"),
				      pages[1],"addArchiverB");
    removeArchiverB = new QPushButton(i18n("Remove"),
				      pages[1],"removeArchiverB");

    createE        ->setEnabled(FALSE);
    addE           ->setEnabled(FALSE);
    addArchiverB   ->setEnabled(FALSE);
    removeArchiverB->setEnabled(FALSE);

    archiversLBox->setFixedSize(120,240);
    createL      ->setFixedSize((createL->sizeHint()).width(),25);
    addL         ->setFixedSize(createL->width(),25);
    createE      ->setFixedSize(130,25);
    addE         ->setFixedSize(130,25);
    paternsL2    ->setFixedSize(createL->width(),25);
    paternsLBox2 ->setFixedSize(130,100);

    int tmpP = archiversLBox->x()+archiversLBox->width()+10;

    archiversLBox ->move(10,10);
    createL       ->move(tmpP+15,15);
    addL          ->move(tmpP+15,createL->y()+30);
    paternsL2     ->move(tmpP+15,addL->y()+30);
    addArchiverB  ->move(tmpP+10,
			 10+archiversLBox->height()-removeArchiverB->height());
    removeArchiverB->move(addArchiverB->x()+addArchiverB->width()+35,
			 10+archiversLBox->height()-removeArchiverB->height());

    tmpP = createL->x()+createL->width()+10;

    createE     ->move(tmpP,15);
    addE        ->move(tmpP,createL->y()+30);
    paternsLBox2->move(tmpP,addL->y()+30);
 
     fillArchiverLBox();
     fillArchiverDetail(0);

     connect(archiversLBox,SIGNAL(highlighted(int)),
	     this, SLOT(fillArchiverDetail(int)) );

    addTab(pages[1],i18n("Archivers"));
  };

void KfOptions::selectFile()
  {
    QString filter;
    QString path(getenv("HOME"));

    switch(formatBox->currentItem())
      {
        case 0: filter =  QString("*.html");break;
        case 1: filter =  QString("");
      }
    QString s( QFileDialog::getOpenFileName(path,filter) );
    if ( s.isNull() )
      return;
    fileE->setText(s);
  };

void KfOptions::setFileSelecting()
  {
    browseB  ->setEnabled(selectfileB->isChecked());
    formatBox->setEnabled(selectfileB->isChecked());
    fileE    ->setEnabled(selectfileB->isChecked());
  };

void KfOptions::initFileSelecting()
{
  if (saving->getSaveStandard()) 
    kfindfileB ->setChecked ( TRUE );
  else
    selectfileB->setChecked ( TRUE );
    
  fileE->setText(saving->getSaveFile());
  if ( strcmp(saving->getSaveFormat(),"HTML")==0)
    formatBox->setCurrentItem(0);
  else
    formatBox->setCurrentItem(1);

  setFileSelecting();
};

void KfOptions::fillArchiverLBox()
{
  KfArchiver *arch;
  
  for ( arch = archivers->first(); arch != 0L; arch = archivers->next() )
    if (arch->getArComment()!="")
      archiversLBox->insertItem(arch->getArComment());
    else
      archiversLBox->insertItem(arch->getArName());
};

void KfOptions::fillArchiverDetail(int archiversLBoxItem)
{
  KfArchiver *arch;
  QString comment(archiversLBox->text(archiversLBoxItem));
  
  arch = archivers->first();
  for (int i=0; i<archiversLBoxItem; i++ )
    arch = archivers->next();

  if (arch!=0L) 
    {
      createE->setText(arch->getOnCreate()); 
      addE   ->setText(arch->getOnUpdate()); 

      QStrList& pats = arch->getArPattern();
      paternsLBox2->clear();
      for (QString pattern=pats.first(); pattern!=0L; pattern = pats.next() )
	paternsLBox2->insertItem( pattern.data() );
    };
};

void KfOptions::applyChanges()
  {
    //    printf("Make all changes!\n");
    KConfig *config = KApplication::getKApplication()->getConfig();
    config->setGroup( "Saving" );
  
    saving->setSaveFile( fileE->text() );
    saving->setSaveFormat( formatBox->text(formatBox->currentItem()) );

    if ( kfindfileB->isChecked() | (strcmp(fileE->text(),"")==0) )
      {
	saving->setSaveStandard(TRUE);
	config->writeEntry( "Filename", QString() );
	config->writeEntry( "Format", QString() );
      }
    else
      {
	saving->setSaveStandard(FALSE);
	config->writeEntry( "Filename", QString( saving->getSaveFile() ) );
	config->writeEntry( "Format", QString( saving->getSaveFormat() ) );
      };
  };

