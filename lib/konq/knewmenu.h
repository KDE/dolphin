/* This file is part of the KDE project
   Copyright (C) 1998, 1999 David Faure <faure@kde.org>
 
   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.
 
   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.
 
   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/     

#ifndef __knewmenu_h
#define __knewmenu_h

#include <qintdict.h>
#include <qstringlist.h>

#include <kaction.h>

/**
 * The manager for the 'New' submenu
 * Fills it with 'Folder' and one item per Template
 * Updates the menu if templates are added (fillTemplates() has to be called)
 *  hmm, perhaps a long-period KDirWatch here ??  (TODO !!)
 *
 * The New menu can be a standalone popupmenu or a
 * popupmenu in a menubar
 */
class KNewMenu : public KActionMenu
{
  Q_OBJECT
public:
    /**
     * Constructor
     */
    KNewMenu( QActionCollection * _collec, const char *name=0L );
    ~KNewMenu() {}

    /**
     * Fills the templates list. Can be called at any time to update it.
     */
    static void fillTemplates();
    
    /**
     * Set the files the popup is shown for
     */
    void setPopupFiles(QStringList & _files);
    void setPopupFiles(QString _file) {
        popupFiles.clear();
        popupFiles.append( _file );
    }

public slots:        
    /**
     * Called when New->* is clicked
     */
    void slotNewFile();
 
    /**
     * Checks if updating the list is necessary
     * IMPORTANT : Call this in the slot for aboutToShow.
     * You should probably call setPopupFiles at the same time
     * (that's for menu in the menubar - otherwise just call both at
     * creation time)
     */
    void slotCheckUpToDate( );

private:

    /**
     * Fills the menu from the templates list.
     */
    void fillMenu();

    /**
     * List of all template files. It is important that they are in
     * the same order as the 'New' menu.
     */
    static QStringList * templatesList;

    QActionCollection * m_actionCollection;

    /**
     * Is increased when templatesList has been updated and
     * menu needs to be re-filled. Menus have their own version and compare it
     * to templatesVersion before showing up
     */
    static int templatesVersion;

    int menuItemsVersion;
    
    /**
     * When the user pressed the right mouse button over an URL a popup menu
     * is displayed. The URL belonging to this popup menu is stored here.
     */
    QStringList popupFiles; 

    /*
     * The destination of the copy, for each job being run (job id is the dict key).
     * Used to popup properties for it
     */
    QIntDict <QString> m_sDest;
};

#endif
