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
#include <kicondialog.h>
#include <qtimer.h>
#include <kmessagebox.h>
#include <klineeditdlg.h>
#include <qdir.h>
#include <config.h>
#include <qpopupmenu.h>
#include <konq_events.h>
#include <kfileitem.h>
#include <kio/netaccess.h>
#include <kpopupmenu.h>

#include <qwhatsthis.h>

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
  menu->insertSeparator();
  menu->insertItem(i18n("Rollback to System Default"),i);
}


void addBackEnd::doRollBack()
{
      if (KMessageBox::questionYesNo(0,i18n("<qt>This removes all your entries from the sidebar and adds the system default ones.<BR><B>This procedure is irreversible</B><BR>Do you want to proceed?</qt>"))==KMessageBox::Yes)
	{
	       KStandardDirs *dirs = KGlobal::dirs();
	       QString loc=dirs->saveLocation("data","konqsidebartng/",true);
	       QDir dir(loc);
	       QStringList dirEntries = dir.entryList( QDir::Dirs | QDir::NoSymLinks );
	       dirEntries.remove(".");
	       dirEntries.remove("..");
		       for ( QStringList::Iterator it = dirEntries.begin(); it != dirEntries.end(); ++it ) {
			if ((*it)!="add") KIO::NetAccess::del(loc+(*it));
       		}
       		emit initialCopyNeeded();
	}
}

void addBackEnd::activatedAddMenu(int id)
{
	kdDebug()<<"activatedAddMenu: " << QString("%1").arg(id)<<endl;
	if (((uint)id)==libNames.size()) doRollBack();
	if(((uint)id)>=libNames.size()) return;

	KLibLoader *loader = KLibLoader::self();

        // try to load the library
	QString libname=*libNames.at(id);
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
	:QWidget(parent,name)
{
	m_initial=true;
	deleting=false;
	connect(this,SIGNAL(destroyed()),this,SLOT(slotDeleted()));
	noUpdate=false;
	myLayout=0;
	activeModule=0;
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


   	ButtonBar=new KMultiTabBar(this,KMultiTabBar::Vertical);
	Menu=new QPopupMenu(this,"Sidebar_Widget::Menu");
	QPopupMenu *addMenu=new QPopupMenu(this,"Sidebar_Widget::addPopup");
	Menu->insertItem(i18n("Add New"),addMenu,0);
	Menu->insertSeparator();
	Menu->insertItem(i18n("Multiple Views"),1);
	Menu->insertItem(i18n("Show Tabs Left"),2);
	Menu->insertItem(i18n("Show Configuration Button"),3);
	Menu->insertSeparator();
	Menu->insertItem(SmallIconSet("remove"),i18n("Close Navigation Panel"),par,SLOT(deleteLater()));
        connect(Menu,SIGNAL(aboutToShow()),this,SLOT(aboutToShowConfigMenu()));
	connect(Menu,SIGNAL(activated(int)),this,SLOT(activatedMenu(int)));

	buttonPopup=new KPopupMenu(this,"Sidebar_Widget::ButtonPopup");
	buttonPopup->insertTitle(SmallIcon("unknown"),"",50);
	buttonPopup->insertItem(SmallIconSet("www"),i18n("URL"),2);
	buttonPopup->insertItem(SmallIconSet("image"),i18n("Icon"),1);
	buttonPopup->insertSeparator();
	buttonPopup->insertItem(SmallIconSet("remove"),i18n("Remove"),3);
	buttonPopup->insertSeparator();
	buttonPopup->insertItem(SmallIconSet("configure"),i18n("Configure Navigation Panel"), Menu, 4);
	connect(buttonPopup,SIGNAL(activated(int)),this,SLOT(buttonPopupActivate(int)));
	addBackEnd *ab=new addBackEnd(this,addMenu,"Sidebar_Widget-addBackEnd");
	connect(ab,SIGNAL(updateNeeded()),this,SLOT(createButtons()));
	connect(ab,SIGNAL(initialCopyNeeded()),this,SLOT(finishRollBack()));

//	ButtonBar->setMinimumHeight(10);
//	ButtonBar=new QButtonGroup(this);
//	ButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
//	ButtonBar->setFixedWidth(25);

	initialCopy();

	QTimer::singleShot(0,this,SLOT(createButtons()));
//	connect(ButtonBar,SIGNAL(toggled(int)),this,SLOT(showHidePage(int)));
	connect(Area,SIGNAL(dockWidgetHasUndocked(KDockWidget*)),this,SLOT(dockWidgetHasUndocked(KDockWidget*)));
}

void Sidebar_Widget::finishRollBack()
{
        PATH=KGlobal::dirs()->saveLocation("data","konqsidebartng/entries/",true);
        initialCopy();
        QTimer::singleShot(0,this,SLOT(createButtons()));
}

void Sidebar_Widget::slotDeleted()
{
	deleting=true;
}

void Sidebar_Widget::doLayout()
{
	if (myLayout) delete myLayout;
	myLayout=new QHBoxLayout(this);
	if  (showTabsRight)
	{
		myLayout->add(Area);
		myLayout->add(ButtonBar);
		ButtonBar->setPosition(KMultiTabBar::Right);
	}
	else
	{
		myLayout->add(ButtonBar);
		myLayout->add(Area);
		ButtonBar->setPosition(KMultiTabBar::Left);

	}
	myLayout->activate();
//	savedWidth=((QWidget*)(parent()))->width();
}


void Sidebar_Widget::aboutToShowConfigMenu()
{
	Menu->setItemChecked(1,!singleWidgetMode);
	Menu->setItemChecked(2,!showTabsRight);
	Menu->setItemChecked(3,showExtraButtons);
}


void Sidebar_Widget::initialCopy()
{
	kdDebug()<<"Initial copy"<<endl;
	   QString dirtree_dir = KGlobal::dirs()->findDirs("data","konqsidebartng/entries/").last();
	    if (dirtree_dir==PATH) return; // oups?
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
			if (KMessageBox::questionYesNo(this,i18n("<qt>Do you really want to remove the <b>\"%1\"</b> tab?</qt>").
				arg(Buttons.at(popupFor)->displayName))==KMessageBox::Yes)
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
				for (uint i=0; i<Buttons.count(); i++)
					if ((int) i != latestViewed)
					{
						if (Buttons.at(i)->dock!=0)
							if (Buttons.at(i)->dock->isVisibleTo(this)) showHidePage(i);
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
					if ((tmpLatestViewed>=0) && (tmpLatestViewed < (int) Buttons.count()))
					if (Buttons.at(tmpLatestViewed))
					{
						if (Buttons.at(tmpLatestViewed)->dock)
						{
							noUpdate=true;
							Buttons.at(tmpLatestViewed)->dock->undock();
			                                Buttons.at(tmpLatestViewed)->dock->setEnableDocking(KDockWidget::DockTop|
                                				KDockWidget::DockBottom/*|KDockWidget::DockDesktop*/);
							kdDebug()<<"Reconfiguring multi view mode"<<endl;
							ButtonBar->setTab(tmpLatestViewed,true);
							showHidePage(tmpLatestViewed);
/*                                			Buttons.at(tmpLatestViewed)->dock->setDockSite(KDockWidget::DockTop|KDockWidget::DockBottom);
							Buttons.at(tmpLatestViewed)->dock->manualDock(dummyMainW,KDockWidget::DockTop,100);
							Buttons.at(tmpLatestViewed)->dock->show();
*/
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
		case 3:
		{
			showExtraButtons = ! showExtraButtons;
			if(showExtraButtons)
			{
				ButtonBar->insertButton(SmallIcon("configure"), -1, Menu, i18n("Configure Sidebar"));
//JW - TEST				ButtonBar->insertButton(SmallIcon("remove"),-2);
//JW - TEST				connect(ButtonBar->getButton(-2),SIGNAL(clicked(int)),partParent,SLOT(deleteLater()));
			}
			else
			{
				KMessageBox::information(this,
				i18n("You have hidden the navigation panel configuration button. To make it visible again, click the right mouse button on any of the navigation panel buttons and select \"Show Configuration Button\"."));


				ButtonBar->removeButton(-1);
//JW -TEST				ButtonBar->removeButton(-2);
			}
			break;
		}
	}
}

void Sidebar_Widget::readConfig()
{
	KConfig conf("konqsidebartng.rc");
	singleWidgetMode=(conf.readEntry("SingleWidgetMode","true")=="true");
	showExtraButtons=(conf.readEntry("ShowExtraButtons","true")=="true");
	showTabsRight=(conf.readEntry("ShowTabsLeft","true")=="false");
	QStringList list=conf.readListEntry("OpenViews");
	kdDebug()<<"readConfig: "<<conf.readEntry("OpenViews")<<endl;
	doLayout();
	if (m_initial) savedWidth=((QWidget*)(parent()))->width();

	bool tmpSomethingVisible=m_initial?false:somethingVisible;
	somethingVisible=false;
	for (uint i=0; i<Buttons.count();i++)
	{
		if (list.contains(Buttons.at(i)->file))
			{
//				tmpSomethingVisible=true;
				somethingVisible=true;
				ButtonBar->setTab(i,true); //showHidePage(i);
				noUpdate=true;
				showHidePage(i);
				if (singleWidgetMode) break;
			}
	}

	if (m_initial)
	{
		somethingVisible=true;
		collapseExpandSidebar();
	}
	else
	{
		if (somethingVisible != tmpSomethingVisible)
		{
			somethingVisible=tmpSomethingVisible;
		}
  		collapseExpandSidebar();
	}
        noUpdate=false;
	m_initial=false;

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
		for (uint i=0;i<Buttons.count();i++)
			{
				if (Buttons.at(i)->dock!=0)
				{
					noUpdate=true;					
					if (Buttons.at(i)->dock->isVisibleTo(this)) showHidePage(i);
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
	
	if(showExtraButtons)
	{
		if (!ButtonBar->getButton(-1))
		{
			ButtonBar->insertButton(SmallIcon("configure"), -1, Menu, i18n("Configure Sidebar"));
//JW - TEST			ButtonBar->insertButton(SmallIcon("remove"),-2);
//JW - TEST			connect(ButtonBar->getButton(-2),SIGNAL(clicked(int)),partParent,SLOT(deleteLater()));
		}
	}
	
	// we want to keep our size when the splitter is resized!
	QWidget* qparent=static_cast<QWidget*>(parent());
	
	QValueList<int> list = ((QSplitter*)parent()->parent())->sizes();
	QValueList<int>::Iterator it = list.begin();
	if (it!=list.end()) (*it)=qparent->width();
	((QSplitter*)parent()->parent())->setSizes(list);
	
	static_cast<QSplitter*>( qparent->parentWidget() )->setResizeMode( qparent, QSplitter::KeepSize );
}

bool Sidebar_Widget::openURL(const class KURL &url)
{
	storedUrl=url;
	stored_url=true;
	ButtonInfo *info;
        bool ret = false;
	for (unsigned int i=0;i<Buttons.count();i++)
		{
			if ((info=Buttons.at(i))->dock!=0)
				{
					if ((info->dock->isVisibleTo(this)) && (info->module))
                                        {
                                            ret = true;
                                            info->module->openURL(url);
                                        }
				}
		}
        return ret;
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
	QString comment=confFile->readEntry("Comment","");
	QString url=confFile->readEntry("URL","");
	QString lib=confFile->readEntry("X-KDE-KonqSidebarModule","");

        delete confFile;



	if (pos==-1)
	{
	  	ButtonBar->insertTab(SmallIcon(icon), lastbtn,name);
		/*int id=*/Buttons.insert(lastbtn,new ButtonInfo(desktoppath,0,url,lib,name,icon,this));
		KMultiTabBarTab *tab=ButtonBar->getTab(lastbtn);
		tab->installEventFilter(this);
		connect(tab,SIGNAL(clicked(int)),this,SLOT(showHidePage(int)));

		// Set Whats This help
		// This uses the comments in the .desktop files
		QWhatsThis::add(tab, comment);
	}

	return true;
}



bool Sidebar_Widget::eventFilter(QObject *obj, QEvent *ev)
{

	if (ev->type()==QEvent::MouseButtonPress && ((QMouseEvent *)ev)->button()==QMouseEvent::RightButton)
	{
			KMultiTabBarTab *bt=dynamic_cast<KMultiTabBarTab*>(obj);
			if (bt)
			{
				kdDebug()<<"Request for popup"<<endl;
				popupFor=-1;
				for (uint i=0;i<Buttons.count();i++)
				{
					if (bt==ButtonBar->getTab(i))
						{popupFor=i; break;}
				}
				
				if (popupFor!=-1)
				{
					buttonPopup->setItemEnabled(2,!Buttons.at(popupFor)->URL.isEmpty());
				        buttonPopup->changeTitle(50,SmallIcon(Buttons.at(popupFor)->iconName),
						Buttons.at(popupFor)->displayName);
       					buttonPopup->changeItem(2,i18n("Set URL"));
        				buttonPopup->changeItem(1,i18n("Set Icon"));
				        buttonPopup->changeItem(3,i18n("Remove"));
					buttonPopup->exec(QCursor::pos());
				}
				return true;
				
			}
	}
	return false;
}

void Sidebar_Widget::mousePressEvent(QMouseEvent *ev)
{
	if (ev->type()==QEvent::MouseButtonPress && ((QMouseEvent *)ev)->button()==QMouseEvent::RightButton)
		Menu->exec(QCursor::pos());
}

KonqSidebarPlugin *Sidebar_Widget::loadModule(QWidget *par,QString &desktopName,QString lib_name,ButtonInfo* bi)
	{

 				KLibLoader *loader = KLibLoader::self();

      				// try to load the library
      				KLibrary *lib = loader->library(QFile::encodeName(lib_name));
      				if (lib)
        			{
         			 	// get the create_ function
          				QString factory("create_%1");
          				void *create = lib->symbol(QFile::encodeName(factory.arg(lib_name)));

          				if (create)
            					{
				        	   	// create the module

					              	KonqSidebarPlugin* (*func)(KInstance*,QObject *, QWidget*, QString&, const char *);
					              	func = (KonqSidebarPlugin* (*)(KInstance*,QObject *, QWidget *, QString&, const char *)) create;
							QString fullPath(PATH+desktopName);
					              	return  (KonqSidebarPlugin*)func(getInstance(),bi,par,fullPath,0);
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
        connect(this,         SIGNAL(fileSelection(const KFileItemList&)),
                data->module, SLOT(openPreview(const KFileItemList&)));

        connect(this,         SIGNAL(fileMouseOver(const KFileItem&)),
                data->module, SLOT(openPreviewOnMouseOver(const KFileItem&)));

			}

			delete confFile;
			return ret;

	}

void Sidebar_Widget::showHidePage(int page)
{
	ButtonInfo *info=Buttons.at(page);
	if (!info->dock)
		{
			if(ButtonBar->isTabRaised(page))
				{
					//SingleWidgetMode
					if (singleWidgetMode)
						if (latestViewed!=-1)
						{
							noUpdate=true;
							showHidePage(latestViewed);
						}
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
					if (singleWidgetMode)
						if (latestViewed!=-1)
						{
							noUpdate=true;
							showHidePage(latestViewed);
						}
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

	if (!noUpdate) collapseExpandSidebar();
	noUpdate=false;
}

void Sidebar_Widget::collapseExpandSidebar()
{
	if ((somethingVisible) && (visibleViews.count()==0))
	{
		QGuardedPtr<QObject> p;
		p=parent();
		if (!p) return;
		p=p->parent();
		if (!p) return;

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
//	if (deleting) return;
	kdDebug()<<" Sidebar_Widget::dockWidgetHasUndocked(KDockWidget*)"<<endl;
	for (unsigned int i=0;i<Buttons.count();i++)
	{
		if (Buttons.at(i)->dock==wid)
			{
				if (ButtonBar->isTabRaised(i))
				{
					ButtonBar->setTab(i,false);
					showHidePage(i);
				}

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
 	if (sender()->parent()->isA("ButtonInfo"))
	{
		ButtonInfo *btninfo=static_cast<ButtonInfo*>(sender()->parent());
//		ButtonInfo *btninfo=dynamic_cast<ButtonInfo*>(sender()->parent());
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
}


bool  Sidebar_Widget::doEnableActions()
{

//	activeModule=dynamic_cast<ButtonInfo*>(sender()->parent());
//	if (!activeModule)
 	if (!(sender()->parent()->isA("ButtonInfo")))
	{
		kdDebug()<<"Couldn't set active module, aborting"<<endl;
		return false;
	}
	else
	{
		activeModule=static_cast<ButtonInfo*>(sender()->parent());
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
        if ((mod->metaObject()->findSignal("started(KIO::Job*)"))!=-1)
		connect(mod,SIGNAL(started(KIO::Job *)),this, SIGNAL(started(KIO::Job*)));

        if ((mod->metaObject()->findSignal("completed()"))!=-1)
		connect(mod,SIGNAL(completed()),this,SIGNAL(completed()));



        if ((mod->metaObject()->findSignal("popupMenu(const QPoint&,const KURL&,const QString&,mode_t)"))!=-1)
		connect(mod,SIGNAL(popupMenu( const QPoint &, const KURL &,
			const QString &, mode_t)),this,SLOT(popupMenu( const
			QPoint &, const KURL&, const QString &, mode_t)));

        if ((mod->metaObject()->findSignal("popupMenu(KXMLGUIClient*,const QPoint&,const KURL&,const QString&,mode_t)"))!=-1)
		connect(mod,SIGNAL(popupMenu( KXMLGUIClient *, const QPoint &,
			const KURL &,const QString &, mode_t)),this,
			SLOT(popupMenu( KXMLGUIClient *, const QPoint &,
			const KURL &,const QString &, mode_t)));

        if ((mod->metaObject()->findSignal("popupMenu(const QPoint&,const KFileItemList&)"))!=-1)
		connect(mod,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )),
			this,SLOT(popupMenu( const QPoint &, const KFileItemList & )));

        if ((mod->metaObject()->findSignal("openURLRequest(const KURL&,const KParts::URLArgs&)"))!=-1)
		connect(mod,SIGNAL(openURLRequest( const KURL &, const KParts::URLArgs &)),
			this,SLOT(openURLRequest( const KURL &, const KParts::URLArgs &)));

        if ((mod->metaObject()->findSignal("enableAction(const char*,bool)"))!=-1)
		connect(mod,SIGNAL(enableAction( const char *, bool)),
			this,SLOT(enableAction(const char *, bool)));

	if ((mod->metaObject()->findSignal("createNewWindow(const KURL&,const KParts::URLArgs&)"))!=-1)
                connect(mod,SIGNAL(createNewWindow( const KURL &, const KParts::URLArgs &)),
                        this,SLOT(createNewWindow( const KURL &, const KParts::URLArgs &)));

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
	conf.writeEntry("ShowExtraButtons",showExtraButtons?"true":"false");
	conf.writeEntry("ShowTabsLeft",showTabsRight?"false":"true");
        conf.writeEntry("OpenViews", visibleViews);
	conf.sync();
	for (uint i=0;i<Buttons.count();i++)
	{
		if (Buttons.at(i)->dock!=0)
			Buttons.at(i)->dock->undock();
	}

}

void Sidebar_Widget::customEvent(QCustomEvent* ev)
{
	if (KonqFileSelectionEvent::test(ev))
		emit fileSelection(static_cast<KonqFileSelectionEvent*>(ev)->selection());
  else if (KonqFileMouseOverEvent::test(ev))
  {
		if (!(static_cast<KonqFileMouseOverEvent*>(ev)->item())) 
			emit fileMouseOver(KFileItem(KURL(),QString::null,KFileItem::Unknown));
		else
		emit fileMouseOver(*static_cast<KonqFileMouseOverEvent*>(ev)->item());
  }

}
