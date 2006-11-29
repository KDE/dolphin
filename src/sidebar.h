/***************************************************************************
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _SIDEBAR_H_
#define _SIDEBAR_H_

#include <QWidget>
#include <QVBoxLayout>

class KUrl;
class QComboBox;
class Q3VBoxLayout;
class SidebarPage;
class DolphinMainWindow;

/**
 * @brief The sidebar allows to access bookmarks, history items and TODO...
 *
 * TODO
 */
class Sidebar : public QWidget
{
	Q_OBJECT

public:
    Sidebar(DolphinMainWindow* mainwindow, QWidget* parent);
    virtual ~Sidebar();

    virtual QSize sizeHint() const;

signals:
    /**
     * The user selected an item on sidebar widget and item has
     * Url property, so inform the parent togo to this Url;
     */
    void urlChanged(const KUrl& url);

private slots:
    void createPage(int index);

private:
    int indexForName(const QString& name) const;

    DolphinMainWindow *m_mainWindow;
    QComboBox* m_pagesSelector;
    SidebarPage* m_page;
    QVBoxLayout* m_layout;
};

#endif // _SIDEBAR_H_
