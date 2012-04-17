/***************************************************************************
 *   Copyright (C) 2006-2009 by Peter Penz <peter.penz19@gmail.com>        *
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

#include "informationpanelcontent.h"
#include <KIO/Job>
#include <KIO/JobUiDelegate>
#include <KDirNotify>
#include <QApplication>
#include <QShowEvent>
#include <QVBoxLayout>

InformationPanel::InformationPanel(QWidget* parent) :
    Panel(parent),
    m_initialized(false),
    m_infoTimer(0),
    m_urlChangedTimer(0),
    m_resetUrlTimer(0),
    m_shownUrl(),
    m_urlCandidate(),
    m_invalidUrlCandidate(),
    m_fileItem(),
    m_selection(),
    m_folderStatJob(0),
    m_content(0)
{
}

InformationPanel::~InformationPanel()
{
}

void InformationPanel::setSelection(const KFileItemList& selection)
{
    m_selection = selection;
    m_fileItem = KFileItem();

    if (!isVisible()) {
        return;
    }

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
    if (!isVisible() || (item.isNull() && m_fileItem.isNull())) {
        return;
    }

    if (QApplication::mouseButtons() & Qt::LeftButton) {
        // Ignore the request of an item information when a rubberband
        // selection is ongoing.
        return;
    }

    cancelRequest();

    if (item.isNull()) {
        // The cursor is above the viewport. If files are selected,
        // show information regarding the selection.
        if (m_selection.size() > 0) {
            m_fileItem = KFileItem();
            m_infoTimer->start();
        }
    } else if (item.url().isValid() && !isEqualToShownUrl(item.url())) {
        // The cursor is above an item that is not shown currently
        m_urlCandidate = item.url();
        m_fileItem = item;
        m_infoTimer->start();
    }
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
        m_fileItem = KFileItem();

        // Update the content with a delay. This gives
        // the directory lister the chance to show the content
        // before expensive operations are done to show
        // meta information.
        m_urlChangedTimer->start();
    }

    return true;
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

        m_shownUrl = url();
        showItemInfo();
    }
}

void InformationPanel::resizeEvent(QResizeEvent* event)
{
    if (isVisible()) {
        m_urlCandidate = m_shownUrl;
        m_infoTimer->start();
    }
    Panel::resizeEvent(event);
}

void InformationPanel::contextMenuEvent(QContextMenuEvent* event)
{
    // TODO: Move code from InformationPanelContent::configureSettings() here
    m_content->configureSettings(customContextMenuActions());
    Panel::contextMenuEvent(event);
}

void InformationPanel::showItemInfo()
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();

    if (m_fileItem.isNull() && (m_selection.count() > 1)) {
        // The information for a selection of items should be shown
        m_content->showItems(m_selection);
    } else {
        // The information for exactly one item should be shown
        KFileItem item;
        if (!m_fileItem.isNull()) {
            item = m_fileItem;
        } else if (!m_selection.isEmpty()) {
            Q_ASSERT(m_selection.count() == 1);
            item = m_selection.first();
        }

        if (item.isNull()) {
            // No item is hovered and no selection has been done: provide
            // an item for the currently shown directory.
            m_folderStatJob = KIO::stat(url(), KIO::HideProgressInfo);
            if (m_folderStatJob->ui()) {
                m_folderStatJob->ui()->setWindow(this);
            }
            connect(m_folderStatJob, SIGNAL(result(KJob*)),
                    this, SLOT(slotFolderStatFinished(KJob*)));
        } else {
            m_content->showItem(item);
        }
    }
}

void InformationPanel::slotFolderStatFinished(KJob* job)
{
    m_folderStatJob = 0;
    const KIO::UDSEntry entry = static_cast<KIO::StatJob*>(job)->statResult();
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
        m_invalidUrlCandidate = KUrl();

        // The current URL is still invalid. Reset
        // the content to show the directory URL.
        m_selection.clear();
        m_shownUrl = url();
        m_fileItem = KFileItem();
        showItemInfo();
    }
}

void InformationPanel::slotFileRenamed(const QString& source, const QString& dest)
{
    if (m_shownUrl == KUrl(source)) {
        m_shownUrl = KUrl(dest);
        m_fileItem = KFileItem(KFileItem::Unknown, KFileItem::Unknown, m_shownUrl);

        if ((m_selection.count() == 1) && (m_selection[0].url() == KUrl(source))) {
            m_selection[0] = m_fileItem;
            // Implementation note: Updating the selection is only required if exactly one
            // item is selected, as the name of the item is shown. If this should change
            // in future: Before parsing the whole selection take care to test possible
            // performance bottlenecks when renaming several hundreds of files.
        }

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
            markUrlAsInvalid();
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
        markUrlAsInvalid();
    }
}

void InformationPanel::cancelRequest()
{
    delete m_folderStatJob;
    m_folderStatJob = 0;

    m_infoTimer->stop();
    m_resetUrlTimer->stop();
    // Don't reset m_urlChangedTimer. As it is assured that the timeout of m_urlChangedTimer
    // has the smallest interval (see init()), it is not possible that the exceeded timer
    // would overwrite an information provided by a selection or hovering.

    m_invalidUrlCandidate.clear();
    m_urlCandidate.clear();
}

bool InformationPanel::isEqualToShownUrl(const KUrl& url) const
{
    return m_shownUrl.equals(url, KUrl::CompareWithoutTrailingSlash);
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
    connect(m_infoTimer, SIGNAL(timeout()),
            this, SLOT(slotInfoTimeout()));

    m_urlChangedTimer = new QTimer(this);
    m_urlChangedTimer->setInterval(200);
    m_urlChangedTimer->setSingleShot(true);
    connect(m_urlChangedTimer, SIGNAL(timeout()),
            this, SLOT(showItemInfo()));

    m_resetUrlTimer = new QTimer(this);
    m_resetUrlTimer->setInterval(1000);
    m_resetUrlTimer->setSingleShot(true);
    connect(m_resetUrlTimer, SIGNAL(timeout()),
            this, SLOT(reset()));

    Q_ASSERT(m_urlChangedTimer->interval() < m_infoTimer->interval());
    Q_ASSERT(m_urlChangedTimer->interval() < m_resetUrlTimer->interval());

    org::kde::KDirNotify* dirNotify = new org::kde::KDirNotify(QString(), QString(),
                                                               QDBusConnection::sessionBus(), this);
    connect(dirNotify, SIGNAL(FileRenamed(QString,QString)), SLOT(slotFileRenamed(QString,QString)));
    connect(dirNotify, SIGNAL(FilesAdded(QString)), SLOT(slotFilesAdded(QString)));
    connect(dirNotify, SIGNAL(FilesChanged(QStringList)), SLOT(slotFilesChanged(QStringList)));
    connect(dirNotify, SIGNAL(FilesRemoved(QStringList)), SLOT(slotFilesRemoved(QStringList)));
    connect(dirNotify, SIGNAL(enteredDirectory(QString)), SLOT(slotEnteredDirectory(QString)));
    connect(dirNotify, SIGNAL(leftDirectory(QString)), SLOT(slotLeftDirectory(QString)));

    m_content = new InformationPanelContent(this);
    connect(m_content, SIGNAL(urlActivated(KUrl)), this, SIGNAL(urlActivated(KUrl)));

    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_content);

    m_initialized = true;
}

#include "informationpanel.moc"
