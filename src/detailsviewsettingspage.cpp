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
#include <dolphinfontrequester.h>
#include <klocale.h>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QGridLayout>
#include <QLabel>
#include <QRadioButton>
#include <QtGui/QSpinBox>

DetailsViewSettingsPage::DetailsViewSettingsPage(QWidget* parent) :
    ViewSettingsPageBase(parent),
    m_smallIconSize(0),
    m_mediumIconSize(0),
    m_largeIconSize(0),
    m_fontRequester(0),
    m_expandableFolders(0)
{
    const int spacing = KDialog::spacingHint();
    const int margin = KDialog::marginHint();
    const QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);

    setSpacing(spacing);
    setMargin(margin);

    // Create "Icon" properties
    QGroupBox* iconSizeBox = new QGroupBox(i18nc("@title:group", "Icon Size"), this);
    iconSizeBox->setSizePolicy(sizePolicy);

    m_smallIconSize  = new QRadioButton(i18nc("@option:radio Icon Size", "Small"), this);
    m_mediumIconSize = new QRadioButton(i18nc("@option:radio Icon Size", "Medium"), this);
    m_largeIconSize  = new QRadioButton(i18nc("@option:radio Icon Size", "Large"), this);
    connect(m_smallIconSize,  SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_mediumIconSize, SIGNAL(toggled(bool)), this, SIGNAL(changed()));
    connect(m_largeIconSize,  SIGNAL(toggled(bool)), this, SIGNAL(changed()));

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
    m_fontRequester = new DolphinFontRequester(textBox);
    connect(m_fontRequester, SIGNAL(changed()), this, SIGNAL(changed()));

    QHBoxLayout* textLayout = new QHBoxLayout(textBox);
    textLayout->addWidget(fontLabel);
    textLayout->addWidget(m_fontRequester);

    // create "Expandable Folders" checkbox
    m_expandableFolders = new QCheckBox(i18nc("@option:check", "Expandable Folders"), this);
    connect(m_expandableFolders, SIGNAL(toggled(bool)), this, SIGNAL(changed()));

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

    int iconSize = KIconLoader::SizeSmall;
    if (m_mediumIconSize->isChecked()) {
        iconSize = KIconLoader::SizeMedium;
    } else if (m_largeIconSize->isChecked()) {
        iconSize = KIconLoader::SizeLarge;
    }
    settings->setIconSize(iconSize);

    const QFont font = m_fontRequester->font();
    settings->setUseSystemFont(m_fontRequester->mode() == DolphinFontRequester::SystemFont);
    settings->setFontFamily(font.family());
    settings->setFontSize(font.pointSize());
    settings->setItalicFont(font.italic());
    settings->setFontWeight(font.weight());

    settings->setExpandableFolders(m_expandableFolders->isChecked());
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

    switch (settings->iconSize()) {
    case KIconLoader::SizeLarge:
        m_largeIconSize->setChecked(true);
        break;

    case KIconLoader::SizeMedium:
        m_mediumIconSize->setChecked(true);
        break;

    case KIconLoader::SizeSmall:
    default:
        m_smallIconSize->setChecked(true);
    }

    if (settings->useSystemFont()) {
        m_fontRequester->setMode(DolphinFontRequester::SystemFont);
    } else {
        QFont font(settings->fontFamily(),
                   settings->fontSize());
        font.setItalic(settings->italicFont());
        font.setWeight(settings->fontWeight());
        m_fontRequester->setMode(DolphinFontRequester::CustomFont);
        m_fontRequester->setCustomFont(font);
    }

    m_expandableFolders->setChecked(settings->expandableFolders());
}

#include "detailsviewsettingspage.moc"
