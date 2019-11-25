/***************************************************************************
 *   Copyright (C) 2019 by David Hallas <david@davidhallas.dk>             *
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

#ifndef DOLPHINBOOKMARKHANDLER_H
#define DOLPHINBOOKMARKHANDLER_H

#include <KBookmarkManager>
#include <QObject>

class DolphinMainWindow;
class DolphinViewContainer;
class KActionCollection;
class KBookmarkManager;
class KBookmarkMenu;
class QMenu;

class DolphinBookmarkHandler : public QObject, public KBookmarkOwner
{
    Q_OBJECT
public:
    DolphinBookmarkHandler(DolphinMainWindow *mainWindow, KActionCollection *collection, QMenu *menu, QObject *parent);
    ~DolphinBookmarkHandler() override;

private:
    QString currentTitle() const override;
    QUrl currentUrl() const override;
    QString currentIcon() const override;
    bool supportsTabs() const override;
    QList<FutureBookmark> currentBookmarkList() const override;
    bool enableOption(BookmarkOption option) const override;
    void openBookmark(const KBookmark &bookmark, Qt::MouseButtons, Qt::KeyboardModifiers) override;
    void openFolderinTabs(const KBookmarkGroup &bookmarkGroup) override;
    void openInNewTab(const KBookmark &bookmark) override;
    void openInNewWindow(const KBookmark &bookmark) override;
    static QString title(DolphinViewContainer* viewContainer);
    static QUrl url(DolphinViewContainer* viewContainer);
    static QString icon(DolphinViewContainer* viewContainer);
private:
    DolphinMainWindow* m_mainWindow;
    KBookmarkManager *m_bookmarkManager;
    QScopedPointer<KBookmarkMenu> m_bookmarkMenu;
};

#endif // DOLPHINBOOKMARKHANDLER_H
