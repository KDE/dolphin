/*
 * SPDX-FileCopyrightText: 2006-2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinstatusbar.h"

#include "dolphin_generalsettings.h"
#include "statusbarspaceinfo.h"
#include "views/dolphinview.h"
#include "views/zoomlevelinfo.h"

#include <KLocalizedString>
#include <KSqueezedTextLabel>

#include <QApplication>
#include <QHBoxLayout>
#include <QHelpEvent>
#include <QIcon>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QProgressBar>
#include <QSlider>
#include <QStyleOption>
#include <QTimer>
#include <QToolButton>

namespace
{
const int UpdateDelay = 50;
}

DolphinStatusBar::DolphinStatusBar(QWidget *parent)
    : AnimatedHeightWidget(parent)
    , m_text()
    , m_defaultText()
    , m_label(nullptr)
    , m_zoomLabel(nullptr)
    , m_spaceInfo(nullptr)
    , m_zoomSlider(nullptr)
    , m_progressBar(nullptr)
    , m_stopButton(nullptr)
    , m_progress(100)
    , m_showProgressBarTimer(nullptr)
    , m_delayUpdateTimer(nullptr)
    , m_textTimestamp()
{
    setProperty("_breeze_statusbar_separator", true);

    QWidget *contentsContainer = prepareContentsContainer();

    // Initialize text label
    m_label = new KSqueezedTextLabel(m_text, contentsContainer);
    m_label->setTextFormat(Qt::PlainText);
    m_label->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::TextSelectableByKeyboard); // for accessibility but also to allow copy-pasting this text.

    // Initialize zoom slider's explanatory label
    m_zoomLabel = new KSqueezedTextLabel(i18nc("Used as a noun, i.e. 'Here is the zoom level:'", "Zoom:"), contentsContainer);

    // Initialize zoom widget
    m_zoomSlider = new QSlider(Qt::Horizontal, contentsContainer);
    m_zoomSlider->setAccessibleName(i18n("Zoom"));
    m_zoomSlider->setAccessibleDescription(i18nc("Description for zoom-slider (accessibility)", "Sets the size of the file icons."));
    m_zoomSlider->setPageStep(1);
    m_zoomSlider->setRange(ZoomLevelInfo::minimumLevel(), ZoomLevelInfo::maximumLevel());

    connect(m_zoomSlider, &QSlider::valueChanged, this, &DolphinStatusBar::zoomLevelChanged);
    connect(m_zoomSlider, &QSlider::valueChanged, this, &DolphinStatusBar::updateZoomSliderToolTip);
    connect(m_zoomSlider, &QSlider::sliderMoved, this, &DolphinStatusBar::showZoomSliderToolTip);

    // Initialize space information
    m_spaceInfo = new StatusBarSpaceInfo(contentsContainer);
    connect(m_spaceInfo, &StatusBarSpaceInfo::showMessage, this, &DolphinStatusBar::showMessage);
    connect(m_spaceInfo,
            &StatusBarSpaceInfo::showInstallationProgress,
            this,
            [this](const QString &currentlyRunningTaskTitle, int installationProgressPercent) {
                showProgress(currentlyRunningTaskTitle, installationProgressPercent, CancelLoading::Disallowed);
            });

    // Initialize progress information
    m_stopButton = new QToolButton(contentsContainer);
    m_stopButton->setIcon(QIcon::fromTheme(QStringLiteral("process-stop")));
    m_stopButton->setAccessibleName(i18n("Stop"));
    m_stopButton->setAutoRaise(true);
    m_stopButton->setToolTip(i18nc("@tooltip", "Stop loading"));
    m_stopButton->hide();
    connect(m_stopButton, &QToolButton::clicked, this, &DolphinStatusBar::stopPressed);

    m_progressTextLabel = new QLabel(contentsContainer);
    m_progressTextLabel->hide();

    m_progressBar = new QProgressBar(contentsContainer);
    m_progressBar->hide();

    m_showProgressBarTimer = new QTimer(this);
    m_showProgressBarTimer->setInterval(500);
    m_showProgressBarTimer->setSingleShot(true);
    connect(m_showProgressBarTimer, &QTimer::timeout, this, &DolphinStatusBar::updateProgressInfo);

    // initialize text updater delay timer
    m_delayUpdateTimer = new QTimer(this);
    m_delayUpdateTimer->setInterval(UpdateDelay);
    m_delayUpdateTimer->setSingleShot(true);
    connect(m_delayUpdateTimer, &QTimer::timeout, this, &DolphinStatusBar::updateLabelText);

    // Initialize top layout and size policies
    const int fontHeight = QFontMetrics(m_label->font()).height();
    const int zoomSliderHeight = m_zoomSlider->minimumSizeHint().height();
    const int buttonHeight = m_stopButton->height();
    const int contentHeight = qMax(qMax(fontHeight, zoomSliderHeight), buttonHeight);

    QFontMetrics fontMetrics(m_label->font());

    m_label->setFixedHeight(contentHeight);
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_zoomSlider->setMaximumWidth(fontMetrics.averageCharWidth() * 15);

    m_spaceInfo->setFixedHeight(contentHeight);
    m_spaceInfo->setMaximumWidth(fontMetrics.averageCharWidth() * 25);
    m_spaceInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    m_progressBar->setFixedHeight(zoomSliderHeight);
    m_progressBar->setMaximumWidth(fontMetrics.averageCharWidth() * 20);

    m_topLayout = new QHBoxLayout(contentsContainer);
    updateContentsMargins();
    m_topLayout->setSpacing(4);
    m_topLayout->addWidget(m_label, 1);
    m_topLayout->addWidget(m_zoomLabel);
    m_topLayout->addWidget(m_zoomSlider, 1);
    m_topLayout->addWidget(m_spaceInfo, 1);
    m_topLayout->addWidget(m_stopButton);
    m_topLayout->addWidget(m_progressTextLabel);
    m_topLayout->addWidget(m_progressBar);

    updateMode();

    setExtensionsVisible(true);
    setWhatsThis(xi18nc("@info:whatsthis Statusbar",
                        "<para>This is "
                        "the <emphasis>Statusbar</emphasis>. It contains three elements "
                        "by default (left to right):<list><item>A <emphasis>text field"
                        "</emphasis> that displays the size of selected items. If only "
                        "one item is selected the name and type is shown as well.</item>"
                        "<item>A <emphasis>zoom slider</emphasis> that allows you "
                        "to adjust the size of the icons in the view.</item>"
                        "<item><emphasis>Space information</emphasis> about the "
                        "current storage device.</item></list></para>"));

    connect(this, &DolphinStatusBar::modeUpdated, this, [this]() {
        updateWidthToContent();
    });
}

DolphinStatusBar::~DolphinStatusBar()
{
}

void DolphinStatusBar::setText(const QString &text)
{
    if (m_text == text) {
        return;
    }

    m_textTimestamp = QTime::currentTime();

    m_text = text;
    // will update status bar text in 50ms
    m_delayUpdateTimer->start();
}

QString DolphinStatusBar::text() const
{
    return m_text;
}

void DolphinStatusBar::showProgress(const QString &currentlyRunningTaskTitle, int progressPercent, CancelLoading cancelLoading)
{
    m_cancelLoading = cancelLoading;

    // Show a busy indicator if a value < 0 is provided:
    m_progressBar->setMaximum((progressPercent < 0) ? 0 : 100);

    progressPercent = qBound(0, progressPercent, 100);
    if (!m_progressBar->isVisible()) {
        // Show the progress bar delayed: In the case if 100 % are reached within
        // a short time, no progress bar will be shown at all.
        if (!m_showProgressBarTimer->isActive()) {
            m_showProgressBarTimer->start();
        } else {
            // The timer is already running. Should we restart it or keep it running?
            if (m_progressTextLabel->text() != currentlyRunningTaskTitle || (progressPercent < 100 && progressPercent < m_progress)) {
                m_showProgressBarTimer->start();
            }
        }
    }
    m_progress = progressPercent;

    m_progressBar->setValue(m_progress);
    if (progressPercent == 100) {
        // The end of the progress has been reached. Assure that the progress bar
        // gets hidden and the extensions widgets get visible again.
        m_showProgressBarTimer->stop();
        updateProgressInfo();
    }

    m_progressTextLabel->setText(currentlyRunningTaskTitle);
}

QString DolphinStatusBar::progressText() const
{
    return m_progressTextLabel->text();
}

int DolphinStatusBar::progress() const
{
    return m_progress;
}

void DolphinStatusBar::resetToDefaultText()
{
    m_text.clear();

    QTime currentTime;
    if (currentTime.msecsTo(m_textTimestamp) < UpdateDelay) {
        m_delayUpdateTimer->start();
    } else {
        updateLabelText();
    }
}

void DolphinStatusBar::setDefaultText(const QString &text)
{
    m_defaultText = text;
    updateLabelText();
}

QString DolphinStatusBar::defaultText() const
{
    return m_defaultText;
}

void DolphinStatusBar::setUrl(const QUrl &url)
{
    if (m_mode == StatusBarMode::FullWidth) {
        m_spaceInfo->setUrl(url);
        Q_EMIT urlChanged();
    }
}

QUrl DolphinStatusBar::url() const
{
    return m_spaceInfo->url();
}

void DolphinStatusBar::setZoomLevel(int zoomLevel)
{
    if (zoomLevel != m_zoomSlider->value()) {
        m_zoomSlider->setValue(zoomLevel);
    }
}

int DolphinStatusBar::zoomLevel() const
{
    return m_zoomSlider->value();
}

void DolphinStatusBar::readSettings()
{
    updateMode();
    setExtensionsVisible(true);
}

void DolphinStatusBar::updateSpaceInfo()
{
    m_spaceInfo->update();
}

void DolphinStatusBar::updateWidthToContent()
{
    if (m_mode == StatusBarMode::Small) {
        setMinimumHeight(35);
        setContentsMargins(5, 0, 0, 2);
        const int textWidth = QFontMetrics(font()).size(Qt::TextSingleLine, m_label->fullText()).width() + 20;
        const int maximumViewWidth = parentWidget()->width() / 2;
        setFixedWidth(qMin(textWidth, maximumViewWidth));
        Q_EMIT widthUpdated();
    } else {
        setMinimumHeight(0);
        setContentsMargins(0, 0, 0, 0);
        setFixedWidth(QWIDGETSIZE_MAX);
        Q_EMIT widthUpdated();
    }
}

void DolphinStatusBar::updateMode()
{
    switch (GeneralSettings::showStatusBar()) {
    case GeneralSettings::EnumShowStatusBar::Small:
        setMode(Small);
        m_spaceInfo->setShown(false);
        m_zoomSlider->setVisible(false);
        m_zoomLabel->setVisible(false);
        setVisible(true, WithAnimation);
        break;
    case GeneralSettings::EnumShowStatusBar::FullWidth:
        setMode(FullWidth);
        m_spaceInfo->setShown(true);
        setVisible(true, WithAnimation);
        break;
    case GeneralSettings::EnumShowStatusBar::Disabled:
        setVisible(false, WithoutAnimation);
        break;
    }
    setAttribute(Qt::WA_TransparentForMouseEvents, GeneralSettings::showStatusBar() == GeneralSettings::EnumShowStatusBar::Small);
}

DolphinStatusBar::StatusBarMode DolphinStatusBar::mode()
{
    return m_mode;
}

void DolphinStatusBar::setMode(StatusBarMode mode)
{
    if (m_mode != mode) {
        m_mode = mode;
        Q_EMIT modeUpdated();
    }
}

void DolphinStatusBar::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event)

    QMenu menu(this);

    QAction *showZoomSliderAction = menu.addAction(i18nc("@action:inmenu", "Show Zoom Slider"));
    showZoomSliderAction->setCheckable(true);
    showZoomSliderAction->setChecked(GeneralSettings::showZoomSlider());

    const QAction *action = menu.exec(event->reason() == QContextMenuEvent::Reason::Mouse ? QCursor::pos() : mapToGlobal(QPoint(width() / 2, height() / 2)));
    if (action == showZoomSliderAction) {
        const bool visible = showZoomSliderAction->isChecked();
        GeneralSettings::setShowZoomSlider(visible);
        m_zoomSlider->setVisible(visible);
        m_zoomLabel->setVisible(visible);
    }
    updateContentsMargins();
    updateMode();
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
        m_stopButton->setVisible(m_cancelLoading == CancelLoading::Allowed);
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
    m_label->setText(text);
    updateMode();
    updateWidthToContent();
}

void DolphinStatusBar::updateZoomSliderToolTip(int zoomLevel)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel);
    m_zoomSlider->setToolTip(i18ncp("@info:tooltip", "Size: 1 pixel", "Size: %1 pixels", size));
}

void DolphinStatusBar::setExtensionsVisible(bool visible)
{
    bool showZoomSlider = visible;
    if (visible) {
        showZoomSlider = GeneralSettings::showZoomSlider();
    }

    m_zoomSlider->setVisible(showZoomSlider);
    m_zoomLabel->setVisible(showZoomSlider);
    updateContentsMargins();
    updateMode();
}

void DolphinStatusBar::updateContentsMargins()
{
    // We reduce the outside margin for the flat button so it visually has the same margin as the status bar text label on the other end of the bar.
    m_topLayout->setContentsMargins(6, 0, 2, 0);
}

void DolphinStatusBar::paintEvent(QPaintEvent *paintEvent)
{
    Q_UNUSED(paintEvent)
    QPainter p(this);
    QStyleOption opt;
    opt.initFrom(this);
    // Draw statusbar only if there is text
    if (m_mode == StatusBarMode::Small) {
        if (m_label && !m_label->fullText().isEmpty()) {
            opt.state = QStyle::State_Sunken;
            QPainterPath path;
            // Clip the left and bottom border off
            QRect clipRect;
            if (layoutDirection() == Qt::RightToLeft) {
                clipRect = QRect(opt.rect.topLeft(), opt.rect.bottomRight()).adjusted(0, 0, -4, -4);
            } else {
                clipRect = QRect(opt.rect.topLeft(), opt.rect.bottomRight()).adjusted(4, 0, 0, -4);
            }
            path.addRect(clipRect);
            p.setClipPath(path);
            style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);
        }
    }
    // Draw regular statusbar
    else {
        style()->drawPrimitive(QStyle::PE_PanelStatusBar, &opt, &p, this);
    }
}

int DolphinStatusBar::preferredHeight() const
{
    return m_spaceInfo->height();
}

#include "moc_dolphinstatusbar.cpp"
