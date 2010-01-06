/***************************************************************************
 *   Copyright (C) 2006-2009 by Peter Penz <peter.penz@gmx.at>             *
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
#include <kdirnotify.h>
#include <QShowEvent>
#include "informationpanelcontent.h"

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
    m_content(0)
{
}

InformationPanel::~InformationPanel()
{
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
            // Update the content with a delay. This gives
            // the directory lister the chance to show the content
            // before expensive operations are done to show
            // meta information.
            m_urlChangedTimer->start();
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
        m_urlCandidate = m_shownUrl;
        m_infoTimer->start();
    }
    Panel::resizeEvent(event);
}

void InformationPanel::contextMenuEvent(QContextMenuEvent* event)
{
    m_content->configureSettings();
    Panel::contextMenuEvent(event);
}

void InformationPanel::showItemInfo()
{
    if (!isVisible()) {
        return;
    }

    cancelRequest();

    if (showMultipleSelectionInfo()) {
        m_content->showItems(m_selection);
        m_shownUrl = KUrl();
    } else {
        KFileItem item;
        if (!m_fileItem.isNull()) {
            item = m_fileItem;
        } else if (!m_selection.isEmpty()) {
            Q_ASSERT(m_selection.count() == 1);
            item = m_selection.first();
        } else {
            // no item is hovered and no selection has been done: provide
            // an item for the directory represented by m_shownUrl
            item = KFileItem(KFileItem::Unknown, KFileItem::Unknown, m_shownUrl);
            item.refresh();
        }

        m_content->showItem(item);
    }
}

void InformationPanel::slotInfoTimeout()
{
    m_shownUrl = m_urlCandidate;
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
    m_infoTimer->stop();
}

bool InformationPanel::showMultipleSelectionInfo() const
{
    return m_fileItem.isNull() && (m_selection.count() > 1);
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
    const int defaultDelay = 300;

    m_infoTimer = new QTimer(this);
    m_infoTimer->setInterval(defaultDelay);
    m_infoTimer->setSingleShot(true);
    connect(m_infoTimer, SIGNAL(timeout()),
            this, SLOT(slotInfoTimeout()));

    m_urlChangedTimer = new QTimer(this);
    m_urlChangedTimer->setInterval(defaultDelay);
    m_urlChangedTimer->setSingleShot(true);
    connect(m_urlChangedTimer, SIGNAL(timeout()),
            this, SLOT(showItemInfo()));

    m_resetUrlTimer = new QTimer(this);
    m_resetUrlTimer->setInterval(defaultDelay * 3);
    m_resetUrlTimer->setSingleShot(true);
    connect(m_resetUrlTimer, SIGNAL(timeout()),
            this, SLOT(reset()));

    org::kde::KDirNotify* dirNotify = new org::kde::KDirNotify(QString(), QString(),
                                                               QDBusConnection::sessionBus(), this);
    connect(dirNotify, SIGNAL(FileRenamed(QString, QString)), SLOT(slotFileRenamed(QString, QString)));
    connect(dirNotify, SIGNAL(FilesAdded(QString)), SLOT(slotFilesAdded(QString)));
    connect(dirNotify, SIGNAL(FilesChanged(QStringList)), SLOT(slotFilesChanged(QStringList)));
    connect(dirNotify, SIGNAL(FilesRemoved(QStringList)), SLOT(slotFilesRemoved(QStringList)));
    connect(dirNotify, SIGNAL(enteredDirectory(QString)), SLOT(slotEnteredDirectory(QString)));
    connect(dirNotify, SIGNAL(leftDirectory(QString)), SLOT(slotLeftDirectory(QString)));

    m_content = new InformationPanelContent(this);
    connect(m_content, SIGNAL(urlActivated(KUrl)), this, SIGNAL(urlActivated(KUrl)));

    m_initialized = true;
}

#include "informationpanel.moc"
