/***************************************************************************
                               konqsidebar.cpp
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "konqsidebar.h"
#include "konqsidebariface_p.h"

#include <konq_events.h>
#include <kdebug.h>
#include <qapplication.h>
#include <kacceleratormanager.h>

KonqSidebar::KonqSidebar( QWidget *parentWidget, const char *widgetName,
                          QObject *parent, const char *name, bool universalMode )
: KParts::ReadOnlyPart(parent),KonqSidebarIface()
{
        setObjectName(name);
	// we need an instance
	setInstance( KonqSidebarFactory::instance() );
	m_extension = 0;
	// this should be your custom internal widget
	m_widget = new Sidebar_Widget( parentWidget, this, widgetName ,universalMode, parentWidget->topLevelWidget()->property("currentProfile").toString() );
	m_extension = new KonqSidebarBrowserExtension( this, m_widget,"KonqSidebar::BrowserExtension" );
	connect(m_widget,SIGNAL(started(KIO::Job *)),
		this, SIGNAL(started(KIO::Job*)));
	connect(m_widget,SIGNAL(completed()),this,SIGNAL(completed()));
	connect(m_extension, SIGNAL(addWebSideBar(const KURL&, const QString&)),
		m_widget, SLOT(addWebSideBar(const KURL&, const QString&)));
        KAcceleratorManager::setNoAccel(m_widget);
	setWidget(m_widget);
}

KInstance *KonqSidebar::getInstance()
{
	kdDebug() << "KonqSidebar::getInstance()" << endl;
	return KonqSidebarFactory::instance(); 
}

KonqSidebar::~KonqSidebar()
{
}

bool KonqSidebar::openFile()
{
	return true;
}

bool KonqSidebar::openURL(const KURL &url) {
	if (m_widget)
		return m_widget->openURL(url);
	else return false;
} 

void KonqSidebar::customEvent(QCustomEvent* ev)
{
	if (KonqFileSelectionEvent::test(ev) ||
	    KonqFileMouseOverEvent::test(ev) ||
	    KonqConfigEvent::test(ev))
	{
		// Forward the event to the widget
		QApplication::sendEvent( m_widget, ev );
	}
}



// It's usually safe to leave the factory code alone.. with the
// notable exception of the KAboutData data
#include <kaboutdata.h>
#include <klocale.h>
#include <kinstance.h>

KInstance*  KonqSidebarFactory::s_instance = 0L;
KAboutData* KonqSidebarFactory::s_about = 0L;

KonqSidebarFactory::KonqSidebarFactory()
    : KParts::Factory()
{
}

KonqSidebarFactory::~KonqSidebarFactory()
{
	delete s_instance;
	s_instance = 0L;
	delete s_about;
	s_about = 0L;
}

KParts::Part* KonqSidebarFactory::createPartObject( QWidget *parentWidget, const char *widgetName,
                                                        QObject *parent, const char *name,
                                                        const char * /*classname*/, const QStringList &args )
{
    // Create an instance of our Part
    KonqSidebar* obj = new KonqSidebar( parentWidget, widgetName, parent, name, args.contains("universal") );

    // See if we are to be read-write or not
//    if (QCString(classname) == "KParts::ReadOnlyPart")
  //      obj->setReadWrite(false);

    return obj;
}

KInstance* KonqSidebarFactory::instance()
{
	if( !s_instance )
	{
		s_about = new KAboutData("konqsidebartng", I18N_NOOP("Extended Sidebar"), "0.1");
		s_about->addAuthor("Joseph WENNINGER", 0, "jowenn@bigfoot.com");
		s_instance = new KInstance(s_about);
	}
	return s_instance;
}

K_EXPORT_COMPONENT_FACTORY( konq_sidebar, KonqSidebarFactory )

#include "konqsidebar.moc"
