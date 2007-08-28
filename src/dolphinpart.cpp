/* This file is part of the KDE project
   Copyright (c) 2007 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "dolphinpart.h"
#include "dolphinsortfilterproxymodel.h"
#include "dolphinview.h"

#include <kdirlister.h>
#include <kdirmodel.h>
#include <kmessagebox.h>
#include <kparts/browserextension.h>
#include <kparts/genericfactory.h>

typedef KParts::GenericFactory<DolphinPart> DolphinPartFactory;
K_EXPORT_COMPONENT_FACTORY(dolphinpart, DolphinPartFactory)

class DolphinPartBrowserExtension : public KParts::BrowserExtension
{
public:
    DolphinPartBrowserExtension( KParts::ReadOnlyPart* part )
        : KParts::BrowserExtension( part ) {}
};

DolphinPart::DolphinPart(QWidget* parentWidget, QObject* parent, const QStringList& args)
    : KParts::ReadOnlyPart(parent)
{
    Q_UNUSED(args)
    setComponentData( DolphinPartFactory::componentData() );
    m_extension = new DolphinPartBrowserExtension(this);

    m_dirLister = new KDirLister;
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(parentWidget->topLevelWidget());
    m_dirLister->setDelayedMimeTypes(true);

    //connect(m_dirLister, SIGNAL(started(KUrl)), this, SLOT(slotStarted()));
    connect(m_dirLister, SIGNAL(completed(KUrl)), this, SLOT(slotCompleted(KUrl)));
    connect(m_dirLister, SIGNAL(canceled(KUrl)), this, SLOT(slotCanceled(KUrl)));

    m_dirModel = new KDirModel(this);
    m_dirModel->setDirLister(m_dirLister);

    m_proxyModel = new DolphinSortFilterProxyModel(this);
    m_proxyModel->setSourceModel(m_dirModel);

    m_view = new DolphinView(parentWidget,
                             KUrl(),
                             m_dirLister,
                             m_dirModel,
                             m_proxyModel);
    setWidget(m_view);

    connect(m_view, SIGNAL(infoMessage(QString)),
            this, SLOT(slotInfoMessage(QString)));
    connect(m_view, SIGNAL(errorMessage(QString)),
            this, SLOT(slotErrorMessage(QString)));
    connect(m_view, SIGNAL(itemTriggered(KFileItem)),
            this, SLOT(slotItemTriggered(KFileItem)));
    connect(m_view, SIGNAL(requestContextMenu(KFileItem, const KUrl&)),
            this, SLOT(slotOpenContextMenu(KFileItem, const KUrl&)));
    connect(m_view, SIGNAL(selectionChanged(QList<KFileItem>)),
            m_extension, SIGNAL(selectionInfo(QList<KFileItem>)));

    connect(m_view, SIGNAL(requestItemInfo(KFileItem)),
            this, SLOT(slotRequestItemInfo(KFileItem)));

    // TODO connect to urlsDropped

    // TODO there was a "always open a new window" (when clicking on a directory) setting in konqueror
    // (sort of spacial navigation)

    // TODO MMB-click should do something like KonqDirPart::mmbClicked

    // TODO updating the paste action
    // if (paste) emit m_extension->setActionText( "paste", actionText );
    // emit m_extension->enableAction( "paste", paste );
}

DolphinPart::~DolphinPart()
{
    delete m_dirLister;
}

KAboutData* DolphinPart::createAboutData()
{
    return new KAboutData("dolphinpart", 0, ki18nc("@title", "Dolphin Part"), "0.1");
}

bool DolphinPart::openUrl(const KUrl& url)
{
    const QString prettyUrl = url.pathOrUrl();
    emit setWindowCaption(prettyUrl);
    emit m_extension->setLocationBarUrl(prettyUrl);
    const bool reload = arguments().reload();
    if (m_view->url() == url && !reload) { // DolphinView won't do anything in that case, so don't emit started
        return true;
    }
    setUrl(url); // remember it at the KParts level
    m_view->setUrl(url);
    if (reload)
        m_view->reload();
    emit started(0); // get the wheel to spin
    return true;
}

void DolphinPart::slotCompleted(const KUrl& url)
{
    Q_UNUSED(url)
    emit completed();
}

void DolphinPart::slotCanceled(const KUrl& url)
{
    slotCompleted(url);
}

void DolphinPart::slotInfoMessage(const QString& msg)
{
    emit setStatusBarText(msg);
}

void DolphinPart::slotErrorMessage(const QString& msg)
{
    KMessageBox::error(m_view, msg);
}

void DolphinPart::slotRequestItemInfo(const KFileItem& item)
{
    emit m_extension->mouseOverInfo(&item);
}

void DolphinPart::slotItemTriggered(const KFileItem& item)
{
    emit m_extension->openUrlRequest(item.url());
}

void DolphinPart::slotOpenContextMenu(const KFileItem& item, const KUrl&)
{
    // TODO KonqKfmIconView had if ( !rootItem->isWritable() )
    //            popupFlags |= KParts::BrowserExtension::NoDeletion;

    // and when clicking on the viewport:
    //        KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::ShowNavigationItems | KParts::BrowserExtension::ShowUp;


    KFileItem* itemCopy = new KFileItem(item); // ugly
    KFileItemList items; items.append(itemCopy);
    emit m_extension->popupMenu( 0, QCursor::pos(), items );
    delete itemCopy;
}

#include "dolphinpart.moc"
