/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef STATUSBARSPACEINFO_H
#define STATUSBARSPACEINFO_H

#include <QUrl>
#include <QWidget>

class QHideEvent;
class QShowEvent;
class QMenu;
class QMouseEvent;
class QToolButton;

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

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    QSize minimumSizeHint() const override;

    void updateMenu();

private Q_SLOTS:
    void slotValuesChanged();

private:
    QScopedPointer<SpaceInfoObserver> m_observer;
    KCapacityBar *m_capacityBar;
    QToolButton *m_textInfoButton;
    QMenu *m_buttonMenu;
    QUrl m_url;
    bool m_ready;
    bool m_shown;
};

#endif
