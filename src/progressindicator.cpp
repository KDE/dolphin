/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "progressindicator.h"
#include "dolphinmainwindow.h"
#include "dolphinstatusbar.h"

ProgressIndicator::ProgressIndicator(DolphinMainWindow* mainWindow,
                                     const QString& progressText,
                                     const QString& finishedText,
                                     int operationsCount)
 :  m_mainWindow(mainWindow),
    m_showProgress(false),
    m_operationsCount(operationsCount),
    m_operationsIndex(0),
    m_startTime(QTime::currentTime()),
    m_finishedText(finishedText)
{
    DolphinStatusBar* statusBar = mainWindow->activeView()->statusBar();
    statusBar->clear();
    statusBar->setProgressText(progressText);
    statusBar->setProgress(0);
}


ProgressIndicator::~ProgressIndicator()
{
    DolphinStatusBar* statusBar = m_mainWindow->activeView()->statusBar();
    statusBar->setProgressText(QString::null);
    statusBar->setProgress(100);
    statusBar->setMessage(m_finishedText, DolphinStatusBar::OperationCompleted);

    if (m_showProgress) {
        m_mainWindow->setEnabled(true);
    }
}

void ProgressIndicator::execOperation()
{
    ++m_operationsIndex;

    if (!m_showProgress) {
        const int elapsed = m_startTime.msecsTo(QTime::currentTime());
        if (elapsed > 500) {
            // the operations took already more than 500 milliseconds,
            // therefore show a progress indication
            m_mainWindow->setEnabled(false);
            m_showProgress = true;
        }
    }

    if (m_showProgress) {
        const QTime currentTime = QTime::currentTime();
        if (m_startTime.msecsTo(currentTime) > 100) {
            m_startTime = currentTime;

            DolphinStatusBar* statusBar = m_mainWindow->activeView()->statusBar();
            statusBar->setProgress((m_operationsIndex * 100) / m_operationsCount);
#warning "EVIL, DANGER, FIRE"
            kapp->processEvents();
            statusBar->repaint();
        }
    }
}


