#ifndef _kmultiverttabbar_h_
#define _kmultiverttabbar_h_

#include <qscrollview.h>
#include <qvbox.h>
#include <qlayout.h>
#include <qstring.h>
#include <qptrlist.h>
#include <qpushbutton.h>

class QPixmap;
class QPainter;
class QFrame;

class KMultiVertTabBar: public QWidget
{
	Q_OBJECT
public:
	KMultiVertTabBar(QWidget *parent);
	~KMultiVertTabBar(){;}

	enum KMultiVertTabBarPosition{Left, Right};
 	//int insertButton(QPixmap,int=-1,const QString& =QString::null);
 	int insertButton(QPixmap,int=-1,QPopupMenu* =0,const QString& =QString::null);
	void removeButton(int);
	int insertTab(QPixmap,int=-1,const QString& =QString::null);
	void removeTab(int);
	void setTab(int,bool);
	bool isTabRaised(int);
	class KMultiVertTabBarButton *getButton(int);
	class KMultiVertTabBarTab *getTab(int);
	QPtrList<class KMultiVertTabBarButton> buttons;
	void setPosition(KMultiVertTabBarPosition pos);
private:
	class KMultiVertTabBarInternal *internal;
	QVBoxLayout *l;
	QFrame *btnTabSep;	
	KMultiVertTabBarPosition position;
};

class KMultiVertTabBarButton: public QPushButton
{
	Q_OBJECT
public:
	KMultiVertTabBarButton(const QPixmap& pic,const QString&, QPopupMenu *popup,
		int id,QWidget *parent, KMultiVertTabBar::KMultiVertTabBarPosition pos);
	~KMultiVertTabBarButton(){;}
	int id(){return m_id;}
	void setPosition(KMultiVertTabBar::KMultiVertTabBarPosition);
protected:
	KMultiVertTabBar::KMultiVertTabBarPosition position;
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
	KMultiVertTabBarTab(const QPixmap& pic,const QString&,int id,QWidget *parent,
		KMultiVertTabBar::KMultiVertTabBarPosition pos);
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
	void removeTab(int);
	void setPosition(enum KMultiVertTabBar::KMultiVertTabBarPosition pos);
private:
	QVBox *box;
	QPtrList<KMultiVertTabBarTab> tabs;
	enum KMultiVertTabBar::KMultiVertTabBarPosition position;
protected:
	virtual void drawContents ( QPainter *, int, int, int, int);

	/**
	 * [contentsM|m]ousePressEvent are reimplemented from QScrollView 
	 * in order to ignore all mouseEvents on the viewport, so that the
	 * parent can handle them.
	 */
	virtual void contentsMousePressEvent(QMouseEvent *);
	virtual void mousePressEvent(QMouseEvent *);
};

#endif
