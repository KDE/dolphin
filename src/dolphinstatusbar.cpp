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
    const int barHeight = size.height();

    m_progressBar->setMaximumWidth(200);
    setMinimumHeight(barHeight);
    m_messageLabel->setMinimumTextHeight(barHeight);
    m_spaceInfo->setFixedHeight(barHeight);

    connect(parent, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(updateSpaceInfoContent(const KUrl&)));
}


DolphinStatusBar::~DolphinStatusBar()
{}

void DolphinStatusBar::setMessage(const QString& msg,
                                  Type type)
{
    m_messageLabel->setMessage(msg, type);

    const int widthGap = m_messageLabel->widthGap();
    if (widthGap > 0) {
        m_progressBar->hide();
        m_progressText->hide();
    }
    showSpaceInfo();
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
    } else if (percent > 100) {
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

    const QString& defaultText = m_messageLabel->defaultText();
    const QString msg(m_messageLabel->text());
    if ((percent == 0) && !msg.isEmpty()) {
        setMessage(QString(), Default);
    } else if ((percent == 100) && (msg != defaultText)) {
        setMessage(defaultText, Default);
    }
}

void DolphinStatusBar::clear()
{
    setMessage(m_messageLabel->defaultText(), Default);
}

void DolphinStatusBar::setDefaultText(const QString& text)
{
    m_messageLabel->setDefaultText(text);
}

const QString& DolphinStatusBar::defaultText() const
{
    return m_messageLabel->defaultText();
}

void DolphinStatusBar::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    QTimer::singleShot(0, this, SLOT(showSpaceInfo()));
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
    } else {
        // hide the progress information and show the space information
        m_progressText->hide();
        m_progressBar->hide();
        showSpaceInfo();
    }
}

void DolphinStatusBar::updateSpaceInfoContent(const KUrl& url)
{
    m_spaceInfo->setUrl(url);
    showSpaceInfo();
}

void DolphinStatusBar::showSpaceInfo()
{
    const int widthGap = m_messageLabel->widthGap();
    const bool isProgressBarVisible = m_progressBar->isVisible();

    if (m_spaceInfo->isVisible()) {
        // The space information is shown currently. Hide it
        // if the progress bar is visible or if the status bar
        // text does not fit into the available width.
        const QSize size(m_progressBar->sizeHint());
        if (isProgressBarVisible || (widthGap > 0)) {
            m_spaceInfo->hide();
        }
    } else if (widthGap + m_spaceInfo->width() <= 0) {
        m_spaceInfo->show();
    }
}

#include "dolphinstatusbar.moc"
