#ifndef __konq_dirmetaview__
#define __konq_dirmetaview__

#include <qframe.h>

#include <kparts/browserextension.h>
#include <kparts/part.h>
#include <kparts/factory.h>

#include <kmimetype.h>
#include <kio/job.h>

class QLabel;
class QHBoxLayout;
class KSeparator;

namespace Konqueror
{

class DirDetailViewFactory : public KParts::Factory
{
  Q_OBJECT
public:
  DirDetailViewFactory( QObject *parent = 0, const char *name = 0 );
  virtual ~DirDetailViewFactory();

  virtual KParts::Part *createPart( QWidget *parentWidget = 0, const char *widgetName = 0, QObject *parent = 0, const char *name = 0, const char *classname = "KParts::Part", const QStringList &args = QStringList() );

  KMimeType::Ptr dirMimeType() { return m_dirMimeType; }

private:
  KMimeType::Ptr m_dirMimeType;
};

class DetailWidget : public QWidget
{
  Q_OBJECT
public:

  DetailWidget( QWidget *parent, const char *name );
  virtual ~DetailWidget();

  void setPixmap( const QPixmap &pix ) { m_pix = pix; m_secondPix = QPixmap(); m_thirdPix = QPixmap(); }
  void setPixmap( const QPixmap &first, const QPixmap &second, const QPixmap &third )
  {
    m_pix = first; m_secondPix = second; m_thirdPix = third;
  }
  void setText( const QString &text ) { m_text = text; }
  void setSecondColumnText( const QString &stext ) { m_stext = stext; }

protected:
  virtual void paintEvent( QPaintEvent * );

  QString m_text;
  QString m_stext;
  QPixmap m_pix;
  QPixmap m_secondPix;
  QPixmap m_thirdPix;
  QPixmap m_bg;
};

class DirDetailView : public KParts::ReadOnlyPart
{
  Q_OBJECT
public:
  DirDetailView( DirDetailViewFactory *factory, QWidget *parentWidget, const char *widgetName, QObject *parent, const char *name );
  virtual ~DirDetailView();

  virtual bool openURL( const KURL &url );

  bool openURL( const KURL &url, KonqFileItemList selection );

  virtual bool openFile() { return true; }

  virtual bool eventFilter( QObject *obj, QEvent *event );

  //move all the metadata stuff into libkonqueror
  void setAnnotationMetaData( const KURL &url, const QString &text );
  QString annotationMetaData( const KURL &url );

protected:
  DetailWidget *m_widget;

  KSeparator *m_separator;

  KURL m_url;

  QDomDocument m_metaData;
  KURL m_metaDataURL;
  bool m_bEditableMetaData;

  DirDetailViewFactory *m_factory;

  KURL::List m_currentSelection;

  KIO::Job *m_job;
};

};

#endif
