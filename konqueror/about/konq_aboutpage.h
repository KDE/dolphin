#ifndef __konq_aboutpage_h__
#define __konq_aboutpage_h__

#include <kparts/factory.h>
#include <khtml_part.h>

class KHTMLPart;
class KInstance;

class KonqAboutPageFactory : public KParts::Factory
{
public:
    KonqAboutPageFactory( QObject *parent = 0, const char *name = 0 );
    virtual ~KonqAboutPageFactory();

    virtual KParts::Part *createPartObject( QWidget *parentWidget, const char *widgetName,
                                            QObject *parent, const char *name,
                                            const char *classname, const QStringList &args );

    static KInstance *instance() { return s_instance; }

    static QString intro();
    static QString specs();
    static QString tips();

private:
    static QString loadFile( const QString& file );
    
    static KInstance *s_instance;
    static QString *s_intro_html, *s_specs_html, *s_tips_html;
};

class KonqAboutPage : public KHTMLPart
{
    Q_OBJECT
public:
    KonqAboutPage( /*KonqMainWindow *mainWindow,*/
                   QWidget *parentWidget, const char *widgetName,
                   QObject *parent, const char *name );
    ~KonqAboutPage();

    virtual bool openURL( const KURL &url );

    virtual bool openFile();

    virtual void saveState( QDataStream &stream );
    virtual void restoreState( QDataStream &stream );

    virtual void urlSelected( const QString &url, int button, int state, const QString &target );

private:
    void serve( const QString& );

    KHTMLPart *m_doc;
    //KonqMainWindow *m_mainWindow;
    QString m_htmlDoc;
};

#endif
