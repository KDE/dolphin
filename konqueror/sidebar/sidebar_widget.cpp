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
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstandarddirs.h>
#include <kdebug.h>
#include <kiconloader.h>
#include "sidebar_widget.h"
#include "sidebar_widget.moc"
#include <limits.h>
#include "konqsidebar.h"
#include <ktoolbarbutton.h>
#include <kicondialog.h>
#include <qtimer.h>
#include <kmessagebox.h>
#include <klineeditdlg.h>
#include <qdir.h>
#include <qdockarea.h>
#include <config.h>
#include <qpopupmenu.h>
#include <qsplitter.h>

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
	if(((uint)id)>=libNames.size()) return;

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


/**************************************************************/
/*                      Sidebar_Widget                        */
/**************************************************************/

Sidebar_Widget::Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const char *name)
	:QWidget(parent,name),KonqSidebar_PluginInterface()
{
	myLayout=0;
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
	dummyMainW=Area->createDockWidget("free",0);
	dummyMainW->setWidget(new QWidget(dummyMainW));
	Area->setMainDockWidget(dummyMainW);
	Area->setMinimumWidth(0);
	dummyMainW->setDockSite(KDockWidget::DockTop);
	dummyMainW->setEnableDocking(KDockWidget::DockNone);
/*                                mainW->setEnableDocking(KDockWidget::DockTop|
                                KDockWidget::DockBottom|KDockWidget::DockDesktop);*/


	connect(Area,SIGNAL(docked()),this,SLOT(updateDock()));
   	ButtonBar=new KMultiVertTabBar(this);

	Menu=new QPopupMenu(this,"Sidebar_Widget::Menu");
	QPopupMenu *addMenu=new QPopupMenu(this,"Sidebar_Widget::addPopup");
	Menu->insertItem(i18n("Add New"),addMenu,0);
	Menu->insertSeparator();
	Menu->insertItem(i18n("Multiple Views"),1);
	Menu->insertItem(i18n("Show tabs left"),2);
        connect(Menu,SIGNAL(aboutToShow()),this,SLOT(aboutToShowConfigMenu()));
	connect(Menu,SIGNAL(activated(int)),this,SLOT(activatedMenu(int)));

	buttonPopup=new QPopupMenu(this,"Sidebar_Widget::ButtonPopup");
	buttonPopup->insertItem(SmallIconSet("www"),i18n("URL"),2);
	buttonPopup->insertItem(SmallIconSet("image"),i18n("Icon"),1);
	buttonPopup->insertSeparator();
	buttonPopup->insertItem(SmallIconSet("remove"),i18n("Remove"),3);
	connect(buttonPopup,SIGNAL(activated(int)),this,SLOT(buttonPopupActivate(int)));
	ButtonBar->insertButton(SmallIcon("remove"),-2);
	connect(ButtonBar->getButton(-2),SIGNAL(clicked(int)),par,SLOT(doCloseMe()));
	// ButtonBar->insertLineSeparator();
	ButtonBar->insertButton(SmallIcon("configure"), -1, Menu,
    	    				i18n("Configure Sidebar"));
	connect(new addBackEnd(this,addMenu,"Sidebar_Widget-addBackEnd"),SIGNAL(updateNeeded()),this,SLOT(createButtons()));
	//	ButtonBar->setMinimumHeight(10);
//	ButtonBar=new QButtonGroup(this);
//	ButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
//	ButtonBar->setFixedWidth(25);

	initialCopy();

	QTimer::singleShot(0,this,SLOT(createButtons()));
//	connect(ButtonBar,SIGNAL(toggled(int)),this,SLOT(showHidePage(int)));
	connect(Area,SIGNAL(dockWidgetHasUndocked(KDockWidget*)),this,SLOT(dockWidgetHasUndocked(KDockWidget*)));
}

void Sidebar_Widget::doLayout()
{
	if (myLayout) delete myLayout;
	myLayout=new QHBoxLayout(this);
	if  (showTabsRight)
	{
		myLayout->add(Area);
		myLayout->add(ButtonBar);
		ButtonBar->setPosition(KMultiVertTabBar::Right);
	}
	else
	{
		myLayout->add(ButtonBar);
		myLayout->add(Area);
		ButtonBar->setPosition(KMultiVertTabBar::Left);

	}
	myLayout->activate();
//	savedWidth=((QWidget*)(parent()))->width();
}


void Sidebar_Widget::aboutToShowConfigMenu()
{
	Menu->setItemChecked(1,!singleWidgetMode);
	Menu->setItemChecked(2,!showTabsRight);
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
                Q_ASSERT( globalDir.isReadable() );
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
	switch (id)
	{
		case 1:
		{
			singleWidgetMode = ! singleWidgetMode;
			if ((singleWidgetMode) && (visibleViews.count()>1))
				for (int i=0; i<Buttons.count(); i++)
					if (i!=latestViewed)
					{
						if (Buttons.at(i)->dock!=0)
							if (Buttons.at(i)->dock->isVisible()) showHidePage(i);
					}
					else
					{
						if (Buttons.at(i)->dock!=0)
						{
							Area->setMainDockWidget(Buttons.at(i)->dock);
							dummyMainW->undock();
						}
					}
			else
				if (!singleWidgetMode)
				{
					int tmpLatestViewed=latestViewed;					
					Area->setMainDockWidget(dummyMainW);
	        			dummyMainW->setDockSite(KDockWidget::DockTop);
				        dummyMainW->setEnableDocking(KDockWidget::DockNone);
					dummyMainW->show();
					if ((tmpLatestViewed>=0) && (tmpLatestViewed<Buttons.count()))
					if (Buttons.at(tmpLatestViewed))
					{
						if (Buttons.at(tmpLatestViewed)->dock)
						{
							Buttons.at(tmpLatestViewed)->dock->undock();
			                                Buttons.at(tmpLatestViewed)->dock->setEnableDocking(KDockWidget::DockTop|
                                				KDockWidget::DockBottom/*|KDockWidget::DockDesktop*/);
							kdDebug()<<"Reconfiguring multi view mode"<<endl;
                                			Buttons.at(tmpLatestViewed)->dock->setDockSite(KDockWidget::DockTop|KDockWidget::DockBottom);
							Buttons.at(tmpLatestViewed)->dock->manualDock(dummyMainW,KDockWidget::DockTop,100);
							Buttons.at(tmpLatestViewed)->dock->show();
						}

					}
				}
			break;
		}
		case 2:
		{
			showTabsRight = ! showTabsRight;
			doLayout();
			break;
		}
	}
}

void Sidebar_Widget::readConfig()
{
	KConfig conf("konqsidebartng.rc");
	singleWidgetMode=(conf.readEntry("SingleWidgetMode","true")=="true");
	showTabsRight=(conf.readEntry("ShowTabsLeft","false")=="false");
	QStringList list=conf.readListEntry("OpenViews");
	kdDebug()<<"readConfig: "<<conf.readEntry("OpenViews")<<endl;
	doLayout();
	savedWidth=((QWidget*)(parent()))->width();
	somethingVisible=true;
	bool tmpSomethingVisible=false;
	for (uint i=0; i<Buttons.count();i++)
	{
		if (list.contains(Buttons.at(i)->file))
			{
				tmpSomethingVisible=true;
				somethingVisible=true;
				ButtonBar->setTab(i,true); //showHidePage(i);
				showHidePage(i);
				if (singleWidgetMode) return;
			}
	}
	if (!tmpSomethingVisible) collapseExpandSidebar();
}

void Sidebar_Widget::updateDock()
{
	kdDebug()<<"updateDock"<<endl;
}

void Sidebar_Widget::stdAction(const char *handlestd)
{
	ButtonInfo* mod=activeModule;
	if (!mod) return;
	if (!(mod->module)) return;

	kdDebug()<<"Try calling >active< module's action"<<handlestd<<endl;	

	int id = mod->module->metaObject()->findSlot( handlestd );
  	if ( id == -1 )return;
	kdDebug()<<"Action slot was found, it will be called now"<<endl;
  	QUObject o[ 1 ];
	mod->module->qt_invoke( id, o );
  	return;
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
				ButtonBar->removeTab(i);

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
	  	ButtonBar->insertTab(SmallIcon(icon), lastbtn,name);
		int id=Buttons.insert(lastbtn,new ButtonInfo(desktoppath,0,url,lib,this));
		KMultiVertTabBarTab *tab=ButtonBar->getTab(lastbtn);
		tab->installEventFilter(this);
		connect(tab,SIGNAL(clicked(int)),this,SLOT(showHidePage(int)));
	}

	return true;
}



bool Sidebar_Widget::eventFilter(QObject *obj, QEvent *ev)
{
	KMultiVertTabBarTab *bt=dynamic_cast<KMultiVertTabBarTab*>(obj);
	if (bt)
		{
			if (ev->type()==QEvent::MouseButtonPress)
				{
					if (((QMouseEvent *)ev)->button()==QMouseEvent::RightButton)
						{
							kdDebug()<<"Request for popup"<<endl;
							popupFor=-1;
							for (int i=0;i<Buttons.count();i++)
							{
								if (bt==ButtonBar->getTab(i))
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

KonqSidebarPlugin *Sidebar_Widget::loadModule(QWidget *par,QString &desktopName,QString lib_name,ButtonInfo* bi)
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
					              	return  (KonqSidebarPlugin*)func(bi,par,fullPath,0);
					            }
			        }
			    	else
		      			kdWarning() << "Module " << lib_name << " doesn't specify a library!" << endl;
			return 0;
	}

/*KParts::ReadOnlyPart *Sidebar_Widget::getPart()
	{
		return partParent;

	}
*/

KParts::BrowserExtension *Sidebar_Widget::getExtension()
	{
		return KParts::BrowserExtension::childObject(partParent);

	}

bool Sidebar_Widget::createView( ButtonInfo *data)
	{
			bool ret=true;
  	  	        KSimpleConfig *confFile;
			confFile=new KSimpleConfig(data->file,true);
			confFile->setGroup("Desktop Entry");

			data->dock=Area->createDockWidget(confFile->readEntry("Name",i18n("Unknown")),0);
			data->module=loadModule(data->dock,data->file,data->libName,data);
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
				KDockWidget::DockBottom/*|KDockWidget::DockDesktop*/);
				data->dock->setDockSite(KDockWidget::DockTop|KDockWidget::DockBottom);
				connectModule(data->module);


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
			if(ButtonBar->isTabRaised(page))
				{
					//SingleWidgetMode
					if (singleWidgetMode) if (latestViewed!=-1) showHidePage(latestViewed);
					if (!createView(info))
						{
							ButtonBar->setTab(page,false);
							return;
						}
						ButtonBar->setTab(page,true);
						if (singleWidgetMode)
						{
							Area->setMainDockWidget(info->dock);
							dummyMainW->undock();
						}
						else
						{
							info->dock->manualDock(dummyMainW,KDockWidget::DockTop,100);
						}
						info->dock->show();

						if (stored_url) info->module->openURL(storedUrl);
						visibleViews<<info->file;
						latestViewed=page;
				}
		 }
	else
		{
			if ((!info->dock->isVisible()) && (ButtonBar->isTabRaised(page)))
				{
					//SingleWidgetMode
					if (singleWidgetMode) if (latestViewed!=-1) showHidePage(latestViewed);
					if (singleWidgetMode)
					{
						Area->setMainDockWidget(info->dock);
						dummyMainW->undock();
					}
					else
					{
						info->dock->manualDock(dummyMainW,KDockWidget::DockTop,100);
					}
					info->dock->show();
					latestViewed=page;
					if (stored_url) info->module->openURL(storedUrl);
					visibleViews<<info->file;
					ButtonBar->setTab(page,true);
				} else
				{
//					if (ButtonBar->
					ButtonBar->setTab(page,false);
					if (singleWidgetMode)
					{
						Area->setMainDockWidget(dummyMainW);
						dummyMainW->show();
					}
					info->dock->undock();
					latestViewed=-1;
					visibleViews.remove(info->file);
				}

		}

	collapseExpandSidebar();	
}

void Sidebar_Widget::collapseExpandSidebar()
{
	if ((somethingVisible) && (visibleViews.count()==0))
	{
    		QValueList<int> list = ((QSplitter*)parent()->parent())->sizes();
		QValueList<int>::Iterator it = list.begin();
		savedWidth=*it;
		(*it)=ButtonBar->width();
		((QSplitter*)parent()->parent())->setSizes(list);

		((QWidget*)parent())->setMaximumWidth(ButtonBar->width());
		somethingVisible=false;
	}
	else
	if ((!somethingVisible) && (visibleViews.count()!=0))
	{
		somethingVisible=true;
		
		((QWidget*)parent())->setMaximumWidth(32767);
    		QValueList<int> list = ((QSplitter*)parent()->parent())->sizes();
		QValueList<int>::Iterator it = list.begin();
		if (it!=list.end()) (*it)=savedWidth;
		((QSplitter*)parent()->parent())->setSizes(list);
	}
}

void Sidebar_Widget::dockWidgetHasUndocked(KDockWidget* wid)
{
	kdDebug()<<" Sidebar_Widget::dockWidgetHasUndocked(KDockWidget*)"<<endl;
	for (unsigned int i=0;i<Buttons.count();i++)
	{
		if (Buttons.at(i)->dock==wid)
			{
				if (ButtonBar->isTabRaised(i)) ButtonBar->setTab(i,false);
//				latestViewed=-1;
//				visibleViews.remove(Buttons.at(i)->file);
//				break;
			}
	}
}

KInstance  *Sidebar_Widget::getInstance()
{
	return ((KonqSidebar*)partParent)->getInstance();
}

void Sidebar_Widget::openURLRequest( const KURL &url, const KParts::URLArgs &args)
{getExtension()->openURLRequest(url,args);}

void Sidebar_Widget::createNewWindow( const KURL &url, const KParts::URLArgs &args)
{getExtension()->createNewWindow(url,args);}

void Sidebar_Widget::createNewWindow( const KURL &url, const KParts::URLArgs &args,
	const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part )
{getExtension()->createNewWindow(url,args,windowArgs,part);}

void Sidebar_Widget::enableAction( const char * name, bool enabled )
{
	ButtonInfo *btninfo=dynamic_cast<ButtonInfo*>(sender()->parent());
	if (btninfo)
	{
		if (QString(name)=="copy") btninfo->copy=enabled;
		if (QString(name)=="cut") btninfo->cut=enabled;
		if (QString(name)=="paste") btninfo->paste=enabled;
		if (QString(name)=="trash") btninfo->trash=enabled;
		if (QString(name)=="del") btninfo->del=enabled;
		if (QString(name)=="shred") btninfo->shred=enabled;
		if (QString(name)=="rename") btninfo->rename=enabled;
	}
}


bool  Sidebar_Widget::doEnableActions()
{
	activeModule=dynamic_cast<ButtonInfo*>(sender()->parent());
	if (activeModule==0)
	{
		kdDebug()<<"Couldn't set active module, aborting"<<endl;
		return false;
	}
	else
	{
		getExtension()->enableAction( "copy", activeModule->copy );
		getExtension()->enableAction( "cut", activeModule->cut );
		getExtension()->enableAction( "paste", activeModule->paste );
		getExtension()->enableAction( "trash", activeModule->trash );
		getExtension()->enableAction( "del", activeModule->del );
		getExtension()->enableAction( "shred", activeModule->shred );
		getExtension()->enableAction( "rename", activeModule->rename );
		return true;
	}

}

void Sidebar_Widget::popupMenu( const QPoint &global, const KFileItemList &items )
{
	if (doEnableActions()) getExtension()->popupMenu(global,items);

}


void Sidebar_Widget::popupMenu( KXMLGUIClient *client, const QPoint &global, const KFileItemList &items )
{
	if (doEnableActions()) getExtension()->popupMenu(client,global,items);
}

void Sidebar_Widget::popupMenu( const QPoint &global, const KURL &url,
	const QString &mimeType, mode_t mode)
{
	if (doEnableActions()) getExtension()->popupMenu(global,url,mimeType,mode);
}

void Sidebar_Widget::popupMenu( KXMLGUIClient *client,
	const QPoint &global, const KURL &url,
	const QString &mimeType, mode_t mode )
{
	if (doEnableActions()) getExtension()->popupMenu(client,global,url,mimeType,mode);
}

void Sidebar_Widget::connectModule(QObject *mod)
{
	connect(mod,SIGNAL(started(KIO::Job *)),this, SIGNAL(started(KIO::Job*)));
	connect(mod,SIGNAL(completed()),this,SIGNAL(completed()));
	connect(mod,SIGNAL(popupMenu( const QPoint &, const KURL &,
		const QString &, mode_t)),this,SLOT(popupMenu( const
		QPoint &, const KURL&, const QString &, mode_t)));
	connect(mod,SIGNAL(popupMenu( KXMLGUIClient *, const QPoint &,
		const KURL &,const QString &, mode_t)),this,
		SLOT(popupMenu( KXMLGUIClient *, const QPoint &,
		const KURL &,const QString &, mode_t)));

	connect(mod,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )),
		this,SLOT(popupMenu( const QPoint &, const KFileItemList & )));

	connect(mod,SIGNAL(openURLRequest( const KURL &, const KParts::URLArgs &)),
		this,SLOT(openURLRequest( const KURL &, const KParts::URLArgs &)));

	connect(mod,SIGNAL(enableAction( const char *, bool)),
		this,SLOT(enableAction(const char *, bool)));

#if 0
/*?????*/
					connect(browserExtCli,SIGNAL(setLocationBarURL( const QString &)),
					browserExtMst,SIGNAL(setLocationBarURL( const QString &)));
					connect(browserExtCli,SIGNAL(setIconURL( const KURL &)),
					browserExtMst,SIGNAL(setIconURL( const KURL &)));
/*?????*/
					connect(browserExtCli,SIGNAL(infoMessage( const QString & )),
					browserExtMst,SIGNAL(infoMessage( const QString & )));
#endif
}


Sidebar_Widget::~Sidebar_Widget()
{
	KConfig conf("konqsidebartng.rc");
	conf.writeEntry("SingleWidgetMode",singleWidgetMode?"true":"false");
        conf.writeEntry("OpenViews", visibleViews);
	conf.sync();
	for (uint i=0;i<Buttons.count();i++)
	{
		if (Buttons.at(i)->dock!=0)
			Buttons.at(i)->dock->undock();
	}

}

