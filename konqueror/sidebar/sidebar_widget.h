#ifndef _SIDEBAR_WIDGET_
#define _SIDEBAR_WIDGET_
#include <qhbox.h>
#include <qpushbutton.h>
#include <kdockwidget.h>
#include <qvector.h>
#include <kurl.h>
#include <ktoolbar.h>
#include <kparts/part.h>
#include <qstring.h>

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


class addBackEnd: public QObject
{
	Q_OBJECT
	public:
		addBackEnd(QObject *parent,class QPopupMenu *addmenu, const char *name=0);
		~addBackEnd(){;}
	private:
		QGuardedPtr<class QPopupMenu> menu;
		QVector<QString> libNames;
		QVector<QString> libParam;
	protected slots:
		void aboutToShowAddMenu();
		void activatedAddMenu(int);
};

class Sidebar_ButtonBar: public KToolBar
{
  Q_OBJECT
  public:
        Sidebar_ButtonBar(QWidget *parent):KToolBar(parent,"Konq::SidebarTNG",true){setAcceptDrops(true);}
	~Sidebar_ButtonBar(){;}  
 protected:
	virtual void dragEnterEvent ( QDragEnterEvent * e){e->accept();} 
	virtual void dragMoveEvent ( QDragEnterEvent * e){e->accept();} 
};

class Sidebar_Widget: public QHBox
{
  Q_OBJECT
  public:
  Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const char * name);
  ~Sidebar_Widget();
  void openURL(const class KURL &url);
  void stdAction(const char *handlestd);
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
	KParts::ReadOnlyPart *partParent;
	ButtonInfo* getActiveModule();
	class QPopupMenu* addMenu;

  protected slots:
	void showHidePage(int value);
};

#endif
