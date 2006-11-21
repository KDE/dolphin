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

#ifndef DOLPHINSTATUSBAR_H
#define DOLPHINSTATUSBAR_H

#include <q3hbox.h>
//Added by qt3to4:
#include <QLabel>
class QProgressBar;
class QLabel;
class QTimer;
class StatusBarMessageLabel;
class StatusBarSpaceInfo;
class DolphinView;
class KUrl;

/**
 * @brief Represents the statusbar of a Dolphin view.
 *
 * The statusbar allows to show messages and progress
 * information.
 *
 * @author Peter Penz
 */
class DolphinStatusBar : public Q3HBox {
    Q_OBJECT

public:
    /**
     * Describes the type of the message text. Dependent
     * from the type a corresponding icon and color is
     * used for the message text.
     */
    enum Type {
        Default,
        OperationCompleted,
        Information,
        Error
    };

    DolphinStatusBar(DolphinView* parent = 0);
    virtual ~DolphinStatusBar();

    /**
     * Sets the message text to \a msg. Dependant
     * from the given type \a type an icon is shown and
     * the color of the text is adjusted. The height of
     * the statusbar is automatically adjusted in a way,
     * that the full text fits into the available width.
     *
     * If a progress is ongoing and a message
     * with the type Type::Error is set, the progress
     * is cleared automatically.
     */
    void setMessage(const QString& msg, Type type);

    QString message() const;
    Type type() const;

    /**
     * Sets the text for the progress information.
     * The text is shown with a delay of 300 milliseconds:
     * if the progress set by DolphinStatusBar::setProgress()
     * does reach 100 % within 300 milliseconds,
     * the progress text is not shown at all. This assures that
     * no flickering occurs for showing a progress of fast
     * operations.
     */
    void setProgressText(const QString& text);
    QString progressText() const;

    /**
     * Sets the progress in percent (0 - 100). The
     * progress is shown with a delay of 300 milliseconds:
     * if the progress does reach 100 % within 300 milliseconds,
     * the progress is not shown at all. This assures that
     * no flickering occurs for showing a progress of fast
     * operations.
     */
    void setProgress(int percent);
    int progress() const { return m_progress; }

    /**
     * Clears the message text of the status bar by replacing
     * the message with the default text, which can be set
     * by DolphinStatusBar::setDefaultText(). The progress
     * information is not cleared.
     */
    void clear();

    /**
     * Sets the default text, which is shown if the status bar
     * is cleared by DolphinStatusBar::clear().
     */
    void setDefaultText(const QString& text);
    const QString& defaultText() const { return m_defaultText; }

private slots:
    void slotProgressTimer();

    /**
     * Is invoked, when the URL of the DolphinView, where the
     * statusbar belongs too, has been changed.
     */
    void slotURLChanged(const KUrl& url);

private:
    StatusBarMessageLabel* m_messageLabel;
    StatusBarSpaceInfo* m_spaceInfo;
    QLabel* m_progressText;
    QProgressBar* m_progressBar;
    QTimer* m_progressTimer;
    int m_progress;
    QString m_defaultText;
};

#endif
