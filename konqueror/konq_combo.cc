/* This file is part of the KDE project
   Copyright (C) 2001 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <qpainter.h>
#include <qstyle.h>
#include <kapplication.h>
#include <kconfig.h>
#include <kcompletionbox.h>
#include <kdebug.h>
#include <kiconloader.h>
#include <kicontheme.h>
#include <konq_pixmapprovider.h>
#include <kstdaccel.h>
#include <kurldrag.h>
#include <konq_mainwindow.h>

#include <dcopclient.h>

#include "konq_combo.h"

KConfig * KonqCombo::s_config = 0L;
const int KonqCombo::temporary = 0;

KonqCombo::KonqCombo( QWidget *parent, const char *name )
          : KHistoryCombo( parent, name ),
            m_returnPressed( false ), 
            m_permanent( false ),
            m_modifier( NoButton ),
	    m_pageSecurity( KonqMainWindow::NotCrypted )
{
    setInsertionPolicy( NoInsertion );
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));

    Q_ASSERT( s_config );

    KConfigGroupSaver cs( s_config, "Location Bar" );
    setMaxCount( s_config->readNumEntry("Maximum of URLs in combo", 20 ));

    // We should also connect the completionBox' highlighted signal to
    // our setEditText() slot, because we're handling the signals ourselves.
    // But we're lazy and let KCompletionBox do this and simply switch off
    // handling of signals later.
    setHandleSignals( true );
    completionBox()->setTabHandling( true );
    
    // Make the lineedit consume the Key_Enter event...
    setTrapReturnKey( true );
    
    connect( this, SIGNAL(cleared() ), SLOT(slotCleared()) );
    connect( this, SIGNAL(highlighted( int )), SLOT(slotSetIcon( int )) );
    connect( this, SIGNAL(activated( const QString& )),
             SLOT(slotActivated( const QString& )) );

    if ( !kapp->dcopClient()->isAttached() )
        kapp->dcopClient()->attach();
}

KonqCombo::~KonqCombo()
{
}

void KonqCombo::init( KCompletion *completion )
{
    setCompletionObject( completion, false ); //KonqMainWindow handles signals
    setAutoDeleteCompletionObject( false );
    setCompletionMode( completion->completionMode() );

    loadItems();
}

void KonqCombo::setURL( const QString& url )
{
    //kdDebug(1202) << "KonqCombo::setURL: " << url << ", returnPressed ? " << m_returnPressed << endl;
    setTemporary( url );
    
    if ( m_returnPressed ) { // Really insert...
        m_returnPressed = false;      
        QByteArray data;
        QDataStream s( data, IO_WriteOnly );
        s << url << kapp->dcopClient()->defaultObject();
        kapp->dcopClient()->send( "konqueror*", "KonquerorIface",
                                  "addToCombo(QString,QCString)", data);
    }
}

void KonqCombo::setTemporary( const QString& text )
{
    setTemporary( text, KonqPixmapProvider::self()->pixmapFor(text) );
}

void KonqCombo::setTemporary( const QString& url, const QPixmap& pix )
{
    //kdDebug(1202) << "KonqCombo::setTemporary: " << url << ", temporary = " << temporary << endl;
    
    // Insert a temporary item when we don't have one yet
    if ( count() == 0 )
      insertItem( pix, url, temporary );
    else
    {
        if (url != temporaryItem())
          applyPermanent();
        
        updateItem( pix, url, temporary );
    }
        
    setCurrentItem( temporary );
}

void KonqCombo::removeDuplicates( int index )
{
    //kdDebug(1202) << "KonqCombo::removeDuplicates: Starting index =  " << index << endl;
    
    QString url (temporaryItem());
    if (url.endsWith("/"))
      url.truncate(url.length()-1);
    
    // Remove all dupes, if available...
    for ( int i = index; i < count(); i++ ) 
    {        
        QString item (text(i));
        if (item.endsWith("/"))
          item.truncate(item.length()-1);
                  
        if ( item == url )
            removeItem( i );
    }
}

// called via DCOP in all instances
void KonqCombo::insertPermanent( const QString& url )
{
    //kdDebug(1202) << "KonqCombo::insertPermanent: URL = " << url << endl;
    saveState();
    setTemporary( url );
    m_permanent = true;
    restoreState();
}

// called right before a new (different!) temporary item will be set. So we
// insert an item that was marked permanent properly at position 1.
void KonqCombo::applyPermanent()
{    
    if ( m_permanent && !temporaryItem().isEmpty() ) {
        
        // Remove as many items as needed to honour maxCount()
        int index = count();
        while ( count() >= maxCount() )
            removeItem( --index );
        
        QString url (temporaryItem());
        insertItem( KonqPixmapProvider::self()->pixmapFor( url ), url, 1 );
        //kdDebug(1202) << "KonqCombo::applyPermanent: " << url << endl;
        
        // Remove all duplicates starting from index = 2
        removeDuplicates( 2 );       
        m_permanent = false;
    }
}


void KonqCombo::updateItem(const QPixmap& pix, const QString& t, int index)
{
    // No need to flicker
    if (text( index ) == t &&
        (pixmap(index) && pixmap(index)->serialNumber() == pix.serialNumber()))
        return;
        
    // kdDebug(1202) << "KonqCombo::updateItem: item='" << t << "', index='"
    //               << index << "'" << endl;
    
    // QComboBox::changeItem() doesn't honour the pixmap when
    // using an editable combobox, so we just remove and insert    
    // ### use QComboBox::changeItem(), once that finally works
    // Well lets try it now as it seems to work fine for me. We
    // can always revert :)
    changeItem (pix, t, index);
    
    /*        
    setUpdatesEnabled( false );
    lineEdit()->setUpdatesEnabled( false );

    removeItem( index );
    insertItem( pix, t, index );

    setUpdatesEnabled( true );
    lineEdit()->setUpdatesEnabled( true );
    update();
    */
}

void KonqCombo::saveState()
{
    m_cursorPos = cursorPosition();
    m_currentText = currentText();
    m_currentIndex = currentItem();
}

void KonqCombo::restoreState()
{
    setTemporary( m_currentText );
    lineEdit()->setCursorPosition( m_cursorPos );
}

void KonqCombo::updatePixmaps()
{
    saveState();

    setUpdatesEnabled( false );
    KonqPixmapProvider *prov = KonqPixmapProvider::self();
    for ( int i = 1; i < count(); i++ ) {
        updateItem( prov->pixmapFor( text( i ) ), text( i ), i );
    }
    setUpdatesEnabled( true );
    repaint();

    restoreState();
}

void KonqCombo::loadItems()
{
    clear();
    int i = 0;

    s_config->setGroup( "History" ); // delete the old 2.0.x completion
    s_config->writeEntry( "CompletionItems", "unused" );

    s_config->setGroup( "Location Bar" );
    QStringList items = s_config->readListEntry( "ComboContents" );
    QStringList::ConstIterator it = items.begin();
    QString item;
    bool first = true;
    while ( it != items.end() ) {
        item = *it;
        if ( !item.isEmpty() ) { // only insert non-empty items
            if( first )
                insertItem( KonqPixmapProvider::self()->pixmapFor(item,
                    KIcon::SizeSmall), item, i++ );
            else
                // icons will be loaded on-demand
                insertItem( item, i++ );
            first = false;
        }
        ++it;
    }

    if ( count() > 0 )
        m_permanent = true; // we want the first loaded item to stay
}

void KonqCombo::slotSetIcon( int index )
{
    if( pixmap( index ) == NULL )
        // on-demand icon loading
        updateItem( KonqPixmapProvider::self()->pixmapFor( text( index ),
            KIcon::SizeSmall), text( index ), index );
    update();
}

void KonqCombo::popup()
{
    for( int i = 0; i < count(); ++i )
    {
        if( pixmap( i ) == NULL )
        {
            // on-demand icon loading
            updateItem( KonqPixmapProvider::self()->pixmapFor( text( i ),
                KIcon::SizeSmall), text( i ), i );
        }
    }
    KHistoryCombo::popup();
}

void KonqCombo::saveItems()
{
    QStringList items;
    int i = m_permanent ? 0 : 1;

    for ( ; i < count(); i++ )
        items.append( text( i ) );

    s_config->setGroup( "Location Bar" );
    s_config->writeEntry( "ComboContents", items );
    KonqPixmapProvider::self()->save( s_config, "ComboIconCache", items );

    s_config->sync();
}

void KonqCombo::clearTemporary( bool makeCurrent )
{
    applyPermanent();
    changeItem( QString::null, temporary ); // ### default pixmap?
    if ( makeCurrent )
      setCurrentItem( temporary );
}

bool KonqCombo::eventFilter( QObject *o, QEvent *ev )
{
    // Handle Ctrl+Del/Backspace etc better than the Qt widget, which always
    // jumps to the next whitespace.
    QLineEdit *edit = lineEdit();
    if ( o == edit ) {
        int type = ev->type();
        if ( type == QEvent::KeyPress ) {
            QKeyEvent *e = static_cast<QKeyEvent *>( ev );

            if ( e->key() == Key_Return || e->key() == Key_Enter ) {
                m_modifier = e->state();
                return false;
            }

            if ( KKey( e ) == KKey( int( KStdAccel::deleteWordBack() ) ) ||
                 KKey( e ) == KKey( int( KStdAccel::deleteWordForward() ) ) ||
                 ((e->state() & ControlButton) &&
                   (e->key() == Key_Left || e->key() == Key_Right) ) ) {
                selectWord(e);
                e->accept();
                return true;
            }
        }

        else if ( type == QEvent::MouseButtonDblClick ) {
            edit->selectAll();
            return true;
        }
    }
    return KComboBox::eventFilter( o, ev );
}

void KonqCombo::keyPressEvent( QKeyEvent *e )
{
    KHistoryCombo::keyPressEvent( e );
    // we have to set it as temporary, otherwise we wouldn't get our nice
    // pixmap. Yes, QComboBox still sucks.
    if ( KKey( e ) == KKey( int( KStdAccel::rotateUp() ) ) ||
         KKey( e ) == KKey( int( KStdAccel::rotateDown() ) ) )
         setTemporary( currentText() );
}

/* 
   Handle Ctrl+Cursor etc better than the Qt widget, which always
   jumps to the next whitespace. This code additionally jumps to
   the next [/#?:], which makes more sense for URLs. The list of 
   chars that will stop the cursor are '/', '.', '?', '#', ':'.
*/
void KonqCombo::selectWord(QKeyEvent *e)
{
    QLineEdit* edit = lineEdit();    
    QString text = edit->text();
    int pos = edit->cursorPosition();
    int pos_old = pos;
    int count = 0;  
    
    // TODO: make these a parameter when in kdelibs/kdeui...
    QValueList<QChar> chars;
    chars << QChar('/') << QChar('.') << QChar('?') << QChar('#') << QChar(':');    
    bool allow_space_break = true;
    
    if( e->key() == Key_Left || e->key() == Key_Backspace ) {
        do {
            pos--;
            count++;
            if( allow_space_break && text[pos].isSpace() && count > 1 )
                break;
        } while( pos >= 0 && (chars.findIndex(text[pos]) == -1 || count <= 1) );
        
        if( e->state() & ShiftButton ) {
                  edit->cursorForward(true, 1-count);
        } 
        else if(  e->key() == Key_Backspace ) {
            edit->cursorForward(false, 1-count);
            QString text = edit->text();
            int pos_to_right = edit->text().length() - pos_old;
            QString cut = text.left(edit->cursorPosition()) + text.right(pos_to_right);
            edit->setText(cut);
            edit->setCursorPosition(pos_old-count+1);
        } 
        else {
            edit->cursorForward(false, 1-count);
        }
     } 
     else if( e->key() == Key_Right || e->key() == Key_Delete ){
        do {
            pos++;
            count++;
                  if( allow_space_break && text[pos].isSpace() )
                      break;
        } while( pos < (int) text.length() && chars.findIndex(text[pos]) == -1 );
        
        if( e->state() & ShiftButton ) {
            edit->cursorForward(true, count+1);
        } 
        else if(  e->key() == Key_Delete ) {
            edit->cursorForward(false, -count-1);
            QString text = edit->text();
            int pos_to_right = text.length() - pos - 1;
            QString cut = text.left(pos_old) +
               (pos_to_right > 0 ? text.right(pos_to_right) : QString::null );
            edit->setText(cut);
            edit->setCursorPosition(pos_old);
        } 
        else {
            edit->cursorForward(false, count+1);
        }
    }
}

void KonqCombo::slotCleared()
{
    QByteArray data;
    QDataStream s( data, IO_WriteOnly );
    s << kapp->dcopClient()->defaultObject();
    kapp->dcopClient()->send( "konqueror*", "KonquerorIface", "comboCleared(QCString)", data);
}

void KonqCombo::removeURL( const QString& url )
{
    setUpdatesEnabled( false );
    lineEdit()->setUpdatesEnabled( false );

    removeFromHistory( url );
    applyPermanent();
    setTemporary( currentText() );

    setUpdatesEnabled( true );
    lineEdit()->setUpdatesEnabled( true );
    update();
}

void KonqCombo::mousePressEvent( QMouseEvent *e )
{
    m_dragStart = QPoint(); // null QPoint

    if ( e->button() == LeftButton && pixmap( currentItem()) ) {
        // check if the pixmap was clicked
        int x = e->pos().x();
        int x0 = QStyle::visualRect( style().querySubControlMetrics( QStyle::CC_ComboBox, this, QStyle::SC_ComboBoxEditField ), this ).x();
	
        if ( x > x0 + 2 && x < lineEdit()->x() ) {
            m_dragStart = e->pos();
            return; // don't call KComboBox::mousePressEvent!
        }
    }

    KComboBox::mousePressEvent( e );
}

void KonqCombo::mouseMoveEvent( QMouseEvent *e )
{
    KComboBox::mouseMoveEvent( e );
    if ( m_dragStart.isNull() || currentText().isEmpty() )
        return;

    if ( e->state() & LeftButton &&
         (e->pos() - m_dragStart).manhattanLength() >
         KGlobalSettings::dndEventDelay() )
    {
        KURL url ( currentText() );
        if ( url.isValid() )
        {
            KURL::List list;
            list.append( url );
            KURLDrag *drag = new KURLDrag( list, this );
            QPixmap pix = KonqPixmapProvider::self()->pixmapFor( currentText(),
                                                                 KIcon::SizeMedium );
            if ( !pix.isNull() )
                drag->setPixmap( pix );
            drag->dragCopy();
        }
    }
}

void KonqCombo::slotActivated( const QString& text )
{
    //kdDebug(1202) << "KonqCombo::slotActivated: " << text << endl;
    applyPermanent();
    m_returnPressed = true;
    emit activated( text, m_modifier );
    m_modifier = NoButton;
}

void KonqCombo::setConfig( KConfig *kc )
{
    s_config = kc;
}

void KonqCombo::paintEvent( QPaintEvent *pe )
{
    QComboBox::paintEvent( pe );

    QLineEdit *edit = lineEdit();
    QRect re = style().querySubControlMetrics( QStyle::CC_ComboBox, this, QStyle::SC_ComboBoxEditField );
    re = QStyle::visualRect(re, this);
    
    if ( m_pageSecurity!=KonqMainWindow::NotCrypted ) {
        QColor color(245, 246, 190);
        bool useColor = kapp->hasSufficientContrast(color,edit->paletteForegroundColor());
      
        QPainter p( this );
        p.setClipRect( re );

	QPixmap pix = KonqPixmapProvider::self()->pixmapFor( currentText() );
	if ( useColor ) {
            p.fillRect( re.x(), re.y(), pix.width() + 4, re.height(), QBrush( color ));
            p.drawPixmap( re.x() + 2, re.y() + ( re.height() - pix.height() ) / 2, pix );
	}

        QRect r = edit->geometry();
        r.setRight( re.right() - pix.width() - 4 );
        if ( r != edit->geometry() )
            edit->setGeometry( r );

	if ( useColor)
	    edit->setPaletteBackgroundColor( color );

        pix = SmallIcon( m_pageSecurity==KonqMainWindow::Encrypted ? "encrypted" : "halfencrypted" );
        p.fillRect( re.right() - pix.width() - 3 , re.y(), pix.width() + 4, re.height(), 
		    QBrush( useColor ? color : edit->paletteBackgroundColor() ));
        p.drawPixmap( re.right() - pix.width() -1 , re.y() + ( re.height() - pix.height() ) / 2, pix );
        p.setClipping( FALSE );
    }
    else {
        QRect r = edit->geometry();
        r.setRight( re.right() );
        if ( r != edit->geometry() )
            edit->setGeometry( r );
        edit->unsetPalette();
    }
}

void KonqCombo::setPageSecurity( int pageSecurity )
{
    m_pageSecurity = pageSecurity;
    repaint();
}

#include "konq_combo.moc"
