
#ifndef __browser_h__
#define __browser_h__

#include <qwidget.h>
#include <qpoint.h>
#include <kfileitem.h>

#include <kaction.h>

class QString;

class PrintingExtension : public QObject
{
  Q_OBJECT
public:
  PrintingExtension( QObject *parent, const char *name = 0L ) : QObject( parent, name ) {}

  virtual void print() = 0;

};

class EditExtension : public QObject
{
  Q_OBJECT
public:
  EditExtension( QObject *parent, const char *name = 0L ) : QObject( parent, name ) {}

  virtual void can( bool &copy, bool &paste, bool &move ) = 0;

  virtual void copySelection() = 0;
  virtual void pasteSelection() = 0;
  virtual void moveSelection( const QString &destinationURL = QString::null ) = 0;

signals:
  void selectionChanged();  

};

class BrowserView : public QWidget
{
  Q_OBJECT
public:
  BrowserView( QWidget *parent = 0L, const char *name = 0L ) : QWidget( parent, name ) {}

  virtual ~BrowserView() { }

  enum ActionFlags
  {
    MenuView  = 0x01,
    MenuEdit  = 0x02,
    ToolBar   = 0x04
  };
  
  struct ViewAction
  {
    ViewAction() : m_action( 0L ) { }
    ViewAction( QAction *action, int flags )
    : m_action( action ), m_flags( flags ) { }
    
    QAction *m_action;
    int m_flags;
  };

  virtual void openURL( const QString &url, bool reload = false,
                        int xOffset = 0, int yOffset = 0 ) = 0;

  virtual QString url() = 0;
  virtual int xOffset() = 0;
  virtual int yOffset() = 0;
  virtual void stop() = 0;

  QValueList<ViewAction> *actions() { return &m_actionCollection; }

signals:
  void openURLRequest( const QString &url, bool reload, int xOffset, int yOffset );
  void started();
  void completed();
  void canceled();
  void setStatusBarText( const QString &text );
  void setLocationBarURL( const QString &url );
  void createNewWindow( const QString &url );
  void loadingProgress( int percent );
  void speedProgress( int bytesPerSecond );
  void popupMenu( const QPoint &_global, KFileItemList _items );
  
private:
  QValueList<ViewAction> m_actionCollection;
};

#endif

