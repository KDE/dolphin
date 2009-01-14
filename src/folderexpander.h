/***************************************************************************
 *   Copyright (C) 2008 by Simon St James <kdedevel@etotheipiplusone.com>  *
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

#ifndef FOLDEREXPANDER_H
#define FOLDEREXPANDER_H

// Needs to be exported as FoldersPanel uses it.
#include "libdolphin_export.h"

#include <QObject>
#include <QPoint>

class QAbstractItemView;
class QTreeView;
class QTimer;
class QSortFilterProxyModel;
class QModelIndex;
class KDirModel;

/**
 * Grants auto expanding functionality to the provided item view.
 * Qt has its own auto expand mechanism, but this works only
 * for QTreeView. Auto expanding of folders is turned on
 * per default.
 *
 * If the provided view is an instance of the class QTreeView, the
 * expansion of the directory is automatically done on hover. Otherwise
 * the enterDir() signal is emitted and the caller needs to ensure that
 * the requested directory is entered.
 *
 * The FolderExpander becomes a child of the provided view.
 */
class LIBDOLPHINPRIVATE_EXPORT FolderExpander : public QObject
{
    Q_OBJECT

public:
    FolderExpander(QAbstractItemView* view, QSortFilterProxyModel* proxyModel);
    virtual ~FolderExpander();

    void setEnabled(bool enabled);
    bool enabled() const;

signals:
    /**
     * Is emitted if the directory \a dirModelIndex should be entered. The
     * signal is not emitted when a QTreeView is used, as the entering of
     * the directory is already provided by expanding the tree node.
     */
    void enterDir(const QModelIndex& dirModelIndex, QAbstractItemView* view);


private slots:
    void viewScrolled();
    void autoExpandTimeout();

private:
    bool m_enabled;

    QAbstractItemView* m_view;
    QSortFilterProxyModel* m_proxyModel;

    QTimer* m_autoExpandTriggerTimer;
    QPoint m_autoExpandPos;

    static const int AUTO_EXPAND_DELAY = 700;

    /**
     * Watchs the drag/move events for the view to decide
     * whether auto expanding of a folder should be triggered.
     */
    bool eventFilter(QObject* watched, QEvent* event);
};
#endif
