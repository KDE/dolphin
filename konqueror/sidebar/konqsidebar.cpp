#include "konqsidebar.h"

#include <kinstance.h>
#include <kaction.h>
#include <kstdaction.h>
#include <kfiledialog.h>
#include <sidebar_widget.h>
#include <qfile.h>
#include <qtextstream.h>
#include <kdebug.h>

KonqSidebar::KonqSidebar( QWidget *parentWidget, const char *widgetName,
                                  QObject *parent, const char *name )
    : KParts::ReadOnlyPart(parent, name)
{
    // we need an instance
    setInstance( KPartAppPartFactory::instance() );

    // this should be your custom internal widget
    m_widget = new Sidebar_Widget( parentWidget,this, widgetName );
    setWidget(m_widget);
    m_extension = new KonqSidebarBrowserExtension( this, m_widget,"KonqSidebar::BrowserExtension" );
}

KonqSidebar::~KonqSidebar()
{
}

bool KonqSidebar::openFile()
{
	return true;
}

bool KonqSidebar::openURL(const KURL &url){if (m_widget) m_widget->openURL(url); return true;} 

// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
#include <kaboutdata.h>
#include <klocale.h>

KInstance*  KPartAppPartFactory::s_instance = 0L;
KAboutData* KPartAppPartFactory::s_about = 0L;

KPartAppPartFactory::KPartAppPartFactory()
    : KParts::Factory()
{
}

KPartAppPartFactory::~KPartAppPartFactory()
{
    delete s_instance;
    delete s_about;

    s_instance = 0L;
}

KParts::Part* KPartAppPartFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
                                                        QObject *parent, const char *name,
                                                        const char *classname, const QStringList &args )
{
    // Create an instance of our Part
    KonqSidebar* obj = new KonqSidebar( parentWidget, widgetName, parent, name );

    // See if we are to be read-write or not
//    if (QCString(classname) == "KParts::ReadOnlyPart")
  //      obj->setReadWrite(false);

    return obj;
}

KInstance* KPartAppPartFactory::instance()
{
    if( !s_instance )
    {
        s_about = new KAboutData("kpartapppart", I18N_NOOP("KPartAppPart"), "0.1");
        s_about->addAuthor("Joseph WENNINGER", 0, "jowenn@bigfoot.com");
        s_instance = new KInstance(s_about);
    }
    return s_instance;
}

extern "C"
{
    void* init_libkonqsidebar()
    {
        return new KPartAppPartFactory;
    }
};

#include "konqsidebar.moc"
