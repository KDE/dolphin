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

#ifndef DOLPHINCONTEXTMENU_H
#define DOLPHINCONTEXTMENU_H

#include <kdesktopfileactions.h>
#include <kfileitem.h>
#include <kservice.h>
#include <kurl.h>

#include <QtCore/QObject>

#include <QtCore/QVector>

class KMenu;
class KFileItem;
class QAction;
class DolphinMainWindow;

/**
 * @brief Represents the context menu which appears when doing a right
 *        click on an item or the viewport of the file manager.
 *
 * Beside static menu entries (e. g. 'Paste' or 'Properties') two
 * dynamic sub menus are shown when opening a context menu above
 * an item:
 * - 'Open With': Contains all applications which are registered to
 *                open items of the given MIME type.
 * - 'Actions':   Contains all actions which can be applied to the
 *                given item.
 */
class DolphinContextMenu : public QObject
{
    Q_OBJECT

public:
    /**
     * @parent        Pointer to the main window the context menu
     *                belongs to.
     * @fileInfo      Pointer to the file item the context menu
     *                is applied. If 0 is passed, the context menu
     *                is above the viewport.
     * @baseUrl       Base URL of the viewport where the context menu
     *                should be opened.
     */
    DolphinContextMenu(DolphinMainWindow* parent,
                       const KFileItem& fileInfo,
                       const KUrl& baseUrl);

    virtual ~DolphinContextMenu();

    /** Opens the context menu model. */
    void open();

private slots:
    void pasteIntoFolder();

private:
    void openTrashContextMenu();
    void openTrashItemContextMenu();
    void openItemContextMenu();
    void openViewportContextMenu();

    void insertDefaultItemActions(KMenu* popup);

    /**
     * Inserts the 'Open With...' submenu to \a popup.
     * @param popup          Menu where the 'Open With...' sub menu should
     *                       be added.
     * @param openWithVector Output parameter which contains all 'Open with...'
     *                       services.
     * @return               Identifier of the first 'Open With...' entry.
     *                       All succeeding identifiers have an increased value of 1
     *                       to the predecessor.
     */
    QList<QAction*> insertOpenWithItems(KMenu* popup,
                                        QVector<KService::Ptr>& openWithVector);

    /**
     * Returns true, if 'menu' contains already
     * an entry with the name 'entryName'.
     */
    bool containsEntry(const KMenu* menu,
                       const QString& entryName) const;

    /**
     * Adds the "Show menubar" action to the menu if the
     * menubar is hidden.
     */
    void addShowMenubarAction(KMenu* menu);

    /**
     * Returns a name for adding the URL \a url to the Places panel.
     */
    QString placesName(const KUrl& url) const;

    QAction* createPasteAction();

private:
    struct Entry
    {
        int type;
        QString name;
        QString filePath;     // empty for separator
        QString templatePath; // same as filePath for template
        QString icon;
        QString comment;
    };

    enum ContextType
    {
        NoContext = 0,
        ItemContext = 1,
        TrashContext = 2
    };

    DolphinMainWindow* m_mainWindow;
    KFileItem m_fileInfo;
    KUrl m_baseUrl;
    KFileItemList m_selectedItems;
    KUrl::List m_selectedUrls;
    int m_context;
};

#endif
