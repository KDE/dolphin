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

#include <kapp.h>
#include <kconfig.h>
#include <kcompletionbox.h>
#include <kdebug.h>
#include <kicontheme.h>
#include <konq_pixmapprovider.h>
#include <kstdaccel.h>
#include <kurldrag.h>

#include <dcopclient.h>

#include "konq_combo.h"

KConfig * KonqCombo::s_config = 0L;

KonqCombo::KonqCombo( QWidget *parent, const char *name )
    : KHistoryCombo( parent, name ),
      m_returnPressed( false ),
      m_permanent( false )
{
    setInsertionPolicy( NoInsertion );
    setSizePolicy( QSizePolicy( QSizePolicy::Expanding, QSizePolicy::Fixed ));
    setDuplicatesEnabled( false );

    ASSERT( s_config );

    KConfigGroupSaver cs( s_config, "Location Bar" );
    setMaxCount( s_config->readNumEntry("Maximum of URLs in combo", 20 ));

    // we should also connect the completionBox' highlighted signal to
    // our setEditText() slot, because we're handling the signals ourselves.
    // But we're lazy and let KCompletionBox do this and simply switch off
    // handling of signals later.
    setHandleSignals( true );
    completionBox()->setTabHandling( true );

    connect( this, SIGNAL( returnPressed()), SLOT( slotReturnPressed() ));
    connect( completionBox(), SIGNAL( activated(const QString&)),
             this, SLOT( slotReturnPressed() ));
    connect( this, SIGNAL( cleared() ), SLOT( slotCleared() ));

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
    setTemporary( url );

    if ( m_returnPressed ) { // really insert
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
    setTemporary( text, KonqPixmapProvider::self()->pixmapFor( text ) );
}

void KonqCombo::setTemporary( const QString& url, const QPixmap& pix )
{
    // kdDebug(1002) << "*** setTemporary: " << url << endl;

    if ( count() == 0 ) // insert a temporary item when we don't have one yet
	insertItem( pix, url, temporary );
	
    else {
	if ( url != temporaryItem() )
	    applyPermanent();
	
	updateItem( pix, url, temporary );
    }
	
    setCurrentItem( temporary );
}

// called via DCOP in all instances
void KonqCombo::insertPermanent( const QString& url )
{
    // kdDebug(1002) << "### insertPermanent!" << endl;

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

	// remove as many items as needed to honour maxCount()
	int i = count();
	while ( count() >= maxCount() )
	    removeItem( --i );

	QString url = temporaryItem();
	insertItem( KonqPixmapProvider::self()->pixmapFor( url ), url, 1 );

	// remove all dupes, if available
	for ( i = 2; i < count(); i++ ) {
	    if ( text( i ) == url )
		removeItem( i );
	}

	m_permanent = false;
    }
}

// ### use QComboBox::changeItem(), once that finally works
void KonqCombo::updateItem(const QPixmap& pix, const QString& t, int index)
{
    // QComboBox::changeItem() doesn't honour the pixmap when
    // using an editable combobox, so we just remove and insert

    // no need to flicker
    if (text( index ) == t &&
	(pixmap(index) && pixmap(index)->serialNumber() == pix.serialNumber()))
	return;

    setUpdatesEnabled( false );
    lineEdit()->setUpdatesEnabled( false );

    removeItem( index );
    insertItem( pix, t, index );

    setUpdatesEnabled( true );
    lineEdit()->setUpdatesEnabled( true );
    update();
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

    KonqPixmapProvider *prov = KonqPixmapProvider::self();
    for ( int i = 1; i < count(); i++ ) {
	updateItem( prov->pixmapFor( text( i ) ), text( i ), i );
    }

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
    KonqPixmapProvider *prov = KonqPixmapProvider::self();
    while ( it != items.end() ) {
        item = *it;
        if ( !item.isEmpty() ) { // only insert non-empty items
	    insertItem( prov->pixmapFor(item, KIcon::SizeSmall), item, i++ );
        }
        ++it;
    }

    if ( count() > 0 )
	m_permanent = true; // we want the first loaded item to stay
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

// we can't directly insert the url here, because it might be an
// incomplete url and we wouldn't get a nice pixmap for that, either.
void KonqCombo::slotReturnPressed()
{
    applyPermanent();
    m_returnPressed = true;
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
            if ( KStdAccel::isEqual( e, KStdAccel::deleteWordBack() ) ||
                 KStdAccel::isEqual( e, KStdAccel::deleteWordForward() ) ||
                 ((e->state() & ControlButton) && 
                   (e->key() == Key_Left || e->key() == Key_Right) ) ) {
                selectWord(e);
                e->accept();
                return true;
            }
        }
    }
    return KComboBox::eventFilter( o, ev );
}

void KonqCombo::keyPressEvent( QKeyEvent *e )
{
    KHistoryCombo::keyPressEvent( e );
    // we have to set it as temporary, otherwise we wouldn't get our nice
    // pixmap. Yes, QComboBox still sucks.
    if ( KStdAccel::isEqual( e, KStdAccel::rotateUp() ) ||
         KStdAccel::isEqual( e, KStdAccel::rotateDown() ) )
	setTemporary( currentText() );
}

void KonqCombo::selectWord(QKeyEvent *e)
{
    // Handle Ctrl+Cursor etc better than the Qt widget, which always
    // jumps to the next whitespace. This code additionally jumps to 
    // the next [/#?:], which makes more sense for URLs.
    QLineEdit *edit = lineEdit();
    QString text = edit->text();
    // The list of chars that will stop the cursor
    // (TODO: make it a parameter when in kdelibs/kdeui)
    typedef QValueList<QChar> JumpChars;
    JumpChars chars;
    chars.append(QChar('/'));   // path seperator
    chars.append(QChar('.'));   // domain part seperator
    chars.append(QChar('?'));   // parameter sperator
    chars.append(QChar('#'));   // local anchor seperator
    chars.append(QChar(':'));   // Konqueror's enhanced browsing seperator
    // TODO: make this a parameter when in kdelibs/kdeui
    bool allow_space_break = true;
    int pos = edit->cursorPosition();
    int pos_old = pos;
    int count = 0;
    if( e->key() == Key_Left || e->key() == Key_Backspace ) {
	do {
	    pos--;
	    count++;
            if( allow_space_break && text[pos].isSpace() && count > 1 )
                break;
	} while( pos >= 0 && (chars.findIndex(text[pos]) == -1 || count <= 1) );
	if( e->state() & ShiftButton ) {
            edit->cursorLeft(true, count-1);
	} else if(  e->key() == Key_Backspace ) {
            edit->cursorLeft(false, count-1);
            QString text = edit->text();
            int pos_to_right = edit->text().length() - pos_old;
            QString cut = text.left(edit->cursorPosition()) + text.right(pos_to_right);
            edit->setText(cut);
            edit->setCursorPosition(pos_old-count+1);
        } else {
            edit->cursorLeft(false, count-1);
        }
     } else if( e->key() == Key_Right || e->key() == Key_Delete ){
	do {
	    pos++;
	    count++;
            if( allow_space_break && text[pos].isSpace() )
                break;
	} while( pos < text.length() && chars.findIndex(text[pos]) == -1 );
	if( e->state() & ShiftButton ) {
            edit->cursorRight(true, count+1);
	} else if(  e->key() == Key_Delete ) {
            edit->cursorLeft(false, count+1);
            QString text = edit->text();
            int pos_to_right = text.length() - pos - 1;
	    QString cut = text.left(pos_old) + 
               (pos_to_right > 0 ? text.right(pos_to_right) : "" );
            edit->setText(cut);
            edit->setCursorPosition(pos_old);
        } else {
            edit->cursorRight(false, count+1);
        }
    }
}

void KonqCombo::slotCleared()
{
    QByteArray data;
    QDataStream s( data, IO_WriteOnly );
    s << kapp->dcopClient()->defaultObject();
    kapp->dcopClient()->send( "konqueror*", "KonquerorIface",
			      "comboCleared(QCString)", data);
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
        if ( x > 2 && x < lineEdit()->x() ) {
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
        KURL url = currentText();
        if ( !url.isMalformed() )
        {
            KURL::List list;
            list.append( url );
            QUriDrag *drag = KURLDrag::newDrag( list, this );
            QPixmap pix = KonqPixmapProvider::self()->pixmapFor( currentText(),
                                                                 KIcon::SizeMedium );
            if ( !pix.isNull() )
                drag->setPixmap( pix );
            drag->dragCopy();
        }
    }
}

void KonqCombo::setConfig( KConfig *kc )
{
    s_config = kc;
}


#include "konq_combo.moc"
