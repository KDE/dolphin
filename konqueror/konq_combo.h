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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef KONQ_COMBO_H
#define KONQ_COMBO_H

#include <QEvent>
//Added by qt3to4:
#include <QPixmap>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QPaintEvent>

#include <kcombobox.h>
#include <konq_historymgr.h>

class KCompletion;
class KConfig;

// we use KHistoryCombo _only_ for the up/down keyboard handling, otherwise
// KComboBox would do fine.
class KonqCombo : public KHistoryCombo
{
    Q_OBJECT

public:
    KonqCombo( QWidget *parent );
    ~KonqCombo();

    // initializes with the completion object and calls loadItems()
    void init( KCompletion * );

    // determines internally if it's temporary or final
    void setURL( const QString& url );

    void setTemporary( const QString& );
    void setTemporary( const QString&, const QPixmap& );
    void clearTemporary( bool makeCurrent = true );
    void removeURL( const QString& url );

    void insertPermanent( const QString& );

    void updatePixmaps();

    void loadItems();
    void saveItems();

    static void setConfig( KConfig * );

    virtual void popup();

    void setPageSecurity( int );

    void insertItem( const QString &text, int index=-1, const QString& title=QString() );
    void insertItem( const QPixmap &pixmap, const QString &text, int index=-1, const QString& title=QString() );

protected:
    virtual void keyPressEvent( QKeyEvent * );
    virtual bool eventFilter( QObject *, QEvent * );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseMoveEvent( QMouseEvent * );
    void paintEvent( QPaintEvent * );
    void selectWord(QKeyEvent *e);

Q_SIGNALS:
    /** 
      Specialized signal that emits the state of the modifier
      keys along with the actual activated text.
     */    
    void activated( const QString &, int );

    /**
      User has clicked on the security lock in the combobar
     */
    void showPageSecurity();

private Q_SLOTS:
    void slotCleared();
    void slotSetIcon( int index );
    void slotActivated( const QString& text );

private:
    void updateItem( const QPixmap& pix, const QString&, int index, const QString& title );
    void saveState();
    void restoreState();
    void applyPermanent();
    QString temporaryItem() const { return itemText( temporary ); }
    void removeDuplicates( int index );
    bool hasSufficientContrast(const QColor &c1, const QColor &c2);

    bool m_returnPressed;
    bool m_permanent;
    int m_cursorPos;
    int m_currentIndex;
    int m_modifier;
    QString m_currentText;
    QPoint m_dragStart;
    int m_pageSecurity;

    void getStyleOption(QStyleOptionComboBox* combo);

    static KConfig *s_config;
    static const int temporary; // the index of our temporary item
};

#endif // KONQ_COMBO_H
