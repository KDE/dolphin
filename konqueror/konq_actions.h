/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_actions_h__
#define __konq_actions_h__

#include <konq_historymgr.h>
#include <kactionmenu.h>
#include <QWidget>
#include <QToolBar>
#include <QList>

class HistoryEntry;
class QMenu;

/**
 * Plug this action into a menu to get a bidirectional history
 * (both back and forward, including current location)
 */
class KonqBidiHistoryAction : public KAction
{
  Q_OBJECT
public:
    KonqBidiHistoryAction( const QString & text, KActionCollection* parent = 0, const char* name = 0 );
    virtual ~KonqBidiHistoryAction();

    void fillGoMenu( const QList<HistoryEntry*> &history, int historyIndex );

    // Used by KonqHistoryAction and KonqBidiHistoryAction
    static void fillHistoryPopup( const QList<HistoryEntry*> &history, int historyIndex,
                           QMenu * popup,
                           bool onlyBack = false,
                           bool onlyForward = false,
                           bool checkCurrentItem = false,
                           int startPos = 0 );

    virtual QWidget* createToolBarWidget(QToolBar* parent);

protected Q_SLOTS:
    void slotTriggered( QAction* action );

Q_SIGNALS:
    void menuAboutToShow();
    // -1 for one step back, 0 for don't move, +1 for one step forward, etc.
    void step( int );
private:
    int m_firstIndex; // first index in the Go menu
    int m_startPos;
    int m_currentPos; // == history.at()
};

/////

class KonqLogoAction : public KAction
{
  Q_OBJECT
public:
    KonqLogoAction( const QString& text, const KShortcut& accel = KShortcut(), KActionCollection* parent = 0, const char* name = 0 );
    KonqLogoAction( const QString& text, const KShortcut& accel,
	            QObject* receiver, const char* slot, KActionCollection* parent, const char* name = 0 );
    KonqLogoAction( const QString& text, const KIcon& pix, const KShortcut& accel = KShortcut(),
	            KActionCollection* parent = 0, const char* name = 0 );
    KonqLogoAction( const QString& text, const KIcon& pix, const KShortcut& accel,
	            QObject* receiver, const char* slot, KActionCollection* parent, const char* name = 0 );
    // text missing !
    KonqLogoAction( const QStringList& icons, QObject* receiver,
                    const char* slot, KActionCollection* parent, const char* name = 0 );

    void start();
    void stop();

    virtual QWidget* createToolBarWidget(QToolBar* parent);

Q_SIGNALS:
    void startAnimation();
    void stopAnimation();

private:
    QStringList iconList;
};

class KonqViewModeAction : public KAction
{
    Q_OBJECT
public:
    KonqViewModeAction( const QString& desktopEntryName,
                        const QString &text, const KIcon &icon,
                        KActionCollection *parent, const char *name );
    virtual ~KonqViewModeAction();

    QString desktopEntryName() const { return m_desktopEntryName; }

    virtual QWidget* createToolBarWidget(QToolBar* parent);

/*private Q_SLOTS:
    void slotPopupAboutToShow();
    void slotPopupActivated();
    void slotPopupAboutToHide();*/

private:
    QString m_desktopEntryName;
    bool m_popupActivated;
};

class KonqMostOftenURLSAction : public KActionMenu
{
    Q_OBJECT

public:
    KonqMostOftenURLSAction( const QString& text, KActionCollection *parent,
			     const char *name );
    virtual ~KonqMostOftenURLSAction();

    static bool numberOfVisitOrder( const KonqHistoryEntry& lhs, const KonqHistoryEntry& rhs ) {
        return lhs.numberOfTimesVisited < rhs.numberOfTimesVisited;
    }

Q_SIGNALS:
    void activated( const KUrl& );

private Q_SLOTS:
    void slotHistoryCleared();
    void slotEntryAdded( const KonqHistoryEntry& entry );
    void slotEntryRemoved( const KonqHistoryEntry& entry );

    void slotFillMenu();
    //void slotClearMenu();

    void slotActivated( int );

private:
    void init();
    void parseHistory();
    static void inSort( const KonqHistoryEntry& entry );

    static KonqHistoryList *s_mostEntries;
    static uint s_maxEntries;
    KUrl::List m_popupList;
};

#endif
