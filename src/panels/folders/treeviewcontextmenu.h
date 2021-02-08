/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef TREEVIEWCONTEXTMENU_H
#define TREEVIEWCONTEXTMENU_H

#include <KFileItem>

#include <QObject>

class QMimeData;
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

    ~TreeViewContextMenu() override;

    /** Opens the context menu modal. */
    void open(const QPoint& pos);

private Q_SLOTS:
    /** Cuts the item m_fileItem. */
    void cut();

    /** Copies the item m_fileItem. */
    void copy();

    /** Paste the clipboard to m_fileItem. */
    void paste();

    /** Renames the item m_fileItem. */
    void rename();

    /** Moves the item m_fileItem to the trash. */
    void moveToTrash();

    /** Deletes the item m_fileItem. */
    void deleteItem();

    /** Shows the properties of the item m_fileItem. */
    void showProperties();

    /**
     * Sets the 'Show Hidden Files' setting for the
     * folders panel to \a show.
     */
    void setShowHiddenFiles(bool show);

    /**
     * Sets the 'Limit folders panel to home' setting for the
     * folders panel to \a enable.
     */
    void setLimitFoldersPanelToHome(bool enable);

    /**
     * Sets the 'Automatic Scrolling' setting for the
     * folders panel to \a enable.
     */
    void setAutoScrolling(bool enable);

private:
    void populateMimeData(QMimeData* mimeData, bool cut);

private:
    FoldersPanel* m_parent;
    KFileItem m_fileItem;
};

#endif
