/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <qpainter.h>
#include <qimage.h>

#include <kapp.h>
#include <kconfig.h>

#include "konq_frame.h"

#define DEFAULT_HEADER_HEIGHT 9

KonqFrameHeader::KonqFrameHeader( KonqFrame *_parent = 0L, const char *_name = 0L ) : QWidget( _parent, _name ), m_pParentKonqFrame( _parent )
{
  KConfig* config;
  QString key;
  //int i;

  //killTimers();

  //config = kapp->getConfig();
  //config = getKApplication()->getConfig();
  //QString global = kapp->kde_configdir() + "kwmrc";
  //QString local = kapp->localconfigdir() + "kwmrc";
  config = new KConfig( kapp->kde_configdir() + "/kwmrc" , kapp->localconfigdir() + "/kwmrc" );

  // this belongs in kapp....
  config->setGroup("WM");

  activeTitleBlend = config->readColorEntry( "activeBlend" , &(Qt::black) );
  inactiveTitleBlend = config->readColorEntry( "inactiveBlend" , &palette().normal().background() );

  config->setGroup( "General" );

  key = config->readEntry("TitlebarLook");
  if( key == "shadedHorizontal")
    frameHeaderLook = H_SHADED;
  else if( key == "shadedVertical")
    frameHeaderLook = V_SHADED;
  else if( key == "plain")
    frameHeaderLook = PLAIN;
  else if( key == "pixmap")
    frameHeaderLook = PIXMAP;
  else{
    config->writeEntry("TitlebarLook", "shadedHorizontal");
    frameHeaderLook = H_SHADED;
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

  //CT 02Dec1998 - optional shaded frame on titlebar
  key = config->readEntry("TitleFrameShaded");
  if (key == "no")
    options.framedActiveTitle = false;
  else if (key == "yes")
    options.framedActiveTitle = true;
  else {
    config->writeEntry("TitleFrameShaded","yes");
    options.framedActiveTitle = true;
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


  //CT 23Sep1998 - fixed the name of the titlebar pixmaps to become
  //   consistent with the buttons pixmaps definition technique
  if (options.TitlebarLook == PIXMAP) {
    *(options.titlebarPixmapActive) = getIconLoader()
      ->reloadIcon("activetitlebar.xpm");
    *(options.titlebarPixmapInactive) = getIconLoader()
      ->reloadIcon("inactivetitlebar.xpm");

    if (options.titlebarPixmapInactive->size() == QSize(0,0))
      *options.titlebarPixmapInactive = *options.titlebarPixmapActive;

    if (options.titlebarPixmapActive->size() == QSize(0,0))
      options.TitlebarLook = PLAIN;
  }

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
}

void
KonqFrameHeader::paintEvent( QPaintEvent* )
{
  bool hasFocus = m_pParentKonqFrame->part()->hasFocus();

  QRect r = rect();
  //bool double_buffering = false;
  //QPixmap* buffer = 0;

  if (frameHeaderLook == H_SHADED || frameHeaderLook == V_SHADED){
    // the new horizontal (and vertical) shading code
    /*if (colors_have_changed){
      aShadepm.resize(0,0);
      iaShadepm.resize(0,0);
      }*/

    // the user selected shading. Do a plain titlebar anyway if the
    // shading would be senseless (higher performance and less memory
    // consumption)
    if ( hasFocus ){
      if ( kapp->activeTitleColor() ==  activeTitleBlend)
        frameHeaderLook = PLAIN;
    }
    else {
      if ( kapp->inactiveTitleColor() ==  inactiveTitleBlend)
        frameHeaderLook = PLAIN;
    }
  }

  QPainter p;

  /*  if (only_label && animate){
    double_buffering = (look == H_SHADED || look == V_SHADED || look == PIXMAP);
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
  p.setClipping(True);

  /*if (look == PIXMAP){
      pm = is_active ? options.titlebarPixmapActive: options.titlebarPixmapInactive;
      for (x = r.x(); x < r.x() + r.width(); x+=pm->width())
	p.drawPixmap(x, r.y(), *pm);
  }
  else*/ if ( frameHeaderLook == H_SHADED || frameHeaderLook == V_SHADED ){
    // the new horizontal shading code
    QPixmap* pm = 0;
    if (hasFocus){
      if (aShadepm.size() != r.size()){
	aShadepm.resize(r.width(), r.height());
	gradientFill( aShadepm, kapp->activeTitleColor(), 
		      activeTitleBlend, frameHeaderLook == V_SHADED );
      }
      pm = &aShadepm;
    }
    else {
      if (iaShadepm.size() != r.size()){
	iaShadepm.resize(r.width(), r.height());
	gradientFill( iaShadepm, kapp->inactiveTitleColor(), 
		      inactiveTitleBlend, frameHeaderLook == V_SHADED );
      }
      pm = &iaShadepm;
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
  p.setClipping(False);

  //CT 02Dec1998 - optional shade, suggested by Nils Meier <nmeier@vossnet.de>
/*  if (is_active && options.framedActiveTitle)
    qDrawShadePanel( &p, r, colorGroup(), true );

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
  if (!options.PixmapUnderTitleText && look == PIXMAP ) {
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

}

void
KonqFrameHeader::mousePressEvent( QMouseEvent* event )
{
  QWidget::mousePressEvent( event );
  cout << "KonqFrameHeader::mousePressEvent" << endl;
  emit headerClicked();
  update();
}

void 
KonqFrameHeader::gradientFill(KPixmap &pm, QColor ca, QColor cb,bool vertShaded)
{
  if(vertShaded == false && QColor::numBitPlanes() >= 15) 
  {    
    int w = pm.width();
    int h = pm.height();
    
    int c_red_a = ca.red() << 16;
    int c_green_a = ca.green() << 16;
    int c_blue_a = ca.blue() << 16;

    int c_red_b = cb.red() << 16;
    int c_green_b = cb.green() << 16;
    int c_blue_b = cb.blue() << 16;
    
    int d_red = (c_red_b - c_red_a) / w;
    int d_green = (c_green_b - c_green_a) / w;
    int d_blue = (c_blue_b - c_blue_a) / w;

    QImage img(w, h, 32);
    uchar *p = img.scanLine(0);

    int r = c_red_a, g = c_green_a, b = c_blue_a;
    for(int x = 0; x < w; x++) {
      *p++ = r >> 16;
      *p++ = g >> 16;
      *p++ = b >> 16;
      p++;
      
      r += d_red;
      g += d_green;
      b += d_blue;
    }

    uchar *src = img.scanLine(0);
    for(int y = 1; y < h; y++)
      memcpy(img.scanLine(y), src, 4*w);

    pm.convertFromImage(img);
  } else
    pm.gradientFill(ca, cb, vertShaded);
}

KonqFrame::KonqFrame( QWidget *_parent = 0L, const char *_name = 0L )
                    : OPFrame( _parent, _name)
{
  m_pHeader = new KonqFrameHeader( this, "KonquerorFrameHeader");
  m_pHeader->setFixedHeight( DEFAULT_HEADER_HEIGHT );
  QObject::connect(m_pHeader, SIGNAL(headerClicked()), this, SLOT(slotHeaderClicked()));
}

void
KonqFrame::slotHeaderClicked()
{
  if ( !CORBA::is_nil( m_rPart ) )
    m_rPart->parent()->mainWindow()->setActivePart( part()->id() );
}

void 
KonqFrame::paintEvent( QPaintEvent* event )
{
  OPFrame::paintEvent( event );
  m_pHeader->repaint();
}

void 
KonqFrame::resizeEvent( QResizeEvent* )
{
  m_pHeader->setGeometry( 0, 0, width(), DEFAULT_HEADER_HEIGHT);

  Window win = (Window)part()->window();
  if ( win != 0)
    XMoveResizeWindow(qt_xdisplay(), win, 0, m_pHeader->height() , width(), height() - m_pHeader->height() );
}

#include "konq_frame.moc"
