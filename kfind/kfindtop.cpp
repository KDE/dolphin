/***********************************************************************
 *
 *  KfindTop.cpp
 *
 ***********************************************************************/

#include <stdio.h>

#include <qmessagebox.h>
#include <qsize.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qkeycode.h>
#include <qaccel.h>

#include <kapp.h>
#include <kiconloader.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <ktmainwindow.h>

#include "kfoptions.h"
#include "kftabdlg.h"
#include "kfind.h"
#include "kfindtop.h"
#include <klocale.h>
#include <kstdaccel.h>

#include "version.h"
#include <kglobal.h> 

KfindTop::KfindTop(const char *searchPath) : KTMainWindow()
  {
//     setCaption(QString("KFind ")+KFIND_VERSION);

    _toolBar = new KToolBar( this, "_toolBar" );
    _toolBar->setBarPos( KToolBar::Top );      
    _toolBar->show();
    enableToolBar( KToolBar::Show, addToolBar( _toolBar ) );

    _kfind = new Kfind(this,"dialog",searchPath);   
    setView( _kfind, FALSE );
    _kfind->show();

    menuInit();
    toolBarInit();

    setMenu(_mainMenu);
    _mainMenu->show();

    _statusBar = new KStatusBar( this, "_statusBar");
    _statusBar->insertItem("0 file(s) found", 0);
    _statusBar->enable(KStatusBar::Hide);
    setStatusBar(_statusBar);
    enableStatusBar(false);

    connect(_kfind,SIGNAL(haveResults(bool)),
            this,SLOT(enableSaveResults(bool)));
    connect(_kfind,SIGNAL(resultSelected(bool)),
	    this,SLOT(enableMenuItems(bool)));
    connect(this,SIGNAL(deleteFile()),
 	    _kfind,SIGNAL(deleteFile()));
    connect(this,SIGNAL(properties()),
 	    _kfind,SIGNAL(properties()));
    connect(this,SIGNAL(openFolder()),
 	    _kfind,SIGNAL(openFolder()));
    connect(this,SIGNAL(saveResults()),
 	    _kfind,SIGNAL(saveResults()));
    connect(this,SIGNAL(addToArchive()),
 	    _kfind,SIGNAL(addToArchive()));
    connect(this,SIGNAL(open()),
 	    _kfind,SIGNAL(open()));
    connect(_kfind ,SIGNAL(statusChanged(const char *)),
	    this,SLOT(statusChanged(const char *)));
    connect(_kfind ,SIGNAL(enableSearchButton(bool)),
	    this,SLOT(enableSearchButton(bool)));
    connect(_kfind ,SIGNAL(enableStatusBar(bool)),
            this,SLOT(enableStatusBar(bool)));
  }

KfindTop::~KfindTop()
  {
    delete _fileMenu;
    delete _editMenu;
    delete _optionMenu;
    delete _helpMenu;
    delete _kfind;
    delete _mainMenu;
    delete _toolBar;
    delete _statusBar;
  };

void KfindTop::menuInit()
{
  KStdAccel stdAccel;
  
  _fileMenu   = new QPopupMenu;
  _editMenu   = new QPopupMenu;
  _optionMenu = new QPopupMenu;
  _helpMenu   = new QPopupMenu;        
  
  fileStart = _fileMenu->insertItem(i18n("&Start search"), _kfind,
				    SLOT(startSearch()), stdAccel.find());
  fileStop = _fileMenu->insertItem(i18n("S&top search"), _kfind,
				   SLOT(stopSearch()), Key_Escape);    
  _fileMenu->setItemEnabled(fileStop, FALSE);
  _fileMenu->insertSeparator();
  
  openWithM = _fileMenu->insertItem(i18n("&Open"),
				    this,SIGNAL(open()), stdAccel.open());
  toArchM = _fileMenu->insertItem(i18n("&Add to archive"),
				  this,SIGNAL(addToArchive()));
  _fileMenu->insertSeparator();
  
  deleteM = _fileMenu->insertItem(i18n("&Delete"),
				  this,SIGNAL(deleteFile()));
  propsM = _fileMenu->insertItem(i18n("&Properties"),
				 this,SIGNAL(properties()));
  _fileMenu->insertSeparator();
  
  openFldrM = _fileMenu->insertItem(i18n("Open Containing &Folder"),
				    this,SIGNAL(openFolder()));
  _fileMenu->insertSeparator();
  
  saveSearchM = _fileMenu->insertItem(i18n("&Save Search"),
				      this, SIGNAL(saveResults()), stdAccel.save());
  _fileMenu->insertSeparator();

  quitM = _fileMenu->insertItem(i18n("&Quit"),qApp,
				SLOT(quit()),stdAccel.quit());
  for(int i=openWithM;i>quitM;i--)
    _fileMenu->setItemEnabled(i,FALSE);  
  
  int undo = _editMenu->insertItem(i18n("&Undo"),
				   this, SIGNAL(undo()), stdAccel.undo() );
  _editMenu->insertSeparator();
  int cut = _editMenu->insertItem(i18n("&Cut"),
				  this, SIGNAL(cut()), stdAccel.cut() );
  editCopy =  _editMenu->insertItem(i18n("&Copy"),
				    this, SLOT(copySelection()), stdAccel.copy() );
  _editMenu->insertSeparator();
  editSelectAll = _editMenu->insertItem(i18n("&Select All"),
					this,SIGNAL(selectAll()) );
  editUnselectAll = _editMenu->insertItem(i18n("Unse&lect All"),
					  this,SIGNAL(unselectAll()) );
  editInvertSelection = _editMenu->insertItem(i18n("&Invert Selection"),
					      this,SIGNAL(invertSelection()) );
  
  _editMenu->setItemEnabled( undo      , FALSE );
  _editMenu->setItemEnabled( cut       , FALSE );
  _editMenu->setItemEnabled( editCopy  , FALSE );
  _editMenu->setItemEnabled( editSelectAll, FALSE );
  _editMenu->setItemEnabled( editUnselectAll, FALSE );
  _editMenu->setItemEnabled( editInvertSelection, FALSE ); 
  
  _optionMenu->insertItem(i18n("&Preferences ..."),
			  this,SLOT(prefs()));
  
  QString tmp = i18n("KFind %1\nFrontend to find utility\nMiroslav Flídr <flidr@kky.zcu.cz>\n\nSpecial thanks to Stephan Kulow\n<coolo@kde.org>")
    .arg(KFIND_VERSION);
  _helpMenu=kapp->getHelpMenu( true, tmp );    
  
  _mainMenu = new KMenuBar(this, "_mainMenu");
  _mainMenu->insertItem( i18n("&File"), _fileMenu);
  _mainMenu->insertItem( i18n("&Edit"), _editMenu);
  _mainMenu->insertItem( i18n("&Options"), _optionMenu);
  _mainMenu->insertSeparator();
  _mainMenu->insertItem( i18n("&Help"), _helpMenu );
}

void KfindTop::toolBarInit()
{
  QPixmap icon;
  
  icon = Icon("search.xpm");
  _toolBar->insertButton( icon, 0, SIGNAL(clicked()),
			  _kfind, SLOT(startSearch()),
			  TRUE, i18n("Start Search"));

  icon = Icon("reload.xpm");
  _toolBar->insertButton( icon, 1, SIGNAL(clicked()),
			  _kfind, SLOT(newSearch()),
			  TRUE, i18n("New Search"));
  
  icon = Icon("stop.xpm");
  _toolBar->insertButton( icon, 2, SIGNAL(clicked()),
			  _kfind, SLOT(stopSearch()),
			  FALSE, i18n("Stop Search"));
  
  _toolBar->insertSeparator();
  
  icon = Icon("openfile.xpm");
  _toolBar->insertButton( icon, 3,SIGNAL(clicked()),
			  _kfind,SIGNAL(open()),
			  FALSE, i18n("Open"));
  
  icon = Icon("archive.xpm");
  _toolBar->insertButton( icon, 4,SIGNAL(clicked()),
			  _kfind,SIGNAL(addToArchive()),
			  FALSE, i18n("Add to archive"));
  
  icon = Icon("delete.xpm");
  _toolBar->insertButton( icon, 5,SIGNAL(clicked()),
			  _kfind,SIGNAL(deleteFile()),
			  FALSE, i18n("Delete"));
  
  icon = Icon("info.xpm");
  _toolBar->insertButton( icon, 6,SIGNAL(clicked()),
			  _kfind,SIGNAL(properties()),
			  FALSE, i18n("Properties"));
  
  icon = Icon("fileopen.xpm");
  _toolBar->insertButton( icon, 7,SIGNAL(clicked()),
			  _kfind,SIGNAL(openFolder()),
			  FALSE, i18n("Open Containing Folder"));
  
  icon = Icon("save.xpm");
  _toolBar->insertButton( icon, 8,SIGNAL(clicked()),
			  _kfind,SIGNAL(saveResults()),
			  FALSE, i18n("Save Search Results"));

  _toolBar->insertSeparator();
  
  icon = Icon("contents.xpm");
  _toolBar->insertButton( icon, 9, SIGNAL( clicked() ),
			  kapp, SLOT( appHelpActivated() ),
			  TRUE, i18n("Help"));
  
  icon = Icon("exit.xpm");
  _toolBar->insertButton( icon, 10, SIGNAL( clicked() ),
                          KApplication::getKApplication(), SLOT( quit() ),  
			  TRUE, i18n("Quit"));
}

void KfindTop::enableSaveResults(bool enable) 
{
  _toolBar->setItemEnabled(8, enable);
  _fileMenu->setItemEnabled(saveSearchM, enable);
  _editMenu->setItemEnabled(editSelectAll, enable);
  _editMenu->setItemEnabled(editUnselectAll, enable);
  _editMenu->setItemEnabled(editInvertSelection, enable);
}

void KfindTop::enableMenuItems(bool enable)
{
  int i;
  for(i=openWithM;i>quitM+1;i--)
    _fileMenu->setItemEnabled(i,enable);
  for(i=3;i<8;i++)
    _toolBar->setItemEnabled(i,enable);
  
  _editMenu->setItemEnabled( editCopy, TRUE );
}                    

void KfindTop::enableSearchButton(bool enable)
{
  _fileMenu->setItemEnabled(fileStart, enable);
  _fileMenu->setItemEnabled(fileStop, !enable);
  
  _toolBar->setItemEnabled(0,enable);
  _toolBar->setItemEnabled(2,!enable);
}

void KfindTop::enableStatusBar(bool enable)
{
  if (enable)
    KTMainWindow::enableStatusBar(KStatusBar::Show);
  else
    KTMainWindow::enableStatusBar(KStatusBar::Hide);
}

void KfindTop::statusChanged(const char *str) 
{
  _statusBar->changeItem((char *)str, 0);
}

void KfindTop::prefs() 
{
  KfOptions *prefs = new KfOptions(0L,0L);
  prefs->show();
}

void KfindTop::copySelection() 
{
  _kfind->copySelection();
}
