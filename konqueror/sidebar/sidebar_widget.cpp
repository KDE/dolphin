/***************************************************************************
                               sidebar_widget.cpp
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include <kdockwidget.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qvector.h>
#include <ktoolbar.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>
#include <klibloader.h>
#include "konqsidebarplugin.h"
#include <qfile.h>
#include <kdebug.h>
#include <qstring.h>
#include <kparts/browserextension.h>
#include <qmetaobject.h>
#include <qpopupmenu.h>
#include <kiconloader.h>
#include "sidebar_widget.h"
#include "sidebar_widget.moc"
#include <qmap.h>
#include <limits.h>
#include "konqsidebar.h"
#include <ktoolbarbutton.h>
#include <kicondialog.h>
#include <qtimer.h>
#include <kmessagebox.h>
#include <klineeditdlg.h>
#include <qdir.h>


QString  Sidebar_Widget::PATH=QString(""); 

addBackEnd::addBackEnd(QObject *parent,class QPopupMenu *addmenu,const char *name):QObject(parent,name)
{
	menu=addmenu;
	connect(menu,SIGNAL(aboutToShow()),this,SLOT(aboutToShowAddMenu()));
	connect(menu,SIGNAL(activated(int)),this,SLOT(activatedAddMenu(int)));
}

void addBackEnd::aboutToShowAddMenu()
{
  if (!menu) return;
  KStandardDirs *dirs = KGlobal::dirs(); 
  QStringList list=dirs->findAllResources("data","konqsidebartng/add/*.desktop",true,true);
  libNames.setAutoDelete(true);
  libNames.resize(0); 	
  libParam.setAutoDelete(true);
  libParam.resize(0); 	
  menu->clear();
  int i=0;
  for (QStringList::Iterator it = list.begin(); it != list.end(); ++it, i++ )
  {
  	KSimpleConfig *confFile;

	confFile=new KSimpleConfig(*it,true);
	confFile->setGroup("Desktop Entry");
    	QString icon=confFile->readEntry("Icon","");
	if (!icon.isEmpty())
		menu->insertItem(SmallIcon(icon),confFile->readEntry("Name",""),i);
	else
		menu->insertItem(confFile->readEntry("Name",""),i);
	libNames.resize(libNames.size()+1);
	libNames.insert(libNames.count(),new QString(confFile->readEntry("X-KDE-KonqSidebarAddModule","")));
	libParam.resize(libParam.size()+1);
	libParam.insert(libParam.count(),new QString(confFile->readEntry("X-KDE-KonqSidebarAddParam","")));
	delete confFile;
	
  }
}

void addBackEnd::activatedAddMenu(int id)
{
	kdDebug()<<"activatedAddMenu: " << QString("%1").arg(id)<<endl;
	if(id>=libNames.size()) return;
	
	KLibLoader *loader = KLibLoader::self();
 
        // try to load the library
        QString libname("lib");
	libname=libname+(*libNames.at(id));
        KLibrary *lib = loader->library(QFile::encodeName(libname));
        if (lib)
                {
                // get the create_ function
                QString factory("add_");
		factory=factory+(*libNames.at(id));
                void *add = lib->symbol(QFile::encodeName(factory));
 
                if (add)
                        {
                        //call the add function
                        bool (*func)(QString*, QString*, QMap<QString,QString> *);
                        QMap<QString,QString> map;
                        func = (bool (*)(QString*, QString*, QMap<QString,QString> *)) add;
                        QString *tmp=new QString("");
                        if (func(tmp,libParam.at(id),&map))
                                {
					KStandardDirs *dirs = KGlobal::dirs();
				        dirs->saveLocation("data","konqsidebartng/entries/",true);
					tmp->prepend("/konqsidebartng/entries/");
					QString myFile;
					QString filename;
					filename=tmp->arg("");
					kdDebug()<<"filename part is "<<filename<<endl;
					bool found=false;
					myFile = locateLocal("data",filename);
					if (QFile::exists(myFile))
						{
							kdDebug()<<"Searching for new possible entry"<<endl;
							for (ulong l=0;l<ULONG_MAX;l++)
								{
									kdDebug()<<myFile<<endl;
									filename=tmp->arg(l);
									myFile=locateLocal("data",filename);
									if (!QFile::exists(myFile))
										{
											found=true;
											break;
										}
								}
						}
					else found=true;
					if (found)
						{
							KSimpleConfig scf(myFile,false);
							scf.setGroup("Desktop Entry");
							for (QMap<QString,QString>::ConstIterator it=map.begin(); it!=map.end();++it)
								scf.writeEntry(it.key(),it.data());
							scf.sync();
							emit updateNeeded();

						}
					else kdWarning()<<"No unique filename found"<<endl;
                                }
                        else
                                {kdWarning()<< "No new entry (error?)"<<endl;}
                        delete tmp;
                        }
                }
                else
                        kdWarning() << "libname:"<< libNames.at(id) << " doesn't specify a library!" << endl;	
}


Sidebar_Widget::Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const char *name):QHBox(parent,name),KonqSidebar_PluginInterface()
{
        kdDebug()<<"**** Sidebar_Widget:SidebarWidget()"<<endl;
        PATH=KGlobal::dirs()->saveLocation("data","konqsidebartng/entries/",true);
	Buttons.resize(0);
	Buttons.setAutoDelete(true);
	stored_url=false;
	latestViewed=-1;
	partParent=par;
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	Area=new KDockArea(this);
	Area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	mainW=Area->createDockWidget("free",0);
	mainW->setWidget(new QWidget(mainW));
	Area->setMainDockWidget(mainW);
	Area->setMinimumWidth(0);
	mainW->setDockSite(KDockWidget::DockTop);
	mainW->setEnableDocking(KDockWidget::DockNone);
	connect(Area,SIGNAL(docked()),this,SLOT(updateDock()));
   	ButtonBar=new Sidebar_ButtonBar(this);
	ButtonBar->setIconText(KToolBar::IconOnly);
#ifdef __GNUC__
#warning setIconSize(16) for the buttonbar is only a temporary solution for missing 22x22 icons
#endif
	ButtonBar->setIconSize(16);
   	ButtonBar->enableMoving(false);
	ButtonBar->setOrientation(Qt::Vertical);

	Menu=new QPopupMenu(this,"Sidebar_Widget::Menu");
	QPopupMenu *addMenu=new QPopupMenu(this,"Sidebar_Widget::addPopup");
	Menu->insertItem(i18n("Add new"),addMenu,0);
	Menu->insertSeparator();
	Menu->insertItem(i18n("Multiple views"),1);
        connect(Menu,SIGNAL(aboutToShow()),this,SLOT(aboutToShowConfigMenu()));
	connect(Menu,SIGNAL(activated(int)),this,SLOT(activatedMenu(int)));

	buttonPopup=new QPopupMenu(this,"Sidebar_Widget::ButtonPopup");
	buttonPopup->insertItem(SmallIconSet("www"),i18n("URL"),2);
	buttonPopup->insertItem(SmallIconSet("image"),i18n("Icon"),1);
	buttonPopup->insertSeparator();
	buttonPopup->insertItem(SmallIconSet("remove"),i18n("Remove"),3);
	connect(buttonPopup,SIGNAL(activated(int)),this,SLOT(buttonPopupActivate(int)));
#ifdef __GNUC__
#warning replace SmallIcon with BarIcon when 22x22 icons are all available
#endif
	ButtonBar->insertButton(SmallIcon("remove"),-2);
	connect(ButtonBar->getButton(-2),SIGNAL(clicked(int)),par,SLOT(doCloseMe()));
	ButtonBar->insertLineSeparator();
#ifdef __GNUC__
#warning replace SmallIcon with BarIcon when 22x22 icons are all available
#endif
	ButtonBar->insertButton(SmallIcon("configure"), -1, Menu,true,
    	    				i18n("Configure Sidebar"));
	connect(new addBackEnd(this,addMenu,"Sidebar_Widget-addBackEnd"),SIGNAL(updateNeeded()),this,SLOT(createButtons()));
	ButtonBar->setMinimumHeight(10);
//	ButtonBar=new QButtonGroup(this);
//	ButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
//	ButtonBar->setFixedWidth(25);

	initialCopy();

	QTimer::singleShot(0,this,SLOT(createButtons()));
	connect(ButtonBar,SIGNAL(toggled(int)),this,SLOT(showHidePage(int)));
	connect(Area,SIGNAL(dockWidgetHasUndocked(KDockWidget*)),this,SLOT(dockWidgetHasUndocked(KDockWidget*)));
}


void Sidebar_Widget::aboutToShowConfigMenu()
{
	Menu->setItemChecked(1,!singleWidgetMode);
}


void Sidebar_Widget::initialCopy()
{
	   QString dirtree_dir = KGlobal::dirs()->findDirs("data","konqsidebartng/entries/").last();
            if ( !dirtree_dir.isEmpty() && dirtree_dir != PATH )
            {
		   KSimpleConfig gcfg(dirtree_dir+".version");
		   KSimpleConfig lcfg(PATH+".version");
		   int gversion=gcfg.readNumEntry("Version",1);
		   if (lcfg.readNumEntry("Version",0)>=gversion) return;

 	        QDir dir(PATH);
    	        QStringList entries = dir.entryList( QDir::Files );
                QStringList dirEntries = dir.entryList( QDir::Dirs | QDir::NoSymLinks );
                dirEntries.remove( "." );
                dirEntries.remove( ".." );

                QDir globalDir( dirtree_dir );
                ASSERT( globalDir.isReadable() );
                // Only copy the entries that don't exist yet in the local dir
                QStringList globalDirEntries = globalDir.entryList();
                QStringList::ConstIterator eIt = globalDirEntries.begin();
                QStringList::ConstIterator eEnd = globalDirEntries.end();
                for (; eIt != eEnd; ++eIt )
                {
                    //kdDebug(1201) << "KonqSidebarTree::scanDir dirtree_dir contains " << *eIt << endl;
                    if ( *eIt != "." && *eIt != ".."
                         && !entries.contains( *eIt ) && !dirEntries.contains( *eIt ) )
                    { // we don't have that one yet -> copy it.
                        QString cp = QString("cp -R %1%2 %3").arg(dirtree_dir).arg(*eIt).arg(PATH);
                        kdDebug() << "SidebarWidget::intialCopy executing " << cp << endl;
                        ::system( cp.local8Bit().data() );
                    }
                }
		lcfg.writeEntry("Version",gversion);
		lcfg.sync();
	   }
}

void Sidebar_Widget::buttonPopupActivate(int id)
{
	switch (id)
	{
		case 1:
		{
			KIconDialog kicd(this);
//			kicd.setStrictIconSize(true);
			QString iconname=kicd.selectIcon(KIcon::Small);
			kdDebug()<<"New Icon Name:"<<iconname<<endl;;
			if (!iconname.isEmpty())
				{
					KSimpleConfig ksc(PATH+Buttons.at(popupFor)->file);
					ksc.setGroup("Desktop Entry");
					ksc.writeEntry("Icon",iconname);
					ksc.sync();
				        QTimer::singleShot(0,this,SLOT(createButtons()));  
				}
			break;
		}
		case 2:
		{
			  bool okval;
			  QString newurl=KLineEditDlg::getText(i18n("Enter a URL"), Buttons.at(popupFor)->URL,&okval,this);
			  if ((okval) && (!newurl.isEmpty()))
				{
                                        KSimpleConfig ksc(PATH+Buttons.at(popupFor)->file);
                                        ksc.setGroup("Desktop Entry");
                                        ksc.writeEntry("Name",newurl);
                                        ksc.writeEntry("URL",newurl);
                                        ksc.sync();
                                        QTimer::singleShot(0,this,SLOT(createButtons()));
				}
			break;
		}
		case 3:
		{
			if (KMessageBox::questionYesNo(this,i18n("Do you really want to delete this entry ?"))==KMessageBox::Yes)
				{
					QFile f(PATH+Buttons.at(popupFor)->file);
					if (!f.remove()) qDebug("Error, file not deleted");
				        QTimer::singleShot(0,this,SLOT(createButtons()));  
				}	
			break;
		}
	}

}

void Sidebar_Widget::activatedMenu(int id)
{
	if (id==1)
		{
			singleWidgetMode = ! singleWidgetMode;
			if ((singleWidgetMode) && (visibleViews.count()>1))
				for (int i=0; i<Buttons.count(); i++)
					if (i!=latestViewed)
						if (Buttons.at(i)->dock!=0)
							if (Buttons.at(i)->dock->isVisible()) showHidePage(i);
		}
}

void Sidebar_Widget::readConfig()
{
	KConfig conf("konqsidebartng.rc");
	singleWidgetMode=(conf.readEntry("SingleWidgetMode","true")=="true");
	QStringList list=conf.readListEntry("OpenViews");
	kdDebug()<<"readConfig: "<<conf.readEntry("OpenViews")<<endl;
	for (int i=0; i<Buttons.count();i++)
	{
		if (list.contains(Buttons.at(i)->file))
			{
				ButtonBar->setButton(i,true); //showHidePage(i);				
				if (singleWidgetMode) return;
			} 
	}
}

void Sidebar_Widget::updateDock()
{
	kdDebug()<<"updateDock"<<endl;
}

ButtonInfo* Sidebar_Widget::getActiveModule()
{
#ifdef __GNUC__
#warning implement correctly for multiple views
#endif
        ButtonInfo *info;
        for (unsigned int i=0;i<Buttons.count();i++)
                if ((info=Buttons.at(i))->dock!=0)                      
                      if ((info->dock->isVisible()) && (info->module)) return info;
	return 0;
}

void Sidebar_Widget::stdAction(const char *handlestd)
{
	ButtonInfo* mod=getActiveModule();
	if (!mod) return;
	KParts::BrowserExtension *ext;
	if ((ext=(KParts::BrowserExtension*)mod->module->provides("KParts::BrowserExtension")))
		{
			QMetaData *md=ext->metaObject()->slot(handlestd);
			if (md)
			{
				(ext->*((void(QObject::*)())md->ptr))();
			}
		}
}


void Sidebar_Widget::createButtons()
{
	//PARSE ALL DESKTOP FILES
	if (Buttons.count()>0)
	{
		for (int i=0;i<Buttons.count();i++)
			{
				if (Buttons.at(i)->dock!=0)
				{
					if (Buttons.at(i)->dock->isVisible()) showHidePage(i);
					if (Buttons.at(i)->module!=0) delete Buttons.at(i)->module;
					delete Buttons.at(i)->dock;
				}
				ButtonBar->removeItem(i);

			}
	}
	Buttons.resize(0);

	if (!PATH.isEmpty())
		{
			kdDebug()<<"PATH: "<<PATH<<endl;
			QDir dir(PATH);
			QStringList list=dir.entryList("*.desktop");
			for (QStringList::Iterator it=list.begin(); it!=list.end(); ++it)
				{
					addButton(*it);
				}
		}

//        QStringList list=dirs->findAllResources("data","konqsidebartng/entries/*.desktop",false,true);
//	if (list.count()==0) kdDebug()<<"*** No Modules found"<<endl;
//  	for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) addButton(*it);
	readConfig();	    	
}

void Sidebar_Widget::openURL(const class KURL &url)
{
	storedUrl=url;
	stored_url=true;
	ButtonInfo *info;
	for (unsigned int i=0;i<Buttons.count();i++)
		{
			if ((info=Buttons.at(i))->dock!=0)
				{
					if ((info->dock->isVisible()) && (info->module)) info->module->openURL(url);
				}
		}
}

bool Sidebar_Widget::addButton(const QString &desktoppath,int pos)
{

	int lastbtn=Buttons.count();
	Buttons.resize(Buttons.size()+1);

  	KSimpleConfig *confFile;
	
	kdDebug()<<"addButton:"<<(PATH+desktoppath)<<endl;

	confFile=new KSimpleConfig(PATH+desktoppath,true);
	confFile->setGroup("Desktop Entry");
 
    	QString icon=confFile->readEntry("Icon","");
	QString name=confFile->readEntry("Name","");
	QString url=confFile->readEntry("URL","");
	QString lib=confFile->readEntry("X-KDE-KonqSidebarModule","");

        delete confFile;



	if (pos==-1)
	{
	  	ButtonBar->insertButton(icon, lastbtn, true,name);
	  	ButtonBar->setToggle(lastbtn);
		int id=Buttons.insert(lastbtn,new ButtonInfo(desktoppath,0,url,lib));
		ButtonBar->getButton(lastbtn)->installEventFilter(this);
	}
	
	return true;
}



bool Sidebar_Widget::eventFilter(QObject *obj, QEvent *ev)
{
	KToolBarButton *bt=dynamic_cast<KToolBarButton*>(obj);
	if (obj)
		{
			if (ev->type()==QEvent::MouseButtonPress)
				{
					if (((QMouseEvent *)ev)->button()==QMouseEvent::RightButton)
						{
							kdDebug()<<"Request for popup"<<endl;
							popupFor=-1;
							for (int i=0;i<Buttons.count();i++)
							{
								if (bt==ButtonBar->getButton(i))
									{popupFor=i; break;}
							}
							
							buttonPopup->setItemEnabled(2,!Buttons.at(popupFor)->URL.isEmpty());
							if (popupFor!=-1)
								buttonPopup->exec(QCursor::pos());
							return true;
						}
					return false;
				}
		}
	return false;
}

KonqSidebarPlugin *Sidebar_Widget::loadModule(QWidget *par,QString &desktopName,QString lib_name)
	{

 				KLibLoader *loader = KLibLoader::self();

      				// try to load the library
      				QString libname("lib%1");
      				KLibrary *lib = loader->library(QFile::encodeName(libname.arg(lib_name)));
      				if (lib)
        			{
         			 	// get the create_ function
          				QString factory("create_%1");
          				void *create = lib->symbol(QFile::encodeName(factory.arg(lib_name)));

          				if (create)
            					{
				        	   	// create the module

					              	KonqSidebarPlugin* (*func)(QObject *, QWidget*, QString&, const char *);
					              	func = (KonqSidebarPlugin* (*)(QObject *, QWidget *, QString&, const char *)) create;
							QString fullPath(PATH+desktopName);
					              	return  (KonqSidebarPlugin*)func(this,par,fullPath,0);
					            }
			        }
			    	else
		      			kdWarning() << "Module " << lib_name << " doesn't specify a library!" << endl;
			return 0;
	}

KParts::ReadOnlyPart *Sidebar_Widget::getPart()
	{
		return partParent;

	}


KParts::BrowserExtension *Sidebar_Widget::getExtension()
	{
		return KParts::BrowserExtension::childObject(partParent);

	}

KInstance  *Sidebar_Widget::getInstance()
	{
		kdDebug()<<"Sidebar_Widget::getInstance()"<<endl;
		return ((KonqSidebar*)partParent)->getInstance();
	}

bool Sidebar_Widget::createView( ButtonInfo *data)
	{
			bool ret=true;
  	  	        KSimpleConfig *confFile;
			confFile=new KSimpleConfig(data->file,true);
			confFile->setGroup("Desktop Entry");

			data->dock=Area->createDockWidget(confFile->readEntry("Name",i18n("Unknown")),0);
			data->module=loadModule(data->dock,data->file,data->libName);
			if (data->module==0)
			{
				delete data->dock;
				data->dock=0;
				ret=false;
				
			}
			else
			{
				data->dock->setWidget(data->module->getWidget());
				data->dock->setEnableDocking(KDockWidget::DockTop|
				KDockWidget::DockBottom|KDockWidget::DockDesktop);
				data->dock->setDockSite(KDockWidget::DockTop|KDockWidget::DockBottom);	
				KParts::BrowserExtension *browserExtCli;
				KParts::BrowserExtension *browserExtMst=KParts::BrowserExtension::childObject(partParent);
				connect(data->module,SIGNAL(started(KIO::Job *)),
					this, SIGNAL(started(KIO::Job*)));
		                connect(data->module,SIGNAL(completed()),this,SIGNAL(completed()));
				if ((browserExtCli=(KParts::BrowserExtension*)data->module->provides("KParts::BrowserExtension"))!=0)
					{
					connect(browserExtCli,SIGNAL(popupMenu( const QPoint &, const KURL &,
				                  const QString &, mode_t)),browserExtMst,SIGNAL(popupMenu( const 
							QPoint &, const KURL&, const QString &, mode_t)));

					connect(browserExtCli,SIGNAL(popupMenu( KXMLGUIClient *, const QPoint &, 
						const KURL &,const QString &, mode_t)),browserExtMst,
						SIGNAL(popupMenu( KXMLGUIClient *, const QPoint &, 
						const KURL &,const QString &, mode_t)));

					connect(browserExtCli,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )),
					browserExtMst,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )));

					connect(browserExtCli,SIGNAL(openURLRequest( const KURL &, const KParts::URLArgs &)),
					browserExtMst,SIGNAL(openURLRequest( const KURL &, const KParts::URLArgs &))); 


/*?????*/
					connect(browserExtCli,SIGNAL(setLocationBarURL( const QString &)),
					browserExtMst,SIGNAL(setLocationBarURL( const QString &)));
					connect(browserExtCli,SIGNAL(setIconURL( const KURL &)),
					browserExtMst,SIGNAL(setIconURL( const KURL &)));
/*?????*/
					connect(browserExtCli,SIGNAL(infoMessage( const QString & )),
					browserExtMst,SIGNAL(infoMessage( const QString & )));
 
					}

			}

			delete confFile;
			return ret;

	}

void Sidebar_Widget::showHidePage(int page)
{
	qDebug("ShowHidePage");
	ButtonInfo *info=Buttons.at(page);
	if (!info->dock)
		{
			if(ButtonBar->isButtonOn(page))
				{
					//SingleWidgetMode
					if (singleWidgetMode) if (latestViewed!=-1) showHidePage(latestViewed);
					if (!createView(info))
						{
							ButtonBar->setButton(page,false);
							return;
						}
						ButtonBar->setButton(page,true);			
						info->dock->manualDock(mainW,KDockWidget::DockTop,100);
						info->dock->show();
						if (stored_url) info->module->openURL(storedUrl);
						visibleViews<<info->file;
						latestViewed=page;
				}
		 }
	else
		{
			if ((!info->dock->isVisible()) && (ButtonBar->isButtonOn(page)))
				{
					//SingleWidgetMode
					if (singleWidgetMode) if (latestViewed!=-1) showHidePage(latestViewed);			
					info->dock->manualDock(mainW,KDockWidget::DockTop,100);
					info->dock->show();
					latestViewed=page;
					if (stored_url) info->module->openURL(storedUrl);
					visibleViews<<info->file;
					ButtonBar->setButton(page,true);
				} else
				{
//					if (ButtonBar->
					ButtonBar->setButton(page,false);
					info->dock->undock();
					latestViewed=-1;
					visibleViews.remove(info->file);
				}

		}
	
}

void Sidebar_Widget::dockWidgetHasUndocked(KDockWidget* wid)
{
	kdDebug()<<" Sidebar_Widget::dockWidgetHasUndocked(KDockWidget*)"<<endl;
	for (unsigned int i=0;i<Buttons.count();i++)
	{
		if (Buttons.at(i)->dock==wid)
			{
				if (ButtonBar->isButtonOn(i)) ButtonBar->setButton(i,false);
//				latestViewed=-1;
//				visibleViews.remove(Buttons.at(i)->file);
//				break;
			}
	}
}


Sidebar_Widget::~Sidebar_Widget()
{
	KConfig conf("konqsidebartng.rc");
	conf.writeEntry("SingleWidgetMode",singleWidgetMode?"true":"false");
        conf.writeEntry("OpenViews", visibleViews);
	conf.sync();
	for (int i=0;i<Buttons.count();i++)
	{
		if (Buttons.at(i)->dock!=0)
			Buttons.at(i)->dock->undock();
	}

}

