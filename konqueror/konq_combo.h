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

#ifndef KONQ_COMBO_H
#define KONQ_COMBO_H

#include <qevent.h>

#include <kcombobox.h>

class KCompletion;
class KConfig;

// we use KHistoryCombo _only_ for the up/down keyboard handling, otherwise
// KComboBox would do fine.
class KonqCombo : public KHistoryCombo
{
    Q_OBJECT

public:
    KonqCombo( QWidget *parent, const char *name );
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

protected:
    virtual void keyPressEvent( QKeyEvent * );
    virtual bool eventFilter( QObject *, QEvent * );
    virtual void mousePressEvent( QMouseEvent * );
    virtual void mouseMoveEvent( QMouseEvent * );
    void selectWord(QKeyEvent *e);

private slots:
    void slotReturnPressed();
    void slotCleared();

private:
    void updateItem( const QPixmap& pix, const QString&, int index );
    void saveState();
    void restoreState();
    void applyPermanent();
    QString temporaryItem() const { return text( temporary ); }

private:
    bool m_returnPressed;
    bool m_permanent;
    int m_cursorPos;
    int m_currentIndex;
    QString m_currentText;
    QPoint m_dragStart;

    static KConfig *s_config;
    static const int temporary = 0; // the index of our temporary item

};


#endif // KONQ_COMBO_H
