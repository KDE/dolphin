/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Michael Reiher <michael.reiher@gmx.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <math.h>

#include <qpainter.h>
#include <qimage.h>
#include <qbitmap.h>
#include <qlayout.h>
#include <qsplitter.h>

#include <kapp.h>
#include <kdebug.h>
#include <kconfig.h>
#include <kiconloader.h>
#include <kpixmap.h>

#include <kbrowser.h>
#include "konq_frame.h"
#include "konq_childview.h"

#include <assert.h>

#define DEFAULT_HEADER_HEIGHT 11

static QBitmap *checkBmp = NULL;

void KonqCheckBox::paintEvent(QPaintEvent *)
{
static unsigned char check_bits[] = {
  0x11, 0x0a, 0x04, 0x0a, 0x11, };

    if(!checkBmp){
        checkBmp = new QBitmap(5, 5, check_bits, true);
        checkBmp->setMask(*checkBmp);
    }
    QPainter p(this);
    QRect r = rect();

    bool active = isOn() || isDown();
    QApplication::style().drawBevelButton(&p, r.x(), r.y(), r.width(),
                                          r.height(), colorGroup(), active);
    if(active){
        p.setPen(colorGroup().text());
        p.drawPixmap((width()-checkBmp->width())/2,
                     (height()-checkBmp->height())/2,
                     *checkBmp);
    }
}


KonqFrameHeader::KonqFrameHeader( KonqFrame *_parent, const char *_name )
                                : QWidget( _parent, _name ),
				  m_pParentKonqFrame( _parent )
{
  QString key;

  //killTimers();

  KConfig* config = new KConfig( "kwmrc", true);

  config->setGroup("WM");

  frameHeaderBlendActive = config->readColorEntry( "activeBlend" , &(Qt::black) );
  frameHeaderBlendInactive = config->readColorEntry( "inactiveBlend" ,
						     &palette().normal().background() );

  config->setGroup( "General" );

  key = config->readEntry("TitlebarLook");
  if( key == "shadedHorizontal")
    frameHeaderLook = HORIZ;
  else if( key == "shadedVertical")
    frameHeaderLook = VERT;
  else if( key == "shadedDiagonal")
    frameHeaderLook = DIAG;
  else if( key == "shadedCrossDiagonal")
    frameHeaderLook = CROSSDIAG;
  else if( key == "shadedRectangle")
    frameHeaderLook = RECT;
  else if( key == "shadedElliptic")
    frameHeaderLook = ELLIP;
  else if( key == "shadedPyramid")
    frameHeaderLook = PYRAM;
  else if( key == "shadedPipeCross")
    frameHeaderLook = PIPE;
  else if( key == "plain")
    frameHeaderLook = Plain;
  else if( key == "pixmap")
    frameHeaderLook = XPixmap;
  else{
    config->writeEntry("TitlebarLook", "shadedHorizontal");
    frameHeaderLook = HORIZ;
  }
  /*key = config->readEntry("TitleAlignment");
  if (key == "left")
    options.alignTitle = AT_LEFT;
  else if (key == "middle")
    options.alignTitle = AT_MIDDLE;
  else if (key == "right")
    options.alignTitle = AT_RIGHT;
  else {
    config->writeEntry("TitleAlignment", "left");
    options.alignTitle = AT_LEFT;
  }

  //CT 02Dec1998 - optional pixmap under the title text
  key = config->readEntry("PixmapUnderTitleText");
  if (key == "no")
    options.PixmapUnderTitleText = false;
  else if (key == "yes")
    options.PixmapUnderTitleText = true;
  else {
    config->writeEntry("PixmapUnderTitleText","yes");
    options.PixmapUnderTitleText = true;
  }
  //CT

  */

  frameHeaderActive = new QPixmap;
  frameHeaderInactive = new QPixmap;

  if ( frameHeaderLook == XPixmap ) {
    KIconLoader iconLoader("kwm");

    *(frameHeaderActive) = iconLoader.reloadIcon("activetitlebar");
    *(frameHeaderInactive) = iconLoader.reloadIcon("inactivetitlebar");

    if (frameHeaderInactive->size() == QSize(0,0))
      *frameHeaderInactive = *frameHeaderActive;

    if (frameHeaderActive->size() == QSize(0,0))
      frameHeaderLook = Plain;
  }
  /*
  if (config->hasKey("TitleAnimation")){
    options.TitleAnimation = config->readNumEntry("TitleAnimation");
    if (options.TitleAnimation < 0)
      options.TitleAnimation = 0;
  }
  else{
    options.TitleAnimation = 50;
    config->writeEntry("TitleAnimation", options.TitleAnimation);
  }

  if (options.TitleAnimation)
      startTimer(options.TitleAnimation);
  */

  m_pPassiveModeCheckBox = new KonqCheckBox( this );

  setFixedHeight( DEFAULT_HEADER_HEIGHT );

  connect( m_pPassiveModeCheckBox, SIGNAL( toggled( bool ) ),
           this, SIGNAL( passiveModeChange( bool ) ) );
}

enum KPixmapEffect::GradientType
KonqFrameHeader::mapShade( KonqFrameHeaderLook look)
{
  switch (look)
    {
    case HORIZ: return KPixmapEffect::HorizontalGradient;
    case VERT: return KPixmapEffect::VerticalGradient;
    case DIAG: return KPixmapEffect::DiagonalGradient;
    case CROSSDIAG: return KPixmapEffect::CrossDiagonalGradient;
    case PYRAM: return KPixmapEffect::PyramidGradient;
    case RECT: return KPixmapEffect::RectangleGradient;
    case PIPE: return KPixmapEffect::PipeCrossGradient;
    case ELLIP: return KPixmapEffect::EllipticGradient;
    default:
      return KPixmapEffect::HorizontalGradient; // keep compiler happy
    }

}

void
KonqFrameHeader::paintEvent( QPaintEvent* )
{
//  kdebug(0, 1202, "KonqFrameHeader::paintEvent( QPaintEvent* ) : part()->hasFocus()=%d",hasFocus);
  if (!isVisible())
  {
//    kdebug(0, 1202, "paintEvent aborted : not visible !");
    return;
  }

  bool hasFocus = m_pParentKonqFrame->isActivePart();

  if ( m_pParentKonqFrame->childView()->passiveMode() )
    hasFocus = false;

  QRect r = rect();
  //bool double_buffering = false;
  //QPixmap* buffer = 0;
  KonqFrameHeaderLook look = frameHeaderLook;

  if ( look == HORIZ || look == VERT || look == DIAG ||
       look == PIPE  || look == RECT ||
       look == CROSSDIAG || look == ELLIP){
    // the new horizontal (and vertical) shading code
    /*if (colors_have_changed){
      aShadepm.resize(0,0);
      iaShadepm.resize(0,0);
      }*/

    // the user selected shading. Do a plain titlebar anyway if the
    // shading would be senseless (higher performance and less memory
    // consumption)
    if ( hasFocus ){
      if ( kapp->activeTitleColor() ==  frameHeaderBlendActive)
        look = Plain;
    }
    else {
      if ( kapp->inactiveTitleColor() ==  frameHeaderBlendInactive)
        look = Plain;
    }
  }
  QPainter p;

  /*  if (only_label && animate){
    double_buffering = ( look == HShaded ||
                         look == VShaded ||
                         look == D_SAHDED ||
                         look == XPixmap);
    if(titlestring_delay) {
      titlestring_delay--;
      return;
    }
    else
      titlestring_offset += titlestring_offset_delta;
    if (!double_buffering){
      if (titlestring_offset_delta > 0)
	bitBlt(this,
	       r.x()+titlestring_offset_delta, r.y(),
	       this,
	       r.x(), r.y(),
	       r.width()-titlestring_offset_delta, r.height());
      else
	bitBlt(this,
	       r.x(), r.y(),
	       this,
	       r.x()-titlestring_offset_delta, r.y(),
	       r.width()+titlestring_offset_delta, r.height());
    }
  }*/

  //if (!double_buffering)
    p.begin( this );
  /*else {
    // enable double buffering to avoid flickering with horizontal shading
    buffer = new QPixmap(r.width(), r.height());
    p.begin(buffer);
    r.setRect(0,0,r.width(),r.height());
  }*/

  QPixmap *pm;
  p.setClipRect(r);
  p.setClipping(true);

  if ( look == XPixmap){
    pm = hasFocus ? frameHeaderActive : frameHeaderInactive;
    for (int x = r.x(); x < r.x() + r.width(); x+=pm->width())
      p.drawPixmap(x, r.y(), *pm);
  }
  else if ( look == HORIZ || look == VERT || look == DIAG ||
	    look == PIPE  || look == RECT ||
	    look == CROSSDIAG || look == ELLIP){
    // the new horizontal shading code
    QPixmap* pm = 0;
    if (hasFocus){
      if (activeShadePm.size() != r.size()){
	activeShadePm.resize(r.width(), r.height());
	KPixmapEffect::gradient(activeShadePm, kapp->activeTitleColor(),
				frameHeaderBlendActive, mapShade(look));
      }
      pm = &activeShadePm;
    }
    else {
      if (inactiveShadePm.size() != r.size()){
	inactiveShadePm.resize(r.width(), r.height());
	KPixmapEffect::gradient(inactiveShadePm, kapp->inactiveTitleColor(),
				frameHeaderBlendInactive, mapShade(look));
      }
      pm = &inactiveShadePm;
    }

    p.drawPixmap( r.x(), r.y(), *pm );
  }
  else { // TitlebarLook == TITLEBAR_PLAIN
    p.setBackgroundColor( hasFocus ? kapp->activeTitleColor()
			           : kapp->inactiveTitleColor());
    /*if (only_label && !double_buffering && animate){
       p.eraseRect(QRect(r.x(), r.y(), TITLE_ANIMATION_STEP+1, r.height()));
       p.eraseRect(QRect(r.x()+r.width()-TITLE_ANIMATION_STEP-1, r.y(),
 			TITLE_ANIMATION_STEP+1, r.height()));
    }
    else {*/
      p.eraseRect(r);
    //}
  }
  p.setClipping(false);

/*
//  p.setPen(is_active ? kapp->activeTextColor() : app->inactiveTextColor());

  p.setFont(myapp->tFont);
  //CT

  titlestring_too_large = (p.fontMetrics().width(QString(" ")+label+" ")>r.width());
  if (titlestring_offset_delta > 0){
    if (!titlestring_delay){
      if (titlestring_offset > 0
      && titlestring_offset > r.width() - p.fontMetrics().width(QString(" ")+label+" ")){
        // delay animation before reversing direction
        titlestring_delay = TITLE_ANIMATION_DELAY;
        titlestring_offset_delta *= -1;
      }
    }
  }
  else {
    if(!titlestring_delay){
      if (titlestring_offset < 0
      && titlestring_offset + p.fontMetrics().width(QString(" ")+label+" ") < r.width()){
        if(titlestring_offset_delta != 0)
          // delay animation before reversing direction
          titlestring_delay = TITLE_ANIMATION_DELAY;
        titlestring_offset_delta *= -1;
      }
    }
  }*/

  //p.setClipRect(r);
  //p.setClipping(True);

  //CT 04Nov1998 - align the title in the bar
  /*int aln = 0;
  int need = p.fontMetrics().width(QString(" ")+label+" ");
  if (options.alignTitle == AT_LEFT || r.width() < need) aln = 0;
  else if (options.alignTitle == AT_MIDDLE )
    aln = r.width()/2 - need/2;
  else if (options.alignTitle == AT_RIGHT)
    aln = r.width() - need;
  //CT see next lines. Replaced two 0 by `aln`. Moved next two lines here
  //  from above.

  if (!titlestring_too_large)
    titlestring_offset = aln;

  //CT 02Dec1998 - optional noPixmap behind text,
  //     suggested by Nils Meier <nmeier@vossnet.de>
  if (!options.PixmapUnderTitleText && look == XPixmap ) {
    // NM 02Dec1998 - Clear bg behind text
    p.setBackgroundColor(is_active ? myapp->activeTitleColor() :
			 myapp->inactiveTitleColor());
    p.eraseRect(
		r.x()+(options.TitleAnimation?titlestring_offset:aln),
		r.y()+(r.height()-p.fontMetrics().height())/2,
		need,
		p.fontMetrics().height());
  }
  //CT

  //CT 02Dec1998 - vertical text centering,
  //     thanks to Nils Meier <nmeier@vossnet.de>
  p.drawText(r.x()+
	     (options.TitleAnimation?titlestring_offset:aln),

	     r.y()+
	     (r.height()-p.fontMetrics().height())/2+
	     p.fontMetrics().ascent(),

	     QString(" ")+label+" ");

  p.setClipping(False);
  p.end();
  if (double_buffering){
    bitBlt(this,
	   title_rect.x(), title_rect.y(),
	   buffer,
	   0,0,
	   buffer->width(), buffer->height());
    delete buffer;
  }*/
  p.end();
//  kdebug(0, 1202, "paintEvent done !");

}

void
KonqFrameHeader::mousePressEvent( QMouseEvent* event )
{
  QWidget::mousePressEvent( event );
  if ( !m_pParentKonqFrame->childView()->passiveMode() )
  {
    emit headerClicked();
    update();
  }
}

void KonqFrameHeader::resizeEvent( QResizeEvent * )
{
  m_pPassiveModeCheckBox->setGeometry( 0, 0, m_pPassiveModeCheckBox->sizeHint().width(), height() );
}

//###################################################################

KonqFrame::KonqFrame( KonqFrameContainer *_parentContainer, const char *_name )
                    : QWidget( _parentContainer, _name)
{
  m_pLayout = 0L;
  m_pChildView = 0L;

  // add the frame header to the layout
  m_pHeader = new KonqFrameHeader( this, "KonquerorFrameHeader");
  QObject::connect(m_pHeader, SIGNAL(headerClicked()), this, SLOT(slotHeaderClicked()));
  connect( m_pHeader, SIGNAL( passiveModeChange( bool ) ), this, SLOT( slotPassiveModeChange( bool ) ) );
}

BrowserView *
KonqFrame::view( void )
{
  return m_pView;
}

bool KonqFrame::isActivePart()
{
  return ( (BrowserView *)m_pView == m_pChildView->mainView()->currentView() );
}

void
KonqFrame::listViews( ChildViewList *viewList )
{
  viewList->append( childView() );
}

void
KonqFrame::saveConfig( KConfig* config, const QString &prefix, int /*id*/, int /*depth*/ )
{
  config->writeEntry( QString::fromLatin1( "URL" ).prepend( prefix ), childView()->url() );
  config->writeEntry( QString::fromLatin1( "ServiceType" ).prepend( prefix ), childView()->serviceType() );
  config->writeEntry( QString::fromLatin1( "ServiceName" ).prepend( prefix ), childView()->service()->name() );
  config->writeEntry( QString::fromLatin1( "PassiveMode" ).prepend( prefix ), childView()->passiveMode() );
}

BrowserView *KonqFrame::attach( const KonqViewFactory &viewFactory )
{
  if (m_pLayout) delete m_pLayout;

  KonqViewFactory factory( viewFactory );
  
  m_pLayout = new QVBoxLayout( this );
  m_pLayout->addWidget( m_pHeader );
  m_pView = factory.create( this, 0L );
  //  m_pView->setGeometry( 0, DEFAULT_HEADER_HEIGHT, width(), height() );
  m_pLayout->addWidget( m_pView );
  m_pView->show();
  m_pHeader->show();
  m_pLayout->activate();
  
  return m_pView;
}

KonqFrameContainer*
KonqFrame::parentContainer()
{
  if( parentWidget()->isA("KonqFrameContainer") )
    return (KonqFrameContainer*)parentWidget();
  else
    return 0L;
}

void
KonqFrame::reparent( QWidget* parent, WFlags f,
		     const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, f, p, showIt );
}

void
KonqFrame::slotHeaderClicked()
{
  m_pChildView->mainView()->setActiveView( m_pView );
/*
  if ( !CORBA::is_nil( m_vView ) )
  {
    OpenParts::MainWindow_var mainWindow = m_vView->mainWindow();
    mainWindow->setActivePart( m_vView->id() );
  }
*/
}

void KonqFrame::slotPassiveModeChange( bool mode )
{
  m_pChildView->setPassiveMode( mode );
}

void
KonqFrame::paintEvent( QPaintEvent* )
{
  m_pHeader->repaint();
}

//###################################################################

KonqFrameContainer::KonqFrameContainer( Orientation o,
					QWidget* parent,
					const char * name)
  : QSplitter( o, parent, name)
{
  m_pFirstChild = 0L;
  m_pSecondChild = 0L;
}

void
KonqFrameContainer::listViews( ChildViewList *viewList )
{
  if( firstChild() )
    firstChild()->listViews( viewList );

  if( secondChild() )
    secondChild()->listViews( viewList );
}

void
KonqFrameContainer::saveConfig( KConfig* config, const QString &prefix, int id, int depth )
{
  int idSecond = id + (int)pow( 2, depth );

  //write own config

  //write children sizes
  config->writeEntry( QString::fromLatin1( "SplitterSizes" ).prepend( prefix ), QVariant( sizes() ) );

  //write children
  QStringList strlst;
  if( firstChild() )
    strlst.append( firstChild()->frameType() + QString("%1").arg(idSecond - 1) );
  if( secondChild() )
    strlst.append( secondChild()->frameType() + QString("%1").arg( idSecond ) );

  config->writeEntry( QString::fromLatin1( "Children" ).prepend( prefix ), strlst );

  //write orientation
  QString o;
  if( orientation() == Qt::Horizontal )
    o = QString::fromLatin1("Horizontal");
  else if( orientation() == Qt::Vertical )
    o = QString::fromLatin1("Vertical");
  config->writeEntry( QString::fromLatin1( "Orientation" ).prepend( prefix ), o );


  //write child configs
  if( firstChild() ) {
//    config->setGroup( firstChild()->frameType() + QString("%1").arg(idSecond - 1) );
    QString newPrefix = firstChild()->frameType() + QString("%1").arg(idSecond - 1);
    newPrefix.append( '_' );
    firstChild()->saveConfig( config, newPrefix, id, depth + 1 );
  }

  if( secondChild() ) {
//    config->setGroup( secondChild()->frameType() + QString("%1").arg( idSecond ) );
    QString newPrefix = secondChild()->frameType() + QString("%1").arg( idSecond );
    newPrefix.append( '_' );
    secondChild()->saveConfig( config, newPrefix, idSecond, depth + 1 );
  }
}

KonqFrameBase*
KonqFrameContainer::otherChild( KonqFrameBase* child )
{
  if( firstChild() == child )
    return secondChild();
  else if( secondChild() == child )
    return firstChild();
  else
    return 0L;
}

KonqFrameContainer*
KonqFrameContainer::parentContainer()
{
  if( parentWidget()->isA("KonqFrameContainer") )
    return (KonqFrameContainer*)parentWidget();
  else
    return 0L;
}

void
KonqFrameContainer::reparent( QWidget* parent, WFlags f, const QPoint & p, bool showIt )
{
  QWidget::reparent( parent, f, p, showIt );
}

void
KonqFrameContainer::childEvent( QChildEvent * ce )
{
  KonqFrameBase* castChild = 0L;

  if( ce->child()->isA("KonqFrame") )
    castChild = ( KonqFrame* )ce->child();
  else if( ce->child()->isA("KonqFrameContainer") )
    castChild = ( KonqFrameContainer* )ce->child();

  if( ce->type() == QEvent::ChildInserted ) {

    if( castChild )
      if( !firstChild() ) {
	setFirstChild( castChild );
      }
      else if( !secondChild() ) {
	setSecondChild( castChild );
      }

  }
  else if( ce->type() == QEvent::ChildRemoved ) {

    if( castChild ) {
      if( firstChild() == ( KonqFrameBase* )castChild ) {
	setFirstChild( 0L );
      }
      else if( secondChild() == ( KonqFrameBase* )castChild ) {
	setSecondChild( 0L );
      }
    }
  }

  QSplitter::childEvent( ce );
}

#include "konq_frame.moc"
