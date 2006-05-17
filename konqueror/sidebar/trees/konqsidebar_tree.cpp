#include "konqsidebar_tree.h"
#include "konqsidebar_tree.moc"
#include "konq_sidebartree.h"
#include <kvbox.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <ksimpleconfig.h>
#include <kinputdialog.h>
#include <kiconloader.h>
#include <k3listviewsearchline.h>

#include <QClipboard>
#include <QToolButton>
#include <QApplication>

KonqSidebar_Tree::KonqSidebar_Tree(KInstance *instance,QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name):
                   KonqSidebarPlugin(instance,parent,widgetParent,desktopName_,name)
{
	KSimpleConfig ksc(desktopName_);
	ksc.setGroup("Desktop Entry");
	int virt= ( (ksc.readEntry("X-KDE-TreeModule","")=="Virtual") ?VIRT_Folder:VIRT_Link);
	if (virt==1) desktopName_=ksc.readEntry("X-KDE-RelURL","");

	widget = new KVBox( widgetParent );

	if (ksc.readEntry("X-KDE-SearchableTreeModule", QVariant(false)).toBool()) {
		KVBox* searchLine = new KVBox(widget);
		tree=new KonqSidebarTree(this,widget,virt,desktopName_);
		new K3ListViewSearchLineWidget(tree,searchLine);
	}
	else {
		tree=new KonqSidebarTree(this,widget,virt,desktopName_);
	}

	connect(tree, SIGNAL( openURLRequest( const KUrl &, const KParts::URLArgs &)),
		this,SIGNAL( openURLRequest( const KUrl &, const KParts::URLArgs &)));

	connect(tree,SIGNAL(createNewWindow( const KUrl &, const KParts::URLArgs &)),
		this,SIGNAL(createNewWindow( const KUrl &, const KParts::URLArgs &)));

	connect(tree,SIGNAL(popupMenu( const QPoint &, const KUrl &, const QString &, mode_t )),
		this,SIGNAL(popupMenu( const QPoint &, const KUrl &, const QString &, mode_t )));

	connect(tree,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )),
		this,SIGNAL(popupMenu( const QPoint &, const KFileItemList & )));

	connect(tree,SIGNAL(enableAction( const char *, bool )),
		this,SIGNAL(enableAction( const char *, bool)));

}


KonqSidebar_Tree::~KonqSidebar_Tree(){;}

void* KonqSidebar_Tree::provides(const QString &) {return 0;}

//void KonqSidebar_Tree::emitStatusBarText (const QString &) {;}

QWidget *KonqSidebar_Tree::getWidget(){return widget;}

void KonqSidebar_Tree::handleURL(const KUrl &url)
    {
	emit started( 0 );
        tree->followURL( url );
        emit completed();
    }

void KonqSidebar_Tree::cut()
{
    QMimeData* mimeData = new QMimeData;
    if ( static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->populateMimeData( mimeData, true ) )
        QApplication::clipboard()->setMimeData( mimeData );
    else
        delete mimeData;
}

void KonqSidebar_Tree::copy()
{
    QMimeData* mimeData = new QMimeData;
    if ( static_cast<KonqSidebarTreeItem*>(tree->selectedItem())->populateMimeData( mimeData, false ) )
        QApplication::clipboard()->setMimeData( mimeData );
    else
        delete mimeData;
}

void KonqSidebar_Tree::paste()
{
    if (tree->currentItem())
        tree->currentItem()->paste();
}

void KonqSidebar_Tree::trash()
{
    if (tree->currentItem())
        tree->currentItem()->trash();
}

void KonqSidebar_Tree::del()
{
    if (tree->currentItem())
        tree->currentItem()->del();
}

void KonqSidebar_Tree::shred()
{
    if (tree->currentItem())
        tree->currentItem()->shred();
}

void KonqSidebar_Tree::rename()
{
    Q_ASSERT( tree->currentItem() );
    if (tree->currentItem())
        tree->currentItem()->rename();
}






extern "C"
{
    KDE_EXPORT void*  create_konqsidebar_tree(KInstance *inst,QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new KonqSidebar_Tree(inst,par,widp,desktopname,name);
    }
}

extern "C"
{
   KDE_EXPORT bool add_konqsidebar_tree(QString* fn, QString*, QMap<QString,QString> *map)
   {
	  KStandardDirs *dirs=KGlobal::dirs();
	  QStringList list=dirs->findAllResources("data","konqsidebartng/dirtree/*.desktop",false,true);
	  QStringList names;
	  for (QStringList::ConstIterator it=list.begin();it!=list.end();++it)
	  {
		KSimpleConfig sc(*it);
		sc.setGroup("Desktop Entry");
		names<<sc.readEntry("Name");
	  }

	QString item = KInputDialog::getItem( i18n( "Select Type" ),
		i18n( "Select type:" ), names );
	if (!item.isEmpty())
		{
			int id=names.indexOf( item );
			if (id==-1) return false;
			KSimpleConfig ksc2(QString(list.at(id)));
			ksc2.setGroup("Desktop Entry");
		        map->insert("Type","Link");
			map->insert("Icon",ksc2.readEntry("Icon"));
			map->insert("Name",ksc2.readEntry("Name"));
		 	map->insert("Open","false");
			map->insert("URL",ksc2.readEntry("X-KDE-Default-URL"));
			map->insert("X-KDE-KonqSidebarModule","konqsidebar_tree");
			map->insert("X-KDE-TreeModule",ksc2.readEntry("X-KDE-TreeModule"));
			map->insert("X-KDE-TreeModule-ShowHidden",ksc2.readEntry("X-KDE-TreeModule-ShowHidden"));
			fn->setLatin1("dirtree%1.desktop");
			return true;
		}
	return false;
   }
}
