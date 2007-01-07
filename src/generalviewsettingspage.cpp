/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#include "generalviewsettingspage.h"
#include "dolphinmainwindow.h"
#include "dolphinsettings.h"
#include "generalsettings.h"
#include "viewproperties.h"

#include <assert.h>

#include <QGroupBox>
#include <QRadioButton>
#include <QVBoxLayout>

#include <kdialog.h>
#include <klocale.h>
#include <kvbox.h>

GeneralViewSettingsPage::GeneralViewSettingsPage(DolphinMainWindow* mainWindow,
                                                 QWidget* parent) :
    KVBox(parent),
    m_mainWindow(mainWindow),
    m_localProps(0),
    m_globalProps(0)
{
    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    assert(settings != 0);

    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);

    QGroupBox* propsBox = new QGroupBox(i18n("View Properties"), this);

    m_localProps = new QRadioButton(i18n("Remember view properties for each folder."), propsBox);
    m_globalProps = new QRadioButton(i18n("Use common view properties for all folders."), propsBox);
    if (settings->globalViewProps()) {
        m_globalProps->setChecked(true);
    }
    else {
        m_localProps->setChecked(true);
    }

    QVBoxLayout* propsBoxLayout = new QVBoxLayout(propsBox);
    propsBoxLayout->addWidget(m_localProps);
    propsBoxLayout->addWidget(m_globalProps);

    QGroupBox* previewBox = new QGroupBox(i18n("File Previews"), this);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);
}


GeneralViewSettingsPage::~GeneralViewSettingsPage()
{
}

void GeneralViewSettingsPage::applySettings()
{
    const KUrl& url = m_mainWindow->activeView()->url();
    ViewProperties props(url);  // read current view properties

    const bool useGlobalProps = m_globalProps->isChecked();

    GeneralSettings* settings = DolphinSettings::instance().generalSettings();
    assert(settings != 0);
    settings->setGlobalViewProps(useGlobalProps);

    if (useGlobalProps) {
        // Remember the global view properties by applying the current view properties.
        // It is important that GeneralSettings::globalViewProps() is set before
        // the class ViewProperties is used, as ViewProperties uses this setting
        // to find the destination folder for storing the view properties.
        ViewProperties globalProps(url);
        globalProps.setDirProperties(props);
    }
}

#include "generalviewsettingspage.moc"
