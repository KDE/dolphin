#ifndef _konqsidebarplugin_h_
#define _konqsidebarplugin_h_
#include <qwidget.h>
#include <qobject.h>
#include <kurl.h>
#include <qstring.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kio/job.h>

class KonqSidebar_PluginInterface;

class KonqSidebarPlugin : public QObject
{
	Q_OBJECT
	public:
		KonqSidebarPlugin(QObject *parent,QWidget *widgetParent,QString &desktopName_, const char* name=0):QObject(parent,name),desktopName(desktopName_){;}
		~KonqSidebarPlugin(){;}
		virtual QWidget *getWidget()=0;
		virtual void *provides(const QString &)=0;
                KonqSidebar_PluginInterface *getInterfaces();   
	protected:
		virtual void handleURL(const KURL &url)=0;
		QString desktopName;
	signals:
		void requestURL(KURL&);
		void started(KIO::Job *);
		void completed();
		
	public slots:
		void openURL(const KURL& url){handleURL(url);}
};

class KonqSidebar_PluginInterface
	{
		public:
		KonqSidebar_PluginInterface(){;}
		virtual ~KonqSidebar_PluginInterface(){;}
		virtual KParts::ReadOnlyPart *getPart()=0;
		virtual KParts::BrowserExtension *getExtension()=0;
  		virtual KInstance *getInstance()=0; 
	};

#endif
