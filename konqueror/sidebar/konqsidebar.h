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
#include <qguardedptr.h>
#include <sidebar_widget.h>

class QWidget;
class QPainter;
class KURL;


class KonqSidebar;
class KonqSidebarFactory;

class KonqSidebarBrowserExtension : public KParts::BrowserExtension
{
	Q_OBJECT
	public:
	KonqSidebarBrowserExtension(KonqSidebar *part_,Sidebar_Widget *widget_,const char *name):
	KParts::BrowserExtension((KParts::ReadOnlyPart*)part_,name),widget(widget_){;}
	~KonqSidebarBrowserExtension(){;}

	protected:
	QGuardedPtr<Sidebar_Widget> widget;

	protected slots:
	    void copy(){if (widget) widget->stdAction("copy");}
	    void cut(){if (widget) widget->stdAction("cut");}
	    void paste(){if (widget) widget->stdAction("paste");}
	    void trash(){if (widget) widget->stdAction("trash");}
	    void del(){if (widget) widget->stdAction("trash");}
	    void shred(){if (widget) widget->stdAction("shred");}
	    void rename(){if (widget) widget->stdAction("rename");}
};

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Joseph WENNINGER <jowenn@bigfoot.com>
 * @version 0.1
 */
class KonqSidebar : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    /**
     * Default constructor
     */
    KonqSidebar(QWidget *parentWidget, const char *widgetName,
                    QObject *parent, const char *name);

    /**
     * Destructor
     */
    virtual ~KonqSidebar();

    virtual bool openURL(const KURL &url);
    KonqSidebarBrowserExtension* extension() const
       { return m_extension; }
    KInstance *getInstance();
protected:
    /**
     * This must be implemented by each part
     */
    KonqSidebarBrowserExtension * m_extension;
    virtual bool openFile();

private:
     class Sidebar_Widget *m_widget;

protected slots:
	void closeMe();
public slots:
	void doCloseMe();
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
