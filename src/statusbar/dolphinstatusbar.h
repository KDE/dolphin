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

#include "konq_statusbarmessagelabel.h"
#include <QTime>
#include <QWidget>

class DolphinView;
class KUrl;
class StatusBarSpaceInfo;
class QLabel;
class QProgressBar;
class QToolButton;
class QSlider;

/**
 * @brief Represents the statusbar of a Dolphin view.
 *
 * The statusbar allows to show messages and progress
 * information.
 */
class DolphinStatusBar : public QWidget
{
    Q_OBJECT

public:
    /**
     * Describes the type of the message text. Dependent
     * from the type a corresponding icon and color is
     * used for the message text.
     */
    enum Type {
        Default = KonqStatusBarMessageLabel::Default,
        OperationCompleted = KonqStatusBarMessageLabel::OperationCompleted,
        Information = KonqStatusBarMessageLabel::Information,
        Error = KonqStatusBarMessageLabel::Error
    };

    explicit DolphinStatusBar(QWidget* parent, DolphinView* view);

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
     * DolphinStatusBar::setProgress() should be invoked
     * afterwards each time the progress changes.
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
    int progress() const;

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
    QString defaultText() const;

    /**
     * Refreshes the status bar to get synchronized with the (updated) Dolphin settings.
     */
    void refresh();

signals:
    /**
     * Is emitted if the stop-button has been pressed during showing a progress.
     */
    void stopPressed();

protected:
    /** @see QWidget::contextMenuEvent() */
    virtual void contextMenuEvent(QContextMenuEvent* event);

private slots:
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

    void zoomOut();
    void zoomIn();
    void showZoomSliderToolTip(int zoomLevel);

private:
    void updateProgressInfo();

    /**
     * Makes the space information widget and zoom slider widget
     * visible, if \a visible is true and the settings allow to show
     * the widgets. showUnknownProgressIf \a visible is false, it is assured that both
     * widgets are hidden.
     */
    void setExtensionsVisible(bool visible);

    /**
     * Updates the text of the zoom slider tooltip to show
     * the currently used size.
     */
    void updateZoomSliderToolTip(int zoomLevel);

private:
    DolphinView* m_view;
    KonqStatusBarMessageLabel* m_messageLabel;
    StatusBarSpaceInfo* m_spaceInfo;

    QWidget* m_zoomWidget;
    QToolButton* m_zoomOut;
    QSlider* m_zoomSlider;
    QToolButton* m_zoomIn;

    QLabel* m_progressText;
    QProgressBar* m_progressBar;
    QToolButton* m_stopButton;
    int m_progress;

    // Timestamp when the last message has been set that has not the type
    // 'Default'. The timestamp is used to prevent that default messages
    // hide more important messages after a very short delay.
    QTime m_messageTimeStamp;
};

#endif
