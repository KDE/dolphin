/***************************************************************************
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef _SIDEBARPAGE_H_
#define _SIDEBARPAGE_H_

#include <QtGui/QWidget>
#include <kurl.h>
#include <kfileitem.h>

/**
 * @brief Base widget for all pages that can be embedded into the Sidebar.
 */
class SidebarPage : public QWidget
{
    Q_OBJECT
public:
    explicit SidebarPage(QWidget* parent = 0);
    virtual ~SidebarPage();

    /** Returns the current set URL of the active Dolphin view. */
    const KUrl& url() const;

    /** Returns the current selected items of the active Dolphin view. */
    const KFileItemList& selection() const;

public slots:
    /**
     * This is invoked every time the folder being displayed in the
     * active Dolphin view changes.
     */
    virtual void setUrl(const KUrl& url);

    /**
     * This is invoked to inform the sidebar that the user has selected a new
     * set of items.
     */
    virtual void setSelection(const KFileItemList& selection);

signals:
    /**
     * This signal is emitted when the sidebar requests an URL-change in the
     * currently active file-management view. The view is not requested to
     * accept this change, if it is accepted the sidebar will be informed via
     * the setUrl() slot.
     */
    void changeUrl(const KUrl& url);

    /**
     * This signal is emitted when the sidebar requests a change in the
     * current selection. The file-management view recieving this signal is
     * not required to select all listed files, limiting the selection to
     * e.g. the current folder. The new selection will be reported via the
     * setSelection slot.
     */
    void changeSelection(const KFileItemList& selection);

    /**
     * This signal is emitted whenever a drop action on this widget needs the
     * MainWindow's attention.
     */
    void urlsDropped(const KUrl::List& urls, const KUrl& destination);

private:
    KUrl m_url;
    KFileItemList m_currentSelection;
};

#endif // _SIDEBARPAGE_H_
