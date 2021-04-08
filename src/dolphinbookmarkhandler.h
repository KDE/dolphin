/*
 * SPDX-FileCopyrightText: 2019 David Hallas <david@davidhallas.dk>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINBOOKMARKHANDLER_H
#define DOLPHINBOOKMARKHANDLER_H

#include <KBookmarkManager>
#include <KBookmarkOwner>
#include <QObject>

class DolphinMainWindow;
class DolphinViewContainer;
class KActionCollection;
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
