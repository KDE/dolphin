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
#include <kparts/genericfactory.h>
#include "dolphinview.h"
#include "dolphinsortfilterproxymodel.h"
#include <kdirmodel.h>
#include <kdirlister.h>

typedef KParts::GenericFactory<DolphinPart> DolphinPartFactory;
K_EXPORT_COMPONENT_FACTORY(dolphinpart, DolphinPartFactory)

DolphinPart::DolphinPart(QWidget* parentWidget, QObject* parent, const QStringList& args)
    : KParts::ReadOnlyPart(parent)
{
    Q_UNUSED(args)
    setComponentData( DolphinPartFactory::componentData() );
    //setBrowserExtension( new DolphinPartBrowserExtension( this ) );

    m_dirLister = new KDirLister;
    m_dirLister->setAutoUpdate(true);
    m_dirLister->setMainWindow(parentWidget->topLevelWidget());
    m_dirLister->setDelayedMimeTypes(true);

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
}

DolphinPart::~DolphinPart()
{
    delete m_dirLister;
}

KAboutData* DolphinPart::createAboutData()
{
    return new KAboutData("dolphinpart", I18N_NOOP( "Dolphin Part" ), "0.1");
}

bool DolphinPart::openUrl(const KUrl& url)
{
    m_view->setUrl(url);
    return true;
}

#include "dolphinpart.moc"
