/* This file is part of the KDE project
   Copyright (c) 2007 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DOLPHINPART_H
#define DOLPHINPART_H

#include <kparts/part.h>
#include <kparts/browserextension.h>
class KAction;
class KFileItemList;
class KFileItem;
class DolphinPartBrowserExtension;
class DolphinSortFilterProxyModel;
class DolphinModel;
class KDirLister;
class DolphinView;
class QLineEdit;
class KAboutData;

class DolphinPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    // Used by konqueror. Technically it means "we want undo enabled if
    // there are things in the undo history and the current part is a dolphin part".
    // Even though it's konqueror doing the undo...
    Q_PROPERTY( bool supportsUndo READ supportsUndo )

public:
    explicit DolphinPart(QWidget* parentWidget, QObject* parent, const QStringList& args);
    ~DolphinPart();

    static KAboutData* createAboutData();

    virtual bool openUrl(const KUrl& url);

    /// see the supportsUndo property
    bool supportsUndo() const { return true; }

    DolphinView* view() { return m_view; }

protected:
    /**
     * We reimplement openUrl so no need to implement openFile.
     */
    virtual bool openFile() { return true; }

private Q_SLOTS:
    void slotCompleted(const KUrl& url);
    void slotCanceled(const KUrl& url);
    void slotInfoMessage(const QString& msg);
    void slotErrorMessage(const QString& msg);
    /**
     * Shows the information for the item \a item inside the statusbar. If the
     * item is null, the default statusbar information is shown.
     */
    void slotRequestItemInfo(const KFileItem& item);
    /**
     * Handles clicking on an item
     */
    void slotItemTriggered(const KFileItem& item);
    /**
     * Opens the context menu on the current mouse position.
     * @item  File item context. If item is 0, the context menu
     *        should be applied to \a url.
     * @url   URL which contains \a item.
     */
    void slotOpenContextMenu(const KFileItem& item, const KUrl& url);
    /**
     * Emitted when the user requested a change of view mode
     */
    void slotViewModeActionTriggered(QAction*);

    /**
     * Asks the host to open the URL \a url if the current view has
     * a different URL.
     */
    void slotUrlChanged(const KUrl& url);

    /**
     * Updates the state of the 'Edit' menu actions and emits
     * the signal selectionChanged().
     */
    void slotSelectionChanged(const KFileItemList& selection);

    /**
     * Same as in DolphinMainWindow: updates the view menu actions
     */
    void updateViewActions();

    /**
     * Updates the text of the paste action dependent from
     * the number of items which are in the clipboard.
     */
    void updatePasteAction();

    /**
     * Connected to the "move_to_trash" action; adds "shift means del" handling.
     */
    void slotTrashActivated(Qt::MouseButtons, Qt::KeyboardModifiers);

    void slotNewDir();

private:
    void createActions();

private:
    DolphinView* m_view;
    KDirLister* m_dirLister;
    DolphinModel* m_dolphinModel;
    DolphinSortFilterProxyModel* m_proxyModel;
    DolphinPartBrowserExtension* m_extension;
    Q_DISABLE_COPY(DolphinPart)
};

class DolphinPartBrowserExtension : public KParts::BrowserExtension
{
    Q_OBJECT
public:
    DolphinPartBrowserExtension( DolphinPart* part )
        : KParts::BrowserExtension( part ), m_part(part) {}

public Q_SLOTS:
    void cut();
    void copy();
    void paste();

private:
    DolphinPart* m_part;
};

#endif /* DOLPHINPART_H */
