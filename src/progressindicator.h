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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#ifndef PROGRESSINDICATOR_H
#define PROGRESSINDICATOR_H

#include <qdatetime.h>

class DolphinMainWindow;

/**
 * Allows to show a progress of synchronous operations. Sample code:
 * \code
 * const int operationsCount = 100;
 * ProgressIndicator progressIndicator(i18n("Loading..."),
 *                                     i18n("Loading finished."),
 *                                     operationsCount);
 * for (int i = 0; i < operationsCount; ++i) {
 *     progressIndicator.execOperation();
 *     // do synchronous operation...
 * }
 * \endcode
 * The progress indicator takes care to show the progress bar only after
 * a delay of around 500 milliseconds. This means if all operations are
 * executing within 500 milliseconds, no progress bar is shown at all.
 * As soon as the progress bar is shown, the application still may process
 * events, but the the Dolphin main widget is disabled.
 *
 *	@author Peter Penz <peter.penz@gmx.at>
 */
class ProgressIndicator
{
public:
    /**
     * @param mainWindow        The mainwindow this statusbar should operate on
     * @param progressText      Text for the progress bar (e. g. "Loading...").
     * @param finishedText      Text which is displayed after the operations have been finished
     *                          (e. g. "Loading finished.").
     * @param operationsCount   Number of operations.
     */
    ProgressIndicator(DolphinMainWindow *mainWindow,
                      const QString& progressText,
                      const QString& finishedText,
                      int operationsCount);

    /**
     * Sets the progress to 100 % and displays the 'finishedText' property
     *  in the status bar.
     */
    ~ProgressIndicator();

    /**
     * Increases the progress and should be invoked
     * before each operation.
     */
    void execOperation();

private:
    DolphinMainWindow *m_mainWindow;
    bool m_showProgress;
    int m_operationsCount;
    int m_operationsIndex;
    QTime m_startTime;
    QString m_finishedText;
};

#endif
