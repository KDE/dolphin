#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <qlabel.h>
#include <qlayout.h>

class SidebarTest : public KonqSidebarPlugin
	{
		Q_OBJECT
		public:
		SidebarTest(QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0):
                   KonqSidebarPlugin(parent,widgetParent,desktopName_,name)
		{
			widget=new QLabel("Init Value",widgetParent);			
		}
		~SidebarTest(){;}
                virtual QWidget *getWidget(){return widget;}   
		virtual void *provides(const QString &) {return 0;}  
		protected:
			QLabel *widget;
			virtual void handleURL(const KURL &url)
				{
					widget->setText(url.url());
				}
	};

#endif
