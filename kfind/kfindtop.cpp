/***********************************************************************
 *
 *  KfindTop.cpp
 *
 ***********************************************************************/

#include <stdio.h>

#include <qsize.h>
#include <qpopupmenu.h>
#include <qlayout.h>
#include <qpixmap.h>

#include <kapp.h>
#include <kiconloader.h>
#include <khelpmenu.h>
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <ktmainwindow.h>

#include "kfoptions.h"
#include "kftabdlg.h"
#include "kfind.h"
#include "kfindtop.h"
#include <klocale.h>
#include <kaccel.h>
#include <kkeydialog.h>

#include "version.h"
#include <kglobal.h>

KfindTop::KfindTop(const char *searchPath) 
  : KTMainWindow()
{
//   setCaption(QString("KFind ")+KFIND_VERSION);
  _accel = new KAccel(this);
  
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
  
  _prefs = NULL;
}


KfindTop::~KfindTop()
{}

void KfindTop::menuInit()
{
  _mainMenu = new KMenuBar(this, "_mainMenu");

  _fileMenu   = new QPopupMenu(this);
  _mainMenu->insertItem( i18n("&File"), _fileMenu);

  _editMenu   = new QPopupMenu(this);
  _mainMenu->insertItem( i18n("&Edit"), _editMenu);


  _optionMenu = new QPopupMenu(this);
  _mainMenu->insertItem( i18n("&Options"), _optionMenu);
  _mainMenu->insertSeparator();

  QString aboutAuthor = i18n(""
    "KFind %1\n"
    "Frontend to find utility\n"
    "Miroslav Flídr <flidr@kky.zcu.cz>\n\n"
    "Special thanks to Stephan Kulow\n<coolo@kde.org>").arg(KFIND_VERSION);
  _mainMenu->insertItem( i18n("&Help"), helpMenu(aboutAuthor) );

  _accel->connectItem(KStdAccel::Find, _kfind, SLOT(startSearch()));
  _accel->connectItem(KStdAccel::Open, this,   SIGNAL(open()));
  _accel->connectItem(KStdAccel::Save, this,   SIGNAL(saveResults()));
  _accel->connectItem(KStdAccel::Undo, this,   SIGNAL(undo()));
  _accel->connectItem(KStdAccel::Copy, this,   SLOT(copySelection()));
  _accel->connectItem(KStdAccel::Cut,  this,   SIGNAL(cut()));
  _accel->connectItem(KStdAccel::Quit, kapp,   SLOT(quit()));
  _accel->insertItem(i18n("Stop Search"), "search", Key_Escape);
  _accel->insertItem(i18n("Delete"),      "delete", Key_Delete);

  _accel->readSettings();

  fileStart = _fileMenu->insertItem(i18n("&Start search"), _kfind,
				    SLOT(startSearch()));
  _accel->changeMenuAccel(_fileMenu, fileStart, KStdAccel::Find);
  fileStop = _fileMenu->insertItem(i18n("S&top search"), _kfind,
				   SLOT(stopSearch()));
  _accel->changeMenuAccel(_fileMenu, fileStop, "search");
  _fileMenu->setItemEnabled(fileStop, FALSE);
  _fileMenu->insertSeparator();

  openWithM = _fileMenu->insertItem(i18n("&Open"), this, SIGNAL(open()));
  _accel->changeMenuAccel(_fileMenu, openWithM, KStdAccel::Open);
  toArchM = _fileMenu->insertItem(i18n("&Add to archive"),
				  this,SIGNAL(addToArchive()));
  _fileMenu->insertSeparator();

  deleteM = _fileMenu->insertItem(i18n("&Delete"),
				  this,SIGNAL(deleteFile()));
  _accel->changeMenuAccel(_fileMenu, deleteM, "delete");
  propsM = _fileMenu->insertItem(i18n("&Properties"),
				 this,SIGNAL(properties()));
  _fileMenu->insertSeparator();

  openFldrM = _fileMenu->insertItem(i18n("&Browse Directory"),
				    this,SIGNAL(openFolder()));
  _fileMenu->insertSeparator();

  saveSearchM = _fileMenu->insertItem(i18n("&Save Search"),
				      this, SIGNAL(saveResults()));
  _accel->changeMenuAccel(_fileMenu, saveSearchM, KStdAccel::Save);
  _fileMenu->insertSeparator();

  quitM = _fileMenu->insertItem(i18n("&Quit"), kapp, SLOT(quit()));
  _accel->changeMenuAccel(_fileMenu, quitM, KStdAccel::Quit);
  for(int i=openWithM;i>quitM;i--)
    _fileMenu->setItemEnabled(i,FALSE);

  int undo = _editMenu->insertItem(i18n("&Undo"),
				   this, SIGNAL(undo()));
  _accel->changeMenuAccel(_editMenu, undo, KStdAccel::Undo);
  _editMenu->insertSeparator();
  int cut = _editMenu->insertItem(i18n("&Cut"),
				  this, SIGNAL(cut()));
  _accel->changeMenuAccel(_editMenu, cut, KStdAccel::Cut);
  editCopy =  _editMenu->insertItem(i18n("&Copy"),
				    this, SLOT(copySelection()));
  _accel->changeMenuAccel(_editMenu, editCopy, KStdAccel::Copy);
  _editMenu->insertSeparator();
  editSelectAll = _editMenu->insertItem(i18n("&Select All"),
					this,SIGNAL(selectAll()) );
  editUnselectAll = _editMenu->insertItem(i18n("Unse&lect All"),
					  this,SIGNAL(unselectAll()) );

  _editMenu->setItemEnabled( undo      , FALSE );
  _editMenu->setItemEnabled( cut       , FALSE );
  _editMenu->setItemEnabled( editCopy  , FALSE );
  _editMenu->setItemEnabled( editSelectAll, FALSE );
  _editMenu->setItemEnabled( editUnselectAll, FALSE );

  _optionMenu->insertItem(i18n("Configure &Key Bindings..."),
			  this, SLOT(keyBindings()));
  _optionMenu->insertItem(i18n("&Preferences ..."),
			  this,SLOT(prefs()));

}

void KfindTop::toolBarInit()
{
  QPixmap icon;

  icon = BarIcon("search");
  _toolBar->insertButton( icon, 0, SIGNAL(clicked()),
			  _kfind, SLOT(startSearch()),
			  TRUE, i18n("Start"));

  icon = BarIcon("stop");
  _toolBar->insertButton( icon, 1, SIGNAL(clicked()),
			  _kfind, SLOT(stopSearch()),
			  FALSE, i18n("Stop"));

  icon = BarIcon("reload");
  _toolBar->insertButton( icon, 2, SIGNAL(clicked()),
			  _kfind, SLOT(newSearch()),
			  TRUE, i18n("New"));

  _toolBar->insertSeparator();

  icon = BarIcon("openfile");
  _toolBar->insertButton( icon, 3,SIGNAL(clicked()),
			  _kfind,SIGNAL(open()),
			  FALSE, i18n("Open"));

  icon = BarIcon("archive");
  _toolBar->insertButton( icon, 4,SIGNAL(clicked()),
			  _kfind,SIGNAL(addToArchive()),
			  FALSE, i18n("Add To Archive"));

  icon = BarIcon("delete");
  _toolBar->insertButton( icon, 5,SIGNAL(clicked()),
			  _kfind,SIGNAL(deleteFile()),
			  FALSE, i18n("Delete"));

  icon = BarIcon("info");
  _toolBar->insertButton( icon, 6,SIGNAL(clicked()),
			  _kfind,SIGNAL(properties()),
			  FALSE, i18n("Properties"));

  icon = BarIcon("fileopen");
  _toolBar->insertButton( icon, 7,SIGNAL(clicked()),
			  _kfind,SIGNAL(openFolder()),
			  FALSE, i18n("Browse"));

  icon = BarIcon("save");
  _toolBar->insertButton( icon, 8,SIGNAL(clicked()),
			  _kfind,SIGNAL(saveResults()),
			  FALSE, i18n("Save Results"));

  _toolBar->insertSeparator();

  icon = BarIcon("contents");
  _toolBar->insertButton( icon, 9, SIGNAL( clicked() ),
			  this, SLOT(appHelpActivated()) );
}

void KfindTop::nameSetFocus()
{
  _kfind->setFocus();
}

void KfindTop::enableSaveResults(bool enable)
{
  _toolBar->setItemEnabled(8, enable);
  _fileMenu->setItemEnabled(saveSearchM, enable);
  _editMenu->setItemEnabled(editSelectAll, enable);
  _editMenu->setItemEnabled(editUnselectAll, enable);
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
  _toolBar->setItemEnabled(1,!enable);
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

void KfindTop::keyBindings()
{
    KKeyDialog::configureKeys(_accel, true, this );
}

void KfindTop::prefs()
{
  if(_prefs == NULL)
    _prefs = new KfOptions( this, 0L, false );
  _prefs->show();
}

void KfindTop::copySelection()
{
  _kfind->copySelection();
}
