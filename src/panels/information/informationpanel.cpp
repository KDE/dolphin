/*
 * SPDX-FileCopyrightText: 2006-2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "informationpanel.h"

#include "informationpanelcontent.h"

#include <KDirNotify>
#include <KIO/Job>
#include <KJobWidgets>
#include <KLocalizedString>

#include <Baloo/FileMetaDataWidget>

#include <QApplication>
#include <QMenu>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

#include "dolphin_informationpanelsettings.h"

InformationPanel::InformationPanel(QWidget *parent)
    : Panel(parent)
    , m_initialized(false)
    , m_infoTimer(nullptr)
    , m_urlChangedTimer(nullptr)
    , m_resetUrlTimer(nullptr)
    , m_shownUrl()
    , m_urlCandidate()
    , m_invalidUrlCandidate()
    , m_hoveredItem()
    , m_selection()
    , m_folderStatJob(nullptr)
    , m_content(nullptr)
{
}

InformationPanel::~InformationPanel()
{
}

void InformationPanel::setSelection(const KFileItemList &selection)
{
    m_selection = selection;

    if (!isVisible()) {
        return;
    }

    const int count = selection.count();
    if (count == 0) {
        if (!isEqualToShownUrl(url())) {
            m_shownUrl = url();
            showItemInfo();
        }
        m_infoTimer->stop();
    } else {
        if ((count == 1) && !selection.first().url().isEmpty()) {
            m_urlCandidate = selection.first().url();
        }
        showItemInfo();
    }
}

void InformationPanel::requestDelayedItemInfo(const KFileItem &item)
{
    if (!isVisible() || !InformationPanelSettings::showHovered()) {
        return;
    }

    if (QApplication::mouseButtons() & Qt::LeftButton) {
        // Ignore the request of an item information when a rubberband
        // selection is ongoing.
        return;
    }

    if (item.isNull()) {
        m_hoveredItem = KFileItem();
        return;
    }

    cancelRequest();

    m_hoveredItem = item;
    m_infoTimer->start();
}

bool InformationPanel::urlChanged()
{
    if (!url().isValid()) {
        return false;
    }

    if (!isVisible()) {
        return true;
    }

    cancelRequest();
    m_selection.clear();

    if (!isEqualToShownUrl(url())) {
        m_shownUrl = url();

        // Update the content with a delay. This gives
        // the directory lister the chance to show the content
        // before expensive operations are done to show
        // meta information.
        m_urlChangedTimer->start();
    }

    return true;
}

void InformationPanel::showEvent(QShowEvent *event)
{
    Panel::showEvent(event);
    if (!event->spontaneous()) {
        if (!m_initialized) {
            // do a delayed initialization so that no performance
            // penalty is given when Dolphin is started with a closed
            // Information Panel
            init();
        }

        m_shownUrl = url();
        showItemInfo();
    }
}

void InformationPanel::resizeEvent(QResizeEvent *event)
{
    if (isVisible()) {
        m_urlCandidate = m_shownUrl;
        m_infoTimer->start();
    }
    Panel::resizeEvent(event);
}

void InformationPanel::contextMenuEvent(QContextMenuEvent *event)
{
    showContextMenu(event->globalPos());
    Panel::contextMenuEvent(event);
}

void InformationPanel::showContextMenu(const QPoint &pos)
{
    QMenu popup(this);

    QAction *previewAction = popup.addAction(i18nc("@action:inmenu", "Preview"));
    previewAction->setIcon(QIcon::fromTheme(QStringLiteral("view-preview")));
    previewAction->setCheckable(true);
    previewAction->setChecked(InformationPanelSettings::previewsShown());

    QAction *previewAutoPlayAction = popup.addAction(i18nc("@action:inmenu", "Auto-Play media files"));
    previewAutoPlayAction->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    previewAutoPlayAction->setCheckable(true);
    previewAutoPlayAction->setChecked(InformationPanelSettings::previewsAutoPlay());

    QAction *showHoveredAction = popup.addAction(i18nc("@action:inmenu", "Show item on hover"));
    showHoveredAction->setIcon(QIcon::fromTheme(QStringLiteral("followmouse")));
    showHoveredAction->setCheckable(true);
    showHoveredAction->setChecked(InformationPanelSettings::showHovered());

    QAction *configureAction = popup.addAction(i18nc("@action:inmenu", "Configure…"));
    configureAction->setIcon(QIcon::fromTheme(QStringLiteral("configure")));
    if (m_inConfigurationMode) {
        configureAction->setEnabled(false);
    }

    QAction *dateformatAction = popup.addAction(i18nc("@action:inmenu", "Condensed Date"));
    dateformatAction->setIcon(QIcon::fromTheme(QStringLiteral("change-date-symbolic")));
    dateformatAction->setCheckable(true);
    dateformatAction->setChecked(InformationPanelSettings::dateFormat() == static_cast<int>(Baloo::DateFormats::ShortFormat));

    popup.addSeparator();
    const auto actions = customContextMenuActions();
    for (QAction *action : actions) {
        popup.addAction(action);
    }

    // Open the popup and adjust the settings for the
    // selected action.
    QAction *action = popup.exec(pos);
    if (!action) {
        return;
    }

    const bool isChecked = action->isChecked();
    if (action == previewAction) {
        InformationPanelSettings::setPreviewsShown(isChecked);
        m_content->refreshPreview();
    } else if (action == previewAutoPlayAction) {
        InformationPanelSettings::setPreviewsAutoPlay(isChecked);
        m_content->setPreviewAutoPlay(isChecked);
    } else if (action == showHoveredAction) {
        InformationPanelSettings::setShowHovered(isChecked);
        if (!isChecked) {
            m_hoveredItem = KFileItem();
            showItemInfo();
        }
    } else if (action == configureAction) {
        m_inConfigurationMode = true;
        m_content->configureShownProperties();
    } else if (action == dateformatAction) {
        int dateFormat = static_cast<int>(isChecked ? Baloo::DateFormats::ShortFormat : Baloo::DateFormats::LongFormat);

        InformationPanelSettings::setDateFormat(dateFormat);
        m_content->refreshMetaData();
    }
}

void InformationPanel::showItemInfo()
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();
    //qDebug() << "showItemInfo" << m_fileItem;

    if (m_hoveredItem.isNull() && (m_selection.count() > 1)) {
        // The information for a selection of items should be shown
        m_content->showItems(m_selection);
    } else {
        // The information for exactly one item should be shown
        KFileItem item;
        if (!m_hoveredItem.isNull() && InformationPanelSettings::showHovered()) {
            item = m_hoveredItem;
        } else if (!m_selection.isEmpty()) {
            Q_ASSERT(m_selection.count() == 1);
            item = m_selection.first();
        }

        if (!item.isNull()) {
            m_shownUrl = item.url();
            m_content->showItem(item);
            return;
        }

        // No item is hovered and no selection has been done: provide
        // an item for the currently shown directory.
        m_shownUrl = url();
        m_folderStatJob = KIO::stat(m_shownUrl, KIO::StatJob::SourceSide, KIO::StatDefaultDetails | KIO::StatRecursiveSize, KIO::HideProgressInfo);
        if (m_folderStatJob->uiDelegate()) {
            KJobWidgets::setWindow(m_folderStatJob, this);
        }
        connect(m_folderStatJob, &KIO::Job::result, this, &InformationPanel::slotFolderStatFinished);
    }
}

void InformationPanel::slotFolderStatFinished(KJob *job)
{
    m_folderStatJob = nullptr;
    const KIO::UDSEntry entry = static_cast<KIO::StatJob *>(job)->statResult();
    m_content->showItem(KFileItem(entry, m_shownUrl));
}

void InformationPanel::slotInfoTimeout()
{
    m_shownUrl = m_urlCandidate;
    m_urlCandidate.clear();
    showItemInfo();
}

void InformationPanel::reset()
{
    if (m_invalidUrlCandidate == m_shownUrl) {
        m_invalidUrlCandidate = QUrl();

        // The current URL is still invalid. Reset
        // the content to show the directory URL.
        m_selection.clear();
        m_shownUrl = url();
        showItemInfo();
    }
}

void InformationPanel::slotFileRenamed(const QString &source, const QString &dest)
{
    auto sourceUrl = QUrl::fromUserInput(source);
    if (m_shownUrl == sourceUrl) {
        auto destUrl = QUrl::fromUserInput(dest);

        if ((m_selection.count() == 1) && (m_selection[0].url() == sourceUrl)) {
            m_selection[0] = KFileItem(destUrl);
            // Implementation note: Updating the selection is only required if exactly one
            // item is selected, as the name of the item is shown. If this should change
            // in future: Before parsing the whole selection take care to test possible
            // performance bottlenecks when renaming several hundreds of files.
        }

        showItemInfo();
    }
}

void InformationPanel::slotFilesAdded(const QString &directory)
{
    if (m_shownUrl == QUrl::fromUserInput(directory)) {
        // If the 'trash' icon changes because the trash has been emptied or got filled,
        // the signal filesAdded("trash:/") will be emitted.
        requestDelayedItemInfo(KFileItem());
    }
}

void InformationPanel::slotFilesItemChanged(const KFileItemList &changedFileItems)
{
    const auto item = changedFileItems.findByUrl(m_shownUrl);
    if (!item.isNull()) {
        showItemInfo();
    }
}

void InformationPanel::slotFilesChanged(const QStringList &files)
{
    for (const QString &fileName : files) {
        if (m_shownUrl == QUrl::fromUserInput(fileName)) {
            showItemInfo();
            break;
        }
    }
}

void InformationPanel::slotFilesRemoved(const QStringList &files)
{
    for (const QString &fileName : files) {
        if (m_shownUrl == QUrl::fromUserInput(fileName)) {
            // the currently shown item has been removed, show
            // the parent directory as fallback
            markUrlAsInvalid();
            break;
        }
    }
}

void InformationPanel::slotEnteredDirectory(const QString &directory)
{
    Q_UNUSED(directory)
}

void InformationPanel::slotLeftDirectory(const QString &directory)
{
    if (m_shownUrl == QUrl::fromUserInput(directory)) {
        // The signal 'leftDirectory' is also emitted when a media
        // has been unmounted. In this case no directory change will be
        // done in Dolphin, but the Information Panel must be updated to
        // indicate an invalid directory.
        markUrlAsInvalid();
    }
}

void InformationPanel::cancelRequest()
{
    delete m_folderStatJob;
    m_folderStatJob = nullptr;

    m_infoTimer->stop();
    m_resetUrlTimer->stop();
    // Don't reset m_urlChangedTimer. As it is assured that the timeout of m_urlChangedTimer
    // has the smallest interval (see init()), it is not possible that the exceeded timer
    // would overwrite an information provided by a selection or hovering.

    m_invalidUrlCandidate.clear();
    m_urlCandidate.clear();
}

bool InformationPanel::isEqualToShownUrl(const QUrl &url) const
{
    return m_shownUrl.matches(url, QUrl::StripTrailingSlash);
}

void InformationPanel::markUrlAsInvalid()
{
    m_invalidUrlCandidate = m_shownUrl;
    m_resetUrlTimer->start();
}

void InformationPanel::init()
{
    m_infoTimer = new QTimer(this);
    m_infoTimer->setInterval(300);
    m_infoTimer->setSingleShot(true);
    connect(m_infoTimer, &QTimer::timeout, this, &InformationPanel::slotInfoTimeout);

    m_urlChangedTimer = new QTimer(this);
    m_urlChangedTimer->setInterval(200);
    m_urlChangedTimer->setSingleShot(true);
    connect(m_urlChangedTimer, &QTimer::timeout, this, &InformationPanel::showItemInfo);

    m_resetUrlTimer = new QTimer(this);
    m_resetUrlTimer->setInterval(1000);
    m_resetUrlTimer->setSingleShot(true);
    connect(m_resetUrlTimer, &QTimer::timeout, this, &InformationPanel::reset);

    Q_ASSERT(m_urlChangedTimer->interval() < m_infoTimer->interval());
    Q_ASSERT(m_urlChangedTimer->interval() < m_resetUrlTimer->interval());

    org::kde::KDirNotify *dirNotify = new org::kde::KDirNotify(QString(), QString(), QDBusConnection::sessionBus(), this);
    connect(dirNotify, &OrgKdeKDirNotifyInterface::FileRenamed, this, &InformationPanel::slotFileRenamed);
    connect(dirNotify, &OrgKdeKDirNotifyInterface::FilesAdded, this, &InformationPanel::slotFilesAdded);
    connect(dirNotify, &OrgKdeKDirNotifyInterface::FilesChanged, this, &InformationPanel::slotFilesChanged);
    connect(dirNotify, &OrgKdeKDirNotifyInterface::FilesRemoved, this, &InformationPanel::slotFilesRemoved);
    connect(dirNotify, &OrgKdeKDirNotifyInterface::enteredDirectory, this, &InformationPanel::slotEnteredDirectory);
    connect(dirNotify, &OrgKdeKDirNotifyInterface::leftDirectory, this, &InformationPanel::slotLeftDirectory);

    m_content = new InformationPanelContent(this);
    connect(m_content, &InformationPanelContent::urlActivated, this, &InformationPanel::urlActivated);
    connect(m_content, &InformationPanelContent::configurationFinished, this, [this]() {
        m_inConfigurationMode = false;
    });
    connect(m_content, &InformationPanelContent::contextMenuRequested, this, &InformationPanel::showContextMenu);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_content);

    m_initialized = true;
}

#include "moc_informationpanel.cpp"
