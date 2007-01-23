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

#include "dolphinstatusbar.h"
#include "dolphinview.h"
#include "statusbarmessagelabel.h"
#include "statusbarspaceinfo.h"

#include <QLabel>
#include <QProgressBar>
#include <QTimer>

#include <kiconloader.h>
#include <kvbox.h>

DolphinStatusBar::DolphinStatusBar(DolphinView* parent) :
    KHBox(parent),
    m_messageLabel(0),
    m_spaceInfo(0),
    m_progressBar(0),
    m_progress(100)
{
    setSpacing(4);

    m_messageLabel = new StatusBarMessageLabel(this);
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_spaceInfo = new StatusBarSpaceInfo(this);
    m_spaceInfo->setUrl(parent->url());

    m_progressText = new QLabel(this);
    m_progressText->hide();

    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();

    const QSize size(m_progressBar->sizeHint());
    m_progressBar->setMaximumWidth(200);
    setMinimumHeight(size.height());
    m_messageLabel->setMinimumTextHeight(size.height());

    connect(parent, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(updateSpaceInfo(const KUrl&)));
}


DolphinStatusBar::~DolphinStatusBar()
{
}

void DolphinStatusBar::setMessage(const QString& msg,
                                  Type type)
{
    m_messageLabel->setText(msg);
    m_messageLabel->setType(type);

    if (type == Error) {
        // assure that enough space is available for the error message and
        // hide the space information and progress information
        m_spaceInfo->hide();
        m_progressBar->hide();
        m_progressText->hide();
    }
    else if (!m_progressBar->isVisible()) {
        m_spaceInfo->show();
    }
}

DolphinStatusBar::Type DolphinStatusBar::type() const
{
    return m_messageLabel->type();
}

QString DolphinStatusBar::message() const
{
    return m_messageLabel->text();
}

void DolphinStatusBar::setProgressText(const QString& text)
{
    m_progressText->setText(text);
}

QString DolphinStatusBar::progressText() const
{
    return m_progressText->text();
}

void DolphinStatusBar::setProgress(int percent)
{
    if (percent < 0) {
        percent = 0;
    }
    else if (percent > 100) {
        percent = 100;
    }

    m_progress = percent;
    if (m_messageLabel->type() == Error) {
        // don't update any widget or status bar text if an
        // error message is shown
        return;
    }

    m_progressBar->setValue(m_progress);
    if (!m_progressBar->isVisible() || (percent == 100)) {
        QTimer::singleShot(500, this, SLOT(updateProgressInfo()));
    }

    const QString msg(m_messageLabel->text());
    if ((percent == 0) && !msg.isEmpty()) {
        setMessage(QString::null, Default);
    }
    else if ((percent == 100) && (msg != m_defaultText)) {
        setMessage(m_defaultText, Default);
    }
}

void DolphinStatusBar::clear()
{
    // TODO: check for timeout, so that it's prevented that
    // a message is cleared too early.
    setMessage(m_defaultText, Default);
}

void DolphinStatusBar::setDefaultText(const QString& text)
{
    m_defaultText = text;
}

void DolphinStatusBar::updateProgressInfo()
{
    const bool isErrorShown = (m_messageLabel->type() == Error);
    if (m_progress < 100) {
        // show the progress information and hide the space information
        m_spaceInfo->hide();
        if (!isErrorShown) {
            m_progressText->show();
            m_progressBar->show();
        }
    }
    else {
        // hide the progress information and show the space information
        m_progressText->hide();
        m_progressBar->hide();
        if (m_messageLabel->type() != Error) {
            m_spaceInfo->show();
        }
    }
}

void DolphinStatusBar::updateSpaceInfo(const KUrl& url)
{
    m_spaceInfo->setUrl(url);
}

#include "dolphinstatusbar.moc"
