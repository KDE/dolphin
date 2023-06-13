/*
 * SPDX-FileCopyrightText: 2008-2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "viewsettingstab.h"

#include "dolphin_compactmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_iconsmodesettings.h"
#include "dolphinfontrequester.h"
#include "global.h"
#include "settings/viewmodes/viewmodesettings.h"
#include "views/zoomlevelinfo.h"

#include <KLocalizedString>

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QHelpEvent>
#include <QRadioButton>
#include <QSpinBox>

ViewSettingsTab::ViewSettingsTab(Mode mode, QWidget *parent)
    : QWidget(parent)
    , m_mode(mode)
    , m_defaultSizeSlider(nullptr)
    , m_previewSizeSlider(nullptr)
    , m_fontRequester(nullptr)
    , m_widthBox(nullptr)
    , m_maxLinesBox(nullptr)
    , m_expandableFolders(nullptr)
{
    QFormLayout *topLayout = new QFormLayout(this);

    // Create "Icon Size" section
    const int minRange = ZoomLevelInfo::minimumLevel();
    const int maxRange = ZoomLevelInfo::maximumLevel();

    m_defaultSizeSlider = new QSlider(Qt::Horizontal);
    m_defaultSizeSlider->setPageStep(1);
    m_defaultSizeSlider->setTickPosition(QSlider::TicksBelow);
    m_defaultSizeSlider->setRange(minRange, maxRange);
    connect(m_defaultSizeSlider, &QSlider::valueChanged, this, &ViewSettingsTab::slotDefaultSliderMoved);
    topLayout->addRow(i18nc("@label:listbox", "Default icon size:"), m_defaultSizeSlider);

    m_previewSizeSlider = new QSlider(Qt::Horizontal);
    m_previewSizeSlider->setPageStep(1);
    m_previewSizeSlider->setTickPosition(QSlider::TicksBelow);
    m_previewSizeSlider->setRange(minRange, maxRange);
    connect(m_previewSizeSlider, &QSlider::valueChanged, this, &ViewSettingsTab::slotPreviewSliderMoved);
    topLayout->addRow(i18nc("@label:listbox", "Preview icon size:"), m_previewSizeSlider);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Create "Label" section
    m_fontRequester = new DolphinFontRequester(this);
    topLayout->addRow(i18nc("@label:listbox", "Label font:"), m_fontRequester);

    switch (m_mode) {
    case IconsMode: {
        m_widthBox = new QComboBox();
        m_widthBox->addItem(i18nc("@item:inlistbox Label width", "Small"));
        m_widthBox->addItem(i18nc("@item:inlistbox Label width", "Medium"));
        m_widthBox->addItem(i18nc("@item:inlistbox Label width", "Large"));
        m_widthBox->addItem(i18nc("@item:inlistbox Label width", "Huge"));
        topLayout->addRow(i18nc("@label:listbox", "Label width:"), m_widthBox);

        m_maxLinesBox = new QComboBox();
        m_maxLinesBox->addItem(i18nc("@item:inlistbox Maximum lines", "Unlimited"));
        m_maxLinesBox->addItem(i18nc("@item:inlistbox Maximum lines", "1"));
        m_maxLinesBox->addItem(i18nc("@item:inlistbox Maximum lines", "2"));
        m_maxLinesBox->addItem(i18nc("@item:inlistbox Maximum lines", "3"));
        m_maxLinesBox->addItem(i18nc("@item:inlistbox Maximum lines", "4"));
        m_maxLinesBox->addItem(i18nc("@item:inlistbox Maximum lines", "5"));
        topLayout->addRow(i18nc("@label:listbox", "Maximum lines:"), m_maxLinesBox);
        break;
    }
    case CompactMode: {
        m_widthBox = new QComboBox();
        m_widthBox->addItem(i18nc("@item:inlistbox Maximum width", "Unlimited"));
        m_widthBox->addItem(i18nc("@item:inlistbox Maximum width", "Small"));
        m_widthBox->addItem(i18nc("@item:inlistbox Maximum width", "Medium"));
        m_widthBox->addItem(i18nc("@item:inlistbox Maximum width", "Large"));
        topLayout->addRow(i18nc("@label:listbox", "Maximum width:"), m_widthBox);
        break;
    }
    case DetailsMode:
        m_expandableFolders = new QCheckBox(i18nc("@option:check", "Expandable"));
        topLayout->addRow(i18nc("@label:checkbox", "Folders:"), m_expandableFolders);

        // Item activation area
        m_entireRow = new QRadioButton(i18nc("@option:radio how files/folders are opened", "By clicking anywhere on the row"));
        m_iconAndNameOnly = new QRadioButton(i18nc("@option:radio how files/folders are opened", "By clicking on icon or name"));

        auto itemActivationAreaGroup = new QButtonGroup(this);
        itemActivationAreaGroup->addButton(m_entireRow);
        itemActivationAreaGroup->addButton(m_iconAndNameOnly);

        // clang-format off
        // i18n: Users can choose here if items are opened by clicking on their name/icon or by clicking in the row.
        topLayout->addRow(i18nc("@title:group", "Open files and folders:"), m_entireRow);
        // clang-format on
        topLayout->addRow(QString(), m_iconAndNameOnly);
        break;
    }

    loadSettings();

    connect(m_defaultSizeSlider, &QSlider::valueChanged, this, &ViewSettingsTab::changed);
    connect(m_previewSizeSlider, &QSlider::valueChanged, this, &ViewSettingsTab::changed);
    connect(m_fontRequester, &DolphinFontRequester::changed, this, &ViewSettingsTab::changed);

    switch (m_mode) {
    case IconsMode:
        connect(m_widthBox, &QComboBox::currentIndexChanged, this, &ViewSettingsTab::changed);
        connect(m_maxLinesBox, &QComboBox::currentIndexChanged, this, &ViewSettingsTab::changed);
        break;
    case CompactMode:
        connect(m_widthBox, &QComboBox::currentIndexChanged, this, &ViewSettingsTab::changed);
        break;
    case DetailsMode:
        connect(m_entireRow, &QCheckBox::toggled, this, &ViewSettingsTab::changed);
        connect(m_expandableFolders, &QCheckBox::toggled, this, &ViewSettingsTab::changed);
        break;
    default:
        break;
    }
}

ViewSettingsTab::~ViewSettingsTab()
{
}

void ViewSettingsTab::applySettings()
{
    switch (m_mode) {
    case IconsMode:
        IconsModeSettings::setTextWidthIndex(m_widthBox->currentIndex());
        IconsModeSettings::setMaximumTextLines(m_maxLinesBox->currentIndex());
        IconsModeSettings::self()->save();
        break;
    case CompactMode:
        CompactModeSettings::setMaximumTextWidthIndex(m_widthBox->currentIndex());
        CompactModeSettings::self()->save();
        break;
    case DetailsMode:
        auto detailsModeSettings = DetailsModeSettings::self();
        // We need side-padding when the full row is a click target to still be able to not click items.
        // So here the default padding is enabled when the full row highlight is enabled.
        if (m_entireRow->isChecked() && !detailsModeSettings->highlightEntireRow()) {
            const bool usedDefaults = detailsModeSettings->useDefaults(true);
            const uint defaultSidePadding = detailsModeSettings->sidePadding();
            detailsModeSettings->useDefaults(usedDefaults);
            if (detailsModeSettings->sidePadding() < defaultSidePadding) {
                detailsModeSettings->setSidePadding(defaultSidePadding);
            }
        } else if (!m_entireRow->isChecked() && detailsModeSettings->highlightEntireRow()) {
            // The full row click target is disabled so now most of the view area can be used to interact
            // with the view background. Having an extra side padding has no usability benefit in this case.
            detailsModeSettings->setSidePadding(0);
        }
        detailsModeSettings->setHighlightEntireRow(m_entireRow->isChecked());
        detailsModeSettings->setExpandableFolders(m_expandableFolders->isChecked());
        detailsModeSettings->save();
        break;
    }

    ViewModeSettings settings(m_mode);

    const int iconSize = ZoomLevelInfo::iconSizeForZoomLevel(m_defaultSizeSlider->value());
    const int previewSize = ZoomLevelInfo::iconSizeForZoomLevel(m_previewSizeSlider->value());
    settings.setIconSize(iconSize);
    settings.setPreviewSize(previewSize);

    const QFont font = m_fontRequester->currentFont();
    const bool useSystemFont = (m_fontRequester->mode() == DolphinFontRequester::SystemFont);

    settings.setUseSystemFont(useSystemFont);
    settings.setViewFont(font);

    settings.save();
}

void ViewSettingsTab::restoreDefaultSettings()
{
    ViewModeSettings settings(m_mode);
    settings.useDefaults(true);
    loadSettings();
    settings.useDefaults(false);
}

void ViewSettingsTab::loadSettings()
{
    switch (m_mode) {
    case IconsMode:
        m_widthBox->setCurrentIndex(IconsModeSettings::textWidthIndex());
        m_maxLinesBox->setCurrentIndex(IconsModeSettings::maximumTextLines());
        break;
    case CompactMode:
        m_widthBox->setCurrentIndex(CompactModeSettings::maximumTextWidthIndex());
        break;
    case DetailsMode:
        m_entireRow->setChecked(DetailsModeSettings::highlightEntireRow());
        m_iconAndNameOnly->setChecked(!m_entireRow->isChecked());
        m_expandableFolders->setChecked(DetailsModeSettings::expandableFolders());
        break;
    default:
        break;
    }

    const ViewModeSettings settings(m_mode);

    const QSize iconSize(settings.iconSize(), settings.iconSize());
    m_defaultSizeSlider->setValue(ZoomLevelInfo::zoomLevelForIconSize(iconSize));

    const QSize previewSize(settings.previewSize(), settings.previewSize());
    m_previewSizeSlider->setValue(ZoomLevelInfo::zoomLevelForIconSize(previewSize));

    m_fontRequester->setMode(settings.useSystemFont() ? DolphinFontRequester::SystemFont : DolphinFontRequester::CustomFont);

    QFont font(settings.viewFont());
    m_fontRequester->setCustomFont(font);
}

void ViewSettingsTab::slotDefaultSliderMoved(int value)
{
    showToolTip(m_defaultSizeSlider, value);
}

void ViewSettingsTab::slotPreviewSliderMoved(int value)
{
    showToolTip(m_previewSizeSlider, value);
}

void ViewSettingsTab::showToolTip(QSlider *slider, int value)
{
    const int size = ZoomLevelInfo::iconSizeForZoomLevel(value);
    slider->setToolTip(i18ncp("@info:tooltip", "Size: 1 pixel", "Size: %1 pixels", size));
    if (!slider->isVisible()) {
        return;
    }
    QPoint global = slider->rect().topLeft();
    global.ry() += slider->height() / 2;
    QHelpEvent toolTipEvent(QEvent::ToolTip, QPoint(0, 0), slider->mapToGlobal(global));
    QApplication::sendEvent(slider, &toolTipEvent);
}
