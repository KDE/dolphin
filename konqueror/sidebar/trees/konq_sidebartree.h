/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef konq_tree_h
#define konq_tree_h

#include <k3listview.h>
#include "konq_sidebartreetoplevelitem.h"
#include "konqsidebar_tree.h"
#include <QMap>
#include <QPoint>
#include <q3strlist.h>
#include <QToolTip>
//Added by qt3to4:
#include <QPixmap>
#include <QByteArray>
#include <QDragLeaveEvent>
#include <Q3PtrList>
#include <QDragMoveEvent>
#include <QEvent>
#include <QDropEvent>
#include <QDragEnterEvent>

class KonqSidebarTreeModule;
class KonqSidebarTreeItem;
class KActionCollection;

class QTimer;

class KonqSidebarTree_Internal;

#define VIRT_Link 0
#define VIRT_Folder 1 // A directory which is parsed for .desktop files

typedef KonqSidebarTreeModule*(*getModule)(KonqSidebarTree*, const bool); 

typedef struct DirTreeConfigData_
{
  KUrl dir;
  int type;
  QString relDir;
} DirTreeConfigData;

typedef enum {
    SidebarTreeMode, // used if the drop is accepted by a KonqSidebarTreeItem. otherwise
    K3ListViewMode    // use K3ListView's dnd implementation. accepts mime types set with setDropFormats()
} DropAcceptType;

/**
 * The multi-purpose tree (listview)
 * It parses its configuration (desktop files), each one corresponding to
 * a toplevel item, and creates the modules that will handle the contents
 * of those items.
 */
class KonqSidebarTree : public K3ListView
{
    Q_OBJECT
public:
    KonqSidebarTree( KonqSidebar_Tree *parent, QWidget *parentWidget, int virt, const QString& path );
    virtual ~KonqSidebarTree();

    void followURL( const KUrl &url );

    /**
     * @return the current (i.e. selected) item
     */
    KonqSidebarTreeItem * currentItem() const;

    void startAnimation( KonqSidebarTreeItem * item, const char * iconBaseName = "kde", uint iconCount = 6, const QPixmap * originalPixmap = 0L );
    void stopAnimation( KonqSidebarTreeItem * item );

    KonqSidebarPlugin * part() { return m_part; }

    void lockScrolling( bool lock ) { m_scrollingLocked = lock; }

    bool isOpeningFirstChild() const { return m_bOpeningFirstChild; }
 
    void enableActions( bool copy, bool cut, bool paste,
                        bool trash, bool del, bool rename = false );

    void itemDestructed( KonqSidebarTreeItem *item );

    void setDropFormats( const QStringList &formats ); // used in K3ListView mode
    
    // Show context menu for toplevel items
    void showToplevelContextMenu();

    // Add an URL
    void addUrl(KonqSidebarTreeTopLevelItem* item, const KUrl&url);

    // If we can use dcop to open tabs
    bool tabSupport();

public slots:
    // Connected to KDirNotify dbus signals
    void slotFilesAdded( const QString & dir );
    void slotFilesRemoved( const QStringList & urls );
    void slotFilesChanged( const QStringList & urls );

    virtual void setContentsPos( int x, int y );

protected:
    virtual void contentsDragEnterEvent( QDragEnterEvent *e );
    virtual void contentsDragMoveEvent( QDragMoveEvent *e );
    virtual void contentsDragLeaveEvent( QDragLeaveEvent *e );
    virtual void contentsDropEvent( QDropEvent *ev );
    virtual bool acceptDrag(QDropEvent* e) const; // used in K3ListView mode

    virtual void leaveEvent( QEvent * );

    virtual Q3DragObject* dragObject();

private slots:
    void slotDoubleClicked( Q3ListViewItem *item );
    void slotExecuted( Q3ListViewItem *item );
    void slotMouseButtonPressed(int _button, Q3ListViewItem* _item, const QPoint&, int col);
    void slotMouseButtonClicked(int _button, Q3ListViewItem* _item, const QPoint&, int col);
    void slotSelectionChanged();

    void slotAnimation();

    void slotAutoOpenFolder();

    void rescanConfiguration();

    void slotItemRenamed(Q3ListViewItem*, const QString &, int);

    void slotCreateFolder();
    void slotDelete();
    void slotRename();
    void slotProperties();
    void slotOpenNewWindow();
    void slotOpenTab();
    void slotCopyLocation();

private:
    void clearTree();
    void scanDir( KonqSidebarTreeItem *parent, const QString &path, bool isRoot = false );
    void loadTopLevelGroup( KonqSidebarTreeItem *parent, const QString &path );
    void loadTopLevelItem( KonqSidebarTreeItem *parent, const QString &filename );

    void loadModuleFactories();
    

private:
    Q3PtrList<KonqSidebarTreeTopLevelItem> m_topLevelItems;
    KonqSidebarTreeTopLevelItem *m_currentTopLevelItem;

    Q3PtrList<KonqSidebarTreeModule> m_lstModules;

    KonqSidebarPlugin  *m_part;

    struct AnimationInfo
    {
        AnimationInfo( const char * _iconBaseName, uint _iconCount, const QPixmap & _originalPixmap )
            : iconBaseName(_iconBaseName), iconCount(_iconCount), iconNumber(1), originalPixmap(_originalPixmap) {}
        AnimationInfo() : iconCount(0) {}
        QByteArray iconBaseName;
        uint iconCount;
        uint iconNumber;
        QPixmap originalPixmap;
    };
    typedef QMap<KonqSidebarTreeItem *, AnimationInfo> MapCurrentOpeningFolders;
    MapCurrentOpeningFolders m_mapCurrentOpeningFolders;

    QTimer *m_animationTimer;

    Q3ListViewItem *m_currentBeforeDropItem; // The item that was current before the drag-enter event happened
    Q3ListViewItem *m_dropItem; // The item we are moving the mouse over (during a drag)
    Q3StrList m_lstDropFormats;

    QTimer *m_autoOpenTimer;

    // The base URL for our configuration directory
    //KUrl m_dirtreeDir;
    DirTreeConfigData m_dirtreeDir;

    bool m_scrollingLocked;

    getModule getPluginFactory(QString name);
    
    QMap<QString, QString>   pluginInfo;
    QMap<QString, getModule> pluginFactories;

    bool m_bOpeningFirstChild;
    KActionCollection *m_collection;

    KonqSidebarTree_Internal *d;

#ifndef Q_MOC_RUN
#undef signals
#define signals public
#endif
signals:
#ifndef Q_MOC_RUN
#undef signals
#define signals protected
#endif
    void openUrlRequest( const KUrl &url, const KParts::URLArgs &args = KParts::URLArgs() );
    void createNewWindow( const KUrl &url, const KParts::URLArgs &args = KParts::URLArgs() );
    void popupMenu( const QPoint &global, const KUrl &url,
         const QString &mimeType, mode_t mode = (mode_t)-1 );
    void popupMenu( const QPoint &global, const KFileItemList &items );
    void enableAction( const char * name, bool enabled );
};

#endif
