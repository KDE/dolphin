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
#include <kdebug.h>
#include <kicontheme.h>
#include <konq_pixmapprovider.h>
#include <kstdaccel.h>

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
    setMaxCount( s_config->readNumEntry("Maximum of URLs in combo", 10 ));

    loadItems();

    connect( this, SIGNAL( returnPressed()), SLOT( slotReturnPressed() ));

    if ( !kapp->dcopClient()->isAttached() )
	kapp->dcopClient()->attach();
}

KonqCombo::~KonqCombo()
{
}

void KonqCombo::setURL( const QString& url )
{
    // kdDebug(1002) << "*** setURL %s ***" << url << endl;
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
    // kdDebug(1002 << "*** setTemporary (%s) ***", url << endl;

    if ( url != temporaryItem() )
	applyPermanent();
	
    updateItem( pix, url, temporary );
    setCurrentItem( temporary );
}

// called via DCOP in all instances
void KonqCombo::insertPermanent( const QString& url )
{
    // kdDebug << "### insertPermanent!" << endl;

    saveState();
    setTemporary( url );
    m_permanent = true;
    restoreState();
}

// called right before a new (different!) temporary item will be set. So we
// insert an item that was marked permanent properly at position 1.
void KonqCombo::applyPermanent()
{
    if ( m_permanent ) {
    qDebug("** applyPermanent!");
	// remove as many items as needed to honour maxCount()
	int i = count();
	while ( count() >= maxCount() )
	    removeItem( --i );

	QString url = temporaryItem();
	insertItem( KonqPixmapProvider::self()->pixmapFor( url ), url, 1 );
	// setCurrentItem( 1 );

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
    repaint();
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
    changeItem( QString::null, temporary ); // ### default pixmap?
    if ( makeCurrent )
	setCurrentItem( temporary );
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

void KonqCombo::setConfig( KConfig *kc )
{
    s_config = kc;
}


#include "konq_combo.moc"
