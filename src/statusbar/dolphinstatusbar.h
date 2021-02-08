/*
 * SPDX-FileCopyrightText: 2006-2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINSTATUSBAR_H
#define DOLPHINSTATUSBAR_H

#include <QTime>
#include <QWidget>

class QUrl;
class StatusBarSpaceInfo;
class QLabel;
class QProgressBar;
class QToolButton;
class QSlider;
class QTimer;
class KSqueezedTextLabel;

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
    explicit DolphinStatusBar(QWidget* parent);
    ~DolphinStatusBar() override;

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
     * progress is shown delayed by 500 milliseconds:
     * If the progress does reach 100 % within 500 milliseconds,
     * the progress is not shown at all.
     */
    void setProgress(int percent);
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
    void setDefaultText(const QString& text);
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

public Q_SLOTS:
    void setText(const QString& text);
    void setUrl(const QUrl& url);
    void setZoomLevel(int zoomLevel);

Q_SIGNALS:
    /**
     * Is emitted if the stop-button has been pressed during showing a progress.
     */
    void stopPressed();

    void zoomLevelChanged(int zoomLevel);

protected:
    void contextMenuEvent(QContextMenuEvent* event) override;

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

private:
    QString m_text;
    QString m_defaultText;
    KSqueezedTextLabel* m_label;
    QLabel* m_zoomLabel;
    StatusBarSpaceInfo* m_spaceInfo;

    QSlider* m_zoomSlider;

    QLabel* m_progressTextLabel;
    QProgressBar* m_progressBar;
    QToolButton* m_stopButton;
    int m_progress;
    QTimer* m_showProgressBarTimer;

    QTimer* m_delayUpdateTimer;
    QTime m_textTimestamp;
};

#endif
