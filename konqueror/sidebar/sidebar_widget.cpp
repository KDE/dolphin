#include "sidebar_widget.h"
#include "sidebar_widget.moc"
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

Sidebar_Widget::Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const char *name):QHBox(parent,name)
{
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
   	ButtonBar=new Sidebar_ButtonBar(this);
	ButtonBar->setIconText(KToolBar::IconOnly);
    	ButtonBar->enableMoving(false);
	ButtonBar->setOrientation(Qt::Vertical);
	ButtonBar->insertButton(QString::fromLatin1("configure"), -1, true,
    	    				i18n("Configure this dialog"));
	ButtonBar->setMinimumHeight(10);
//	ButtonBar=new QButtonGroup(this);
//	ButtonBar->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
//	ButtonBar->setFixedWidth(25);
	createButtons();
	connect(ButtonBar,SIGNAL(toggled(int)),this,SLOT(showHidePage(int)));
}


ButtonInfo* Sidebar_Widget::getActiveModule()
{
#warning implement correctly for multiple views
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
	if (ext=(KParts::BrowserExtension*)mod->module->provides("KParts::BrowserExtension"))
		{
			QMetaData *md=ext->metaObject()->slot(handlestd);
			if (md)
			{
				((void(*)())md->ptr)();
			}
		}
}


void Sidebar_Widget::createButtons()
{
	//PARSE ALL DESKTOP FILES
	Buttons.resize(0);
	KStandardDirs *dirs = KGlobal::dirs(); 
        QStringList list=dirs->findAllResources("data","konqsidebartng/*.desktop",false,true);
 	
  	for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) addButton(*it);
	    	
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

	confFile=new KSimpleConfig(desktoppath,true);
	confFile->setGroup("Desktop Entry");
 
    	QString icon=confFile->readEntry("Icon","");
	QString name=confFile->readEntry("Name","");

        delete confFile;



	if (pos==-1)
	{
	  	ButtonBar->insertButton(icon, lastbtn, true,name);
	  	ButtonBar->setToggle(lastbtn);
		Buttons.insert(lastbtn,new ButtonInfo(desktoppath,0,0));
	}
	
	return true;
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
					              	return  (KonqSidebarPlugin*)func(this,par,desktopName,0);
					            }
			        }		    
			    	else
		      			kdWarning() << "Module " << lib_name << " doesn't specify a library!" << endl;
			return 0;
	}

bool Sidebar_Widget::createView( ButtonInfo *data)
	{
			bool ret=true;
  	  	        KSimpleConfig *confFile;
			confFile=new KSimpleConfig(data->file,true);
			confFile->setGroup("Desktop Entry");

			data->dock=Area->createDockWidget(confFile->readEntry("Name",i18n("Unknown")),0);
			data->module=loadModule(data->dock,data->file,confFile->readEntry("X-KDE-KonqSidebarModule",""));
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
 
//					connect(browserExtCli,SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs)),
//					browserExtMst,SIGNAL( openURLRequest( const KURL &, const KParts::URLArgs))); 
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
			//SingleWidgetMode
			if (latestViewed!=-1) showHidePage(latestViewed);
			if (!createView(info))
				{
					ButtonBar->setButton(page,false);
					return;
				}
			
			info->dock->manualDock(mainW,KDockWidget::DockTop,100);
			info->dock->show();
			if (stored_url) info->module->openURL(storedUrl);
			latestViewed=page;
		}
	else
		{
			if (!info->dock->isVisible())
				{
					//SingleWidgetMode
					if (latestViewed!=-1) showHidePage(latestViewed);			
					info->dock->manualDock(mainW,KDockWidget::DockTop,100);
					info->dock->show();
					latestViewed=page;
					if (stored_url) info->module->openURL(storedUrl);
				} else {ButtonBar->setButton(page,false); info->dock->undock(); latestViewed=-1;};

		}
	
}

Sidebar_Widget::~Sidebar_Widget()
{
}
