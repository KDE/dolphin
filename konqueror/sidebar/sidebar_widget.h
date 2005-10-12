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

#include <q3ptrvector.h>
#include <qtimer.h>
#include <qstring.h>
#include <qpointer.h>

#include <k3dockwidget.h>
#include <kurl.h>
#include <ktoolbar.h>
#include <kparts/part.h>
#include <kmultitabbar.h>

#include "konqsidebarplugin.h"
#include "konqsidebariface_p.h"

class KDockWidget;
class QHBoxLayout;
class QSplitter;
class QStringList;

class ButtonInfo: public QObject, public KonqSidebarIface
{
	Q_OBJECT
public:
	ButtonInfo(const QString& file_, class KonqSidebarIface *part, class KDockWidget *dock_,
			const QString &url_,const QString &lib,
			const QString &dispName_, const QString &iconName_,
			QObject *parent)
		: QObject(parent), file(file_), dock(dock_), URL(url_),
		libName(lib), displayName(dispName_), iconName(iconName_), m_part(part)
		{
		copy = cut = paste = trash = del = rename =false;
		}

	~ButtonInfo() {}

	QString file;
	KDockWidget *dock;
	KonqSidebarPlugin *module;
	QString URL;
	QString libName;
	QString displayName;
	QString iconName;
	bool copy;
	bool cut;
	bool paste;
	bool trash;
	bool del;
        bool rename;
        KonqSidebarIface *m_part;
	virtual bool universalMode() {return m_part->universalMode();}
};


class addBackEnd: public QObject
{
	Q_OBJECT
public:
	addBackEnd(QWidget *parent,class QMenu *addmenu, bool univeral, const QString &currentProfile, const char *name=0);
	~addBackEnd(){;}
protected slots:
	void aboutToShowAddMenu();
	void activatedAddMenu(int);
signals:
	void updateNeeded();
	void initialCopyNeeded();
private:
	QPointer<class QMenu> menu;
	Q3PtrVector<QString> libNames;
	Q3PtrVector<QString> libParam;
	bool m_universal;
	QString m_currentProfile;
	void doRollBack();
	QWidget *m_parent;
};

class KDE_EXPORT Sidebar_Widget: public QWidget
{
	Q_OBJECT
public:
	friend class ButtonInfo;
public:
	Sidebar_Widget(QWidget *parent, KParts::ReadOnlyPart *par,
						const char * name,bool universalMode, 
						const QString &currentProfile);
	~Sidebar_Widget();
	bool openURL(const class KURL &url);
	void stdAction(const char *handlestd);
	//virtual KParts::ReadOnlyPart *getPart();
	KParts::BrowserExtension *getExtension();
        virtual QSize sizeHint() const;	

public slots:
	void addWebSideBar(const KURL& url, const QString& name);

protected:
	void customEvent(QCustomEvent* ev);
	void resizeEvent(QResizeEvent* ev);
	virtual bool eventFilter(QObject*,QEvent*);
	virtual void mousePressEvent(QMouseEvent*);

protected slots:
	void showHidePage(int value);
	void createButtons();
	void updateButtons();
	void finishRollBack();
	void activatedMenu(int id);
	void buttonPopupActivate(int);
  	void dockWidgetHasUndocked(KDockWidget*);
	void aboutToShowConfigMenu();
	void saveConfig();

signals:
	void started(KIO::Job *);
	void completed();
	void fileSelection(const KFileItemList& iems);
	void fileMouseOver(const KFileItem& item);

public:
	/* interface KonqSidebar_PluginInterface*/
	KInstance  *getInstance();
//        virtual void showError(QString &);      for later extension
//        virtual void showMessage(QString &);    for later extension
	/* end of interface implementation */


 /* The following public slots are wrappers for browserextension fields */
public slots:
	void openURLRequest( const KURL &url, const KParts::URLArgs &args = KParts::URLArgs() );
	/* @internal
	 * @since 3.2
	 * ### KDE4 remove me
	 */
	void submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&);
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
	void userMovedSplitter();
	
private:
	QSplitter *splitter() const;
	bool addButton(const QString &desktoppath,int pos=-1);
	bool createView(ButtonInfo *data);
	KonqSidebarPlugin *loadModule(QWidget *par,QString &desktopName,QString lib_name,ButtonInfo *bi);
	void readConfig();
	void initialCopy();
	void doLayout();
	void connectModule(QObject *mod);
	void collapseExpandSidebar();
	bool doEnableActions();
	bool m_universalMode;
	bool m_userMovedSplitter;
private:
	KParts::ReadOnlyPart *m_partParent;
	KDockArea *m_area;
	KDockWidget *m_mainDockWidget;

	KMultiTabBar *m_buttonBar;
	Q3PtrVector<ButtonInfo> m_buttons;
	QHBoxLayout *m_layout;
	KMenu *m_buttonPopup;
	QMenu *m_menu;
	QPointer<ButtonInfo> m_activeModule;
	QPointer<ButtonInfo> m_currentButton;
	
	KConfig *m_config;
	QTimer m_configTimer;
	
	KURL m_storedUrl;
	int m_savedWidth;
	int m_latestViewed;

	bool m_hasStoredUrl;
	bool m_singleWidgetMode;
	bool m_showTabsLeft;
	bool m_hideTabs;
	bool m_showExtraButtons;
	bool m_somethingVisible;
	bool m_noUpdate;
	bool m_initial;

	QString m_path;
	QString m_relPath;
	QString m_currentProfile;
	QStringList m_visibleViews; // The views that are actually open
	QStringList m_openViews; // The views that should be opened

signals:
	void panelHasBeenExpanded(bool);
};

#endif
