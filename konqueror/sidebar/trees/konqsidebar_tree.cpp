#include "konqsidebar_tree.h"
#include <klocale.h>
#include "konqsidebar_tree.moc"
#include "konq_sidebartree.h"
#include <kdebug.h>
#include <ksimpleconfig.h>
#include <kconfig.h>
#include <kstddirs.h>
#include <kmessagebox.h>

KonqSidebar_Tree::KonqSidebar_Tree(QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name):
                   KonqSidebarPlugin(parent,widgetParent,desktopName_,name)
	{
		KSimpleConfig ksc(desktopName_);
		ksc.setGroup("Desktop Entry");
		int virt= ( (ksc.readEntry("X-KDE-TreeModule","")=="Virtual") ?VIRT_Folder:VIRT_Link);
		if (virt==1) desktopName_=ksc.readEntry("X-KDE-RelURL","");
        	tree=new KonqSidebarTree(this,widgetParent,virt,desktopName_);
        }


KonqSidebar_Tree::~KonqSidebar_Tree(){;}

void* KonqSidebar_Tree::provides(const QString &) {return 0;}

void KonqSidebar_Tree::emitStatusBarText (const QString &) {;}

QWidget *KonqSidebar_Tree::getWidget(){return tree;}

void KonqSidebar_Tree::enableActions( bool copy, bool cut, bool paste,
                        bool trash, bool del, bool shred,
                        bool rename) {;}

void KonqSidebar_Tree::handleURL(const KURL &url)
    {
	emit started( 0 );
        tree->followURL( url );
        emit completed();
    }




extern "C"
{
    void* create_konqsidebar_tree(QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new KonqSidebar_Tree(par,widp,desktopname,name);
    }
};

extern "C"
{
   bool add_konqsidebar_tree(QString* fn, QString* param, QMap<QString,QString> *map)
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
	KonqSidebarTreeSelectionDialog dlg(0,names);
	if (dlg.exec()==QDialog::Accepted)
		{
			int id=dlg.getValue();
			if (id==-1) return false;
			KSimpleConfig ksc2(*list.at(id));
			ksc2.setGroup("Desktop Entry");
		        map->insert("Type","Link");
			map->insert("Icon",ksc2.readEntry("Icon"));
			map->insert("Name",ksc2.readEntry("Name"));
		 	map->insert("Open","false");
			map->insert("URL",ksc2.readEntry("X-KDE-Default-URL"));
			map->insert("X-KDE-KonqSidebarModule","konqsidebar_tree");
			map->insert("X-KDE-TreeModule",ksc2.readEntry("X-KDE-TreeModule"));
			fn->setLatin1("dirtree%1.desktop");
			return true;
		}
	return false;
   }
};
