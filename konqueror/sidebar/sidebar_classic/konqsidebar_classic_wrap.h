#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <qlabel.h>
#include <qlayout.h>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <klibloader.h>
class SidebarClassic : public KonqSidebarPlugin
	{
		Q_OBJECT
		public:
		SidebarClassic(QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0):
                   KonqSidebarPlugin(parent,widgetParent,desktopName_,name)
		{

		    KLibFactory *factory = KLibLoader::self()->factory("libkonqtree");
		    if (factory)
		    {
			dirtree = static_cast<KParts::ReadOnlyPart *>
			(static_cast<KParts::Factory *>(factory)->createPart(widgetParent,"sidebar classic",parent));
			
//		        dirtree=static_cast<KParts::ReadOnlyPart *>(factory->create(widgetParent,
//	                                "dirtree_classic", "KParts::ReadOnlyPart" ));
//		        ((KParts::ReadOnlyPart*)parent)->createGUI(dirtree);
        	}
//    	}



//		widget=new QLabel("Init Value",widgetParent);			
		}
		~SidebarClassic(){;}
                virtual QWidget *getWidget(){return dirtree->widget();}   
		protected:
			KParts::ReadOnlyPart *dirtree;
			virtual void handleURL(const KURL &url)
				{
					dirtree->openURL(url);
//					widget->setText(url.url());
				}
	};

#endif
