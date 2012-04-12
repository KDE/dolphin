/***************************************************************************
 *   Copyright (C) 2006-2012 by Peter Penz <peter.penz19@gmail.com>        *
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

#include <QWidget>

class KUrl;
class StatusBarSpaceInfo;
class QLabel;
class QProgressBar;
class QToolButton;
class QSlider;
class QTimer;

/**
 * @brief Represents the statusbar of a Dolphin view.
 *
 * The statusbar allows to show messages, progress
 * information and space-information of a disk.
 */
class DolphinStatusBar : public QWidget
{
    Q_OBJECT

public:
    DolphinStatusBar(QWidget* parent);
    virtual ~DolphinStatusBar();

    QString text() const;

    /**
     * Sets the text for the progress information.
     * DolphinStatusBar::setProgress() should be invoked
     * afterwards each time the progress changes.
     */
    void setProgressText(const QString& text);
    QString progressText() const;

    /**
     * Sets the progress in percent (0 - 100). The
     * progress is shown delayed by 1 second:
     * If the progress does reach 100 % within 1 second,
     * the progress is not shown at all.
     */
    void setProgress(int percent);
    int progress() const;

    /**
     * Replaces the text set by setText() by the text that
     * has been set by setDefaultText(). DolphinStatusBar::text()
     * will return an empty string afterwards.
     */
    void resetToDefaultText();

    /**
     * Sets the default text, which is shown if the status bar
     * is rest by DolphinStatusBar::resetToDefaultText().
     */
    void setDefaultText(const QString& text);
    QString defaultText() const;

    KUrl url() const;
    int zoomLevel() const;

    /**
     * Refreshes the status bar to get synchronized with the (updated) Dolphin settings.
     */
    void readSettings();

public slots:
    void setText(const QString& text);
    void setUrl(const KUrl& url);
    void setZoomLevel(int zoomLevel);

signals:
    /**
     * Is emitted if the stop-button has been pressed during showing a progress.
     */
    void stopPressed();

    void zoomLevelChanged(int zoomLevel);

protected:
    virtual void contextMenuEvent(QContextMenuEvent* event);
    virtual bool eventFilter(QObject* obj, QEvent* event);

private slots:
    void showZoomSliderToolTip(int zoomLevel);
    void updateProgressInfo();

    /**
     * Updates the text for m_label and does an eliding in
     * case if the text does not fit into the available width.
     */
    void updateLabelText();

private:
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

    void applyFixedWidgetSize(QWidget* widget, const QSize& size);

private:
    QString m_text;
    QString m_defaultText;
    QLabel* m_label;
    StatusBarSpaceInfo* m_spaceInfo;

    QSlider* m_zoomSlider;

    QLabel* m_progressTextLabel;
    QProgressBar* m_progressBar;
    QToolButton* m_stopButton;
    int m_progress;
    QTimer* m_showProgressBarTimer;
};

#endif
