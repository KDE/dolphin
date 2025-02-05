/*
 * SPDX-FileCopyrightText: 2006-2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINSTATUSBAR_H
#define DOLPHINSTATUSBAR_H

#include "animatedheightwidget.h"

#include <KMessageWidget>

#include <QTime>

class QUrl;
class StatusBarSpaceInfo;
class QLabel;
class QProgressBar;
class QToolButton;
class QSlider;
class QTimer;
class KSqueezedTextLabel;
class QHBoxLayout;

/**
 * @brief Represents the statusbar of a Dolphin view.
 *
 * The statusbar allows to show messages, progress
 * information and space-information of a disk.
 */
class DolphinStatusBar : public AnimatedHeightWidget
{
    Q_OBJECT

public:
    explicit DolphinStatusBar(QWidget *parent);
    ~DolphinStatusBar() override;

    QString text() const;

    enum class CancelLoading { Allowed, Disallowed };

    enum StatusBarMode {
        FullWidth, // Regular full width statusbar
        Small // Statusbar takes least amount of space possible
    };
    /**
     * Shows progress for a task on the status bar.
     *
     * Allows us to inform the user about various tasks progressing in the background.
     * This method can be called from various places in any order but only the most recent call will be displayed.
     * @param currentlyRunningTaskTitle The task that is currently progressing.
     * @param progressPercent           The percent value shown in a progress bar next to the @p currentlyRunningTaskTitle.
     *                                  A negative @p progressPercent value will be interpreted as indeterminate/unknown progress.
     *                                  The progress is shown delayed by 500 milliseconds: If the progress does reach 100 % within 500 milliseconds,
     *                                  the progress is not shown at all.
     * @param cancelLoading             Whether a "Stop" button for cancelling the task should be available next to the progress reporting.
     *
     * @note Make sure you also hide the progress information by calling this with a @p progressPercent equal or greater than 100.
     */
    void showProgress(const QString &currentlyRunningTaskTitle, int progressPercent, CancelLoading cancelLoading = CancelLoading::Allowed);
    QString progressText() const;
    int progress() const;

    /**
     * Replaces the text set by setText() by the text that
     * has been set by setDefaultText(). DolphinStatusBar::text()
     * will return an empty string after the reset has been done.
     */
    void resetToDefaultText();

    /**
     * Sets the default text, which is shown if the status bar
     * is rest by DolphinStatusBar::resetToDefaultText().
     */
    void setDefaultText(const QString &text);
    QString defaultText() const;

    QUrl url() const;
    int zoomLevel() const;

    /**
     * Refreshes the status bar to get synchronized with the (updated) Dolphin settings.
     */
    void readSettings();

    /**
     * Refreshes the disk space information.
     */
    void updateSpaceInfo();

    /**
     * Changes the statusbar between transient and regular
     * depending on settings enabled
     */
    void updateMode();
    StatusBarMode mode();
    void setMode(StatusBarMode mode);

    /**
     * Updates the statusbar width to fit all content
     */
    void updateWidthToContent();

public Q_SLOTS:
    void setText(const QString &text);
    void setUrl(const QUrl &url);
    void setZoomLevel(int zoomLevel);

Q_SIGNALS:
    /**
     * Is emitted if the stop-button has been pressed during showing a progress.
     */
    void stopPressed();

    void zoomLevelChanged(int zoomLevel);

    /**
     * Requests for @p message with the given @p messageType to be shown to the user in a non-modal way.
     */
    void showMessage(const QString &message, KMessageWidget::MessageType messageType);

    /**
     * Emitted when statusbar mode is changed
     */
    void modeUpdated();

    /**
     * Emitted when statusbar width is updated to fit content
     */
    void widthUpdated();

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    void paintEvent(QPaintEvent *paintEvent) override;

private Q_SLOTS:
    void showZoomSliderToolTip(int zoomLevel);

    void updateProgressInfo();

    /**
     * Updates the text for m_label and does an eliding in
     * case if the text does not fit into the available width.
     */
    void updateLabelText();

    /**
     * Updates the text of the zoom slider tooltip to show
     * the currently used size.
     */
    void updateZoomSliderToolTip(int zoomLevel);

private:
    /**
     * Makes the space information widget and zoom slider widget
     * visible, if \a visible is true and the settings allow to show
     * the widgets. showUnknownProgressIf \a visible is false, it is assured that both
     * widgets are hidden.
     */
    void setExtensionsVisible(bool visible);

    void updateContentsMargins();

    /** @see AnimatedHeightWidget::preferredHeight() */
    int preferredHeight() const override;

private:
    QString m_text;
    QString m_defaultText;
    KSqueezedTextLabel *m_label;
    QLabel *m_zoomLabel;
    StatusBarSpaceInfo *m_spaceInfo;

    QSlider *m_zoomSlider;

    QLabel *m_progressTextLabel;
    CancelLoading m_cancelLoading = CancelLoading::Allowed;
    QProgressBar *m_progressBar;
    QToolButton *m_stopButton;
    int m_progress;
    QTimer *m_showProgressBarTimer;

    QTimer *m_delayUpdateTimer;
    QTime m_textTimestamp;

    QHBoxLayout *m_topLayout;

    StatusBarMode m_mode;
};

#endif
