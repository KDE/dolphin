#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <qlabel.h>
#include <qlayout.h>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <klibloader.h>
#include <kxmlgui.h>
class SidebarClassic : public KonqSidebarPlugin
	{
		Q_OBJECT
		public:
		SidebarClassic(QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0):
                   KonqSidebarPlugin(parent,widgetParent,desktopName_,name)
		{

		    KLibFactory *lfactory = KLibLoader::self()->factory("libkonqtree");
		    if (lfactory)
		    {
			dirtree = static_cast<KParts::ReadOnlyPart *>
			(static_cast<KParts::Factory *>(lfactory)->createPart(widgetParent,"sidebar classic",parent));
        	}
	}
		~SidebarClassic(){;}
		virtual void *provides(const QString &pro)
		{
			if (pro=="KParts::ReadOnlyPart") return dirtree;
			return 0;
		}
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
