/***************************************************************************
 *   Copyright (C) 2008 by Fredrik Höglund <fredrik@kde.org>               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include "ktooltip.h"
#include "ktooltipwindow_p.h"
#include <QLabel>
#include <QPoint>
#include <QWidget>

class KToolTipManager
{
public:
    ~KToolTipManager();

    static KToolTipManager* instance();

    void showTip(const QPoint& pos, QWidget* content);
    void hideTip();

private:
    KToolTipManager();

    KToolTipWindow* m_window;
    static KToolTipManager *s_instance;
};

KToolTipManager *KToolTipManager::s_instance = 0;

KToolTipManager::KToolTipManager() :
    m_window(0)
{
}

KToolTipManager::~KToolTipManager()
{
    delete m_window;
    m_window = 0;
}

KToolTipManager* KToolTipManager::instance()
{
    if (s_instance == 0) {
        s_instance = new KToolTipManager();
    }

    return s_instance;
}

void KToolTipManager::showTip(const QPoint& pos, QWidget* content)
{
    hideTip();
    Q_ASSERT(m_window == 0);
    m_window = new KToolTipWindow(content);
    m_window->move(pos);
    m_window->show();
}

void KToolTipManager::hideTip()
{
    if (m_window != 0) {
        m_window->hide();
        delete m_window;
        m_window = 0;
    }
}

namespace KToolTip
{
    void showText(const QPoint& pos, const QString& text)
    {
        QLabel* label = new QLabel(text);
        label->setForegroundRole(QPalette::ToolTipText);
        showTip(pos, label);
    }

    void showTip(const QPoint& pos, QWidget* content)
    {
        KToolTipManager::instance()->showTip(pos, content);
    }

    void hideTip()
    {
        KToolTipManager::instance()->hideTip();
    }
}

