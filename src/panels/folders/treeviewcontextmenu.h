/***************************************************************************
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef TREEVIEWCONTEXTMENU_H
#define TREEVIEWCONTEXTMENU_H

#include <QtCore/QObject>
#include <KFileItem>

class FoldersPanel;

/**
 * @brief Represents the context menu which appears when doing a right
 *        click on an item of the treeview.
 */
class TreeViewContextMenu : public QObject
{
    Q_OBJECT

public:
    /**
     * @parent        Pointer to the folders panel the context menu
     *                belongs to.
     * @fileInfo      Pointer to the file item the context menu
     *                is applied. If 0 is passed, the context menu
     *                is above the viewport.
     */
    TreeViewContextMenu(FoldersPanel* parent,
                        const KFileItem& fileInfo);

    virtual ~TreeViewContextMenu();

    /** Opens the context menu modal. */
    void open();

private slots:
    /** Cuts the item m_fileInfo. */
    void cut();

    /** Copies the item m_fileInfo. */
    void copy();

    /** Paste the clipboard to m_fileInfo. */
    void paste();

    /** Renames the item m_fileInfo. */
    void rename();

    /** Moves the item m_fileInfo to the trash. */
    void moveToTrash();

    /** Deletes the item m_fileInfo. */
    void deleteItem();

    /** Shows the properties of the item m_fileInfo. */
    void showProperties();

    /**
     * Sets the 'Show Hidden Files' setting for the
     * folders panel to \a show.
     */
    void setShowHiddenFiles(bool show);

    /**
     * Sets the 'Automatic Scrolling' setting for the
     * folders panel to \a enable.
     */
    void setAutoScrolling(bool enable);

private:
    void populateMimeData(QMimeData* mimeData, bool cut);

private:
    FoldersPanel* m_parent;
    KFileItem m_fileInfo;
};

#endif
