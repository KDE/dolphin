#ifndef _SIDEBAR_WIDGET_
#define _SIDEBAR_WIDGET_
#include <qhbox.h>
#include <qpushbutton.h>
#include <kdockwidget.h>
#include <qvector.h>
#include <kurl.h>
#include <ktoolbar.h>

class ButtonInfo: public QObject
{
	Q_OBJECT
	public:
	ButtonInfo(const QString& file_,class KDockWidget *dock_, int tmp):
		file(file_),dock(dock_)
		{;}
	~ButtonInfo(){;}
	class QString file;
	class KDockWidget *dock;
	class KonqSidebarPlugin *module;	
};

class Sidebar_ButtonBar: public KToolBar
{
  Q_OBJECT
  public:
        Sidebar_ButtonBar(QWidget *parent):KToolBar(parent,"Konq::SidebarTNG",true){setAcceptDrops(true);}
	~Sidebar_ButtonBar(){;}  
};

class Sidebar_Widget: public QHBox
{
  Q_OBJECT
  public:
  Sidebar_Widget(QWidget *parent, const char * name);
  ~Sidebar_Widget();
  void openURL(const class KURL &url);
  private:
	class KDockArea *Area;
	class KToolBar *ButtonBar;
        QVector<ButtonInfo> Buttons;
	void createButtons();
	bool addButton(const QString &desktoppath,int pos=-1);
	bool createView(ButtonInfo *data);
	class KDockWidget *mainW;
	int latestViewed;
	class KonqSidebarPlugin *loadModule(QWidget *par,QString &desktopName,QString lib_name);
	KURL storedUrl;
	bool stored_url;
  protected slots:
	void showHidePage(int value);
};

#endif
