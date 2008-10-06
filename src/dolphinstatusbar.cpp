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
#include "dolphinsettings.h"
#include "dolphinview.h"
#include "dolphin_generalsettings.h"
#include "statusbarmessagelabel.h"
#include "statusbarspaceinfo.h"
#include "zoomlevelinfo.h"

#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtCore/QTimer>

#include <kiconloader.h>
#include <kvbox.h>

DolphinStatusBar::DolphinStatusBar(QWidget* parent, DolphinView* view) :
    KHBox(parent),
    m_view(view),
    m_messageLabel(0),
    m_spaceInfo(0),
    m_zoomSlider(0),
    m_progressBar(0),
    m_progress(100)
{
    setSpacing(4);

    connect(m_view, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(updateSpaceInfoContent(const KUrl&)));

    // initialize message label
    m_messageLabel = new StatusBarMessageLabel(this);
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // initialize space information
    m_spaceInfo = new StatusBarSpaceInfo(this);
    m_spaceInfo->setUrl(m_view->url());
    
    // initialize zoom slider
    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setPageStep(1);
    
    const int min = ZoomLevelInfo::minimumLevel();
    const int max = ZoomLevelInfo::maximumLevel();
    m_zoomSlider->setRange(min, max);
    m_zoomSlider->setValue(view->zoomLevel());
    
    connect(m_zoomSlider, SIGNAL(sliderMoved(int)),
            this, SLOT(setZoomLevel(int)));
    connect(m_view, SIGNAL(zoomLevelChanged(int)),
            m_zoomSlider, SLOT(setValue(int)));
            
    // initialize progress informatino
    m_progressText = new QLabel(this);
    m_progressText->hide();

    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();

    // initialize sizes
    const int contentHeight = QFontMetrics(m_messageLabel->font()).height() + 4;
    const int barHeight = contentHeight + 4;

    setMinimumHeight(barHeight);
    m_messageLabel->setMinimumTextHeight(barHeight);
    m_spaceInfo->setFixedHeight(contentHeight);
    m_progressBar->setFixedHeight(contentHeight);
    m_progressBar->setMaximumWidth(200);
    m_zoomSlider->setMaximumWidth(100);
    
    setExtensionsVisible(true);
}


DolphinStatusBar::~DolphinStatusBar()
{
}

void DolphinStatusBar::setMessage(const QString& msg,
                                  Type type)
{
    m_messageLabel->setMessage(msg, type);

    const int widthGap = m_messageLabel->widthGap();
    if (widthGap > 0) {
        m_progressBar->hide();
        m_progressText->hide();
    }
    assureVisibleText();
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
        QTimer::singleShot(300, this, SLOT(updateProgressInfo()));
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
    QMetaObject::invokeMethod(this, "assureVisibleText", Qt::QueuedConnection);
}

void DolphinStatusBar::updateProgressInfo()
{
    const bool isErrorShown = (m_messageLabel->type() == Error);
    if (m_progress < 100) {
        // show the progress information and hide the extensions
        setExtensionsVisible(false);
        if (!isErrorShown) {
            m_progressText->show();
            m_progressBar->show();
        }
    } else {
        // hide the progress information and show the extensions
        m_progressText->hide();
        m_progressBar->hide();
        assureVisibleText();
    }
}

void DolphinStatusBar::updateSpaceInfoContent(const KUrl& url)
{
    m_spaceInfo->setUrl(url);
    assureVisibleText();
}

void DolphinStatusBar::setZoomLevel(int zoomLevel)
{
    m_view->setZoomLevel(zoomLevel);
}

void DolphinStatusBar::assureVisibleText()
{
    const int widthGap = m_messageLabel->widthGap();
    const bool isProgressBarVisible = m_progressBar->isVisible();
    
    const int spaceInfoWidth  = m_spaceInfo->isVisible()  ? m_spaceInfo->width()  : 0;
    const int zoomSliderWidth = m_zoomSlider->isVisible() ? m_zoomSlider->width() : 0;
    const int widgetsWidth = spaceInfoWidth + zoomSliderWidth;

    if (widgetsWidth > 0) {
        // The space information and (or) the zoom slider are (is) shown.
        // Hide them if the status bar text does not fit into the available width.
        if (widthGap > 0) {
            setExtensionsVisible(false);
        }
    } else if (!isProgressBarVisible && (widthGap + widgetsWidth <= 0)) {
        setExtensionsVisible(true);
    }
}

void DolphinStatusBar::setExtensionsVisible(bool visible)
{
    bool spaceInfoVisible = visible;
    bool zoomSliderVisible = visible;
    if (visible) {
        const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
        spaceInfoVisible = settings->showSpaceInfo();
        zoomSliderVisible = settings->showZoomSlider();
    }
    
    m_spaceInfo->setVisible(spaceInfoVisible);
    m_zoomSlider->setVisible(zoomSliderVisible);
}

#include "dolphinstatusbar.moc"
