/*
 * SPDX-FileCopyrightText: 2006 Cvetoslav Ludmiloff <ludmiloff@gmail.com>
 * SPDX-FileCopyrightText: 2006-2010 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "panel.h"

Panel::Panel(QWidget *parent)
    : QWidget(parent)
    , m_url()
    , m_customContextMenuActions()
{
}

Panel::~Panel()
{
}

QUrl Panel::url() const
{
    return m_url;
}

void Panel::setCustomContextMenuActions(const QList<QAction *> &actions)
{
    m_customContextMenuActions = actions;
}

QList<QAction *> Panel::customContextMenuActions() const
{
    return m_customContextMenuActions;
}

QSize Panel::sizeHint() const
{
    // The size hint will be requested already when starting Dolphin even
    // if the panel is invisible. For performance reasons most panels delay
    // the creation and initialization of widgets until a showEvent() is called.
    // Because of this the size-hint of the embedded widgets cannot be used
    // and a default size is provided:
    return QSize(180, 180);
}

void Panel::setUrl(const QUrl &url)
{
    if (url.matches(m_url, QUrl::StripTrailingSlash)) {
        return;
    }

    const QUrl oldUrl = m_url;
    m_url = url;
    if (!urlChanged()) {
        m_url = oldUrl;
    }
}

void Panel::readSettings()
{
}
