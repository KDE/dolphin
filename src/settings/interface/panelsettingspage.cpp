/*
 * SPDX-FileCopyrightText: 2024 Benedikt Thiemer <numerfolt@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "panelsettingspage.h"
#include "dolphin_informationpanelsettings.h"
#include "global.h"
#include "kformat.h"
#include "qbuttongroup.h"

#include <KLocalizedString>

#include <QCheckBox>
#include <QFormLayout>
#include <QLabel>
#include <QRadioButton>
#include <QSpacerItem>

PanelSettingsPage::PanelSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
    , m_showPreview(nullptr)
    , m_autoPlayMedia(nullptr)
    , m_showHovered(nullptr)
    , m_dateFormatLong(nullptr)
    , m_dateFormatShort(nullptr)

{
    QFormLayout *topLayout = new QFormLayout(this);

    QString m_longDateTime = (new KFormat)->formatRelativeDateTime(QDateTime(QDate(2024, 02, 28), QTime(10, 0)), QLocale::LongFormat);
    QString m_shortDateTime = (new KFormat)->formatRelativeDateTime(QDateTime(QDate(2024, 02, 28), QTime(10, 0)), QLocale::ShortFormat);

    m_showPreview = new QCheckBox(i18nc("@option:check", "Show previews"), this);
    m_autoPlayMedia = new QCheckBox(i18nc("@option:check", "Auto-play media files"), this);
    m_showHovered = new QCheckBox(i18nc("@option:check", "Show item on hover"), this);
    m_dateFormatLong = new QRadioButton(i18nc("@option:check", "Use &long date, for example '%1'", m_longDateTime), this);
    m_dateFormatShort = new QRadioButton(i18nc("@option:check", "Use &condensed date, for example '%1'", m_shortDateTime), this);

    QButtonGroup *dateFormatGroup = new QButtonGroup(this);
    dateFormatGroup->addButton(m_dateFormatLong);
    dateFormatGroup->addButton(m_dateFormatShort);

    topLayout->addRow(i18nc("@label:checkbox", "Information Panel:"), m_showPreview);
    topLayout->addRow(QString(), m_autoPlayMedia);
    topLayout->addRow(QString(), m_showHovered);
    topLayout->addRow(QString(), m_dateFormatLong);
    topLayout->addRow(QString(), m_dateFormatShort);
    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    QLabel *contextMenuHint =
        new QLabel(i18nc("@info", "Panel settings are also available through their context menu. Open it by pressing the right mouse button on a panel."),
                   this);
    contextMenuHint->setWordWrap(true);
    topLayout->addRow(contextMenuHint);

    loadSettings();

    connect(m_showPreview, &QCheckBox::toggled, this, &PanelSettingsPage::changed);
    connect(m_showPreview, &QCheckBox::toggled, this, &PanelSettingsPage::showPreviewToggled);
    connect(m_autoPlayMedia, &QCheckBox::toggled, this, &PanelSettingsPage::changed);
    connect(m_showHovered, &QCheckBox::toggled, this, &PanelSettingsPage::changed);
    connect(m_dateFormatLong, &QRadioButton::toggled, this, &PanelSettingsPage::changed);
    connect(m_dateFormatShort, &QRadioButton::toggled, this, &PanelSettingsPage::changed);
}

PanelSettingsPage::~PanelSettingsPage()
{
}

void PanelSettingsPage::applySettings()
{
    InformationPanelSettings *settings = InformationPanelSettings::self();
    settings->setPreviewsShown(m_showPreview->isChecked());
    settings->setPreviewsAutoPlay(m_autoPlayMedia->isChecked());
    settings->setShowHovered(m_showHovered->isChecked());
    settings->setDateFormat(m_dateFormatShort->isChecked());
    settings->save();
}

void PanelSettingsPage::restoreDefaults()
{
    InformationPanelSettings *settings = InformationPanelSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void PanelSettingsPage::loadSettings()
{
    m_showPreview->setChecked(InformationPanelSettings::previewsShown());
    m_autoPlayMedia->setChecked(InformationPanelSettings::previewsAutoPlay());
    m_autoPlayMedia->setEnabled(InformationPanelSettings::previewsShown());
    m_showHovered->setChecked(InformationPanelSettings::showHovered());
    m_dateFormatLong->setChecked(!InformationPanelSettings::dateFormat());
    m_dateFormatShort->setChecked(InformationPanelSettings::dateFormat());
}

void PanelSettingsPage::showPreviewToggled()
{
    const bool checked = m_showPreview->isChecked();
    m_autoPlayMedia->setEnabled(checked);
}

#include "moc_panelsettingspage.cpp"
