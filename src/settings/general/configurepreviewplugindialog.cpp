/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "configurepreviewplugindialog.h"

#include <KLibrary>
#include <KLocale>
#include <KIO/NetAccess>
#include <kio/thumbcreator.h>

#include <QApplication>
#include <QDir>
#include <QVBoxLayout>

ConfigurePreviewPluginDialog::ConfigurePreviewPluginDialog(const QString& pluginName,
                                                           const QString& desktopEntryName,
                                                           QWidget* parent) :
    KDialog(parent),
    m_configurationWidget(0),
    m_previewPlugin(0)
{
    KLibrary library(desktopEntryName);
    if (library.load()) {
        newCreator create = (newCreator)library.resolveFunction("new_creator");
        if (create) {
            m_previewPlugin = dynamic_cast<ThumbCreatorV2*>(create());
        }
    }

    setCaption(i18nc("@title:window", "Configure Preview for %1", pluginName));
    setMinimumWidth(400);
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    QWidget* mainWidget = new QWidget(this);
    mainWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    QVBoxLayout* layout = new QVBoxLayout(mainWidget);
    if (m_previewPlugin) {
        m_configurationWidget = m_previewPlugin->createConfigurationWidget();
        layout->addWidget(m_configurationWidget);
    }
    layout->addStretch(1);

    setMainWidget(mainWidget);

    connect(this, SIGNAL(okClicked()), this, SLOT(slotOk()));
}

ConfigurePreviewPluginDialog::~ConfigurePreviewPluginDialog()
{
}

void ConfigurePreviewPluginDialog::slotOk()
{
    m_previewPlugin->writeConfiguration(m_configurationWidget);
    // TODO: It would be great having a mechanism to tell PreviewJob that only previews
    // for a specific MIME-type should be regenerated. As this is not available yet we
    // delete the whole thumbnails directory.
    QApplication::changeOverrideCursor(Qt::BusyCursor);
    KIO::NetAccess::del(QDir::homePath() + "/.thumbnails/", this);
    QApplication::restoreOverrideCursor();

}

#include "configurepreviewplugindialog.moc"
