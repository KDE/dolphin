#ifndef KPARTAPPPART_H
#define KPARTAPPPART_H

#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/event.h>

class QWidget;
class QPainter;
class KURL;

/**
 * This is a "Part".  It that does all the real work in a KPart
 * application.
 *
 * @short Main Part
 * @author Joseph WENNINGER <jowenn@bigfoot.com>
 * @version 0.1
 */
class KPartAppPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
public:
    /**
     * Default constructor
     */
    KPartAppPart(QWidget *parentWidget, const char *widgetName,
                    QObject *parent, const char *name);

    /**
     * Destructor
     */
    virtual ~KPartAppPart();

    virtual bool openURL(const KURL &url);
protected:
    /**
     * This must be implemented by each part
     */
    virtual bool openFile();
    virtual void guiActivateEvent(KParts::GUIActivateEvent *event);

private:
     class Sidebar_Widget *m_widget;
};

class KInstance;
class KAboutData;

class KPartAppPartFactory : public KParts::Factory
{
    Q_OBJECT
public:
    KPartAppPartFactory();
    virtual ~KPartAppPartFactory();
    virtual KParts::Part* createPartObject( QWidget *parentWidget, const char *widgetName,
                                            QObject *parent, const char *name,
                                            const char *classname, const QStringList &args );
    static KInstance* instance();
 
private:
    static KInstance* s_instance;
    static KAboutData* s_about;
};

#endif // KPARTAPPPART_H
