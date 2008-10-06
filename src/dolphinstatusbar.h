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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef DOLPHINSTATUSBAR_H
#define DOLPHINSTATUSBAR_H

#include <khbox.h>

class DolphinView;
class KUrl;
class StatusBarMessageLabel;
class StatusBarSpaceInfo;
class QLabel;
class QProgressBar;
class QSlider;
class QTimer;

/**
 * @brief Represents the statusbar of a Dolphin view.
 *
 * The statusbar allows to show messages and progress
 * information.
 */
class DolphinStatusBar : public KHBox
{
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

    DolphinStatusBar(QWidget* parent, DolphinView* view);

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
    int progress() const
    {
        return m_progress;
    }

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
    const QString& defaultText() const;
        
    /**
     * Refreshes the status bar to get synchronized with the (updated) Dolphin settings.
     */
    void refresh();

protected:
    /** @see QWidget::resizeEvent() */
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void updateProgressInfo();

    /**
     * Is invoked, when the URL of the DolphinView, where the
     * statusbar belongs too, has been changed. The space information
     * content is updated.
     */
    void updateSpaceInfoContent(const KUrl& url);
    
    /**
     * Sets the zoom level of the item view to \a zoomLevel.
     */
    void setZoomLevel(int zoomLevel);

    /**
     * Assures that the text of the statusbar stays visible by hiding
     * the space information widget or the zoom slider widget if not
     * enough width is available.
     */
    void assureVisibleText();
    
private:
    /**
     * Makes the space information widget and zoom slider widget
     * visible, if \a visible is true and the settings allow to show
     * the widgets. If \a visible is false, it is assured that both
     * widgets are hidden.
     */
    void setExtensionsVisible(bool visible);

private:
    DolphinView* m_view;
    StatusBarMessageLabel* m_messageLabel;
    StatusBarSpaceInfo* m_spaceInfo;
    QSlider* m_zoomSlider;

    QLabel* m_progressText;
    QProgressBar* m_progressBar;
    int m_progress;
};

#endif
