/*
  $Id$

    Copyright (C) 1998, 1999 Jochen Wilhelmy
                             digisnap@cs.tu-berlin.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef _TOPLEVEL_H_
#define _TOPLEVEL_H_

#include <kparts/mainwindow.h>
#include "../part/kateview.h"
#include "../part/katedocument.h"
#include "../part/katefactory.h"

class KAction;
class KToggleAction;
class KSelectAction;
class KRecentFilesAction;

class TopLevel : public KParts::MainWindow
{
  Q_OBJECT

  public:
    TopLevel(KateDocument * = 0L);
    ~TopLevel();

    void init(); //initialize caption, status and show

    void loadURL(const KURL &url);

  protected:
    virtual bool queryClose();
    virtual bool queryExit();

    void setupEditWidget(KateDocument *);
    void setupActions();
    void setupStatusBar();

    virtual void dragEnterEvent( QDragEnterEvent * );
    virtual void dropEvent( QDropEvent * );

    KateView *kateView;

    KRecentFilesAction * m_recentFiles;
    KToggleAction * m_paShowPath;
    KToggleAction * m_paShowToolBar;
    KToggleAction * m_paShowStatusBar;

    QTimer *statusbarTimer;

  public slots:
    void slotNew();
    void slotOpen();
    void slotOpen( const KURL& url );
    void newView();
    void toggleToolBar();
    void toggleStatusBar();
    void editKeys();
    void editToolbars();

  public slots:
    void printNow();
    void printDlg();

    void newCurPos();
    void newStatus();
    void timeout();
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

