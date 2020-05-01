/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "informationpanelcontent.h"

#include <KIO/JobUiDelegate>
#include <KIO/PreviewJob>
#include <KIconEffect>
#include <KIconLoader>
#include <KJobWidgets>
#include <KLocalizedString>
#include <KSeparator>
#include <KStringHandler>
#include <QPainterPath>

#include <QIcon>
#include <QTextDocument>

#include <Baloo/FileMetaDataWidget>

#include <panels/places/placesitem.h>
#include <panels/places/placesitemmodel.h>

#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>

#include <QLabel>
#include <QDialogButtonBox>
#include <QScrollArea>
#include <QTextLayout>
#include <QTimer>
#include <QVBoxLayout>
#include <QStyle>
#include <QPainter>
#include <QBitmap>
#include <QLinearGradient>
#include <QPolygon>

#include "dolphin_informationpanelsettings.h"
#include "phononwidget.h"
#include "pixmapviewer.h"

const int PLAY_ARROW_SIZE = 24;
const int PLAY_ARROW_BORDER_SIZE = 2;

InformationPanelContent::InformationPanelContent(QWidget* parent) :
    QWidget(parent),
    m_item(),
    m_previewJob(nullptr),
    m_outdatedPreviewTimer(nullptr),
    m_preview(nullptr),
    m_phononWidget(nullptr),
    m_nameLabel(nullptr),
    m_metaDataWidget(nullptr),
    m_metaDataArea(nullptr),
    m_placesItemModel(nullptr),
    m_isVideo(false)
{
    parent->installEventFilter(this);

    // Initialize timer for disabling an outdated preview with a small
    // delay. This prevents flickering if the new preview can be generated
    // within a very small timeframe.
    m_outdatedPreviewTimer = new QTimer(this);
    m_outdatedPreviewTimer->setInterval(100);
    m_outdatedPreviewTimer->setSingleShot(true);
    connect(m_outdatedPreviewTimer, &QTimer::timeout,
            this, &InformationPanelContent::markOutdatedPreview);

    QVBoxLayout* layout = new QVBoxLayout(this);

    // preview
    const int minPreviewWidth = KIconLoader::SizeEnormous + KIconLoader::SizeMedium;

    m_preview = new PixmapViewer(parent);
    m_preview->setMinimumWidth(minPreviewWidth);
    m_preview->setMinimumHeight(KIconLoader::SizeEnormous);

    m_phononWidget = new PhononWidget(parent);
    m_phononWidget->hide();
    m_phononWidget->setMinimumWidth(minPreviewWidth);
    m_phononWidget->setAutoPlay(InformationPanelSettings::previewsAutoPlay());
    connect(m_phononWidget, &PhononWidget::hasVideoChanged,
            this, &InformationPanelContent::slotHasVideoChanged);

    // name
    m_nameLabel = new QLabel(parent);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    m_nameLabel->setFont(font);
    m_nameLabel->setTextFormat(Qt::PlainText);
    m_nameLabel->setAlignment(Qt::AlignHCenter);
    m_nameLabel->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    const bool previewsShown = InformationPanelSettings::previewsShown();
    m_preview->setVisible(previewsShown);

    m_metaDataWidget = new Baloo::FileMetaDataWidget(parent);
    m_metaDataWidget->setDateFormat(static_cast<Baloo::DateFormats>(InformationPanelSettings::dateFormat()));
    connect(m_metaDataWidget, &Baloo::FileMetaDataWidget::urlActivated,
            this, &InformationPanelContent::urlActivated);
    m_metaDataWidget->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    m_metaDataWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);

    // Configuration
    m_configureLabel = new QLabel(i18nc("@label::textbox",
                                        "Select which data should be shown:"), this);
    m_configureLabel->setWordWrap(true);
    m_configureLabel->setVisible(false);

    m_configureButtons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    m_configureButtons->setVisible(false);
    connect(m_configureButtons, &QDialogButtonBox::accepted, this, [this]() {
                m_metaDataWidget->setConfigurationMode(Baloo::ConfigurationMode::Accept);
                m_configureButtons->setVisible(false);
                m_configureLabel->setVisible(false);
                emit configurationFinished();
            }
    );
    connect(m_configureButtons, &QDialogButtonBox::rejected, this, [this]() {
                m_metaDataWidget->setConfigurationMode(Baloo::ConfigurationMode::Cancel);
                m_configureButtons->setVisible(false);
                m_configureLabel->setVisible(false);
                emit configurationFinished();
            }
    );

    m_metaDataArea = new QScrollArea(parent);
    m_metaDataArea->setWidget(m_metaDataWidget);
    m_metaDataArea->setWidgetResizable(true);
    m_metaDataArea->setFrameShape(QFrame::NoFrame);

    QWidget* viewport = m_metaDataArea->viewport();
    viewport->installEventFilter(this);

    layout->addWidget(m_preview);
    layout->addWidget(m_phononWidget);
    layout->addWidget(m_nameLabel);
    layout->addWidget(new KSeparator());
    layout->addWidget(m_configureLabel);
    layout->addWidget(m_metaDataArea);
    layout->addWidget(m_configureButtons);

    m_placesItemModel = new PlacesItemModel(this);
}

InformationPanelContent::~InformationPanelContent()
{
    InformationPanelSettings::self()->save();
}

void InformationPanelContent::showItem(const KFileItem& item)
{
    // compares item entries, comparing items only compares urls
    if (m_item.entry() != item.entry()) {
        m_item = item;
        m_preview->stopAnimatedImage();
        refreshMetaData();
    }

    refreshPreview();
}

void InformationPanelContent::refreshPixmapView()
{
    // If there is a preview job, kill it to prevent that we have jobs for
    // multiple items running, and thus a race condition (bug 250787).
    if (m_previewJob) {
        m_previewJob->kill();
    }

    // try to get a preview pixmap from the item...

    // Mark the currently shown preview as outdated. This is done
    // with a small delay to prevent a flickering when the next preview
    // can be shown within a short timeframe.
    m_outdatedPreviewTimer->start();

    QStringList plugins = KIO::PreviewJob::availablePlugins();
    m_previewJob = new KIO::PreviewJob(KFileItemList() << m_item,
                                       QSize(m_preview->width(), m_preview->height()),
                                       &plugins);
    m_previewJob->setScaleType(KIO::PreviewJob::Unscaled);
    m_previewJob->setIgnoreMaximumSize(m_item.isLocalFile());
    if (m_previewJob->uiDelegate()) {
        KJobWidgets::setWindow(m_previewJob, this);
    }

    connect(m_previewJob.data(), &KIO::PreviewJob::gotPreview,
            this, &InformationPanelContent::showPreview);
    connect(m_previewJob.data(), &KIO::PreviewJob::failed,
            this, &InformationPanelContent::showIcon);
}

void InformationPanelContent::refreshPreview()
{
    // If there is a preview job, kill it to prevent that we have jobs for
    // multiple items running, and thus a race condition (bug 250787).
    if (m_previewJob) {
        m_previewJob->kill();
    }

    m_preview->setCursor(Qt::ArrowCursor);
    setNameLabelText(m_item.text());
    if (InformationPanelSettings::previewsShown()) {

        const QUrl itemUrl = m_item.url();
        const bool isSearchUrl = itemUrl.scheme().contains(QLatin1String("search")) && m_item.localPath().isEmpty();
        if (isSearchUrl) {
            m_preview->show();
            m_phononWidget->hide();

            // in the case of a search-URL the URL is not readable for humans
            // (at least not useful to show in the Information Panel)
            m_preview->setPixmap(
                QIcon::fromTheme(QStringLiteral("baloo")).pixmap(m_preview->height(), m_preview->width())
            );
        } else {

            refreshPixmapView();

            const QString mimeType = m_item.mimetype();
            const bool isAnimatedImage = m_preview->isAnimatedImage(itemUrl.toLocalFile());
            m_isVideo = !isAnimatedImage && mimeType.startsWith(QLatin1String("video/"));
            bool usePhonon = m_isVideo || mimeType.startsWith(QLatin1String("audio/"));

            if (usePhonon) {
                // change the cursor of the preview
                m_preview->setCursor(Qt::PointingHandCursor);
                m_preview->installEventFilter(m_phononWidget);
                m_phononWidget->show();

                // if the video is playing, has been paused or stopped
                // we don't need to update the preview/phonon widget states
                // unless the previewed file has changed,
                // or the setting previewshown has changed
                if ((m_phononWidget->state() != Phonon::State::PlayingState &&
                     m_phononWidget->state() != Phonon::State::PausedState &&
                     m_phononWidget->state() != Phonon::State::StoppedState) ||
                        m_item.targetUrl() != m_phononWidget->url() ||
                        (!m_preview->isVisible() &&! m_phononWidget->isVisible())) {

                    if (InformationPanelSettings::previewsAutoPlay() && m_isVideo) {
                        // hides the preview now to avoid flickering when the autoplay video starts
                        m_preview->hide();
                    } else {
                        // the video won't play before the preview is displayed
                        m_preview->show();
                    }

                    m_phononWidget->setUrl(m_item.targetUrl(), m_isVideo ? PhononWidget::MediaKind::Video : PhononWidget::MediaKind::Audio);
                    adjustWidgetSizes(parentWidget()->width());
                }
            } else {
                if (isAnimatedImage) {
                    m_preview->setAnimatedImageFileName(itemUrl.toLocalFile());
                }
                // When we don't need it, hide the phonon widget first to avoid flickering
                m_phononWidget->hide();
                m_preview->show();
                m_preview->removeEventFilter(m_phononWidget);
                m_phononWidget->clearUrl();
            }
        }
    } else {
        m_preview->stopAnimatedImage();
        m_preview->hide();
        m_phononWidget->hide();
    }
}

void InformationPanelContent::configureShownProperties()
{
    m_configureLabel->setVisible(true);
    m_configureButtons->setVisible(true);
    m_metaDataWidget->setConfigurationMode(Baloo::ConfigurationMode::ReStart);
}

void InformationPanelContent::refreshMetaData()
{
    m_metaDataWidget->setDateFormat(static_cast<Baloo::DateFormats>(InformationPanelSettings::dateFormat()));
    m_metaDataWidget->show();
    m_metaDataWidget->setItems(KFileItemList() << m_item);
}

void InformationPanelContent::showItems(const KFileItemList& items)
{
    // If there is a preview job, kill it to prevent that we have jobs for
    // multiple items running, and thus a race condition (bug 250787).
    if (m_previewJob) {
        m_previewJob->kill();
    }

    m_preview->stopAnimatedImage();

    m_preview->setPixmap(
        QIcon::fromTheme(QStringLiteral("dialog-information")).pixmap(m_preview->height(), m_preview->width())
    );
    setNameLabelText(i18ncp("@label", "%1 item selected", "%1 items selected", items.count()));

    m_metaDataWidget->setItems(items);

    m_phononWidget->hide();

    m_item = KFileItem();
}

bool InformationPanelContent::eventFilter(QObject* obj, QEvent* event)
{
    switch (event->type()) {
    case QEvent::Resize: {
        QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
        if (obj == m_metaDataArea->viewport()) {
            // The size of the meta text area has changed. Adjust the fixed
            // width in a way that no horizontal scrollbar needs to be shown.
            m_metaDataWidget->setFixedWidth(resizeEvent->size().width());
        } else if (obj == parent()) {
            adjustWidgetSizes(resizeEvent->size().width());
        }
        break;
    }

    case QEvent::Polish:
        adjustWidgetSizes(parentWidget()->width());
        break;

    case QEvent::FontChange:
        m_metaDataWidget->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
        break;

    default:
        break;
    }

    return QWidget::eventFilter(obj, event);
}

void InformationPanelContent::showIcon(const KFileItem& item)
{
    m_outdatedPreviewTimer->stop();
    QPixmap pixmap = QIcon::fromTheme(item.iconName()).pixmap(m_preview->height(), m_preview->width());
    KIconLoader::global()->drawOverlays(item.overlays(), pixmap, KIconLoader::Desktop);
    m_preview->setPixmap(pixmap);
}

void InformationPanelContent::showPreview(const KFileItem& item,
                                          const QPixmap& pixmap)
{
    m_outdatedPreviewTimer->stop();

    QPixmap p = pixmap;
    KIconLoader::global()->drawOverlays(item.overlays(), p, KIconLoader::Desktop);

    if (m_isVideo) {
        // adds a play arrow

        // compute relative pixel positions
        const int zeroX = static_cast<int>(p.width() / 2 - PLAY_ARROW_SIZE / 2 / devicePixelRatio());
        const int zeroY = static_cast<int>(p.height() / 2 - PLAY_ARROW_SIZE / 2 / devicePixelRatio());

        QPolygon arrow;
        arrow << QPoint(zeroX, zeroY);
        arrow << QPoint(zeroX, zeroY + PLAY_ARROW_SIZE);
        arrow << QPoint(zeroX + PLAY_ARROW_SIZE, zeroY + PLAY_ARROW_SIZE / 2);

        QPainterPath path;
        path.addPolygon(arrow);

        QLinearGradient gradient(QPointF(zeroX, zeroY),
                                 QPointF(zeroX + PLAY_ARROW_SIZE,zeroY + PLAY_ARROW_SIZE));

        QColor whiteColor = Qt::white;
        QColor blackColor = Qt::black;
        gradient.setColorAt(0, whiteColor);
        gradient.setColorAt(1, blackColor);

        QBrush brush(gradient);

        QPainter painter(&p);

        QPen pen(blackColor, PLAY_ARROW_BORDER_SIZE, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        painter.setPen(pen);

        painter.setRenderHint(QPainter::Antialiasing);
        painter.drawPolygon(arrow);
        painter.fillPath(path, brush);
    }

    m_preview->setPixmap(p);
}

void InformationPanelContent::markOutdatedPreview()
{
    if (m_item.isDir()) {
        // directory preview can be long
        // but since we always have icons to display
        // use it until the preview is done
        showIcon(m_item);
    } else {
        KIconEffect *iconEffect = KIconLoader::global()->iconEffect();
        QPixmap disabledPixmap = iconEffect->apply(m_preview->pixmap(),
                                                   KIconLoader::Desktop,
                                                   KIconLoader::DisabledState);
        m_preview->setPixmap(disabledPixmap);
    }
}

KFileItemList InformationPanelContent::items()
{
    return m_metaDataWidget->items();
}

void InformationPanelContent::slotHasVideoChanged(bool hasVideo)
{
    m_preview->setVisible(InformationPanelSettings::previewsShown() && !hasVideo);
    if (m_preview->isVisible() && m_preview->size().width() != m_preview->pixmap().size().width()) {
        // in case the information panel has been resized when the preview was not displayed
        // we need to refresh its content
        refreshPixmapView();
    }
}

void InformationPanelContent::setPreviewAutoPlay(bool autoPlay) {
    m_phononWidget->setAutoPlay(autoPlay);
}

void InformationPanelContent::setNameLabelText(const QString& text)
{
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    const QString processedText = Qt::mightBeRichText(text) ? text : KStringHandler::preProcessWrap(text);

    QTextLayout textLayout(processedText);
    textLayout.setFont(m_nameLabel->font());
    textLayout.setTextOption(textOption);

    QString wrappedText;
    wrappedText.reserve(processedText.length());

    // wrap the text to fit into the width of m_nameLabel
    textLayout.beginLayout();
    QTextLine line = textLayout.createLine();
    while (line.isValid()) {
        line.setLineWidth(m_nameLabel->width());
        wrappedText += processedText.midRef(line.textStart(), line.textLength());

        line = textLayout.createLine();
        if (line.isValid()) {
            wrappedText += QChar::LineSeparator;
        }
    }
    textLayout.endLayout();

    m_nameLabel->setText(wrappedText);
}

void InformationPanelContent::adjustWidgetSizes(int width)
{
    // If the text inside the name label or the info label cannot
    // get wrapped, then the maximum width of the label is increased
    // so that the width of the information panel gets increased.
    // To prevent this, the maximum width is adjusted to
    // the current width of the panel.
    const int maxWidth = width - style()->layoutSpacing(QSizePolicy::DefaultType, QSizePolicy::DefaultType, Qt::Horizontal) * 4;
    m_nameLabel->setMaximumWidth(maxWidth);

    // The metadata widget also contains a text widget which may return
    // a large preferred width.
    m_metaDataWidget->setMaximumWidth(maxWidth);

    // try to increase the preview as large as possible
    m_preview->setSizeHint(QSize(maxWidth, maxWidth));

    if (m_phononWidget->isVisible()) {
        // assure that the size of the video player is the same as the preview size
        m_phononWidget->setVideoSize(QSize(maxWidth, maxWidth));
    }
}

