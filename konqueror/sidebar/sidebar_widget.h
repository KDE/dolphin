#ifndef _SIDEBAR_WIDGET_
#define _SIDEBAR_WIDGET_
#include <qhbox.h>
#include <qpushbutton.h>
#include <kdockwidget.h>
#include <qvector.h>

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
	class QWidget *widget;	
};

class Sidebar_Widget: public QHBox
{
  Q_OBJECT
  public:
  Sidebar_Widget(QWidget *parent, const char * name);
  ~Sidebar_Widget();
  private:
	class KDockArea *Area;
	class KToolBar *ButtonBar;
        QVector<ButtonInfo> Buttons;
	void createButtons();
	bool addButton(const QString &desktoppath,int pos=-1);
	bool createView(ButtonInfo *data);
	class KDockWidget *mainW;
	int latestViewed;
  protected slots:
	void showHidePage(int value);
};

#endif
