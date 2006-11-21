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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include "detailsviewsettingspage.h"
#include <qcheckbox.h>
#include <klocale.h>
#include <kdialog.h>
#include <kfontcombo.h>
#include <qspinbox.h>
#include <qlabel.h>
#include <q3grid.h>
#include <assert.h>
#include <q3buttongroup.h>
#include <qradiobutton.h>
#include <qcombobox.h>
#include <q3groupbox.h>
#include <q3groupbox.h>
#include "dolphinsettings.h"
#include "detailsmodesettings.h"
#include "dolphindetailsview.h"

DetailsViewSettingsPage::DetailsViewSettingsPage(QWidget *parent) :
    Q3VBox(parent),
    m_dateBox(0),
    m_permissionsBox(0),
    m_ownerBox(0),
    m_groupBox(0),
    m_smallIconSize(0),
    m_mediumIconSize(0),
    m_largeIconSize(0)
{
    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);

    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    assert(settings != 0);

    // create "Columns" properties
    Q3GroupBox* columnsGroup = new Q3GroupBox(4, Qt::Vertical, i18n("Columns"), this);
    columnsGroup->setSizePolicy(sizePolicy);
    columnsGroup->setMargin(margin);

    Q3HBox* visibleColumnsLayout = new Q3HBox(columnsGroup);
    m_dateBox = new QCheckBox(i18n("Date"), visibleColumnsLayout);
    m_dateBox->setChecked(settings->showDate());

    m_permissionsBox = new QCheckBox(i18n("Permissions"), visibleColumnsLayout);
    m_permissionsBox->setChecked(settings->showPermissions());

    m_ownerBox = new QCheckBox(i18n("Owner"), visibleColumnsLayout);
    m_ownerBox->setChecked(settings->showOwner());

    m_groupBox = new QCheckBox(i18n("Group"), visibleColumnsLayout);
    m_groupBox->setChecked(settings->showGroup());

    // Create "Icon" properties
    Q3ButtonGroup* iconSizeGroup = new Q3ButtonGroup(3, Qt::Horizontal, i18n("Icon Size"), this);
    iconSizeGroup->setSizePolicy(sizePolicy);
    iconSizeGroup->setMargin(margin);
    m_smallIconSize  = new QRadioButton(i18n("Small"), iconSizeGroup);
    m_mediumIconSize = new QRadioButton(i18n("Medium"), iconSizeGroup);
    m_largeIconSize  = new QRadioButton(i18n("Large"), iconSizeGroup);
    switch (settings->iconSize()) {
        case KIcon::SizeLarge:
            m_largeIconSize->setChecked(true);
            break;

        case KIcon::SizeMedium:
            m_mediumIconSize->setChecked(true);
            break;

        case KIcon::SizeSmall:
        default:
            m_smallIconSize->setChecked(true);
    }

    //new QLabel(i18n("Icon size:"), iconGroup);
    //m_iconSizeBox = new QComboBox(iconGroup);
    //m_iconSizeBox->insertItem(i18n("Small"));
    //m_iconSizeBox->insertItem(i18n("Medium"));
    //m_iconSizeBox->insertItem(i18n("Large"));

    // create "Text" properties
    Q3GroupBox* textGroup = new Q3GroupBox(2, Qt::Horizontal, i18n("Text"), this);
    textGroup->setSizePolicy(sizePolicy);
    textGroup->setMargin(margin);

    new QLabel(i18n("Font family:"), textGroup);
    m_fontFamilyBox = new KFontCombo(textGroup);
    m_fontFamilyBox->setCurrentFont(settings->fontFamily());

    new QLabel(i18n("Font size:"), textGroup);
    m_fontSizeBox = new QSpinBox(6, 99, 1, textGroup);
    m_fontSizeBox->setValue(settings->fontSize());

    // Add a dummy widget with no restriction regarding
    // a vertical resizing. This assures that the dialog layout
    // is not stretched vertically.
    new QWidget(this);
}


DetailsViewSettingsPage::~DetailsViewSettingsPage()
{
}

void DetailsViewSettingsPage::applySettings()
{
    DetailsModeSettings* settings = DolphinSettings::instance().detailsModeSettings();
    assert(settings != 0);

    settings->setShowDate(m_dateBox->isChecked());
    settings->setShowPermissions(m_permissionsBox->isChecked());
    settings->setShowOwner(m_ownerBox->isChecked());
    settings->setShowGroup(m_groupBox->isChecked());

    int iconSize = KIcon::SizeSmall;
    if (m_mediumIconSize->isChecked()) {
        iconSize = KIcon::SizeMedium;
    }
    else if (m_largeIconSize->isChecked()) {
        iconSize = KIcon::SizeLarge;
    }
    settings->setIconSize(iconSize);

    settings->setFontFamily(m_fontFamilyBox->currentFont());
    settings->setFontSize(m_fontSizeBox->value());
}

#include "detailsviewsettingspage.moc"
