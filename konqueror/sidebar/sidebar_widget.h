/***************************************************************************
                               sidebar_widget.h
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef _SIDEBAR_WIDGET_
#define _SIDEBAR_WIDGET_
#include <qhbox.h>
#include <qpushbutton.h>
#include <kdockwidget.h>
#include <qptrvector.h>
#include <kurl.h>
#include <ktoolbar.h>
#include <kparts/part.h>
#include <qstring.h>
#include "konqsidebarplugin.h"
#include <qlayout.h>
#include "kmultiverttabbar.h"
#include <qguardedptr.h>

class KDockWidget;

class ButtonInfo: public QObject
{
	Q_OBJECT
	public:
	ButtonInfo(const QString& file_,class KDockWidget *dock_, const QString &url_,const QString &lib, QObject *parent):
		QObject(parent),file(file_),dock(dock_),URL(url_),libName(lib)
		{copy=cut=paste=trash=del=shred=rename=false;}
	~ButtonInfo(){;}

	class QString file;
	class KDockWidget *dock;
	class KonqSidebarPlugin *module;
	class QString URL;
	class QString libName;
	bool copy;
	bool cut;
	bool paste;
	bool trash;
	bool del;
	bool shred;
        bool rename;
};


class addBackEnd: public QObject
{
	Q_OBJECT
	public:
		addBackEnd(QObject *parent,class QPopupMenu *addmenu, const char *name=0);
		~addBackEnd(){;}
	private:
		QGuardedPtr<class QPopupMenu> menu;
		QPtrVector<QString> libNames;
		QPtrVector<QString> libParam;
	protected slots:
		void aboutToShowAddMenu();
		void activatedAddMenu(int);
	signals:
		void updateNeeded();

};

class Sidebar_Widget: public QWidget, public KonqSidebar_PluginInterface
{
  Q_OBJECT
  public:
  Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par, const char * name);
  ~Sidebar_Widget();
  void openURL(const class KURL &url);
  void stdAction(const char *handlestd);
  //virtual KParts::ReadOnlyPart *getPart();
  KParts::BrowserExtension *getExtension();

  static QString PATH;
  private:
	class KDockArea *Area;
	class KMultiVertTabBar *ButtonBar;
        QPtrVector<ButtonInfo> Buttons;
	bool addButton(const QString &desktoppath,int pos=-1);
	bool createView(ButtonInfo *data);
	//class KDockWidget *mainW;
	int latestViewed;
	class KonqSidebarPlugin *loadModule(QWidget *par,QString &desktopName,QString lib_name,ButtonInfo *bi);
	KURL storedUrl;
	bool stored_url;
	KParts::ReadOnlyPart *partParent;
	//ButtonInfo* getActiveModule();
	bool singleWidgetMode;
	bool showTabsRight;
	bool showExtraButtons;
	void readConfig();
	QHBoxLayout *myLayout;
	class QStringList visibleViews;
	class QPopupMenu *buttonPopup;
	class QPopupMenu *Menu;
	int popupFor;
	void initialCopy();
	void doLayout();
	void connectModule(QObject *mod);
	int savedWidth;
	bool somethingVisible;
	void collapseExpandSidebar();
	KDockWidget *dummyMainW;
	bool doEnableActions();
	bool noUpdate;
  protected:
	virtual bool eventFilter(QObject*,QEvent*);
	friend class ButtonInfo;
	QGuardedPtr<ButtonInfo>activeModule;
  protected slots:
	void showHidePage(int value);
	void updateDock();
	void createButtons();
	void activatedMenu(int id);
	void buttonPopupActivate(int);
  	void dockWidgetHasUndocked(KDockWidget*);  
	void aboutToShowConfigMenu();
  signals:
		void started(KIO::Job *);
                void completed();
  public:
	/* interface KonqSidebar_PluginInterface*/
	virtual KInstance  *getInstance();
        virtual void showError(QString &);      //for later extension
        virtual void showMessage(QString &);    //for later extension
	/* end of interface implementation */
	

 /* The following public slots are wrappers for browserextension fields */
 public slots:
	void openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
  	void createNewWindow( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
	void createNewWindow( const KURL &url, const KParts::URLArgs &args,
             const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part );

	void popupMenu( const QPoint &global, const KFileItemList &items );
  	void popupMenu( KXMLGUIClient *client, const QPoint &global, const KFileItemList &items );
	void popupMenu( const QPoint &global, const KURL &url,
		const QString &mimeType, mode_t mode = (mode_t)-1 );
	void popupMenu( KXMLGUIClient *client,
		const QPoint &global, const KURL &url,
		const QString &mimeType, mode_t mode = (mode_t)-1 );
	void enableAction( const char * name, bool enabled );

};

#endif
