/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (<peter.penz@gmx.at>)                *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
 *   Copyright (C) 2006 by Patrice Tremblay                                *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "urlnavigator.h"

#include "bookmarkselector.h"
#include "dolphinsettings.h"
#include "generalsettings.h"
#include "protocolcombo.h"
#include "urlnavigatorbutton.h"

#include <assert.h>

#include <kfileitem.h>
#include <klocale.h>
#include <kprotocolinfo.h>
#include <kurlcombobox.h>
#include <kurlcompletion.h>

#include <QDir>
#include <QCheckBox>
#include <QLabel>
#include <QLineEdit>

UrlNavigator::HistoryElem::HistoryElem() :
    m_url(),
    m_currentFileName(),
    m_contentsX(0),
    m_contentsY(0)
{
}

UrlNavigator::HistoryElem::HistoryElem(const KUrl& url) :
    m_url(url),
    m_currentFileName(),
    m_contentsX(0),
    m_contentsY(0)
{
}

UrlNavigator::HistoryElem::~HistoryElem()
{
}

UrlNavigator::UrlNavigator(const KUrl& url,
                           QWidget* parent) :
    KHBox(parent),
    m_active(true),
    m_historyIndex(0),
    m_protocols(0),
    m_protocolSeparator(0),
    m_host(0)
{
    m_history.prepend(HistoryElem(url));

    QFontMetrics fontMetrics(font());
    setMinimumHeight(fontMetrics.height() + 8);

    m_toggleButton = new QCheckBox(this);
    //m_toggleButton->setFlat(true);
    //m_toggleButton->setToggleButton(true);
    m_toggleButton->setFocusPolicy(Qt::NoFocus);
    m_toggleButton->setMinimumHeight(minimumHeight());
    connect(m_toggleButton, SIGNAL(clicked()),
            this, SLOT(slotClicked()));
    if (DolphinSettings::instance().generalSettings()->editableUrl()) {
        m_toggleButton->toggle();
    }

    m_bookmarkSelector = new BookmarkSelector(this);
    connect(m_bookmarkSelector, SIGNAL(bookmarkActivated(const KUrl&)),
            this, SLOT(setUrl(const KUrl&)));

    m_pathBox = new KUrlComboBox(KUrlComboBox::Directories, true, this);

    KUrlCompletion* kurlCompletion = new KUrlCompletion(KUrlCompletion::DirCompletion);
    m_pathBox->setCompletionObject(kurlCompletion);
    m_pathBox->setAutoDeleteCompletionObject(true);

    connect(m_pathBox, SIGNAL(returnPressed(const QString&)),
            this, SLOT(slotReturnPressed(const QString&)));
    connect(m_pathBox, SIGNAL(urlActivated(const KUrl&)),
            this, SLOT(slotUrlActivated(const KUrl&)));

    //connect(dolphinView, SIGNAL(redirection(const KUrl&, const KUrl&)),
    //        this, SLOT(slotRedirection(const KUrl&, const KUrl&)));
    updateContent();
}

UrlNavigator::~UrlNavigator()
{
}

const KUrl& UrlNavigator::url() const
{
    assert(!m_history.empty());
    QLinkedList<HistoryElem>::const_iterator it = m_history.begin();
    it += m_historyIndex;
    return (*it).url();
}

KUrl UrlNavigator::url(int index) const
{
    assert(index >= 0);
    QString path(url().pathOrUrl());
    path = path.section('/', 0, index);

    if ( path.length() >= 1 && path.at(path.length()-1) != '/')
    {
        path.append('/');
    }

    return path;
}

const QLinkedList<UrlNavigator::HistoryElem>& UrlNavigator::history(int& index) const
{
    index = m_historyIndex;
    return m_history;
}

void UrlNavigator::goBack()
{
    updateHistoryElem();

    const int count = m_history.count();
    if (m_historyIndex < count - 1) {
        ++m_historyIndex;
        updateContent();
        emit urlChanged(url());
        emit historyChanged();
    }
}

void UrlNavigator::goForward()
{
    if (m_historyIndex > 0) {
        --m_historyIndex;
        updateContent();
        emit urlChanged(url());
        emit historyChanged();
    }
}

void UrlNavigator::goUp()
{
    setUrl(url().upUrl());
}

void UrlNavigator::goHome()
{
    setUrl(DolphinSettings::instance().generalSettings()->homeUrl());
}

void UrlNavigator::setUrlEditable(bool editable)
{
    if (isUrlEditable() != editable) {
        m_toggleButton->toggle();
        slotClicked();
    }
}

bool UrlNavigator::isUrlEditable() const
{
    return m_toggleButton->isChecked();
}

void UrlNavigator::editUrl(bool editOrBrowse)
{
    setUrlEditable(editOrBrowse);
    if (editOrBrowse) {
        m_pathBox->setFocus();
    }
}

void UrlNavigator::setActive(bool active)
{
    if (active != m_active) {
        m_active = active;
        update();
        if (active) {
            emit activated();
        }
    }
}

void UrlNavigator::dropUrls(const KUrl::List& urls,
                            const KUrl& destination)
{
    emit urlsDropped(urls, destination);
}

void UrlNavigator::setUrl(const KUrl& url)
{
    QString urlStr(url.pathOrUrl());
    //kDebug() << "setUrl(" << url << ")" << endl;
    if ( urlStr.length() > 0 && urlStr.at(0) == '~') {
        // replace '~' by the home directory
        urlStr.remove(0, 1);
        urlStr.insert(0, QDir::home().path());
    }

    const KUrl transformedUrl(urlStr);

    if (m_historyIndex > 0) {
        // Check whether the previous element of the history has the same Url.
        // If yes, just go forward instead of inserting a duplicate history
        // element.
        QLinkedList<HistoryElem>::const_iterator it = m_history.begin();
        it += m_historyIndex - 1;
        const KUrl& nextUrl = (*it).url();
        if (transformedUrl == nextUrl) {
            goForward();
//             kDebug() << "goin' forward in history" << endl;
            return;
        }
    }

    QLinkedList<HistoryElem>::iterator it = m_history.begin() + m_historyIndex;
    const KUrl& currUrl = (*it).url();
    if (currUrl == transformedUrl) {
        // don't insert duplicate history elements
//         kDebug() << "currUrl == transformedUrl" << endl;
        return;
    }

    updateHistoryElem();
    m_history.insert(it, HistoryElem(transformedUrl));

    updateContent();

    emit urlChanged(transformedUrl);
    emit historyChanged();

    // Prevent an endless growing of the history: remembering
    // the last 100 Urls should be enough...
    if (m_historyIndex > 100) {
        m_history.erase(m_history.begin());
        --m_historyIndex;
    }

/*    kDebug() << "history starting ====================" << endl;
    int i = 0;
    for (QValueListIterator<UrlNavigator::HistoryElem> it = m_history.begin();
         it != m_history.end();
         ++it, ++i)
    {
        kDebug() << i << ": " << (*it).url() << endl;
    }
    kDebug() << "history done ========================" << endl;*/

    requestActivation();
}

void UrlNavigator::requestActivation()
{
    setActive(true);
}

void UrlNavigator::storeContentsPosition(int x, int y)
{
    QLinkedList<HistoryElem>::iterator it = m_history.begin() + m_historyIndex;
    (*it).setContentsX(x);
    (*it).setContentsY(y);
}

void UrlNavigator::keyReleaseEvent(QKeyEvent* event)
{
    KHBox::keyReleaseEvent(event);
    if (isUrlEditable() && (event->key() == Qt::Key_Escape)) {
        setUrlEditable(false);
    }
}

void UrlNavigator::slotReturnPressed(const QString& text)
{
    // Parts of the following code have been taken
    // from the class KateFileSelector located in
    // kate/app/katefileselector.hpp of Kate.
    // Copyright (C) 2001 Christoph Cullmann <cullmann@kde.org>
    // Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>
    // Copyright (C) 2001 Anders Lund <anders.lund@lund.tdcadsl.dk>

    KUrl typedUrl(text);
    if (typedUrl.hasPass()) {
        typedUrl.setPass(QString::null);
    }

    QStringList urls = m_pathBox->urls();
    urls.removeAll(typedUrl.url());
    urls.prepend(typedUrl.url());
    m_pathBox->setUrls(urls, KUrlComboBox::RemoveBottom);

    setUrl(typedUrl);
    // The URL might have been adjusted by UrlNavigator::setUrl(), hence
    // synchronize the result in the path box.
    m_pathBox->setUrl(url());
}

void UrlNavigator::slotUrlActivated(const KUrl& url)
{
    setUrl(url);
}

void UrlNavigator::slotRemoteHostActivated()
{
    KUrl u = url();

    QString host = m_host->text();
    QString user;

    int marker = host.indexOf("@");
    if (marker != -1)
    {
        user = host.left(marker);
        u.setUser(user);
        host = host.right(host.length() - marker - 1);
    }

    marker = host.indexOf("/");
    if (marker != -1)
    {
        u.setPath(host.right(host.length() - marker));
        host.truncate(marker);
    }
    else
    {
        u.setPath("");
    }

    if (m_protocols->currentProtocol() != u.protocol() ||
        host != u.host() ||
        user != u.user())
    {
        u.setProtocol(m_protocols->currentProtocol());
        u.setHost(m_host->text());

        //TODO: get rid of this HACK for file:///!
        if (u.protocol() == "file")
        {
            u.setHost("");
            if (u.path().isEmpty())
            {
                u.setPath("/");
            }
        }

        setUrl(u);
    }
}

void UrlNavigator::slotProtocolChanged(const QString& protocol)
{
    KUrl url;
    url.setProtocol(protocol);
    //url.setPath(KProtocolInfo::protocolClass(protocol) == ":local" ? "/" : "");
    url.setPath("/");
    QLinkedList<UrlNavigatorButton*>::const_iterator it = m_navButtons.begin();
    const QLinkedList<UrlNavigatorButton*>::const_iterator itEnd = m_navButtons.end();
    while (it != itEnd) {
        (*it)->close();
        (*it)->deleteLater();
        ++it;
    }
    m_navButtons.clear();

    if (KProtocolInfo::protocolClass(protocol) == ":local") {
        setUrl(url);
    }
    else {
        if (!m_host) {
            m_protocolSeparator = new QLabel("://", this);
            m_host = new QLineEdit(this);

            connect(m_host, SIGNAL(lostFocus()),
                    this, SLOT(slotRemoteHostActivated()));
            connect(m_host, SIGNAL(returnPressed()),
                    this, SLOT(slotRemoteHostActivated()));
        }
        else {
            m_host->setText("");
        }
        m_protocolSeparator->show();
        m_host->show();
        m_host->setFocus();
    }
}

void UrlNavigator::slotRedirection(const KUrl& oldUrl, const KUrl& newUrl)
{
// kDebug() << "received redirection to " << newUrl << endl;
kDebug() << "received redirection from " << oldUrl << " to " << newUrl << endl;
/*    UrlStack::iterator it = m_urls.find(oldUrl);
    if (it != m_urls.end())
    {
        m_urls.erase(++it, m_urls.end());
    }

    m_urls.append(newUrl);*/
}

void UrlNavigator::slotClicked()
{
    if (isUrlEditable()) {
        m_pathBox->setFocus();
        updateContent();
    }
    else {
        setUrl(m_pathBox->currentText());
        emit requestActivation();
    }
}

void UrlNavigator::updateHistoryElem()
{
    assert(m_historyIndex >= 0);
    const KFileItem* item = 0; // TODO: m_dolphinView->currentFileItem();
    if (item != 0) {
        QLinkedList<HistoryElem>::iterator it = m_history.begin() + m_historyIndex;
        (*it).setCurrentFileName(item->name());
    }
}

void UrlNavigator::updateContent()
{
    m_bookmarkSelector->updateSelection(url());

    m_toggleButton->setToolTip(QString());
    QString path(url().pathOrUrl());

    // TODO: prevent accessing the DolphinMainWindow out from this scope
    //const QAction* action = dolphinView()->mainWindow()->actionCollection()->action("editable_location");
    // TODO: registry of default shortcuts
    //QString shortcut = action? action->shortcut().toString() : "Ctrl+L";
    const QString shortcut = "Ctrl+L";

    if (m_toggleButton->isChecked()) {
        delete m_protocols; m_protocols = 0;
        delete m_protocolSeparator; m_protocolSeparator = 0;
        delete m_host; m_host = 0;
        deleteButtons();

        m_toggleButton->setToolTip(i18n("Browse (%1, Escape)", shortcut));

        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Fixed);
        m_pathBox->show();
        m_pathBox->setUrl(url());
    }
    else {
        m_toggleButton->setToolTip(i18n("Edit location (%1)", shortcut));

        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_pathBox->hide();

        // get the data from the currently selected bookmark
        KBookmark bookmark = m_bookmarkSelector->selectedBookmark();
        //int bookmarkIndex = m_bookmarkSelector->selectedIndex();

        QString bookmarkPath;
        if (bookmark.isNull()) {
            // No bookmark is a part of the current Url.
            // The following code tries to guess the bookmark
            // path. E. g. "fish://root@192.168.0.2/var/lib" writes
            // "fish://root@192.168.0.2" to 'bookmarkPath', which leads to the
            // navigation indication 'Custom Path > var > lib".
            int idx = path.indexOf(QString("//"));
            idx = path.indexOf("/", (idx < 0) ? 0 : idx + 2);
            bookmarkPath = (idx < 0) ? path : path.left(idx);
        }
        else {
            bookmarkPath = bookmark.url().pathOrUrl();
        }
        const uint len = bookmarkPath.length();

        // calculate the start point for the URL navigator buttons by counting
        // the slashs inside the bookmark URL
        int slashCount = 0;
        for (uint i = 0; i < len; ++i) {
            if (bookmarkPath.at(i) == QChar('/')) {
                ++slashCount;
            }
        }
        if ((len > 0) && bookmarkPath.at(len - 1) == QChar('/')) {
            assert(slashCount > 0);
            --slashCount;
        }

        if (!url().isLocalFile() && bookmark.isNull()) {
            QString protocol = url().protocol();
            if (!m_protocols) {
                deleteButtons();
                m_protocols = new ProtocolCombo(protocol, this);
                connect(m_protocols, SIGNAL(activated(const QString&)),
                        this, SLOT(slotProtocolChanged(const QString&)));
            }
            else {
                m_protocols->setProtocol(protocol);
            }
            m_protocols->show();

            if (KProtocolInfo::protocolClass(protocol) != ":local") {
                QString hostText = url().host();

                if (!url().user().isEmpty()) {
                    hostText = url().user() + "@" + hostText;
                }

                if (!m_host) {
                    m_protocolSeparator = new QLabel("://", this);
                    m_host = new QLineEdit(hostText, this);

                    connect(m_host, SIGNAL(lostFocus()),
                            this, SLOT(slotRemoteHostActivated()));
                    connect(m_host, SIGNAL(returnPressed()),
                            this, SLOT(slotRemoteHostActivated()));
                }
                else {
                    m_host->setText(hostText);
                }
                m_protocolSeparator->show();
                m_host->show();
            }
            else {
                delete m_protocolSeparator; m_protocolSeparator = 0;
                delete m_host; m_host = 0;
            }
        }
        else if (m_protocols) {
            m_protocols->hide();

            if (m_host) {
                m_protocolSeparator->hide();
                m_host->hide();
            }
        }

        updateButtons(path, slashCount);
    }
}

void UrlNavigator::updateButtons(const QString& path, int startIndex)
{
    QLinkedList<UrlNavigatorButton*>::iterator it = m_navButtons.begin();
    const QLinkedList<UrlNavigatorButton*>::const_iterator itEnd = m_navButtons.end();
    bool createButton = false;

    int idx = startIndex;
    bool hasNext = true;
    do {
        createButton = (it == itEnd);

        const QString dirName = path.section('/', idx, idx);
        const bool isFirstButton = (idx == startIndex);
        hasNext = isFirstButton || !dirName.isEmpty();
        if (hasNext) {
            QString text;
            if (isFirstButton) {
                // the first URL navigator button should get the name of the
                // bookmark instead of the directory name
                const KBookmark bookmark = m_bookmarkSelector->selectedBookmark();
                text = bookmark.text();
                if (text.isEmpty()) {
                    if (url().isLocalFile()) {
                        text = i18n("Custom Path");
                    }
                    else {
                        ++idx;
                        continue;
                    }
                }
            }

            UrlNavigatorButton* button = 0;
            if (createButton) {
                button = new UrlNavigatorButton(idx, this);
            }
            else {
                button = *it;
                button->setIndex(idx);
            }

            if (isFirstButton) {
                button->setText(text);
            }

            if (createButton) {
                button->show();
                m_navButtons.append(button);
            }
            else {
                ++it;
            }
            ++idx;
        }
    } while (hasNext);

    // delete buttons which are not used anymore
    QLinkedList<UrlNavigatorButton*>::iterator itBegin = it;
    while (it != itEnd) {
        (*it)->close();
        (*it)->deleteLater();
        ++it;
    }
    m_navButtons.erase(itBegin, m_navButtons.end());
}

void UrlNavigator::deleteButtons()
{
    QLinkedList<UrlNavigatorButton*>::iterator itBegin = m_navButtons.begin();
    QLinkedList<UrlNavigatorButton*>::iterator itEnd = m_navButtons.end();
    QLinkedList<UrlNavigatorButton*>::iterator it = itBegin;
    while (it != itEnd) {
        (*it)->close();
        (*it)->deleteLater();
        ++it;
    }
    m_navButtons.erase(itBegin, itEnd);
}

#include "urlnavigator.moc"
