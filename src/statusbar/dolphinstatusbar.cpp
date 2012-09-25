/***************************************************************************
 *   Copyright (C) 2006-2012 by Peter Penz <peter.penz19@gmail.com>        *
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

namespace {
    const int ResetToDefaultTimeout = 1000;
}

DolphinStatusBar::DolphinStatusBar(QWidget* parent) :
    QWidget(parent),
    m_text(),
    m_defaultText(),
    m_label(0),
    m_spaceInfo(0),
    m_zoomSlider(0),
    m_progressBar(0),
    m_stopButton(0),
    m_progress(100),
    m_showProgressBarTimer(0),
    m_resetToDefaultTextTimer(0),
    m_textTimestamp()
{
    // Initialize text label
    m_label = new QLabel(this);
    m_label->setWordWrap(true);
    m_label->installEventFilter(this);

    // Initialize zoom widget
    m_zoomSlider = new QSlider(Qt::Horizontal, this);
    m_zoomSlider->setAccessibleName(i18n("Zoom"));
    m_zoomSlider->setAccessibleDescription(i18nc("Description for zoom-slider (accessibility)", "Sets the size of the file icons."));
    m_zoomSlider->setPageStep(1);
    m_zoomSlider->setRange(ZoomLevelInfo::minimumLevel(), ZoomLevelInfo::maximumLevel());

    connect(m_zoomSlider, SIGNAL(valueChanged(int)), this, SIGNAL(zoomLevelChanged(int)));
    connect(m_zoomSlider, SIGNAL(sliderMoved(int)), this, SLOT(showZoomSliderToolTip(int)));

    // Initialize space information
    m_spaceInfo = new StatusBarSpaceInfo(this);

    // Initialize progress information
    m_stopButton = new QToolButton(this);
    m_stopButton->setIcon(KIcon("process-stop"));
    m_stopButton->setAccessibleName(i18n("Stop"));
    m_stopButton->setAutoRaise(true);
    m_stopButton->setToolTip(i18nc("@tooltip", "Stop loading"));
    m_stopButton->hide();
    connect(m_stopButton, SIGNAL(clicked()), this, SIGNAL(stopPressed()));

    m_progressTextLabel = new QLabel(this);
    m_progressTextLabel->hide();

    m_progressBar = new QProgressBar(this);
    m_progressBar->hide();

    m_showProgressBarTimer = new QTimer(this);
    m_showProgressBarTimer->setInterval(500);
    m_showProgressBarTimer->setSingleShot(true);
    connect(m_showProgressBarTimer, SIGNAL(timeout()), this, SLOT(updateProgressInfo()));

    m_resetToDefaultTextTimer = new QTimer(this);
    m_resetToDefaultTextTimer->setInterval(ResetToDefaultTimeout);
    m_resetToDefaultTextTimer->setSingleShot(true);
    connect(m_resetToDefaultTextTimer, SIGNAL(timeout()), this, SLOT(slotResetToDefaultText()));

    // Initialize top layout and size policies
    const int fontHeight = QFontMetrics(m_label->font()).height();
    const int zoomSliderHeight = m_zoomSlider->minimumSizeHint().height();
    const int contentHeight = qMax(fontHeight, zoomSliderHeight);

    m_label->setMinimumHeight(contentHeight);
    m_label->setMaximumHeight(contentHeight);
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    const QSize size(150, contentHeight);
    applyFixedWidgetSize(m_spaceInfo, size);
    applyFixedWidgetSize(m_progressBar, size);
    applyFixedWidgetSize(m_zoomSlider, size);

    QHBoxLayout* topLayout = new QHBoxLayout(this);
    topLayout->setMargin(0);
    topLayout->setSpacing(4);
    topLayout->addWidget(m_label);
    topLayout->addWidget(m_zoomSlider);
    topLayout->addWidget(m_spaceInfo);
    topLayout->addWidget(m_stopButton);
    topLayout->addWidget(m_progressTextLabel);
    topLayout->addWidget(m_progressBar);

    setExtensionsVisible(true);
}

DolphinStatusBar::~DolphinStatusBar()
{
}

void DolphinStatusBar::setText(const QString& text)
{
    if (m_text == text) {
        return;
    }

    m_textTimestamp = QTime::currentTime();

    if (text.isEmpty()) {
        // Assure that the previous set text won't get
        // cleared immediatelly.
        m_resetToDefaultTextTimer->start();
    } else {
        m_text = text;

        if (m_resetToDefaultTextTimer->isActive()) {
            m_resetToDefaultTextTimer->start();
        }

        updateLabelText();
    }
}

QString DolphinStatusBar::text() const
{
    return m_text;
}

void DolphinStatusBar::setProgressText(const QString& text)
{
    m_progressTextLabel->setText(text);
}

QString DolphinStatusBar::progressText() const
{
    return m_progressTextLabel->text();
}

void DolphinStatusBar::setProgress(int percent)
{
    // Show a busy indicator if a value < 0 is provided:
    m_progressBar->setMaximum((percent < 0) ? 0 : 100);

    percent = qBound(0, percent, 100);
    const bool progressRestarted = (percent < 100) && (percent < m_progress);
    m_progress = percent;
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
}

int DolphinStatusBar::progress() const
{
    return m_progress;
}

void DolphinStatusBar::resetToDefaultText()
{
    QTime currentTime;
    if (currentTime.msecsTo(m_textTimestamp) < ResetToDefaultTimeout) {
        m_resetToDefaultTextTimer->start();
    } else {
        m_resetToDefaultTextTimer->stop();
        slotResetToDefaultText();
    }
}

void DolphinStatusBar::setDefaultText(const QString& text)
{
    m_defaultText = text;
    updateLabelText();
}

QString DolphinStatusBar::defaultText() const
{
    return m_defaultText;
}

void DolphinStatusBar::setUrl(const KUrl& url)
{
    m_spaceInfo->setUrl(url);
}

KUrl DolphinStatusBar::url() const
{
    return m_spaceInfo->url();
}

void DolphinStatusBar::setZoomLevel(int zoomLevel)
{
    if (zoomLevel != m_zoomSlider->value()) {
        m_zoomSlider->setValue(zoomLevel);
        updateZoomSliderToolTip(zoomLevel);
    }
}

int DolphinStatusBar::zoomLevel() const
{
    return m_zoomSlider->value();
}

void DolphinStatusBar::readSettings()
{
    setExtensionsVisible(true);
}

void DolphinStatusBar::contextMenuEvent(QContextMenuEvent* event)
{
    Q_UNUSED(event);

    KMenu menu(this);

    QAction* copyAction = menu.addAction(i18nc("@action:inmenu", "Copy Text"));
    QAction* showZoomSliderAction = menu.addAction(i18nc("@action:inmenu", "Show Zoom Slider"));
    showZoomSliderAction->setCheckable(true);
    showZoomSliderAction->setChecked(GeneralSettings::showZoomSlider());

    QAction* showSpaceInfoAction = menu.addAction(i18nc("@action:inmenu", "Show Space Information"));
    showSpaceInfoAction->setCheckable(true);
    showSpaceInfoAction->setChecked(GeneralSettings::showSpaceInfo());

    const QAction* action = menu.exec(QCursor::pos());
    if (action == copyAction) {
        QMimeData* mimeData = new QMimeData();
        mimeData->setText(text());
        QApplication::clipboard()->setMimeData(mimeData);
    } else if (action == showZoomSliderAction) {
        const bool visible = showZoomSliderAction->isChecked();
        GeneralSettings::setShowZoomSlider(visible);
        m_zoomSlider->setVisible(visible);
    } else if (action == showSpaceInfoAction) {
        const bool visible = showSpaceInfoAction->isChecked();
        GeneralSettings::setShowSpaceInfo(visible);
        m_spaceInfo->setVisible(visible);
    }
}

bool DolphinStatusBar::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_label && event->type() == QEvent::Resize) {
        updateLabelText();
    }
    return QWidget::eventFilter(obj, event);
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
    if (m_progress < 100) {
        // Show the progress information and hide the extensions
        m_stopButton->show();
        m_progressTextLabel->show();
        m_progressBar->show();
        setExtensionsVisible(false);
    } else {
        // Hide the progress information and show the extensions
        m_stopButton->hide();
        m_progressTextLabel->hide();
        m_progressBar->hide();
        setExtensionsVisible(true);
    }
}

void DolphinStatusBar::updateLabelText()
{
    const QString text = m_text.isEmpty() ? m_defaultText : m_text;

    QFontMetrics fontMetrics(m_label->font());
    const QString elidedText = fontMetrics.elidedText(text, Qt::ElideRight, m_label->width());
    m_label->setText(elidedText);
    m_label->setToolTip(text == elidedText ? QString() : text);
}

void DolphinStatusBar::slotResetToDefaultText()
{
    m_text.clear();
    updateLabelText();
}

void DolphinStatusBar::setExtensionsVisible(bool visible)
{
    bool showSpaceInfo = visible;
    bool showZoomSlider = visible;
    if (visible) {
        showSpaceInfo = GeneralSettings::showSpaceInfo();
        showZoomSlider = GeneralSettings::showZoomSlider();
    }
    m_spaceInfo->setVisible(showSpaceInfo);
    m_zoomSlider->setVisible(showZoomSlider);
}

void DolphinStatusBar::updateZoomSliderToolTip(int zoomLevel)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel);
    m_zoomSlider->setToolTip(i18ncp("@info:tooltip", "Size: 1 pixel", "Size: %1 pixels", size));
}

void DolphinStatusBar::applyFixedWidgetSize(QWidget* widget, const QSize& size)
{
    widget->setMinimumSize(size);
    widget->setMaximumSize(size);
    widget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

#include "dolphinstatusbar.moc"
