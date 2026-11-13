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
#include <QTextDocumentFragment>
#include <QTimer>
#include <QToolButton>

#include <chrono>

using namespace std::chrono_literals;

namespace
{
constexpr std::chrono::milliseconds minimumTimeBetweenTextChanges = 50ms;
constexpr std::chrono::seconds temporaryRichTextTimeout = 1s;
}

DolphinStatusBar::DolphinStatusBar(QWidget *parent)
    : AnimatedHeightWidget(parent)
    , m_temporaryRichText()
    , m_hoveredItemText()
    , m_defaultText()
    , m_label(nullptr)
    , m_zoomLabel(nullptr)
    , m_spaceInfo(nullptr)
    , m_zoomSlider(nullptr)
    , m_progressBar(nullptr)
    , m_stopButton(nullptr)
    , m_progress(100)
    , m_showProgressBarTimer(nullptr)
    , m_clearTemporaryRichTextTimer(nullptr)
    , m_updateLabelTextTimer(nullptr)
{
    setProperty("_breeze_statusbar_separator", true);
    setAutoFillBackground(true);

    QWidget *contentsContainer = prepareContentsContainer();
    contentsContainer->setContentsMargins(0, 0, 0, 0);
    contentsContainer->setAutoFillBackground(false);

    // Initialize text label
    m_label = new KSqueezedTextLabel{contentsContainer};
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

    // initialize timers for delayed replacing which text is shown
    m_clearTemporaryRichTextTimer = new QTimer(this);
    m_clearTemporaryRichTextTimer->setInterval(temporaryRichTextTimeout);
    m_clearTemporaryRichTextTimer->setSingleShot(true);
    connect(m_clearTemporaryRichTextTimer, &QTimer::timeout, this, &DolphinStatusBar::clearTemporaryRichText);

    m_updateLabelTextTimer = new QTimer(this);
    m_updateLabelTextTimer->setInterval(minimumTimeBetweenTextChanges);
    m_updateLabelTextTimer->setSingleShot(true);
    connect(m_updateLabelTextTimer, &QTimer::timeout, this, &DolphinStatusBar::updateLabelText);

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

    readSettings();
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
}

DolphinStatusBar::~DolphinStatusBar() = default;

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
    updateWidthToContent();
}

QString DolphinStatusBar::progressText() const
{
    return m_progressTextLabel->text();
}

int DolphinStatusBar::progress() const
{
    return m_progress;
}

void DolphinStatusBar::setTemporaryRichText(const QString &temporaryRichText)
{
    if (m_temporaryRichText == temporaryRichText) {
        return;
    }

    m_temporaryRichText = temporaryRichText;
    updateLabelText(); // Show the text instantly because we only show it temporarily anyway.
    m_clearTemporaryRichTextTimer->start();
}

void DolphinStatusBar::setHoveredItemText(const QString &hoveredItemText)
{
    m_hoveredItemText = hoveredItemText;
    m_updateLabelTextTimer->start();
}

void DolphinStatusBar::setDefaultText(const QString &text)
{
    m_defaultText = text;
    m_hoveredItemText.clear(); // We want to show the new default text instead of whatever was hovered.
    m_updateLabelTextTimer->start();
}

void DolphinStatusBar::setUrl(const QUrl &url)
{
    if (GeneralSettings::showStatusBar() == GeneralSettings::EnumShowStatusBar::FullWidth && m_spaceInfo && m_spaceInfo->url() != url) {
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
    if (GeneralSettings::showStatusBar() == GeneralSettings::EnumShowStatusBar::Small) {
        QStyleOptionSlider opt;
        opt.initFrom(this);
        opt.orientation = Qt::Vertical;
        const QSize labelSize = m_label->textFormat() == Qt::PlainText
            ? QFontMetrics(font()).size(Qt::TextSingleLine, m_label->fullText())
            : QFontMetrics(font()).size(Qt::TextSingleLine, QTextDocumentFragment::fromHtml(m_label->fullText()).toPlainText());
        // Make sure minimum height takes clipping into account.
        setMinimumHeight(m_label->height());
        const int scrollbarWidth = style()->pixelMetric(QStyle::PM_ScrollBarExtent, &opt, this);
        // Make sure maximumViewWidth does not go below 0.
        const int maximumViewWidth = qMax(0, parentWidget()->width() - scrollbarWidth);
        if (m_stopButton->isVisible() || m_progressTextLabel->isVisible() || m_progressBar->isVisible()) {
            // Use maximum width when interactable elements are shown, to keep them
            // from "jumping around" when user tries to interact with them.
            setFixedWidth(maximumViewWidth);
        } else {
            // Make sure we have room for the text
            const int contentWidth = labelSize.width() + QFontMetrics(font()).averageCharWidth();
            setFixedWidth(qMin(contentWidth, maximumViewWidth));
        }
        Q_EMIT widthUpdated();
    } else {
        setMinimumHeight(0);
        setFixedWidth(QWIDGETSIZE_MAX);
        Q_EMIT widthUpdated();
    }
}

void DolphinStatusBar::updateMode()
{
    switch (GeneralSettings::showStatusBar()) {
    case GeneralSettings::EnumShowStatusBar::Small:
        setEnabled(true);
        m_spaceInfo->setShown(false);
        m_zoomSlider->setVisible(false);
        m_zoomLabel->setVisible(false);
        setVisible(true, WithAnimation);
        break;
    case GeneralSettings::EnumShowStatusBar::FullWidth:
        setEnabled(true);
        m_spaceInfo->setShown(true);
        setVisible(true, WithAnimation);
        break;
    case GeneralSettings::EnumShowStatusBar::Disabled:
        setEnabled(false);
        setVisible(false, WithoutAnimation);
        break;
    }
    Q_EMIT modeUpdated();
    updateWidthToContent();
}

void DolphinStatusBar::contextMenuEvent(QContextMenuEvent *event)
{
    Q_UNUSED(event)

    // Do not show the context menu on small statusbar.
    if (GeneralSettings::showStatusBar() == GeneralSettings::EnumShowStatusBar::Small) {
        return;
    }

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
    updateWidthToContent();
}

void DolphinStatusBar::clearTemporaryRichText()
{
    if (m_clearTemporaryRichTextTimer->isActive()) {
        return;
    }
    m_temporaryRichText.clear();
    m_updateLabelTextTimer->start();
}

void DolphinStatusBar::updateLabelText()
{
    if (!m_temporaryRichText.isEmpty()) {
        m_label->setTextFormat(Qt::RichText);
        m_label->setTextElideMode(Qt::ElideNone);
        m_label->setText(m_temporaryRichText);
    } else {
        m_label->setTextFormat(Qt::PlainText);
        m_label->setTextElideMode(Qt::ElideMiddle);
        m_label->setText(m_hoveredItemText.isEmpty() ? m_defaultText : m_hoveredItemText);
    }
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
        showZoomSlider = GeneralSettings::showZoomSlider() && GeneralSettings::showStatusBar() == GeneralSettings::EnumShowStatusBar::FullWidth;
    }

    m_zoomSlider->setVisible(showZoomSlider);
    m_zoomLabel->setVisible(showZoomSlider);
    updateContentsMargins();
}

void DolphinStatusBar::updateContentsMargins()
{
    // We reduce the outside margin for the flat button so it visually has the same margin as the status bar text label on the other end of the bar.
    m_topLayout->setContentsMargins(4, 0, 2, 0);
    setContentsMargins(0, 0, 0, 0);
}

void DolphinStatusBar::paintEvent(QPaintEvent *paintEvent)
{
    Q_UNUSED(paintEvent)
    QPainter p(this);
    QStyleOptionFrame opt;
    opt.initFrom(this);
    // Draw statusbar only if there is text.
    if (GeneralSettings::showStatusBar() == GeneralSettings::EnumShowStatusBar::Small) {
        if (m_label && !m_label->fullText().isEmpty()) {
            opt.palette.setColor(QPalette::Base, palette().window().color());
            opt.frameShape = QFrame::StyledPanel;
            opt.state = QStyle::State_Sunken; // Breeze QStyle needs this to draw background.
            opt.lineWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);
            style()->drawPrimitive(QStyle::PE_Frame, &opt, &p, this);
        }
    }
    // Draw regular statusbar.
    else {
        style()->drawPrimitive(QStyle::PE_PanelStatusBar, &opt, &p, this);
    }
}

int DolphinStatusBar::preferredHeight() const
{
    return m_spaceInfo->height();
}

#include "moc_dolphinstatusbar.cpp"
