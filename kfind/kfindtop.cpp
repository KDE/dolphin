/***********************************************************************
 *
 *  KfindTop.cpp
 *
 ***********************************************************************/

#include <kapp.h>
#include "ktopwidget.h"
#include <kmenubar.h>
#include <ktoolbar.h>
#include <kstatusbar.h>
#include <kmsgbox.h>
#include <qsize.h>

#include <qpopmenu.h>
#include <qlayout.h>
#include <qpixmap.h>
#include <qkeycode.h>
#include <qaccel.h>

#include "kfoptions.h"
#include "kftabdlg.h"
#include "kfind.h"
#include "kfindtop.h"
#include <klocale.h>

#include "version.h" 

KfindTop::KfindTop(const char *searchPath) : KTopLevelWidget()
  {
    setCaption(QString("KFind ")+KFIND_VERSION);

    _mainMenu = new KMenuBar(this, "_mainMenu");
    _mainMenu->show();
    setMenu(_mainMenu);
    _mainMenu->enableMoving(false);

    _toolBar = new KToolBar( this, "_toolBar" );
    _toolBar->setBarPos( KToolBar::Top );      
    _toolBar->show();
    enableToolBar( KToolBar::Show, addToolBar( _toolBar ) );

    _kfind = new Kfind(this,"dialog",searchPath);   
    setView( _kfind, FALSE );
    _kfind->show();

    toolBarInit();
    menuInit();

    _statusBar = new KStatusBar( this, "_statusBar");
    _statusBar->insertItem("0 file(s) found", 0);
    _statusBar->enable(KStatusBar::Hide);
    setStatusBar( _statusBar );

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

    int width=(440>_toolBar->width())?440:_toolBar->width();
    int height=_mainMenu->height()+_toolBar->height()+(_kfind->sizeHint()).height()+10;
    //the window never should be smaller
    setMinimumSize(width,height);
    setMaximumSize(9999,height);

    resize(width,height);
   };    

KfindTop::~KfindTop()
  {
    delete _fileMenu;
    delete _editMenu;
    delete _optionMenu;
    delete _helpMenu;
  };

void KfindTop::resizeEvent( QResizeEvent *e )
  {
    KTopLevelWidget::resizeEvent(e);
  };

void KfindTop::help()
  {
    KApplication::getKApplication()->invokeHTMLHelp("", "");
  };

void KfindTop::about()
  {
    QString tmp;
    
    tmp.sprintf(trans.translate("_abouttext",
				"KFind %s\nFrontend to find utility
                                \nMiroslav Flídr <flidr@kky.zcu.cz>"),
		KFIND_VERSION);

    KMsgBox::message(this,"about box",tmp,KMsgBox::INFORMATION, "OK");
  };

void KfindTop::menuInit()
  {
    _fileMenu   = new QPopupMenu;
    _editMenu   = new QPopupMenu;
    _optionMenu = new QPopupMenu;
    _helpMenu   = new QPopupMenu;        

    _mainMenu->insertItem( trans.translate("&File"), _fileMenu);
    _mainMenu->insertItem( trans.translate("&Edit"), _editMenu);
    _mainMenu->insertItem( trans.translate("&Options"), _optionMenu);
    _mainMenu->insertSeparator();
    _mainMenu->insertItem( trans.translate("&Help"), _helpMenu );


    openWithM  = _fileMenu->insertItem(trans.translate("Open"),
				       this,SIGNAL(open()), CTRL+Key_O );
    toArchM    = _fileMenu->insertItem(trans.translate("Add to archive"),
				       this,SIGNAL(addToArchive()));
    _fileMenu             ->insertSeparator();
    deleteM    = _fileMenu->insertItem(trans.translate("Delete"),
				       this,SIGNAL(deleteFile()));
    propsM     = _fileMenu->insertItem(trans.translate("Properties"),
				       this,SIGNAL(properties()));
    _fileMenu             ->insertSeparator();
    openFldrM  = _fileMenu->insertItem(trans.translate("Open Containing Folder"),
				       this,SIGNAL(openFolder()));
    _fileMenu             ->insertSeparator();
    saveSearchM= _fileMenu->insertItem(trans.translate("Save Search"),
				       this,SIGNAL(saveResults()),CTRL+Key_S);
    _fileMenu             ->insertSeparator();
    quitM      = _fileMenu->insertItem(trans.translate("Quit"),qApp,
				       SLOT(quit()),ALT+Key_Q);

    for(int i=openWithM;i>quitM;i--)
       _fileMenu->setItemEnabled(i,FALSE);  
   
    int undo =       _editMenu->insertItem(trans.translate("Undo"),
					   this, SIGNAL(undo()) );
    _editMenu                 ->insertSeparator();
    int cut  =       _editMenu->insertItem(trans.translate("Cut"),
					   this, SIGNAL(cut()) );
    int copy =       _editMenu->insertItem(trans.translate("Copy"),
					   this,SIGNAL(copy()) );
    _editMenu                 ->insertSeparator();
    int select_all = _editMenu->insertItem(trans.translate("Select All"),
					   this,SIGNAL(selectAll()) );
    int invert_sel = _editMenu->insertItem(trans.translate("Invert Selection"),
					   this,SIGNAL(invertSelection()) );

    _editMenu->setItemEnabled( undo      , FALSE );
    _editMenu->setItemEnabled( cut       , FALSE );
    _editMenu->setItemEnabled( copy      , FALSE );
    _editMenu->setItemEnabled( select_all, FALSE );
    _editMenu->setItemEnabled( invert_sel, FALSE ); 

    CHECK_PTR( _optionMenu ); 

    _optionMenu->insertItem(trans.translate("Preferences ..."),
			    this,SLOT(prefs()));
    //_optionMenu->insertItem("Configure key bindings",this,SIGNAL(keys()));

    _helpMenu->insertItem(trans.translate("Kfind help"),
			  this, SLOT(help()),ALT+Key_H );
    _helpMenu->insertSeparator();
    _helpMenu->insertItem(trans.translate("About"), this, SLOT( about() ) );  
  };

void KfindTop::toolBarInit()
  {
    QString directory = KApplication::getKApplication()->kdedir()
      + QString("/lib/pics/");
    QPixmap icon;


    icon.load(directory + "toolbar/viewzoom.xpm");
    _toolBar->insertButton( icon, 0, SIGNAL(clicked()),
			    _kfind, SLOT(startSearch()),
			    TRUE, trans.translate("Start Search"));

    icon.load(directory + "toolbar/reload.xpm");
    _toolBar->insertButton( icon, 1, SIGNAL(clicked()),
			    _kfind, SLOT(newSearch()),
			    TRUE, trans.translate("New Search"));

    icon.load(directory + "toolbar/stop.xpm");
    _toolBar->insertButton( icon, 2, SIGNAL(clicked()),
			    _kfind, SLOT(stopSearch()),
			    FALSE, trans.translate("Stop Search"));

    _toolBar->insertSeparator();


    icon.load(directory + "toolbar/idea.xpm");
    _toolBar->insertButton( icon, 3,SIGNAL(clicked()),
			    _kfind,SIGNAL(open()),
			    FALSE, trans.translate("Open"));

    icon.load(directory + "toolbar/archive.xpm");
    _toolBar->insertButton( icon, 4,SIGNAL(clicked()),
			    _kfind,SIGNAL(addToArchive()),
			    FALSE, trans.translate("Add to archive"));

    icon.load(directory + "toolbar/delete.xpm");
    _toolBar->insertButton( icon, 5,SIGNAL(clicked()),
			    _kfind,SIGNAL(deleteFile()),
			    FALSE, trans.translate("Delete"));

    icon.load(directory + "toolbar/info.xpm");
    _toolBar->insertButton( icon, 6,SIGNAL(clicked()),
			    _kfind,SIGNAL(properties()),
			    FALSE, trans.translate("Properties"));

    icon.load(directory + "toolbar/fileopen.xpm");
    _toolBar->insertButton( icon, 7,SIGNAL(clicked()),
			    _kfind,SIGNAL(openFolder()),
			    FALSE, trans.translate("Open Containing Folder"));

    icon.load(directory + "toolbar/save.xpm");
    _toolBar->insertButton( icon, 8,SIGNAL(clicked()),
			    _kfind,SIGNAL(saveResults()),
			    FALSE, trans.translate("Save Search Results"));

    _toolBar->insertSeparator();
    icon.load(directory + "toolbar/contents.xpm");
    _toolBar->insertButton( icon, 9, SIGNAL( clicked() ),
			  this, SLOT( help() ),
			  TRUE, trans.translate("Help"));

    icon.load(directory + "toolbar/exit.xpm");
    _toolBar->insertButton( icon, 10, SIGNAL( clicked() ),
                          KApplication::getKApplication(), SLOT( quit() ),  
			  TRUE, trans.translate("Quit"));
  };

void KfindTop::enableSaveResults(bool enable)
  {
    _fileMenu->setItemEnabled(saveSearchM,enable);
    _toolBar->setItemEnabled(8,enable);
  };

void KfindTop::enableMenuItems(bool enable)
  {
    for(int i=openWithM;i>quitM+1;i--)
      _fileMenu->setItemEnabled(i,enable);
    for(int i=3;i<8;i++)
      _toolBar->setItemEnabled(i,enable);    
  };                    

void KfindTop::enableSearchButton(bool enable)
  {
    _toolBar->setItemEnabled(0,enable);
    _toolBar->setItemEnabled(2,!enable);
  };

void KfindTop::enableStatusBar(bool enable)
  {
    int height=_mainMenu->height()+_toolBar->height()+(_kfind->sizeHint()).height()+10;
    if ( enable )
      {
	setMaximumSize(9999,9999);
	resize(width(),height+_statusBar->height()+200);
	_statusBar->enable(KStatusBar::Show);
      }
    else
      {
	_statusBar->enable(KStatusBar::Hide);
	setMaximumSize(9999,height);
	resize(width(),height);
      };
  };

void KfindTop::statusChanged(const char *str)
  {
    _statusBar->changeItem((char *)str, 0);
  };

void KfindTop::prefs()
  {
    //    KfOptions *prefs = new KfOptions(0L,0L);
    KfOptions *prefs = new KfOptions(0L,0L);

    prefs->show();
  };
