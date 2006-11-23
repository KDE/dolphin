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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "dolphinstatusbar.h"
#include <qprogressbar.h>
#include <qlabel.h>
#include <qtimer.h>
#include <kiconloader.h>
#include <kvbox.h>

#include "dolphinview.h"
#include "statusbarmessagelabel.h"
#include "statusbarspaceinfo.h"

DolphinStatusBar::DolphinStatusBar(DolphinView* parent) :
    KHBox(parent),
    m_messageLabel(0),
    m_spaceInfo(0),
    m_progressBar(0),
    m_progress(100)
{
    m_messageLabel = new StatusBarMessageLabel(this);
    m_messageLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    m_spaceInfo = new StatusBarSpaceInfo(this);
    m_spaceInfo->setUrl(parent->url());

    m_progressText = new QLabel(this);
    m_progressText->hide();

    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();

    m_progressTimer = new QTimer(this);
    connect(m_progressTimer, SIGNAL(timeout()),
            this, SLOT(slotProgressTimer()));

    const QSize size(m_progressBar->sizeHint());
    m_progressBar->setMaximumWidth(size.width());
    setMinimumHeight(size.height());
    m_messageLabel->setMinimumTextHeight(size.height());

    connect(parent, SIGNAL(signalUrlChanged(const KUrl&)),
            this, SLOT(slotUrlChanged(const KUrl&)));
}


DolphinStatusBar::~DolphinStatusBar()
{
}

void DolphinStatusBar::setMessage(const QString& msg,
                                  Type type)
{
    m_messageLabel->setText(msg);
    if (msg.isEmpty() || (msg == m_defaultText)) {
        type = Default;
    }
    m_messageLabel->setType(type);

    if ((type == Error) && (m_progress < 100)) {
        // If an error message is shown during a progress is ongoing,
        // the (never finishing) progress information should be hidden immediately
        // (invoking 'setProgress(100)' only leads to a delayed hiding).
        m_progressBar->hide();
        m_progressText->hide();
        setProgress(100);
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
    m_progressBar->setValue(m_progress);
    m_progressTimer->start(300, true);

    const QString msg(m_messageLabel->text());
    if (msg.isEmpty() || (msg == m_defaultText)) {
        if (percent == 0) {
            m_messageLabel->setText(QString::null);
            m_messageLabel->setType(Default);
        }
        else if (percent == 100) {
            m_messageLabel->setText(m_defaultText);
        }
    }
}

void DolphinStatusBar::clear()
{
    // TODO: check for timeout, so that it's prevented that
    // a message is cleared too early.
    m_messageLabel->setText(m_defaultText);
    m_messageLabel->setType(Default);
}

void DolphinStatusBar::setDefaultText(const QString& text)
{
    m_defaultText = text;
}

void DolphinStatusBar::slotProgressTimer()
{
    if (m_progress < 100) {
        // progress should be shown
        m_progressBar->show();
        m_progressText->show();
        m_spaceInfo->hide();
    }
    else {
        // progress should not be shown anymore
        m_progressBar->hide();
        m_progressText->hide();
        m_spaceInfo->show();
    }
}

void DolphinStatusBar::slotUrlChanged(const KUrl& url)
{
    m_spaceInfo->setUrl(url);
}

#include "dolphinstatusbar.moc"
