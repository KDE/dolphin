#ifndef _konqsidebarplugin_h_
#define _konqsidebarplugin_h_
#include <qwidget.h>
#include <qobject.h>
#include <kurl.h>
#include <qstring.h>

class KonqSidebarPlugin : public QObject
{
	Q_OBJECT
	public:
		KonqSidebarPlugin(QObject *parent,QWidget *widgetParent,QString &desktopName_, const char* name=0):QObject(parent,name),desktopName(desktopName_){;}
		~KonqSidebarPlugin(){;}
		virtual QWidget *getWidget()=0;
		virtual void *provides(const QString &)=0;
	protected:
		virtual void handleURL(const KURL &url)=0;
		QString desktopName;
	signals:
		void requestURL(KURL&);
	public slots:
		void openURL(const KURL& url){handleURL(url);}
};

#endif
