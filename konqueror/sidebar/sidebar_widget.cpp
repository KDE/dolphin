#include "sidebar_widget.h"
#include <kdockwidget.h>
#include <qwidget.h>
#include <qpushbutton.h>
#include <qbuttongroup.h>
#include <qvector.h>
#include <ktoolbar.h>
#include <klocale.h>
#include <ksimpleconfig.h>
#include <kstddirs.h>

Sidebar_Widget::Sidebar_Widget(QWidget *parent, const char *name):QHBox(parent,name)
{
	latestViewed=-1;
	setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	Area=new KDockArea(this);
	Area->setSizePolicy(QSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding));
	mainW=Area->createDockWidget("free",0);
	mainW->setWidget(new QWidget(mainW));
	Area->setMainDockWidget(mainW);
	Area->setMinimumWidth(0);
	mainW->setDockSite(KDockWidget::DockTop);
	mainW->setEnableDocking(KDockWidget::DockNone);
   	ButtonBar=new KToolBar(this,"hallo",true);
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

void Sidebar_Widget::createButtons()
{
	//PARSE ALL DESKTOP FILES
	Buttons.resize(0);
	KStandardDirs *dirs = KGlobal::dirs(); 
        QStringList list=dirs->findAllResources("data","konqsidebartng/*.desktop",false,true);
 	
  	for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) addButton(*it);
	    	
}



bool Sidebar_Widget::addButton(const QString &desktoppath,int pos)
{

	int lastbtn=Buttons.count();
	Buttons.resize(Buttons.size()+1);

  	KSimpleConfig *confFile;

	confFile=new KSimpleConfig(desktoppath,true);
	confFile->setGroup("Desktop Entry");
 
    	QString icon=confFile->readEntry("Icon","");
//	QString libname=confFile->readEntry("author","");

        delete confFile;



	if (pos==-1)
	{
	  	ButtonBar->insertButton(icon, lastbtn, true,
    	    				i18n("Configure this dialog"));
	  	ButtonBar->setToggle(lastbtn);
		Buttons.insert(lastbtn,new ButtonInfo(desktoppath,0,0));
	}
	
	return true;
}

bool Sidebar_Widget::createView( ButtonInfo *data)
	{
  	  	        KSimpleConfig *confFile;
			confFile=new KSimpleConfig(data->file,true);
			confFile->setGroup("Desktop Entry");

			data->dock=Area->createDockWidget(confFile->readEntry("Name",i18n("Unknown")),0);
//			data->widget=createPlugin(data->dock,data->->file);			
			data->widget=new QLabel(data->file,data->dock);
			data->dock->setWidget(data->widget);
			data->dock->setEnableDocking(KDockWidget::DockTop|
			KDockWidget::DockBottom|KDockWidget::DockDesktop);
			data->dock->setDockSite(KDockWidget::DockTop|KDockWidget::DockBottom);	

			delete confFile;
			return true;

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
					return;
				}
			
			info->dock->manualDock(mainW,KDockWidget::DockTop,100);
			info->dock->show();
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
				} else {ButtonBar->setButton(page,false); info->dock->undock(); latestViewed=-1;};

		}
	
}

Sidebar_Widget::~Sidebar_Widget()
{
}
