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

#include "dolphin_generalsettings.h"

#include <KIconLoader>
#include <KIcon>
#include <KLocale>
#include <KMenu>
#include <KVBox>

#include "settings/dolphinsettings.h"
#include "statusbarspaceinfo.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QToolButton>
#include <QTime>
#include <QTimer>

#include <views/dolphinview.h>
#include <views/zoomlevelinfo.h>

DolphinStatusBar::DolphinStatusBar(QWidget* parent, DolphinView* view) :
    QWidget(parent),
    m_view(view),
    m_messageLabel(0),
    m_spaceInfo(0),
    m_zoomSlider(0),
    m_progressBar(0),
    m_stopButton(0),
    m_progress(100),
    m_showProgressBarTimer(0),
    m_messageTimeStamp()
{
    connect(m_view, SIGNAL(urlChanged(KUrl)),
            this, SLOT(updateSpaceInfoContent(KUrl)));

    // Initialize message label
    m_messageLabel = new KonqStatusBarMessageLabel(this);

    // Initialize zoom widget
    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setPageStep(1);

    const int min = ZoomLevelInfo::minimumLevel();
    const int max = ZoomLevelInfo::maximumLevel();
    m_zoomSlider->setRange(min, max);
    m_zoomSlider->setValue(view->zoomLevel());
    updateZoomSliderToolTip(view->zoomLevel());

    connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SLOT(setZoomLevel(int)));
    connect(m_zoomSlider, SIGNAL(sliderMoved(int)), this, SLOT(showZoomSliderToolTip(int)));
    connect(m_view, SIGNAL(zoomLevelChanged(int,int)), this, SLOT(slotZoomLevelChanged(int,int)));

    // Initialize space information
    m_spaceInfo = new StatusBarSpaceInfo(this);
    m_spaceInfo->setUrl(m_view->url());

    // Initialize progress information
    m_stopButton = new QToolButton(this);
    m_stopButton->setIcon(KIcon("process-stop"));
    m_stopButton->setAccessibleName(i18n("Stop"));
    // TODO: Add tooltip for KDE SC 4.7.0, if new strings are allowed again
    m_stopButton->setAutoRaise(true);
    m_stopButton->hide();
    connect(m_stopButton, SIGNAL(clicked()), this, SIGNAL(stopPressed()));

    m_progressText = new QLabel(this);
    m_progressText->hide();

    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();

    m_showProgressBarTimer = new QTimer(this);
    m_showProgressBarTimer->setInterval(1000);
    m_showProgressBarTimer->setSingleShot(true);
    connect(m_showProgressBarTimer, SIGNAL(timeout()), this, SLOT(updateProgressInfo()));

    // Initialize top layout and size policies
    const int fontHeight = QFontMetrics(m_messageLabel->font()).height();
    const int zoomSliderHeight = m_zoomSlider->minimumSizeHint().height();
    const int contentHeight = qMax(fontHeight, zoomSliderHeight);

    m_messageLabel->setMinimumTextHeight(contentHeight);

    m_spaceInfo->setMaximumSize(200, contentHeight - 5);
    m_spaceInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_progressBar->setMaximumSize(200, contentHeight);
    m_zoomSlider->setMaximumSize(150, contentHeight);
    m_zoomSlider->setMinimumWidth(30);

    QHBoxLayout* topLayout = new QHBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(4);
    topLayout->addWidget(m_messageLabel);
    topLayout->addWidget(m_zoomSlider);
    topLayout->addWidget(m_spaceInfo);
    topLayout->addWidget(m_stopButton);
    topLayout->addWidget(m_progressText);
    topLayout->addWidget(m_progressBar);

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

    KonqStatusBarMessageLabel::Type konqType = static_cast<KonqStatusBarMessageLabel::Type>(type);
    if ((message == m_messageLabel->text()) && (konqType == m_messageLabel->type())) {
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

    m_messageLabel->setMessage(message, konqType);
    if (type != Default) {
        m_messageTimeStamp = currentTime;
    }
}

DolphinStatusBar::Type DolphinStatusBar::type() const
{
    return static_cast<Type>(m_messageLabel->type());
}

QString DolphinStatusBar::message() const
{
    return m_messageLabel->text();
}

void DolphinStatusBar::setProgressText(const QString& text)
{
    m_progressText->setText(text);
}

int DolphinStatusBar::progress() const
{
    return m_progress;
}

QString DolphinStatusBar::progressText() const
{
    return m_progressText->text();
}

void DolphinStatusBar::setProgress(int percent)
{
    // Show a busy indicator if a value < 0 is provided:
    m_progressBar->setMaximum((percent < 0) ? 0 : 100);

    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }

    const bool progressRestarted = (percent < 100) && (percent < m_progress);
    m_progress = percent;
    if (m_messageLabel->type() == KonqStatusBarMessageLabel::Error) {
        // Don't update any widget or status bar text if an
        // error message is shown
        return;
    }

    if (progressRestarted && !m_progressBar->isVisible()) {
        // Show the progress bar delayed: In the case if 100 % are reached within
        // a short time, no progress bar will be shown at all.
        m_showProgressBarTimer->start();
    }

    m_progressBar->setValue(m_progress);
    if (percent == 100) {
        // The end of the progress has been reached. Assure that the progress bar
        // gets hidden and the extensions widgets get visible again.
        m_showProgressBarTimer->stop();
        updateProgressInfo();
    }

    const QString defaultText = m_messageLabel->defaultText();
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

QString DolphinStatusBar::defaultText() const
{
    return m_messageLabel->defaultText();
}

void DolphinStatusBar::refresh()
{
    setExtensionsVisible(true);
}

void DolphinStatusBar::contextMenuEvent(QContextMenuEvent* event)
{
    Q_UNUSED(event);

    KMenu menu(this);

    QAction* copyAction = 0;
    switch (type()) {
    case Default:
    case OperationCompleted:
    case Information:
        copyAction = menu.addAction(i18nc("@action:inmenu", "Copy Information Message"));
        break;
    case Error:
        copyAction = menu.addAction(i18nc("@action:inmenu", "Copy Error Message"));
        break;
    default: break;
    }

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();

    QAction* showZoomSliderAction = menu.addAction(i18nc("@action:inmenu", "Show Zoom Slider"));
    showZoomSliderAction->setCheckable(true);
    showZoomSliderAction->setChecked(settings->showZoomSlider());

    QAction* showSpaceInfoAction = menu.addAction(i18nc("@action:inmenu", "Show Space Information"));
    showSpaceInfoAction->setCheckable(true);
    showSpaceInfoAction->setChecked(settings->showSpaceInfo());

    const QAction* action = menu.exec(QCursor::pos());
    if (action == copyAction) {
        QMimeData* mimeData = new QMimeData();
        mimeData->setText(message());
        QApplication::clipboard()->setMimeData(mimeData);
    } else if (action == showZoomSliderAction) {
        const bool visible = showZoomSliderAction->isChecked();
        settings->setShowZoomSlider(visible);
        m_zoomSlider->setVisible(visible);
    } else if (action == showSpaceInfoAction) {
        const bool visible = showSpaceInfoAction->isChecked();
        settings->setShowSpaceInfo(visible);
        m_spaceInfo->setVisible(visible);
    }
}

void DolphinStatusBar::updateSpaceInfoContent(const KUrl& url)
{
    m_spaceInfo->setUrl(url);
}

void DolphinStatusBar::setZoomLevel(int zoomLevel)
{
    m_view->setZoomLevel(zoomLevel);
    updateZoomSliderToolTip(zoomLevel);
}

void DolphinStatusBar::showZoomSliderToolTip(int zoomLevel)
{
    updateZoomSliderToolTip(zoomLevel);

    QPoint global = m_zoomSlider->rect().topLeft();
    global.ry() += m_zoomSlider->height() / 2;
    QHelpEvent toolTipEvent(QEvent::ToolTip, QPoint(0, 0), m_zoomSlider->mapToGlobal(global));
    QApplication::sendEvent(m_zoomSlider, &toolTipEvent);
}

void DolphinStatusBar::slotZoomLevelChanged(int current, int previous)
{
    Q_UNUSED(previous);
    m_zoomSlider->setValue(current);
}

void DolphinStatusBar::updateProgressInfo()
{
    const bool isErrorShown = (m_messageLabel->type() == KonqStatusBarMessageLabel::Error);
    if (m_progress < 100) {
        // Show the progress information and hide the extensions
        setExtensionsVisible(false);
        if (!isErrorShown) {
            m_stopButton->show();
            m_progressText->show();
            m_progressBar->show();
        }
    } else {
        // Hide the progress information and show the extensions
        m_stopButton->hide();
        m_progressText->hide();
        m_progressBar->hide();
        setExtensionsVisible(true);
    }
}

void DolphinStatusBar::setExtensionsVisible(bool visible)
{
    bool showSpaceInfo = visible;
    bool showZoomSlider = visible;
    if (visible) {
        const GeneralSettings* settings = DolphinSettings::instance().generalSettings();
        showSpaceInfo = settings->showSpaceInfo();
        showZoomSlider = settings->showZoomSlider();
    }
    m_spaceInfo->setVisible(showSpaceInfo);
    m_zoomSlider->setVisible(showZoomSlider);
}

void DolphinStatusBar::updateZoomSliderToolTip(int zoomLevel)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel);
    m_zoomSlider->setToolTip(i18ncp("@info:tooltip", "Size: 1 pixel", "Size: %1 pixels", size));
}

#include "dolphinstatusbar.moc"
