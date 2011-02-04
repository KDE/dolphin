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

#include <KDialog>
#include <KFileItem>
#include <KFileMetaDataWidget>
#include <KFilePlacesModel>
#include <KGlobalSettings>
#include <KIO/PreviewJob>
#include <KIconEffect>
#include <KIconLoader>
#include <KLocale>
#include <KMenu>
#include <kseparator.h>
#include <KStringHandler>

#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>
#include <Phonon/SeekSlider>

#include <QEvent>
#include <QLabel>
#include <QPixmap>
#include <QPointer>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTextDocument>
#include <QTextLayout>
#include <QTextLine>
#include <QTimer>
#include <QVBoxLayout>

#include "dolphin_informationpanelsettings.h"
#include "filemetadataconfigurationdialog.h"
#include "settings/dolphinsettings.h"
#include "phononwidget.h"
#include "pixmapviewer.h"

InformationPanelContent::InformationPanelContent(QWidget* parent) :
    QWidget(parent),
    m_item(),
    m_pendingPreview(false),
    m_outdatedPreviewTimer(0),
    m_preview(0),
    m_phononWidget(0),
    m_nameLabel(0),
    m_metaDataWidget(0),
    m_metaDataArea(0),
    m_enabledPlugins()
{
    parent->installEventFilter(this);

    // Initialize timer for disabling an outdated preview with a small
    // delay. This prevents flickering if the new preview can be generated
    // within a very small timeframe.
    m_outdatedPreviewTimer = new QTimer(this);
    m_outdatedPreviewTimer->setInterval(300);
    m_outdatedPreviewTimer->setSingleShot(true);
    connect(m_outdatedPreviewTimer, SIGNAL(timeout()),
            this, SLOT(markOutdatedPreview()));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setSpacing(KDialog::spacingHint());

    // preview
    const int minPreviewWidth = KIconLoader::SizeEnormous + KIconLoader::SizeMedium;

    m_preview = new PixmapViewer(parent);
    m_preview->setMinimumWidth(minPreviewWidth);
    m_preview->setMinimumHeight(KIconLoader::SizeEnormous);

    m_phononWidget = new PhononWidget(parent);
    m_phononWidget->hide();
    m_phononWidget->setMinimumWidth(minPreviewWidth);
    connect(m_phononWidget, SIGNAL(playingStarted()),
            this, SLOT(slotPlayingStarted()));
    connect(m_phononWidget, SIGNAL(playingStopped()),
            this, SLOT(slotPlayingStopped()));

    // name
    m_nameLabel = new QLabel(parent);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    m_nameLabel->setFont(font);
    m_nameLabel->setAlignment(Qt::AlignHCenter);
    m_nameLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

    const bool showPreview = InformationPanelSettings::showPreview();
    m_preview->setVisible(showPreview);

    m_metaDataWidget = new KFileMetaDataWidget(parent);
    m_metaDataWidget->setFont(KGlobalSettings::smallestReadableFont());
    m_metaDataWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    connect(m_metaDataWidget, SIGNAL(urlActivated(KUrl)), this, SIGNAL(urlActivated(KUrl)));

    // Encapsulate the MetaDataWidget inside a container that has a dummy widget
    // at the bottom. This prevents that the meta data widget gets vertically stretched
    // in the case where the height of m_metaDataArea > m_metaDataWidget.
    QWidget* metaDataWidgetContainer = new QWidget(parent);
    QVBoxLayout* containerLayout = new QVBoxLayout(metaDataWidgetContainer);
    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    containerLayout->addWidget(m_metaDataWidget);
    containerLayout->addStretch();

    m_metaDataArea = new QScrollArea(parent);
    m_metaDataArea->setWidget(metaDataWidgetContainer);
    m_metaDataArea->setWidgetResizable(true);
    m_metaDataArea->setFrameShape(QFrame::NoFrame);

    QWidget* viewport = m_metaDataArea->viewport();
    viewport->installEventFilter(this);

    QPalette palette = viewport->palette();
    palette.setColor(viewport->backgroundRole(), QColor(Qt::transparent));
    viewport->setPalette(palette);

    layout->addWidget(m_preview);
    layout->addWidget(m_phononWidget);
    layout->addWidget(m_nameLabel);
    layout->addWidget(new KSeparator());
    layout->addWidget(m_metaDataArea);
}

InformationPanelContent::~InformationPanelContent()
{
    InformationPanelSettings::self()->writeConfig();
}

void InformationPanelContent::showItem(const KFileItem& item)
{
    m_pendingPreview = false;

    const KUrl itemUrl = item.url();
    const bool isSearchUrl = itemUrl.protocol().contains("search") && item.nepomukUri().isEmpty();
    if (!applyPlace(itemUrl)) {
        setNameLabelText(item.text());
        if (isSearchUrl) {
            // in the case of a search-URL the URL is not readable for humans
            // (at least not useful to show in the Information Panel)
            KIconLoader iconLoader;
            QPixmap icon = iconLoader.loadIcon("nepomuk",
                                               KIconLoader::NoGroup,
                                               KIconLoader::SizeEnormous);
            m_preview->setPixmap(icon);
        } else {
            // try to get a preview pixmap from the item...
            m_pendingPreview = true;

            // Mark the currently shown preview as outdated. This is done
            // with a small delay to prevent a flickering when the next preview
            // can be shown within a short timeframe. This timer is not started
            // for directories, as directory previews might fail and return the
            // same icon.
            if (!item.isDir()) {
                m_outdatedPreviewTimer->start();
            }

            if (m_enabledPlugins.isEmpty()) {
                const KConfigGroup globalConfig(KGlobal::config(), "PreviewSettings");
                m_enabledPlugins = globalConfig.readEntry("Plugins", QStringList()
                                                                     << "directorythumbnail"
                                                                     << "imagethumbnail"
                                                                     << "jpegthumbnail");
            }

            KIO::PreviewJob* job = KIO::filePreview(KFileItemList() << item, m_preview->width(), m_preview->height(),
                                                    0, 0, false, true, &m_enabledPlugins);

            connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
                    this, SLOT(showPreview(const KFileItem&, const QPixmap&)));
            connect(job, SIGNAL(failed(const KFileItem&)),
                    this, SLOT(showIcon(const KFileItem&)));
        }
    }

    if (m_metaDataWidget != 0) {
        m_metaDataWidget->show();
        m_metaDataWidget->setItems(KFileItemList() << item);
    }

    if (InformationPanelSettings::showPreview()) {
        const QString mimeType = item.mimetype();
        const bool usePhonon = Phonon::BackendCapabilities::isMimeTypeAvailable(mimeType) &&
                               (mimeType != "image/png");  // TODO: workaround, as Phonon
                                                           // thinks it supports PNG images
        if (usePhonon) {
            m_phononWidget->show();
            PhononWidget::Mode mode = mimeType.startsWith(QLatin1String("video"))
                                      ? PhononWidget::Video
                                      : PhononWidget::Audio;
            m_phononWidget->setMode(mode);
            m_phononWidget->setUrl(item.targetUrl());
            if ((mode == PhononWidget::Video) && m_preview->isVisible()) {
                m_phononWidget->setVideoSize(m_preview->size());
            }
        } else {
            m_phononWidget->hide();
            m_preview->setVisible(true);
        }
    } else {
        m_phononWidget->hide();
    }

    m_item = item;
}

void InformationPanelContent::showItems(const KFileItemList& items)
{
    m_pendingPreview = false;

    KIconLoader iconLoader;
    QPixmap icon = iconLoader.loadIcon("dialog-information",
                                       KIconLoader::NoGroup,
                                       KIconLoader::SizeEnormous);
    m_preview->setPixmap(icon);
    setNameLabelText(i18ncp("@info", "%1 item selected", "%1 items selected", items.count()));

    if (m_metaDataWidget != 0) {
        m_metaDataWidget->setItems(items);
    }

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

    default:
        break;
    }

    return QWidget::eventFilter(obj, event);
}

void InformationPanelContent::configureSettings(const QList<QAction*>& customContextMenuActions)
{
    KMenu popup(this);

    QAction* previewAction = popup.addAction(i18nc("@action:inmenu", "Preview"));
    previewAction->setIcon(KIcon("view-preview"));
    previewAction->setCheckable(true);
    previewAction->setChecked(InformationPanelSettings::showPreview());

    QAction* configureAction = popup.addAction(i18nc("@action:inmenu", "Configure..."));
    configureAction->setIcon(KIcon("configure"));

    popup.addSeparator();
    foreach (QAction* action, customContextMenuActions) {
        popup.addAction(action);
    }

    // Open the popup and adjust the settings for the
    // selected action.
    QAction* action = popup.exec(QCursor::pos());
    if (action == 0) {
        return;
    }

    const bool isChecked = action->isChecked();
    if (action == previewAction) {
        m_preview->setVisible(isChecked);
        InformationPanelSettings::setShowPreview(isChecked);
    } else if (action == configureAction) {
        FileMetaDataConfigurationDialog* dialog = new FileMetaDataConfigurationDialog();
        dialog->setDescription(i18nc("@label::textbox",
                                     "Select which data should be shown in the information panel:"));
        dialog->setItems(m_metaDataWidget->items());
        dialog->setAttribute(Qt::WA_DeleteOnClose);
        dialog->show();
        dialog->raise();
        dialog->activateWindow();
        connect(dialog, SIGNAL(destroyed()), this, SLOT(refreshMetaData()));
    }
}

void InformationPanelContent::showIcon(const KFileItem& item)
{
    m_outdatedPreviewTimer->stop();
    m_pendingPreview = false;
    if (!applyPlace(item.targetUrl())) {
        m_preview->setPixmap(item.pixmap(KIconLoader::SizeEnormous));
    }
}

void InformationPanelContent::showPreview(const KFileItem& item,
                                          const QPixmap& pixmap)
{
    m_outdatedPreviewTimer->stop();

    Q_UNUSED(item);
    if (m_pendingPreview) {
        m_preview->setPixmap(pixmap);
        m_pendingPreview = false;
    }
}

void InformationPanelContent::markOutdatedPreview()
{
    KIconEffect *iconEffect = KIconLoader::global()->iconEffect();
    QPixmap disabledPixmap = iconEffect->apply(m_preview->pixmap(),
                                               KIconLoader::Desktop,
                                               KIconLoader::DisabledState);
    m_preview->setPixmap(disabledPixmap);
}

void InformationPanelContent::slotPlayingStarted()
{
    m_preview->setVisible(m_phononWidget->mode() != PhononWidget::Video);
}

void InformationPanelContent::slotPlayingStopped()
{
    m_preview->setVisible(true);
}

void InformationPanelContent::refreshMetaData()
{
    if (!m_item.isNull() && m_item.nepomukUri().isValid()) {
        showItem(m_item);
    }
}

bool InformationPanelContent::applyPlace(const KUrl& url)
{
    KFilePlacesModel* placesModel = DolphinSettings::instance().placesModel();
    int count = placesModel->rowCount();

    for (int i = 0; i < count; ++i) {
        QModelIndex index = placesModel->index(i, 0);

        if (url.equals(placesModel->url(index), KUrl::CompareWithoutTrailingSlash)) {
            setNameLabelText(placesModel->text(index));
            m_preview->setPixmap(placesModel->icon(index).pixmap(128, 128));
            return true;
        }
    }

    return false;
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
        wrappedText += processedText.mid(line.textStart(), line.textLength());

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
    const int maxWidth = width - KDialog::spacingHint() * 4;
    m_nameLabel->setMaximumWidth(maxWidth);

    // The metadata widget also contains a text widget which may return
    // a large preferred width.
    if (m_metaDataWidget != 0) {
        m_metaDataWidget->setMaximumWidth(maxWidth);
    }

    // try to increase the preview as large as possible
    m_preview->setSizeHint(QSize(maxWidth, maxWidth));

    if (m_phononWidget->isVisible() && (m_phononWidget->mode() == PhononWidget::Video)) {
        // assure that the size of the video player is the same as the preview size
        m_phononWidget->setVideoSize(QSize(maxWidth, maxWidth));
    }
}

#include "informationpanelcontent.moc"
