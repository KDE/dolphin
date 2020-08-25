/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef STATUSBARSPACEINFO_H
#define STATUSBARSPACEINFO_H

#include <KCapacityBar>

#include <QUrl>

class QHideEvent;
class QShowEvent;
class QMouseEvent;

class SpaceInfoObserver;

/**
 * @short Shows the available space for the volume represented
 *        by the given URL as part of the status bar.
 */
class StatusBarSpaceInfo : public KCapacityBar
{
    Q_OBJECT

public:
    explicit StatusBarSpaceInfo(QWidget* parent = nullptr);
    ~StatusBarSpaceInfo() override;

    /**
     * Use this to set the widget visibility as it can hide itself
     */
    void setShown(bool);
    void setUrl(const QUrl& url);
    QUrl url() const;

    void update();

protected:
    void showEvent(QShowEvent* event) override;
    void hideEvent(QHideEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private slots:
    void slotValuesChanged();

private:
    QScopedPointer<SpaceInfoObserver> m_observer;
    QUrl m_url;
    bool m_ready;
    bool m_shown;
};

#endif
