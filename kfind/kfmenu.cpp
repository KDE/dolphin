/***********************************************************************
 *
 *  kfmenu.cpp
 *
 ***********************************************************************/

#include <unistd.h>

#include <qpopmenu.h>
#include <qkeycode.h>
#include <qframe.h>
#include <qmsgbox.h>
#include <qapp.h>
#include <stdio.h>
#include <qaccel.h>  
#include <stdlib.h>

#include <kmsgbox.h>

#include "kfmenu.h"


KfindMenu::KfindMenu( QWidget *parent, const char *name )
    : QWidget( parent, name )
  {
    QFont menu_font("Helvetica", 12,QFont::Bold);       

    file = new QPopupMenu;
    CHECK_PTR( file );
    //file->setFont(menu_font );


    openWithM  = file->insertItem("Open"          ,this,SIGNAL(open())
				  ,CTRL+Key_O );
    toArchM    = file->insertItem("Add to archive",this,SIGNAL(addToArchive()));
    file             ->insertSeparator();
    deleteM    = file->insertItem("Delete"        ,this,SIGNAL(deleteFile()));
    //renameM    = file->insertItem("Rename"        ,this,SIGNAL(renameFile()));
    propsM     = file->insertItem("Properties"    ,this,SIGNAL(properties()));
    file             ->insertSeparator();
    openFldrM  = file->insertItem("Open Containing Folder"
                                                  ,this,SIGNAL(openFolder()));
    file             ->insertSeparator();
    saveSearchM= file->insertItem("Save Search"   ,this,SIGNAL(saveResults())
				  ,CTRL+Key_S);
    file             ->insertSeparator();
    quitM      = file->insertItem("Quit"          ,qApp,SLOT(quit())
				  ,ALT+Key_Q);

    for(int i=openWithM;i>quitM;i--)
       file->setItemEnabled(i,FALSE);

    QPopupMenu *edit = new QPopupMenu;
    CHECK_PTR( edit );
    //    edit->setFont(menu_font );

    int undo =       edit->insertItem("Undo"      , this, SIGNAL(undo()) );
    edit                 ->insertSeparator();
    int cut  =       edit->insertItem("Cut"       , this, SIGNAL(cut()) );
    int copy =       edit->insertItem("Copy"      , this, SIGNAL(copy()) );
    edit                 ->insertSeparator();
    int select_all = edit->insertItem("Select All", this, SIGNAL(selectAll()) );
    int invert_sel = edit->insertItem("Invert Selection", 
                                              this, SIGNAL(invertSelection()) );

    edit->setItemEnabled( undo      , FALSE );
    edit->setItemEnabled( cut       , FALSE );
    edit->setItemEnabled( copy      , FALSE );
    edit->setItemEnabled( select_all, FALSE );
    edit->setItemEnabled( invert_sel, FALSE );

    QPopupMenu *options = new QPopupMenu;
    CHECK_PTR( options );
    //    options->setFont(menu_font );

    options->insertItem("Preferences ...",this,SIGNAL(prefs()));
    /*
    int saving    = options->insertItem("Saving  "   ,this,SIGNAL(saving()));
    int archives  = options->insertItem("Archives  " ,this,SIGNAL(archives()));
    int filetypes = options->insertItem("Filetypes  ",this,SIGNAL(filetypes()));
    */

    QPopupMenu *help = new QPopupMenu;
    CHECK_PTR( help );
    help->insertItem( "About"     , this, SIGNAL(about()));
    help->insertItem( "KFind help", this, SLOT(kfind_help()), ALT+Key_H);
    //    help->setFont(menu_font );

    menu = new QMenuBar( this );
    CHECK_PTR( menu );
    //    menu->setFont(menu_font );

    menu->insertItem( "&File"   , file );
    menu->insertItem( "&Edit"   , edit );
    menu->insertItem( "&Options", options );
    menu->insertSeparator();
    menu->insertItem( "&Help"   , help );

  };

void KfindMenu::enableSaveResults(bool enable)
  {
    file->setItemEnabled(saveSearchM,enable);    
  };

void KfindMenu::enableMenuItems(bool enable)
  {
    for(int i=openWithM;i>quitM+1;i--)
       file->setItemEnabled(i,enable);
    
  };

void KfindMenu::kfind_help()
  {
    if ( fork() == 0 )
      {
	QString helpurl = getenv("KDEDIR");
	if ( helpurl.isNull() )
	  helpurl = DOCSDIR;

	helpurl += "/doc/HTML/kfind.html";

        execlp( "kdehelp", "kdehelp", helpurl.data(), 0 );
        exit( 1 );
      }
  }; 
