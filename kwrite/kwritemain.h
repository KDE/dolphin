/***************************************************************************
                          kwritemain.h  -  description
                             -------------------
    begin                : Mon Jan 15 2001
    copyright            : (C) 2001 by Christoph Cullmann
    email                : cullmann@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KWRITE_MAIN_H__
#define __KWRITE_MAIN_H__

#include <kparts/mainwindow.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>

class KAction;
class KToggleAction;
class KSelectAction;
class KRecentFilesAction;

class KWrite : public KParts::MainWindow
{
  Q_OBJECT

  public:
    KWrite(KTextEditor::Document * = 0L);
    ~KWrite();

    void init(); //initialize caption, status and show

    void loadURL(const KURL &url);

  protected:
    class KLibFactory *factory;

    virtual bool queryClose();
    virtual bool queryExit();

    void setupEditWidget(KTextEditor::Document *);
    void setupActions();
    void setupStatusBar();

    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dropEvent( QDropEvent * );

    KTextEditor::View *kateView;

    KRecentFilesAction * m_recentFiles;
    KToggleAction * m_paShowPath;
    KToggleAction * m_paShowToolBar;
    KToggleAction * m_paShowStatusBar;

  private:
	KURL delayedURL;

  public slots:
    void slotNew();
    void slotFlush ();
    void slotOpen();
    void slotOpen( const KURL& url);
    void newView();
    void toggleToolBar();
    void toggleStatusBar();
    void editKeys();
    void editToolbars();

  public slots:
    void printNow();
    void printDlg();

    void newStatus(const QString &msg);
    void newCaption();

    void slotDropEvent(QDropEvent *);

    void slotEnableActions( bool enable );

  //config file functions
  public:
    //common config
    void readConfig(KConfig *);
    void writeConfig(KConfig *);
    //config file
    void readConfig();

  public slots:
    void writeConfig();

  //session management
  public:
    void restore(KConfig *,int);

  protected:
    virtual void readProperties(KConfig *);
    virtual void saveProperties(KConfig *);
    virtual void saveGlobalProperties(KConfig *);
		
	private:
	  QString encoding;
};

#endif

