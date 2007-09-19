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

#include "detailsviewsettingspage.h"

#include "dolphinsettings.h"
#include "dolphin_detailsmodesettings.h"

#include <kdialog.h>
#include <kfontrequester.h>
#include <klocale.h>

#include <QtGui/QButtonGroup>
#include <QtGui/QCheckBox>
#include <QtGui/QComboBox>
#include <QtGui/QGroupBox>
#include <QtGui/QGridLayout>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QSpinBox>

DetailsViewSettingsPage::DetailsViewSettingsPage(DolphinMainWindow* mainWindow,
                                                 QWidget* parent) :
    KVBox(parent),
    m_mainWindow(mainWindow),
    m_dateBox(0),
    m_permissionsBox(0),
    m_ownerBox(0),
    m_groupBox(0),
    m_smallIconSize(0),
    m_mediumIconSize(0),
    m_largeIconSize(0),
    m_fontRequester(0)
{
    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);

    // create "Columns" properties
    QGroupBox* columnsBox = new QGroupBox(i18nc("@title:group", "Columns"), this);
    columnsBox->setSizePolicy(sizePolicy);

    m_dateBox = new QCheckBox(i18nc("@option:check Columns", "Date"), this);
    m_permissionsBox = new QCheckBox(i18nc("@option:check Columns", "Permissions"), this);
    m_ownerBox = new QCheckBox(i18nc("@option:check Columns", "Owner"), this);
    m_groupBox = new QCheckBox(i18nc("@option:check Columns", "Group"), this);
    m_typeBox = new QCheckBox(i18nc("@option:check Columns", "Type"), this);

    QGridLayout* columnsLayout = new QGridLayout(columnsBox);
    columnsLayout->addWidget(m_dateBox, 0, 0);
    columnsLayout->addWidget(m_permissionsBox, 1, 0);
    columnsLayout->addWidget(m_ownerBox, 2, 0);
    columnsLayout->addWidget(m_groupBox, 0, 1);
    columnsLayout->addWidget(m_typeBox, 1, 1);

    // Create "Icon" properties
    QGroupBox* iconSizeBox = new QGroupBox(i18nc("@title:group", "Icon Size"), this);
    iconSizeBox->setSizePolicy(sizePolicy);

    m_smallIconSize  = new QRadioButton(i18nc("@option:radio Icon Size", "Small"), this);
    m_mediumIconSize = new QRadioButton(i18nc("@option:radio Icon Size", "Medium"), this);
    m_largeIconSize  = new QRadioButton(i18nc("@option:radio Icon Size", "Large"), this);

    QButtonGroup* iconSizeGroup = new QButtonGroup(this);
    iconSizeGroup->addButton(m_smallIconSize);
    iconSizeGroup->addButton(m_mediumIconSize);
    iconSizeGroup->addButton(m_largeIconSize);

    QHBoxLayout* iconSizeLayout = new QHBoxLayout(iconSizeBox);
    iconSizeLayout->addWidget(m_smallIconSize);
    iconSizeLayout->addWidget(m_mediumIconSize);
    iconSizeLayout->addWidget(m_largeIconSize);

    // create "Text" properties
    QGroupBox* textBox = new QGroupBox(i18nc("@title:group", "Text"), this);
    textBox->setSizePolicy(sizePolicy);

    QLabel* fontLabel = new QLabel(i18nc("@label:listbox", "Font:"), textBox);
    m_fontRequester = new KFontRequester(textBox);

    QHBoxLayout* textLayout = new QHBoxLayout(textBox);
    textLayout->addWidget(fontLabel);
    textLayout->addWidget(m_fontRequester);

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);

    loadSettings();
}

DetailsViewSettingsPage::~DetailsViewSettingsPage()
{
}

void DetailsViewSettingsPage::applySettings()
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();

    settings->setShowDate(m_dateBox->isChecked());
    settings->setShowPermissions(m_permissionsBox->isChecked());
    settings->setShowOwner(m_ownerBox->isChecked());
    settings->setShowGroup(m_groupBox->isChecked());
    settings->setShowType(m_typeBox->isChecked());

    int iconSize = K3Icon::SizeSmall;
    if (m_mediumIconSize->isChecked()) {
        iconSize = K3Icon::SizeMedium;
    } else if (m_largeIconSize->isChecked()) {
        iconSize = K3Icon::SizeLarge;
    }
    settings->setIconSize(iconSize);

    const QFont font = m_fontRequester->font();
    settings->setFontFamily(font.family());
    settings->setFontSize(font.pointSize());
    settings->setItalicFont(font.italic());
    settings->setBoldFont(font.bold());
}

void DetailsViewSettingsPage::restoreDefaults()
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    settings->setDefaults();
    loadSettings();
}

void DetailsViewSettingsPage::loadSettings()
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();

    m_dateBox->setChecked(settings->showDate());
    m_permissionsBox->setChecked(settings->showPermissions());
    m_ownerBox->setChecked(settings->showOwner());
    m_groupBox->setChecked(settings->showGroup());
    m_typeBox->setChecked(settings->showType());

    switch (settings->iconSize()) {
    case K3Icon::SizeLarge:
        m_largeIconSize->setChecked(true);
        break;

    case K3Icon::SizeMedium:
        m_mediumIconSize->setChecked(true);
        break;

    case K3Icon::SizeSmall:
    default:
        m_smallIconSize->setChecked(true);
    }

    QFont font(settings->fontFamily(),
               settings->fontSize());
    font.setItalic(settings->italicFont());
    font.setBold(settings->boldFont());
    m_fontRequester->setFont(font);
}

#include "detailsviewsettingspage.moc"
