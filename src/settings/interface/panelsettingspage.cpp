/*
 * SPDX-FileCopyrightText: 2024 Benedikt Thiemer <numerfolt@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#include "panelsettingspage.h"
#include "dolphin_folderspanelsettings.h"
#if HAVE_BALOO
#include "dolphin_informationpanelsettings.h"
#endif
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
    , m_limitFoldersPanelToHome(nullptr)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setVerticalSizeConstraint(QLayout::SetFixedSize);

    QFormLayout *topLayout = new QFormLayout();
    layout->addLayout(topLayout);
    topLayout->setFormAlignment(Qt::AlignHCenter);
    topLayout->setSizeConstraint(QLayout::SizeConstraint::SetFixedSize);

#if HAVE_BALOO
    KFormat formatter;
    QString m_longDateTime = formatter.formatRelativeDateTime(QDateTime(QDate(2024, 02, 28), QTime(10, 0)), QLocale::LongFormat);
    QString m_shortDateTime = formatter.formatRelativeDateTime(QDateTime(QDate(2024, 02, 28), QTime(10, 0)), QLocale::ShortFormat);

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
#endif

    m_limitFoldersPanelToHome = new QCheckBox(i18nc("@option:check", "Limit to home directory when inside it"), this);
    m_limitFoldersPanelToHome->setToolTip(i18nc("@info:tooltip", "Start the folder tree of the Folders panel at your home folder while you are inside it."));
    m_limitFoldersPanelToHome->setWhatsThis(xi18nc("@info:whatsthis",
                                                   "<para>When this is enabled and the current folder is in your home folder, the Folders panel "
                                                   "shows only your home folder and the folders inside it. The root folder and the other drives "
                                                   "are not shown in the panel then.</para>"
                                                   "<para>Outside your home folder, the panel shows all folders, starting from the root "
                                                   "folder.</para>"));
    topLayout->addRow(i18nc("@label:checkbox", "Folders Panel:"), m_limitFoldersPanelToHome);

    QLabel *contextMenuHint =
        new QLabel(i18nc("@info", "Panel settings are also available through their context menu. Open it by pressing the right mouse button on a panel."),
                   this);
    contextMenuHint->setWordWrap(true);
    contextMenuHint->setMinimumWidth(topLayout->minimumSize().width());
    layout->addWidget(contextMenuHint, 0, Qt::AlignTop | Qt::AlignHCenter);

    loadSettings();

#if HAVE_BALOO
    connect(m_showPreview, &QCheckBox::toggled, this, &PanelSettingsPage::changed);
    connect(m_showPreview, &QCheckBox::toggled, this, &PanelSettingsPage::showPreviewToggled);
    connect(m_autoPlayMedia, &QCheckBox::toggled, this, &PanelSettingsPage::changed);
    connect(m_showHovered, &QCheckBox::toggled, this, &PanelSettingsPage::changed);
    connect(m_dateFormatLong, &QRadioButton::toggled, this, &PanelSettingsPage::changed);
    connect(m_dateFormatShort, &QRadioButton::toggled, this, &PanelSettingsPage::changed);
#endif
    connect(m_limitFoldersPanelToHome, &QCheckBox::toggled, this, &PanelSettingsPage::changed);
}

PanelSettingsPage::~PanelSettingsPage()
{
}

void PanelSettingsPage::applySettings()
{
#if HAVE_BALOO
    InformationPanelSettings *settings = InformationPanelSettings::self();
    settings->setPreviewsShown(m_showPreview->isChecked());
    settings->setPreviewsAutoPlay(m_autoPlayMedia->isChecked());
    settings->setShowHovered(m_showHovered->isChecked());
    settings->setDateFormat(m_dateFormatShort->isChecked());
    settings->save();
#endif

    FoldersPanelSettings *foldersPanelSettings = FoldersPanelSettings::self();
    foldersPanelSettings->setLimitFoldersPanelToHome(m_limitFoldersPanelToHome->isChecked());
    foldersPanelSettings->save();
}

void PanelSettingsPage::restoreDefaults()
{
#if HAVE_BALOO
    InformationPanelSettings *settings = InformationPanelSettings::self();
    settings->useDefaults(true);
#endif
    FoldersPanelSettings *foldersPanelSettings = FoldersPanelSettings::self();
    foldersPanelSettings->useDefaults(true);
    loadSettings();
    foldersPanelSettings->useDefaults(false);
#if HAVE_BALOO
    settings->useDefaults(false);
#endif
}

void PanelSettingsPage::loadSettings()
{
#if HAVE_BALOO
    m_showPreview->setChecked(InformationPanelSettings::previewsShown());
    m_autoPlayMedia->setChecked(InformationPanelSettings::previewsAutoPlay());
    m_autoPlayMedia->setEnabled(InformationPanelSettings::previewsShown());
    m_showHovered->setChecked(InformationPanelSettings::showHovered());
    m_dateFormatLong->setChecked(!InformationPanelSettings::dateFormat());
    m_dateFormatShort->setChecked(InformationPanelSettings::dateFormat());
#endif
    m_limitFoldersPanelToHome->setChecked(FoldersPanelSettings::limitFoldersPanelToHome());
}

#if HAVE_BALOO
void PanelSettingsPage::showPreviewToggled()
{
    const bool checked = m_showPreview->isChecked();
    m_autoPlayMedia->setEnabled(checked);
}
#endif

#include "moc_panelsettingspage.cpp"
