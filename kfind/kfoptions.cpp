/***********************************************************************
 *
 *  KfOptions.cpp
 *
 **********************************************************************/

#include <stdlib.h>

#include <qtabdialog.h>
#include <qbuttongroup.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qcombobox.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qcheckbox.h>
#include <qpushbutton.h>
#include <qfiledialog.h>
#include <qstrlist.h>

#include <kfiledialog.h>
#include <kdialog.h>
#include <kconfig.h>
#include <kapp.h>

#include "kfarch.h"
#include "kfoptions.h"
#include "kfsave.h"

extern KfSaveOptions *saving;
extern QList<KfArchiver> *archivers;



KfOptions::KfOptions( QWidget *parent, const char *name, bool modal )
  :KDialogBase( Tabbed, i18n("Preferences"), Help|Apply|Ok|Cancel,
		Ok, parent, name, modal )
{
  setHelp( "kfind/kfind.html", QString::null );

  setupSavingPage();
  setupArchiversPage();
}


KfOptions::~KfOptions() 
{
  delete bg;
}


void KfOptions::setupSavingPage( void )
{
  QFrame *page = addPage( i18n("Saving") );
  QVBoxLayout *topLayout = new QVBoxLayout( page, 0, spacingHint() );
  if( topLayout == 0 ) { return; }
  
  formatL     = new QLabel(i18n("File format:"), page);
  browseB     = new QPushButton(i18n("Browse"), page);
  formatBox   = new QComboBox(page);
  fileE       = new QLineEdit(page);
  kfindfileB  = new QRadioButton(
    "Save results to file ~/.kfind-results.html", page);
  selectfileB = new QRadioButton("Save results to file:", page );

  bg          = new QButtonGroup();

  bg->insert(kfindfileB);
  bg->insert(selectfileB);
  bg->setExclusive(TRUE);
    
  formatBox->insertItem("HTML");
  formatBox->insertItem(i18n("Plain Text"));
  
  initFileSelecting();


  //topLayout->addSpacing(15);
  topLayout->addWidget(kfindfileB);
  QHBoxLayout *l1 = new QHBoxLayout(topLayout);
  l1->addWidget(selectfileB);
  l1->addWidget(fileE);
  l1->addWidget(browseB);
  QHBoxLayout *l2 = new QHBoxLayout(topLayout);
  l2->addWidget(formatL);
  l2->addWidget(formatBox);
  l2->addStretch(1);
  topLayout->addStretch(1);

  connect( kfindfileB  ,SIGNAL(clicked()),
	   this, SLOT(setFileSelecting()) );
  connect( selectfileB ,SIGNAL(clicked()),
	   this, SLOT(setFileSelecting()) );
  connect( browseB     ,SIGNAL(clicked()),
	   this, SLOT(selectFile()) );
}



void KfOptions::setupArchiversPage( void )
{
  QFrame *page = addPage( i18n("Archivers") );
  QGridLayout *topLayout = new QGridLayout( page, 6, 3, 0, spacingHint() );
  if( topLayout == 0 ) { return; }

  archiversLBox   = new QListBox(page);
  createL         = new QLabel(i18n("Create Archive:"), page);
  addL            = new QLabel(i18n("Add to Archive:"), page);
  createE         = new QLineEdit(page);
  addE            = new QLineEdit(page);
  paternsL2       = new QLabel(i18n("Patterns:"), page);
  paternsLBox2    = new QListBox(page);
  addArchiverB    = new QPushButton(i18n("Add New"), page);
  removeArchiverB = new QPushButton(i18n("Remove"), page);
    
  createE        ->setEnabled(FALSE);
  addE           ->setEnabled(FALSE);
  addArchiverB   ->setEnabled(FALSE);
  removeArchiverB->setEnabled(FALSE);
  
  fillArchiverLBox();
  fillArchiverDetail(0);
  
  topLayout->addMultiCellWidget(archiversLBox, 0, 5, 0, 0);
  topLayout->addWidget(createL, 0, 1);
  topLayout->addWidget(addL, 1, 1);
  topLayout->addWidget(paternsL2, 2, 1);
  topLayout->addWidget(createE, 0, 2);
  topLayout->addWidget(addE, 1, 2);
  topLayout->addMultiCellWidget(paternsLBox2, 2, 3, 2, 2);
  topLayout->addWidget(addArchiverB, 4, 2);
  topLayout->addWidget(removeArchiverB, 5, 2);
  topLayout->setRowStretch(3, 1);

  connect(archiversLBox, SIGNAL(highlighted(int)),
	  this, SLOT(fillArchiverDetail(int)) );
}




#if 0
KfOptions::KfOptions( QWidget *parent, const char *name ):QTabDialog( parent, name)
{
  insertPages();
  
  setOkButton(i18n("OK"));
  setCancelButton(i18n("Cancel"));
  setCaption(i18n("Preferences"));
  
  connect(this,SIGNAL(applyButtonPressed()),
          this,SLOT(applyChanges()));
}

KfOptions::~KfOptions() 
{
  delete bg;
}

void KfOptions::insertPages()
{
    setFocusPolicy(QWidget::StrongFocus);
    
    // First page of tab preferences dialog
    QWidget *page1 = new QWidget(this, "page1");
    
    formatL     = new QLabel(i18n("File format:"), page1);
    browseB     = new QPushButton(i18n("Browse"), page1);
    formatBox   = new QComboBox(page1);
    fileE       = new QLineEdit(page1);
    kfindfileB  = new QRadioButton("Save results to file ~/.kfind-results.html",
                                   page1);
    selectfileB = new QRadioButton("Save results to file:", 
                                   page1);
    bg          = new QButtonGroup();

    bg->insert(kfindfileB);
    bg->insert(selectfileB);
    bg->setExclusive(TRUE);
    
    formatBox->insertItem("HTML");
    formatBox->insertItem(i18n("Plain Text"));
    
    initFileSelecting();

    QVBoxLayout *topL = new QVBoxLayout(page1, 
                                        KDialog::marginHint(), 
                                        KDialog::spacingHint());
    topL->addSpacing(15);
    topL->addWidget(kfindfileB);
    QHBoxLayout *l1 = new QHBoxLayout(topL);
    l1->addWidget(selectfileB);
    l1->addWidget(fileE);
    l1->addWidget(browseB);
    QHBoxLayout *l2 = new QHBoxLayout(topL);
    l2->addWidget(formatL);
    l2->addWidget(formatBox);
    l2->addStretch(1);
    topL->addStretch(1);

    connect( kfindfileB  ,SIGNAL(clicked()),
             this, SLOT(setFileSelecting()) );
    connect( selectfileB ,SIGNAL(clicked()),
             this, SLOT(setFileSelecting()) );
    connect( browseB     ,SIGNAL(clicked()),
             this, SLOT(selectFile()) );
    
    addTab(page1, i18n("Saving"));
    
    // Third page of tab preferences dialog
    QWidget *page2 = new QWidget(this);

    archiversLBox   = new QListBox(page2);
    createL         = new QLabel(i18n("Create Archive:"), page2);
    addL            = new QLabel(i18n("Add to Archive:"), page2);
    createE         = new QLineEdit(page2);
    addE            = new QLineEdit(page2);
    paternsL2       = new QLabel(i18n("Patterns:"), page2);
    paternsLBox2    = new QListBox(page2);
    addArchiverB    = new QPushButton(i18n("Add New"), page2);
    removeArchiverB = new QPushButton(i18n("Remove"), page2);
    
    createE        ->setEnabled(FALSE);
    addE           ->setEnabled(FALSE);
    addArchiverB   ->setEnabled(FALSE);
    removeArchiverB->setEnabled(FALSE);
    
    fillArchiverLBox();
    fillArchiverDetail(0);
    
    QGridLayout *topG = new QGridLayout(page2, 6, 3,
                                        KDialog::marginHint(), 
                                        KDialog::spacingHint());
    topG->addMultiCellWidget(archiversLBox, 0, 5, 0, 0);
    topG->addWidget(createL, 0, 1);
    topG->addWidget(addL, 1, 1);
    topG->addWidget(paternsL2, 2, 1);
    topG->addWidget(createE, 0, 2);
    topG->addWidget(addE, 1, 2);
    topG->addMultiCellWidget(paternsLBox2, 2, 3, 2, 2);
    topG->addWidget(addArchiverB, 4, 2);
    topG->addWidget(removeArchiverB, 5, 2);
    topG->setRowStretch(3, 1);

    connect(archiversLBox, SIGNAL(highlighted(int)),
            this, SLOT(fillArchiverDetail(int)) );
    
    addTab(page2, i18n("Archivers"));
}
#endif


void KfOptions::selectFile()
  {
    QString filter;
    QString path(getenv("HOME"));

    switch(formatBox->currentItem())
      {
        case 0: filter =  "*.html"; break;
        case 1: filter =  "";
      }
    QString s( KFileDialog::getOpenFileName(path,filter) );
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
  if ( saving->getSaveFormat() == "HTML" )
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
	paternsLBox2->insertItem( pattern );
    };
};

void KfOptions::applyChanges()
  {
    //    printf("Make all changes!\n");
    KConfig *config = KApplication::kApplication()->config();
    config->setGroup( "Saving" );

    saving->setSaveFile( fileE->text() );
    saving->setSaveFormat( formatBox->text(formatBox->currentItem()) );

    if ( kfindfileB->isChecked() | fileE->text().isEmpty() )
      {
	saving->setSaveStandard(TRUE);
	config->writeEntry( "Filename", QString() );
	config->writeEntry( "Format", QString() );
      }
    else
      {
	saving->setSaveStandard(FALSE);
	config->writeEntry( "Filename", saving->getSaveFile() );
	config->writeEntry( "Format", saving->getSaveFormat() );
      };
  };

