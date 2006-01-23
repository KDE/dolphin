/***************************************************************************
                               konqsidebar.h
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
#ifndef KONQSIDEBARPART_H
#define KONQSIDEBARPART_H

#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/event.h>
#include <kparts/browserextension.h>
#include <qpointer.h>
#include "sidebar_widget.h"
#include "konqsidebariface_p.h"

class QWidget;
class QPainter;
class QCustomEvent;
class KUrl;


class KonqSidebar;
class KonqSidebarFactory;

class KonqSidebarBrowserExtension : public KParts::BrowserExtension
{
	Q_OBJECT
	public:
	KonqSidebarBrowserExtension(KonqSidebar *part_,Sidebar_Widget *widget_,const char *name):
	KParts::BrowserExtension((KParts::ReadOnlyPart*)part_),widget(widget_){setObjectName(name);}
	~KonqSidebarBrowserExtension(){;}

	protected:
	QPointer<Sidebar_Widget> widget;


// The following slots are needed for konqueror's standard actions
	protected Q_SLOTS:
	    void copy(){if (widget) widget->stdAction("copy()");}
	    void cut(){if (widget) widget->stdAction("cut()");}
	    void paste(){if (widget) widget->stdAction("paste()");}
	    void pasteTo(const KURL&){if (widget) widget->stdAction("paste()");}
	    void trash(){if (widget) widget->stdAction("trash()");}
	    void del(){if (widget) widget->stdAction("del()");}
	    void rename(){if (widget) widget->stdAction("rename()");}
  	    void properties() {if (widget) widget->stdAction("properties()");}
  	    void editMimeType() {if (widget) widget->stdAction("editMimeType()");}
	    //  @li @p print : Print :-) not supported
	    void reparseConfiguration() {if (widget) widget->stdAction("reparseConfiguration()");}
	    void refreshMimeTypes () { if (widget) widget->stdAction("refreshMimeTypes()");}
};

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Joseph WENNINGER <jowenn@bigfoot.com>
 * @version 0.1
 */
class KonqSidebar : public KParts::ReadOnlyPart, public KonqSidebarIface
{
    Q_OBJECT
public:
    /**
     * Default constructor
     */
    KonqSidebar(QWidget *parentWidget, const char *widgetName,
                    QObject *parent, const char *name,bool universalMode);

    /**
     * Destructor
     */
    virtual ~KonqSidebar();

    virtual bool openURL(const KURL &url);
    KonqSidebarBrowserExtension* extension() const
       { return m_extension; }
    KInstance *getInstance();
    virtual bool universalMode() {return m_universalMode;}
protected:
    /**
     * This must be implemented by each part
     */
    KonqSidebarBrowserExtension * m_extension;
    virtual bool openFile();

    virtual void customEvent(QCustomEvent* ev);

private:
     class Sidebar_Widget *m_widget;
     bool m_universalMode;
};

class KInstance;
class KAboutData;

class KonqSidebarFactory : public KParts::Factory
{
    Q_OBJECT
public:
    KonqSidebarFactory();
    virtual ~KonqSidebarFactory();
    virtual KParts::Part* createPartObject( QWidget *parentWidget, const char *widgetName,
                                            QObject *parent, const char *name,
                                            const char *classname, const QStringList &args );
    static KInstance* instance();

private:
    static KInstance* s_instance;
    static KAboutData* s_about;
};

#endif // KPARTAPPPART_H
