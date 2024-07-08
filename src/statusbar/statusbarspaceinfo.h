/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef STATUSBARSPACEINFO_H
#define STATUSBARSPACEINFO_H

#include <KMessageWidget>

#include <QUrl>
#include <QWidget>

class QHideEvent;
class QShowEvent;
class QMenu;
class QMouseEvent;
class QToolButton;
class QWidgetAction;

class KCapacityBar;

class SpaceInfoObserver;

/**
 * @short Shows the available space for the volume represented
 *        by the given URL as part of the status bar.
 */
class StatusBarSpaceInfo : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarSpaceInfo(QWidget *parent = nullptr);
    ~StatusBarSpaceInfo() override;

    /**
     * Use this to set the widget visibility as it can hide itself
     */
    void setShown(bool);
    void setUrl(const QUrl &url);
    QUrl url() const;

    void update();

Q_SIGNALS:
    /**
     * Requests for @p message with the given @p messageType to be shown to the user in a non-modal way.
     */
    void showMessage(const QString &message, KMessageWidget::MessageType messageType);

    /**
     * Requests for a progress update to be shown to the user in a non-modal way.
     * @param currentlyRunningTaskTitle     The task that is currently progressing.
     * @param installationProgressPercent   The current percentage of completion.
     */
    void showInstallationProgress(const QString &currentlyRunningTaskTitle, int installationProgressPercent);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    QSize minimumSizeHint() const override;

    void updateMenu();

private Q_SLOTS:
    /**
     * Asynchronously starts a Filelight installation using DolphinPackageInstaller. @see DolphinPackageInstaller.
     * Installation success or failure is reported through showMessage(). @see StatusBarSpaceInfo::showMessage().
     * Installation progress is reported through showInstallationProgress(). @see StatusBarSpaceInfo::showInstallationProgress().
     */
    void slotInstallFilelightButtonClicked();

    void slotValuesChanged();

private:
    /**
     * Creates a new QWidgetAction that contains a UI to install Filelight.
     * m_installFilelightWidgetAction is initialised after calling this method once.
     */
    void initialiseInstallFilelightWidgetAction();

private:
    QScopedPointer<SpaceInfoObserver> m_observer;
    KCapacityBar *m_capacityBar;
    QToolButton *m_textInfoButton;
    QMenu *m_buttonMenu;
    /** An action containing a UI to install Filelight. */
    QWidgetAction *m_installFilelightWidgetAction;
    QUrl m_url;
    bool m_ready;
    bool m_shown;
};

#endif
