/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#include "informationpanel.h"

#include <config-nepomuk.h>

#include <kdialog.h>
#include <kdirnotify.h>
#include <kfileitem.h>
#include <kfilemetainfo.h>
#include <kfileplacesmodel.h>
#include <kglobalsettings.h>
#include <kio/previewjob.h>
#include <kiconeffect.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmenu.h>
#include <kseparator.h>

#ifdef HAVE_NEPOMUK

#define DISABLE_NEPOMUK_LEGACY

#include <Nepomuk/Resource>
#include <Nepomuk/Types/Property>
#include <Nepomuk/Variant>
#endif

#include <Phonon/BackendCapabilities>
#include <Phonon/MediaObject>
#include <Phonon/SeekSlider>

#include <QEvent>
#include <QInputDialog>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QResizeEvent>
#include <QScrollArea>
#include <QTextLayout>
#include <QTextLine>
#include <QTimer>
#include <QScrollBar>
#include <QVBoxLayout>

#include "dolphin_informationpanelsettings.h"
#include "settings/dolphinsettings.h"
#include "metadatawidget.h"
#include "metatextlabel.h"
#include "phononwidget.h"
#include "pixmapviewer.h"

/**
 * Helper function for sorting items with qSort() in
 * InformationPanel::contextMenu().
 */
bool lessThan(const QAction* action1, const QAction* action2)
{
    return action1->text() < action2->text();
}


InformationPanel::InformationPanel(QWidget* parent) :
    Panel(parent),
    m_initialized(false),
    m_pendingPreview(false),
    m_infoTimer(0),
    m_outdatedPreviewTimer(0),
    m_shownUrl(),
    m_urlCandidate(),
    m_fileItem(),
    m_selection(),
    m_nameLabel(0),
    m_preview(0),
    m_previewSeparator(0),
    m_phononWidget(0),
    m_metaDataWidget(0),
    m_metaDataSeparator(0),
    m_metaTextArea(0),
    m_metaTextLabel(0)
{
}

InformationPanel::~InformationPanel()
{
    InformationPanelSettings::self()->writeConfig();
}

QSize InformationPanel::sizeHint() const
{
    QSize size = Panel::sizeHint();
    size.setWidth(minimumSizeHint().width());
    return size;
}

void InformationPanel::setUrl(const KUrl& url)
{
    Panel::setUrl(url);
    if (url.isValid() && !isEqualToShownUrl(url)) {
        if (isVisible()) {
            cancelRequest();
            m_shownUrl = url;
            showItemInfo();
        } else {
            m_shownUrl = url;
        }
    }
}

void InformationPanel::setSelection(const KFileItemList& selection)
{
    if (!isVisible()) {
        return;
    }

    if ((selection.count() == 0) && (m_selection.count() == 0)) {
        // The selection has not really changed, only the current index.
        // QItemSelectionModel emits a signal in this case and it is less
        // expensive doing the check this way instead of patching
        // DolphinView::emitSelectionChanged().
        return;
    }

    m_selection = selection;

    const int count = selection.count();
    if (count == 0) {
        if (!isEqualToShownUrl(url())) {
            m_shownUrl = url();
            showItemInfo();
        }
    } else {
        if ((count == 1) && !selection.first().url().isEmpty()) {
            m_urlCandidate = selection.first().url();
        }
        m_infoTimer->start();
    }
}

void InformationPanel::requestDelayedItemInfo(const KFileItem& item)
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();

    m_fileItem = KFileItem();
    if (item.isNull()) {
        // The cursor is above the viewport. If files are selected,
        // show information regarding the selection.
        if (m_selection.size() > 0) {
            m_pendingPreview = false;
            m_infoTimer->start();
        }
    } else {
        const KUrl url = item.url();
        if (url.isValid() && !isEqualToShownUrl(url)) {
            m_urlCandidate = item.url();
            m_fileItem = item;
            m_infoTimer->start();
        }
    }
}

void InformationPanel::showEvent(QShowEvent* event)
{
    Panel::showEvent(event);
    if (!event->spontaneous()) {
        if (!m_initialized) {
            // do a delayed initialization so that no performance
            // penalty is given when Dolphin is started with a closed
            // Information Panel
            init();
        }
        showItemInfo();
    }
}

void InformationPanel::resizeEvent(QResizeEvent* event)
{
    if (isVisible()) {
        // If the text inside the name label or the info label cannot
        // get wrapped, then the maximum width of the label is increased
        // so that the width of the information panel gets increased.
        // To prevent this, the maximum width is adjusted to
        // the current width of the panel.
        const int maxWidth = event->size().width() - KDialog::spacingHint() * 4;
        m_nameLabel->setMaximumWidth(maxWidth);

        // The metadata widget also contains a text widget which may return
        // a large preferred width.
        if (m_metaDataWidget != 0) {
            m_metaDataWidget->setMaximumWidth(maxWidth);
        }

        // try to increase the preview as large as possible
        m_preview->setSizeHint(QSize(maxWidth, maxWidth));
        m_urlCandidate = m_shownUrl; // reset the URL candidate if a resizing is done
        m_infoTimer->start();

        if (m_phononWidget->isVisible() && (m_phononWidget->mode() == PhononWidget::Video)) {
            // assure that the size of the video player is the same as the preview size
            m_phononWidget->setVideoSize(QSize(maxWidth, maxWidth));
        }
    }
    Panel::resizeEvent(event);
}

bool InformationPanel::eventFilter(QObject* obj, QEvent* event)
{
    // Check whether the size of the meta text area has changed and adjust
    // the fixed width in a way that no horizontal scrollbar needs to be shown.
    if ((obj == m_metaTextArea->viewport()) && (event->type() == QEvent::Resize)) {
        QResizeEvent* resizeEvent = static_cast<QResizeEvent*>(event);
        m_metaTextLabel->setFixedWidth(resizeEvent->size().width());
    }
    return Panel::eventFilter(obj, event);
}

void InformationPanel::contextMenuEvent(QContextMenuEvent* event)
{
    Panel::contextMenuEvent(event);

#ifdef HAVE_NEPOMUK
    if (showMultipleSelectionInfo()) {
        return;
    }

    KMenu popup(this);

    QAction* previewAction = popup.addAction(i18nc("@action:inmenu", "Preview"));
    previewAction->setIcon(KIcon("view-preview"));
    previewAction->setCheckable(true);
    previewAction->setChecked(InformationPanelSettings::showPreview());

    const bool metaDataAvailable = MetaDataWidget::metaDataAvailable();

    QAction* ratingAction = popup.addAction(i18nc("@action:inmenu", "Rating"));
    ratingAction->setIcon(KIcon("rating"));
    ratingAction->setCheckable(true);
    ratingAction->setChecked(InformationPanelSettings::showRating());
    ratingAction->setEnabled(metaDataAvailable);

    QAction* commentAction = popup.addAction(i18nc("@action:inmenu", "Comment"));
    commentAction->setIcon(KIcon("text-plain"));
    commentAction->setCheckable(true);
    commentAction->setChecked(InformationPanelSettings::showComment());
    commentAction->setEnabled(metaDataAvailable);

    QAction* tagsAction = popup.addAction(i18nc("@action:inmenu", "Tags"));
    tagsAction->setCheckable(true);
    tagsAction->setChecked(InformationPanelSettings::showTags());
    tagsAction->setEnabled(metaDataAvailable);

    KConfig config("kmetainformationrc", KConfig::NoGlobals);
    KConfigGroup settings = config.group("Show");
    initMetaInfoSettings(settings);

    QList<QAction*> actions;

    // Get all meta information labels that are available for
    // the currently shown file item and add them to the popup.
    Nepomuk::Resource res(fileItem().url());
    QHash<QUrl, Nepomuk::Variant> properties = res.properties();
    QHash<QUrl, Nepomuk::Variant>::const_iterator it = properties.constBegin();
    while (it != properties.constEnd()) {
        Nepomuk::Types::Property prop(it.key());
        const QString key = prop.label();

        // Meta information provided by Nepomuk that is already
        // available from KFileItem should not be configurable.
        bool skip = (key == "fileExtension") ||
                    (key == "name") ||
                    (key == "sourceModified") ||
                    (key == "size") ||
                    (key == "mime type");
        if (!skip) {
            // Check whether there is already a meta information
            // having the same label. In this case don't show it
            // twice in the menu.
            foreach (const QAction* action, actions) {
                if (action->data().toString() == key) {
                    skip = true;
                    break;
                }
            }
        }

        if (!skip) {
            const QString label = tunedLabel(key); // TODO
            QAction* action = new QAction(label, &popup);
            action->setCheckable(true);
            action->setChecked(settings.readEntry(key, true));
            action->setData(key);
            actions.append(action);
        }

        ++it;
    }

    if (actions.count() > 0) {
        popup.addSeparator();

        // add all items alphabetically sorted to the popup
        qSort(actions.begin(), actions.end(), lessThan);
        foreach (QAction* action, actions) {
            popup.addAction(action);
        }
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
        m_previewSeparator->setVisible(isChecked);
        InformationPanelSettings::setShowPreview(isChecked);
        updatePhononWidget();
    } else if (action == ratingAction) {
        m_metaDataWidget->setRatingVisible(isChecked);
        InformationPanelSettings::setShowRating(isChecked);
    } else if (action == commentAction) {
        m_metaDataWidget->setCommentVisible(isChecked);
        InformationPanelSettings::setShowComment(isChecked);
    } else if (action == tagsAction) {
        m_metaDataWidget->setTagsVisible(isChecked);
        InformationPanelSettings::setShowTags(isChecked);
    } else {
        settings.writeEntry(action->data().toString(), action->isChecked());
        settings.sync();
        showMetaInfo();
    }

    if (m_metaDataWidget != 0) {
        const bool visible = m_metaDataWidget->isRatingVisible() ||
                             m_metaDataWidget->isCommentVisible() ||
                             m_metaDataWidget->areTagsVisible();
        m_metaDataSeparator->setVisible(visible);
    }
#endif
}

void InformationPanel::showItemInfo()
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();

    if (showMultipleSelectionInfo()) {
        KIconLoader iconLoader;
        QPixmap icon = iconLoader.loadIcon("dialog-information",
                                           KIconLoader::NoGroup,
                                           KIconLoader::SizeEnormous);
        m_preview->setPixmap(icon);
        setNameLabelText(i18ncp("@info", "%1 item selected", "%1 items selected",  m_selection.count()));
        m_shownUrl = KUrl();
    } else {
        const KFileItem item = fileItem();
        const KUrl itemUrl = item.url();
        if (!applyPlace(itemUrl)) {
            // try to get a preview pixmap from the item...
            m_pendingPreview = true;

            // Mark the currently shown preview as outdated. This is done
            // with a small delay to prevent a flickering when the next preview
            // can be shown within a short timeframe.
            m_outdatedPreviewTimer->start();

            KIO::PreviewJob* job = KIO::filePreview(KFileItemList() << item,
                                                    m_preview->width(),
                                                    m_preview->height(),
                                                    0,
                                                    0,
                                                    false,
                                                    true);

            connect(job, SIGNAL(gotPreview(const KFileItem&, const QPixmap&)),
                    this, SLOT(showPreview(const KFileItem&, const QPixmap&)));
            connect(job, SIGNAL(failed(const KFileItem&)),
                    this, SLOT(showIcon(const KFileItem&)));

            setNameLabelText(itemUrl.fileName());
        }
    }

    showMetaInfo();
}

void InformationPanel::slotInfoTimeout()
{
    m_shownUrl = m_urlCandidate;
    showItemInfo();
}

void InformationPanel::markOutdatedPreview()
{
    KIconEffect iconEffect;
    QPixmap disabledPixmap = iconEffect.apply(m_preview->pixmap(),
                                              KIconLoader::Desktop,
                                              KIconLoader::DisabledState);
    m_preview->setPixmap(disabledPixmap);
}

void InformationPanel::showIcon(const KFileItem& item)
{
    m_outdatedPreviewTimer->stop();
    m_pendingPreview = false;
    if (!applyPlace(item.url())) {
        m_preview->setPixmap(item.pixmap(KIconLoader::SizeEnormous));
    }
}

void InformationPanel::showPreview(const KFileItem& item,
                                  const QPixmap& pixmap)
{
    m_outdatedPreviewTimer->stop();

    Q_UNUSED(item);
    if (m_pendingPreview) {
        m_preview->setPixmap(pixmap);
        m_pendingPreview = false;
    }
}

void InformationPanel::slotFileRenamed(const QString& source, const QString& dest)
{
    if (m_shownUrl == KUrl(source)) {
        m_shownUrl = KUrl(dest);
        m_fileItem = KFileItem(KFileItem::Unknown, KFileItem::Unknown, m_shownUrl);
        showItemInfo();
    }
}

void InformationPanel::slotFilesAdded(const QString& directory)
{
    if (m_shownUrl == KUrl(directory)) {
        // If the 'trash' icon changes because the trash has been emptied or got filled,
        // the signal filesAdded("trash:/") will be emitted.
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, KUrl(directory));
        requestDelayedItemInfo(item);
    }
}

void InformationPanel::slotFilesChanged(const QStringList& files)
{
    foreach (const QString& fileName, files) {
        if (m_shownUrl == KUrl(fileName)) {
            showItemInfo();
            break;
        }
    }
}

void InformationPanel::slotFilesRemoved(const QStringList& files)
{
    foreach (const QString& fileName, files) {
        if (m_shownUrl == KUrl(fileName)) {
            // the currently shown item has been removed, show
            // the parent directory as fallback
            reset();
            break;
        }
    }
}

void InformationPanel::slotEnteredDirectory(const QString& directory)
{
    if (m_shownUrl == KUrl(directory)) {
        KFileItem item(KFileItem::Unknown, KFileItem::Unknown, KUrl(directory));
        requestDelayedItemInfo(item);
    }
}

void InformationPanel::slotLeftDirectory(const QString& directory)
{
    if (m_shownUrl == KUrl(directory)) {
        // The signal 'leftDirectory' is also emitted when a media
        // has been unmounted. In this case no directory change will be
        // done in Dolphin, but the Information Panel must be updated to
        // indicate an invalid directory.
        reset();
    }
}

void InformationPanel::slotPlayingStarted()
{
    m_preview->setVisible(m_phononWidget->mode() != PhononWidget::Video);
}

void InformationPanel::slotPlayingStopped()
{
    m_preview->setVisible(true);
}

bool InformationPanel::applyPlace(const KUrl& url)
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

void InformationPanel::cancelRequest()
{
    m_infoTimer->stop();
}

void InformationPanel::showMetaInfo()
{
    m_metaTextLabel->clear();

    if (showMultipleSelectionInfo()) {
        if (m_metaDataWidget != 0) {
            KUrl::List urls;
            foreach (const KFileItem& item, m_selection) {
                urls.append(item.targetUrl());
            }
            m_metaDataWidget->setFiles(urls);
        }

        quint64 totalSize = 0;
        foreach (const KFileItem& item, m_selection) {
            // Only count the size of files, not dirs to match what
            // DolphinViewContainer::selectionStatusBarText() does.
            if (!item.isDir() && !item.isLink()) {
                totalSize += item.size();
            }
        }
        m_metaTextLabel->add(i18nc("@label", "Total size:"), KIO::convertSize(totalSize));
    } else {
        const KFileItem item = fileItem();
        if (item.isDir()) {
            m_metaTextLabel->add(i18nc("@label", "Type:"), i18nc("@label", "Folder"));
            m_metaTextLabel->add(i18nc("@label", "Modified:"), item.timeString());
        } else {
            m_metaTextLabel->add(i18nc("@label", "Type:"), item.mimeComment());

            m_metaTextLabel->add(i18nc("@label", "Size:"), KIO::convertSize(item.size()));
            m_metaTextLabel->add(i18nc("@label", "Modified:"), item.timeString());

#ifdef HAVE_NEPOMUK
            KConfig config("kmetainformationrc", KConfig::NoGlobals);
            KConfigGroup settings = config.group("Show");
            initMetaInfoSettings(settings);

            Nepomuk::Resource res(item.url());

            QHash<QUrl, Nepomuk::Variant> properties = res.properties();
            QHash<QUrl, Nepomuk::Variant>::const_iterator it = properties.constBegin();
            while (it != properties.constEnd()) {
                Nepomuk::Types::Property prop(it.key());
                const QString label = prop.label();
                if (settings.readEntry(label, true)) {
                    // TODO #1: use Nepomuk::formatValue(res, prop) if available
                    // instead of it.value().toString()
                    // TODO #2: using tunedLabel() is a workaround for KDE 4.3 until
                    // we get translated labels
                    m_metaTextLabel->add(tunedLabel(label) + ':', it.value().toString());
                }
                ++it;
            }
#endif
        }

        if (m_metaDataWidget != 0) {
            m_metaDataWidget->setFile(item.targetUrl());
        }
    }

    updatePhononWidget();
}

KFileItem InformationPanel::fileItem() const
{
    if (!m_fileItem.isNull()) {
        return m_fileItem;
    }

    if (!m_selection.isEmpty()) {
        Q_ASSERT(m_selection.count() == 1);
        return m_selection.first();
    }

    // no item is hovered and no selection has been done: provide
    // an item for the directory represented by m_shownUrl
    KFileItem item(KFileItem::Unknown, KFileItem::Unknown, m_shownUrl);
    item.refresh();
    return item;
}

bool InformationPanel::showMultipleSelectionInfo() const
{
    return m_fileItem.isNull() && (m_selection.count() > 1);
}

bool InformationPanel::isEqualToShownUrl(const KUrl& url) const
{
    return m_shownUrl.equals(url, KUrl::CompareWithoutTrailingSlash);
}

void InformationPanel::setNameLabelText(const QString& text)
{
    QTextOption textOption;
    textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);

    QTextLayout textLayout(text);
    textLayout.setFont(m_nameLabel->font());
    textLayout.setTextOption(textOption);

    QString wrappedText;
    wrappedText.reserve(text.length());

    // wrap the text to fit into the width of m_nameLabel
    textLayout.beginLayout();
    QTextLine line = textLayout.createLine();
    while (line.isValid()) {
        line.setLineWidth(m_nameLabel->width());
        wrappedText += text.mid(line.textStart(), line.textLength());

        line = textLayout.createLine();
        if (line.isValid()) {
            wrappedText += QChar::LineSeparator;
        }
    }
    textLayout.endLayout();

    m_nameLabel->setText(wrappedText);
}

void InformationPanel::reset()
{
    m_selection.clear();
    m_shownUrl = url();
    m_fileItem = KFileItem();
    showItemInfo();
}

void InformationPanel::initMetaInfoSettings(KConfigGroup& group)
{
    if (!group.readEntry("initialized", false)) {
        // The resource file is read the first time. Assure
        // that some meta information is disabled per default.

        static const char* disabledProperties[] = {
            "asText", "contentSize", "depth", "fileExtension",
            "fileName", "fileSize", "isPartOf", "mimetype", "name",
            "parentUrl", "plainTextContent", "sourceModified",
            "size", "url",
            0 // mandatory last entry
        };

        int i = 0;
        while (disabledProperties[i] != 0) {
            group.writeEntry(disabledProperties[i], false);
            ++i;
        }

        // mark the group as initialized
        group.writeEntry("initialized", true);
    }
}

void InformationPanel::updatePhononWidget()
{
    const bool multipleSelections = showMultipleSelectionInfo();
    const bool showPreview = InformationPanelSettings::showPreview();

    if (multipleSelections || !showPreview) {
        m_phononWidget->hide();
    } else if (!multipleSelections && showPreview) {
        const KFileItem item = fileItem();
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
            m_phononWidget->setUrl(item.url());
            if ((mode == PhononWidget::Video) && m_preview->isVisible()) {
                m_phononWidget->setVideoSize(m_preview->size());
            }
        } else {
            m_phononWidget->hide();
            m_preview->setVisible(true);
        }
    }
}

QString InformationPanel::tunedLabel(const QString& label) const
{
    QString tunedLabel;
    const int labelLength = label.length();
    if (labelLength > 0) {
        tunedLabel.reserve(labelLength);
        tunedLabel = label[0].toUpper();
        for (int i = 1; i < labelLength; ++i) {
            if (label[i].isUpper() && !label[i - 1].isSpace() && !label[i - 1].isUpper()) {
                tunedLabel += ' ';
                tunedLabel += label[i].toLower();
            } else {
                tunedLabel += label[i];
            }
        }
    }
    return tunedLabel;
}

void InformationPanel::init()
{
    m_infoTimer = new QTimer(this);
    m_infoTimer->setInterval(300);
    m_infoTimer->setSingleShot(true);
    connect(m_infoTimer, SIGNAL(timeout()),
            this, SLOT(slotInfoTimeout()));

    // Initialize timer for disabling an outdated preview with a small
    // delay. This prevents flickering if the new preview can be generated
    // within a very small timeframe.
    m_outdatedPreviewTimer = new QTimer(this);
    m_outdatedPreviewTimer->setInterval(300);
    m_outdatedPreviewTimer->setSingleShot(true);
    connect(m_outdatedPreviewTimer, SIGNAL(timeout()),
            this, SLOT(markOutdatedPreview()));

    QVBoxLayout* layout = new QVBoxLayout;
    layout->setSpacing(KDialog::spacingHint());

    // name
    m_nameLabel = new QLabel(this);
    QFont font = m_nameLabel->font();
    font.setBold(true);
    m_nameLabel->setFont(font);
    m_nameLabel->setAlignment(Qt::AlignHCenter);
    m_nameLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_nameLabel->setMaximumWidth(KIconLoader::SizeEnormous);

    // preview
    const int minPreviewWidth = KIconLoader::SizeEnormous + KIconLoader::SizeMedium;

    m_preview = new PixmapViewer(this);
    m_preview->setMinimumWidth(minPreviewWidth);
    m_preview->setMinimumHeight(KIconLoader::SizeEnormous);

    m_phononWidget = new PhononWidget(this);
    m_phononWidget->setMinimumWidth(minPreviewWidth);
    connect(m_phononWidget, SIGNAL(playingStarted()),
            this, SLOT(slotPlayingStarted()));
    connect(m_phononWidget, SIGNAL(playingStopped()),
            this, SLOT(slotPlayingStopped()));

    m_previewSeparator = new KSeparator(this);

    const bool showPreview = InformationPanelSettings::showPreview();
    m_preview->setVisible(showPreview);
    m_previewSeparator->setVisible(showPreview);

    if (MetaDataWidget::metaDataAvailable()) {
        // rating, comment and tags
        m_metaDataWidget = new MetaDataWidget(this);
        m_metaDataWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        m_metaDataWidget->setMaximumWidth(KIconLoader::SizeEnormous);

        const bool showRating  = InformationPanelSettings::showRating();
        const bool showComment = InformationPanelSettings::showComment();
        const bool showTags    = InformationPanelSettings::showTags();

        m_metaDataWidget->setRatingVisible(showRating);
        m_metaDataWidget->setCommentVisible(showComment);
        m_metaDataWidget->setTagsVisible(showTags);

        m_metaDataSeparator = new KSeparator(this);
        m_metaDataSeparator->setVisible(showRating || showComment || showTags);
    }

    // general meta text information
    m_metaTextLabel = new MetaTextLabel(this);
    m_metaTextLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    m_metaTextArea = new QScrollArea(this);
    m_metaTextArea->setWidget(m_metaTextLabel);
    m_metaTextArea->setWidgetResizable(true);
    m_metaTextArea->setFrameShape(QFrame::NoFrame);

    QWidget* viewport = m_metaTextArea->viewport();
    viewport->installEventFilter(this);

    QPalette palette = viewport->palette();
    palette.setColor(viewport->backgroundRole(), QColor(Qt::transparent));
    viewport->setPalette(palette);

    layout->addWidget(m_nameLabel);
    layout->addWidget(new KSeparator(this));
    layout->addWidget(m_preview);
    layout->addWidget(m_phononWidget);
    layout->addWidget(m_previewSeparator);
    if (m_metaDataWidget != 0) {
        layout->addWidget(m_metaDataWidget);
        layout->addWidget(m_metaDataSeparator);
    }
    layout->addWidget(m_metaTextArea);
    setLayout(layout);

    org::kde::KDirNotify* dirNotify = new org::kde::KDirNotify(QString(), QString(),
                                                               QDBusConnection::sessionBus(), this);
    connect(dirNotify, SIGNAL(FileRenamed(QString, QString)), SLOT(slotFileRenamed(QString, QString)));
    connect(dirNotify, SIGNAL(FilesAdded(QString)), SLOT(slotFilesAdded(QString)));
    connect(dirNotify, SIGNAL(FilesChanged(QStringList)), SLOT(slotFilesChanged(QStringList)));
    connect(dirNotify, SIGNAL(FilesRemoved(QStringList)), SLOT(slotFilesRemoved(QStringList)));
    connect(dirNotify, SIGNAL(enteredDirectory(QString)), SLOT(slotEnteredDirectory(QString)));
    connect(dirNotify, SIGNAL(leftDirectory(QString)), SLOT(slotLeftDirectory(QString)));

    m_initialized = true;
}

#include "informationpanel.moc"
