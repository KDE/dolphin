#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <qlabel.h>
#include <qlayout.h>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/browserextension.h>

class KonqSidebarTree;

class KonqSidebar_Tree: public KonqSidebarPlugin
        {
                Q_OBJECT
                public:
                KonqSidebar_Tree(QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0);
                ~KonqSidebar_Tree();
                virtual void *provides(const QString &);
		void emitStatusBarText (const QString &);
                virtual QWidget *getWidget();
	        void enableActions( bool copy, bool cut, bool paste,
                        bool trash, bool del, bool shred,
                        bool rename = false );
                protected:
                        class KonqSidebarTree *tree;
                        virtual void handleURL(const KURL &url);
        };
#endif
