/*
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 * SPDX-FileCopyrightText: 2006-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PANEL_H
#define PANEL_H

#include <QUrl>
#include <QWidget>

/**
 * @brief Base widget for all panels that can be docked on the window borders.
 *
 * Derived panels should provide a context menu that at least offers the
 * actions from Panel::customContextMenuActions().
 */
class Panel : public QWidget
{
    Q_OBJECT

public:
    explicit Panel(QWidget* parent = nullptr);
    ~Panel() override;

    /** Returns the current set URL of the active Dolphin view. */
    QUrl url() const;

    /**
     * Sets custom context menu actions that are added to the panel specific
     * context menu actions. Allows an application to apply custom actions to
     * the panel.
     */
    void setCustomContextMenuActions(const QList<QAction*>& actions);
    QList<QAction*> customContextMenuActions() const;

    QSize sizeHint() const override;

public slots:
    /**
     * This is invoked every time the folder being displayed in the
     * active Dolphin view changes.
     */
    void setUrl(const QUrl &url);

    /**
     * Refreshes the view to get synchronized with the settings.
     */
    virtual void readSettings();

protected:
    /**
     * Must be implemented by derived classes and is invoked when
     * the URL has been changed (see Panel::setUrl()).
     * @return True, if the new URL will get accepted by the derived
     *         class. If false is returned,
     *         the URL will be reset to the previous URL.
     */
    virtual bool urlChanged() = 0;

private:
    QUrl m_url;
    QList<QAction*> m_customContextMenuActions;
};

#endif // PANEL_H
