/***************************************************************************
 *   Copyright (C) 2006 by Cvetoslav Ludmiloff <ludmiloff@gmail.com>       *
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz@gmx.at>             *
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

#ifndef PANEL_H
#define PANEL_H

#include <QtGui/QWidget>
#include <kurl.h>
#include <kfileitem.h>

/**
 * @brief Base widget for all panels that can be docked on the window borders.
 *
 * Derived panels should provide a context menu that at least offers the
 * actions from Panel::customContextMenuActions().
 */
class Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Panel(QWidget* parent = 0);
    virtual ~Panel();

    /** Returns the current set URL of the active Dolphin view. */
    KUrl url() const;

    /**
     * Sets custom context menu actions that are added to the panel specific
     * context menu actions. Allows an application to apply custom actions to
     * the panel.
     */
    void setCustomContextMenuActions(const QList<QAction*>& actions);
    QList<QAction*> customContextMenuActions() const;

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

public slots:
    /**
     * This is invoked every time the folder being displayed in the
     * active Dolphin view changes.
     */
    void setUrl(const KUrl& url);

protected:
    /**
     * Must be implemented by derived classes and is invoked when
     * the URL has been changed (see Panel::setUrl()).
     * @return True, if the new URL will get accepted by the derived
     *         class. If false is returned,
     *         the URL will be reset to the previous URL.
     */
    virtual bool urlChanged() = 0;

private:
    KUrl m_url;
    QList<QAction*> m_customContextMenuActions;
};

#endif // PANEL_H
