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
#include "settings/dolphinsettings.h"
#include "dolphinview.h"
#include "dolphin_generalsettings.h"
#include "statusbarmessagelabel.h"
#include "statusbarspaceinfo.h"
#include "zoomlevelinfo.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QTimer>

#include <kiconloader.h>
#include <kicon.h>
#include <klocale.h>
#include <kvbox.h>

DolphinStatusBar::DolphinStatusBar(QWidget* parent, DolphinView* view) :
    KHBox(parent),
    m_view(view),
    m_messageLabel(0),
    m_spaceInfo(0),
    m_zoomWidget(0),
    m_zoomOut(0),
    m_zoomSlider(0),
    m_zoomIn(0),
    m_progressBar(0),
    m_progress(100),
    m_messageTimeStamp()
{
    setMargin(0);
    setSpacing(4);

    connect(m_view, SIGNAL(urlChanged(const KUrl&)),
            this, SLOT(updateSpaceInfoContent(const KUrl&)));

    // initialize message label
    m_messageLabel = new StatusBarMessageLabel(this);
    m_messageLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    // initialize zoom slider
    m_zoomWidget = new QWidget(this);

    m_zoomOut = new QToolButton(m_zoomWidget);
    m_zoomOut->setIcon(KIcon("zoom-out"));
    m_zoomOut->setAutoRaise(true);

    m_zoomSlider = new QSlider(Qt::Horizontal, m_zoomWidget);
    m_zoomSlider->setPageStep(1);

    const int min = ZoomLevelInfo::minimumLevel();
    const int max = ZoomLevelInfo::maximumLevel();
    m_zoomSlider->setRange(min, max);
    m_zoomSlider->setValue(view->zoomLevel());
    updateZoomSliderToolTip(view->zoomLevel());

    m_zoomIn = new QToolButton(m_zoomWidget);
    m_zoomIn->setIcon(KIcon("zoom-in"));
    m_zoomIn->setAutoRaise(true);

    QHBoxLayout* zoomWidgetLayout = new QHBoxLayout(m_zoomWidget);
    zoomWidgetLayout->setSpacing(0);
    zoomWidgetLayout->setMargin(0);
    zoomWidgetLayout->addWidget(m_zoomOut);
    zoomWidgetLayout->addWidget(m_zoomSlider);
    zoomWidgetLayout->addWidget(m_zoomIn);

    connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setZoomLevel(int)));
    connect(m_zoomSlider, SIGNAL(sliderMoved(int)), this, SLOT(showZoomSliderToolTip(int)));
    connect(m_view, SIGNAL(zoomLevelChanged(int)), m_zoomSlider, SLOT(setValue(int)));
    connect(m_zoomOut, SIGNAL(clicked()), this, SLOT(zoomOut()));
    connect(m_zoomIn, SIGNAL(clicked()), this, SLOT(zoomIn()));

    // initialize space information
    m_spaceInfo = new StatusBarSpaceInfo(this);
    m_spaceInfo->setUrl(m_view->url());

    // initialize progress information
    m_progressText = new QLabel(this);
    m_progressText->hide();

    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();

    // initialize sizes
    const int fontHeight = QFontMetrics(m_messageLabel->font()).height();
    const int zoomWidgetHeight = m_zoomWidget->minimumSizeHint().height();
    const int contentHeight = qMax(fontHeight, zoomWidgetHeight);

    m_messageLabel->setMinimumTextHeight(contentHeight);
    m_spaceInfo->setFixedHeight(contentHeight - 5);
    m_progressBar->setFixedSize(200, contentHeight);
    m_zoomWidget->setFixedSize(150, contentHeight);

    setExtensionsVisible(true);
}

DolphinStatusBar::~DolphinStatusBar()
{
}

void DolphinStatusBar::setMessage(const QString& msg,
                                  Type type)
{
    int timeout = 1000; // Timeout in milliseconds until default
                        // messages may overwrite other messages.

    QString message = msg;
    if (message.isEmpty()) {
        // Show the default text as fallback. An empty text indicates
        // a clearing of the information message.
        if (m_messageLabel->defaultText().isEmpty()) {
            return;
        }
        message = m_messageLabel->defaultText();
        type = Default;
        timeout = 0;
    }

    if ((message == m_messageLabel->text()) && (type == m_messageLabel->type())) {
        // the message is already shown
        return;
    }

    const QTime currentTime = QTime::currentTime();
    const bool skipMessage = (type == Default) &&
                             m_messageTimeStamp.isValid() &&
                             (m_messageTimeStamp.msecsTo(currentTime) < timeout);
    if (skipMessage) {
        // A non-default message is shown just for a very short time. Don't hide
        // the message by a default message, so that the user gets the chance to
        // read the information.
        return;
    }

    m_messageLabel->setMessage(message, type);
    if (type != Default) {
        m_messageTimeStamp = currentTime;
    }

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
    // show a busy indicator if a value < 0 is provided:
    m_progressBar->setMaximum((percent < 0) ? 0 : 100);

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
        updateProgressInfo();
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

void DolphinStatusBar::refresh()
{
    setExtensionsVisible(true);
    assureVisibleText();
}

void DolphinStatusBar::resizeEvent(QResizeEvent* event)
{
    // It is important to hide the widgets before invoking
    // QWidget::resizeEvent(), otherwise it is possible that
    // the dock widgets are shrinked when having a small
    // statusbar width (bug #202147).
    m_zoomWidget->hide();
    m_spaceInfo->hide();

    QWidget::resizeEvent(event);
    QMetaObject::invokeMethod(this, "assureVisibleText", Qt::QueuedConnection);
}

void DolphinStatusBar::updateSpaceInfoContent(const KUrl& url)
{
    m_spaceInfo->setUrl(url);
    assureVisibleText();
}

void DolphinStatusBar::setZoomLevel(int zoomLevel)
{
    m_zoomOut->setEnabled(zoomLevel > m_zoomSlider->minimum());
    m_zoomIn->setEnabled(zoomLevel < m_zoomSlider->maximum());
    m_view->setZoomLevel(zoomLevel);
    updateZoomSliderToolTip(zoomLevel);
}

void DolphinStatusBar::assureVisibleText()
{
    const int widthGap = m_messageLabel->widthGap();
    if (m_spaceInfo->isVisible() || m_zoomWidget->isVisible()) {
        // At least the space information or the zoom slider is shown.
        // Hide them if the status bar text does not fit into the available width.
        setExtensionsVisible(widthGap <= 0);
    } else if (!m_progressBar->isVisible()) {
        const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
        const int spaceInfoWidth  = settings->showSpaceInfo()  ? m_spaceInfo->minimumWidth()  : 0;
        const int zoomWidgetWidth = settings->showZoomSlider() ? m_zoomWidget->minimumWidth() : 0;
        const int widgetsWidth = spaceInfoWidth + zoomWidgetWidth;
        setExtensionsVisible(widthGap + widgetsWidth <= 0);
    }
}

void DolphinStatusBar::zoomOut()
{
    const int value = m_zoomSlider->value();
    m_zoomSlider->setValue(value - 1);
}

void DolphinStatusBar::zoomIn()
{
    const int value = m_zoomSlider->value();
    m_zoomSlider->setValue(value + 1);
}

void DolphinStatusBar::showZoomSliderToolTip(int zoomLevel)
{
    updateZoomSliderToolTip(zoomLevel);

    QPoint global = m_zoomSlider->rect().topLeft();
    global.ry() += m_zoomSlider->height() / 2;
    QHelpEvent toolTipEvent(QEvent::ToolTip, QPoint(0, 0), m_zoomSlider->mapToGlobal(global));
    QApplication::sendEvent(m_zoomSlider, &toolTipEvent);
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
    m_zoomWidget->setVisible(zoomSliderVisible);
}

void DolphinStatusBar::updateZoomSliderToolTip(int zoomLevel)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel);
    m_zoomSlider->setToolTip(i18ncp("@info:tooltip", "Size: 1 pixel", "Size: %1 pixels", size));
}

#include "dolphinstatusbar.moc"
