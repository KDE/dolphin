#ifndef _kmultiverttabbar_h_
#define _kmultiverttabbar_h_

#include <qscrollview.h>
#include <qvbox.h>
#include <qlist.h>
#include <qlayout.h>
#include <qstring.h>
#include <qpushbutton.h>
#include <qptrlist.h>

class QPixmap;
class QPainter;

class KMultiVertTabBarButton: public QPushButton
{
	Q_OBJECT
public:
	KMultiVertTabBarButton(const QPixmap& pic,const QString&, QPopupMenu *popup,int id,QWidget *parent);
	~KMultiVertTabBarButton(){;}
	int id(){return m_id;}
private:
	int m_id;
signals:
	void clicked(int);
private slots:
	void slotClicked();
};


class KMultiVertTabBarTab: public KMultiVertTabBarButton
{
	Q_OBJECT
public:
	KMultiVertTabBarTab(const QPixmap& pic,const QString&,int id,QWidget *parent);
	~KMultiVertTabBarTab(){;}
protected:
	virtual void drawButton(QPainter *);
};

class KMultiVertTabBarInternal: public QScrollView
{
	Q_OBJECT
public:
	KMultiVertTabBarInternal(QWidget *parent);
	int insertTab(QPixmap,int=-1,const QString& =QString::null);
	KMultiVertTabBarTab *getTab(int);	
private:
	QVBox *box;
	QPtrList<KMultiVertTabBarTab> tabs;
};


class KMultiVertTabBar: public QWidget
{
	Q_OBJECT
public:
	KMultiVertTabBar(QWidget *parent);
	~KMultiVertTabBar(){;}
 	//int insertButton(QPixmap,int=-1,const QString& =QString::null);
 	int insertButton(QPixmap,int=-1,QPopupMenu* =0,const QString& =QString::null);
	void removeButton(int);
	int insertTab(QPixmap,int=-1,const QString& =QString::null);
	void removeTab(int);
	void setTab(int,bool);
	bool isTabRaised(int);
	KMultiVertTabBarButton *getButton(int);
	KMultiVertTabBarTab *getTab(int);
	QPtrList<KMultiVertTabBarButton> buttons;
private:
	KMultiVertTabBarInternal *internal;
	QVBoxLayout *l;
};

#endif
