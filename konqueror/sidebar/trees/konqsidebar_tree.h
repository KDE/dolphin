#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <QLabel>
#include <QLayout>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/browserextension.h>
#include <kdialog.h>
#include <QComboBox>
#include <QStringList>
#include <klocale.h>
#include <QLineEdit>
class KonqSidebarTree;

class KonqSidebar_Tree: public KonqSidebarPlugin
        {
                Q_OBJECT
                public:
                KonqSidebar_Tree(KInstance *instance,QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0);
                ~KonqSidebar_Tree();
                virtual void *provides(const QString &);
//		void emitStatusBarText (const QString &);
                virtual QWidget *getWidget();
                protected:
                        class QWidget *widget;
                        class KonqSidebarTree *tree;
                        virtual void handleURL(const KUrl &url);
		protected Q_SLOTS:
			void copy();
			void cut();
			void paste();
			void trash();
			void del();
			void shred();
			void rename();
Q_SIGNALS:
			void openURLRequest( const KUrl &url, const KParts::URLArgs &args = KParts::URLArgs() );
  			void createNewWindow( const KUrl &url, const KParts::URLArgs &args = KParts::URLArgs() );
			void popupMenu( const QPoint &global, const KUrl &url,
					const QString &mimeType, mode_t mode = (mode_t)-1 );
			void popupMenu( const QPoint &global, const KFileItemList &items );
			void enableAction( const char * name, bool enabled );
        };

#endif
