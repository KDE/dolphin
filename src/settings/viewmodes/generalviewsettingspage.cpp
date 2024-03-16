/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "generalviewsettingspage.h"
#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"
#include "views/viewproperties.h"

#include <KLocalizedString>

#include <QButtonGroup>
#include <QCheckBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QMimeDatabase>
#include <QVBoxLayout>

GeneralViewSettingsPage::GeneralViewSettingsPage(const QUrl &url, QWidget *parent)
    : SettingsPageBase(parent)
    , m_url(url)
{
    QFormLayout *topLayout = new QFormLayout(this);

    // Display style
    m_globalViewProps = new QRadioButton(i18nc("@option:radio", "Use common display style for all folders"));
    // i18n: The information in this sentence contradicts the preceding sentence. That's what the word "still" is communicating.
    // The previous sentence is "Use common display style for all folders".
    QLabel *globalViewPropsLabel = new QLabel(i18nc("@info", "Some special views like search, recent files, or trash will still use a custom display style."));
    globalViewPropsLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    globalViewPropsLabel->setWordWrap(true);
    globalViewPropsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    m_localViewProps = new QRadioButton(i18nc("@option:radio", "Remember display style for each folder"));
    QLabel *localViewPropsLabel = new QLabel(i18nc("@info", "Dolphin will create a hidden .directory file in each folder you change view properties for."));
    localViewPropsLabel->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    localViewPropsLabel->setWordWrap(true);
    localViewPropsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QButtonGroup *viewGroup = new QButtonGroup(this);
    viewGroup->addButton(m_globalViewProps);
    viewGroup->addButton(m_localViewProps);
    topLayout->addRow(i18nc("@title:group", "Display style: "), m_globalViewProps);
    topLayout->addRow(QString(), globalViewPropsLabel);
    topLayout->addRow(QString(), m_localViewProps);
    topLayout->addRow(QString(), localViewPropsLabel);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

    // Browsing
    m_openArchivesAsFolder = new QCheckBox(i18nc("@option:check", "Open archives as folder"));
    m_autoExpandFolders = new QCheckBox(i18nc("option:check", "Open folders during drag operations"));
    topLayout->addRow(i18nc("@title:group", "Browsing: "), m_openArchivesAsFolder);
    topLayout->addRow(QString(), m_autoExpandFolders);

    topLayout->addItem(new QSpacerItem(0, Dolphin::VERTICAL_SPACER_HEIGHT, QSizePolicy::Fixed, QSizePolicy::Fixed));

#if HAVE_BALOO
    // 'Show tooltips'
    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show tooltips"));
    topLayout->addRow(i18nc("@title:group", "Miscellaneous: "), m_showToolTips);
#endif

    // 'Show selection marker'
    m_showSelectionToggle = new QCheckBox(i18nc("@option:check", "Show selection marker"));
#if HAVE_BALOO
    topLayout->addRow(QString(), m_showSelectionToggle);
#else
    topLayout->addRow(i18nc("@title:group", "Miscellaneous: "), m_showSelectionToggle);
#endif

    // 'Inline renaming of items'
    m_renameInline = new QCheckBox(i18nc("option:check", "Rename inline"));
    topLayout->addRow(QString(), m_renameInline);

    m_hideXtrashFiles = new QCheckBox(i18nc("option:check", "Also hide backup files while hiding hidden files"));
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(QStringLiteral("application/x-trash"));
    m_hideXtrashFiles->setToolTip(i18nc("@info:tooltip %1 are the file patterns for mimetype application/x-trash",
                                        "Backup files are the files whose mime-type is application/x-trash, patterns: %1",
                                        (mime.globPatterns().join(", "))));
    topLayout->addRow(QString(), m_hideXtrashFiles);

    loadSettings();

    connect(m_localViewProps, &QRadioButton::toggled, this, &GeneralViewSettingsPage::changed);
    connect(m_globalViewProps, &QRadioButton::toggled, this, &GeneralViewSettingsPage::changed);

    connect(m_openArchivesAsFolder, &QCheckBox::toggled, this, &GeneralViewSettingsPage::changed);
    connect(m_autoExpandFolders, &QCheckBox::toggled, this, &GeneralViewSettingsPage::changed);
#if HAVE_BALOO
    connect(m_showToolTips, &QCheckBox::toggled, this, &GeneralViewSettingsPage::changed);
#endif
    connect(m_showSelectionToggle, &QCheckBox::toggled, this, &GeneralViewSettingsPage::changed);
    connect(m_renameInline, &QCheckBox::toggled, this, &GeneralViewSettingsPage::changed);
    connect(m_hideXtrashFiles, &QCheckBox::toggled, this, &GeneralViewSettingsPage::changed);
}

GeneralViewSettingsPage::~GeneralViewSettingsPage()
{
}

void GeneralViewSettingsPage::applySettings()
{
    GeneralSettings *settings = GeneralSettings::self();
    ViewProperties props(m_url); // read current view properties
    const bool useGlobalViewProps = m_globalViewProps->isChecked();
    settings->setGlobalViewProps(useGlobalViewProps);
#if HAVE_BALOO
    settings->setShowToolTips(m_showToolTips->isChecked());
#endif
    settings->setShowSelectionToggle(m_showSelectionToggle->isChecked());
    settings->setRenameInline(m_renameInline->isChecked());
    settings->setHideXTrashFile(m_hideXtrashFiles->isChecked());
    settings->setAutoExpandFolders(m_autoExpandFolders->isChecked());
    settings->setBrowseThroughArchives(m_openArchivesAsFolder->isChecked());
    settings->save();
    if (useGlobalViewProps) {
        // Remember the global view properties by applying the current view properties.
        // It is important that GeneralSettings::globalViewProps() is set before
        // the class ViewProperties is used, as ViewProperties uses this setting
        // to find the destination folder for storing the view properties.
        ViewProperties globalProps(m_url);
        globalProps.setDirProperties(props);
    }
}

void GeneralViewSettingsPage::restoreDefaults()
{
    GeneralSettings *settings = GeneralSettings::self();
    settings->useDefaults(true);
    loadSettings();
    settings->useDefaults(false);
}

void GeneralViewSettingsPage::loadSettings()
{
    const bool useGlobalViewProps = GeneralSettings::globalViewProps();
    m_openArchivesAsFolder->setChecked(GeneralSettings::browseThroughArchives());
    m_autoExpandFolders->setChecked(GeneralSettings::autoExpandFolders());
#if HAVE_BALOO
    m_showToolTips->setChecked(GeneralSettings::showToolTips());
#endif
    m_showSelectionToggle->setChecked(GeneralSettings::showSelectionToggle());
    m_renameInline->setChecked(GeneralSettings::renameInline());
    m_hideXtrashFiles->setChecked(GeneralSettings::hideXTrashFile());

    m_localViewProps->setChecked(!useGlobalViewProps);
    m_globalViewProps->setChecked(useGlobalViewProps);
}

#include "moc_generalviewsettingspage.cpp"
