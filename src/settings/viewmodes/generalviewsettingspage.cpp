/*
 * SPDX-FileCopyrightText: 2006 Peter Penz (peter.penz@gmx.at) and Patrice Tremblay
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "generalviewsettingspage.h"
#include "dolphin_generalsettings.h"
#include "dolphindebug.h"
#include "dolphinmainwindow.h"
#include "views/viewproperties.h"

#include <KActionCollection>
#include <KLocalizedString>

#include <QApplication>
#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFontDatabase>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
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
    QLabel *localViewPropsLabel = new QLabel(i18nc("@info",
                                                   "Dolphin will add file system metadata to folders you change view properties for. If that is not possible, "
                                                   "a hidden .directory file is created instead."));
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
    m_showToolTips = new QCheckBox(i18nc("@option:check", "Show item information on hover"));
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
    m_renameInline = new QCheckBox(i18nc("option:check", "Rename single items inline"));
    m_renameInline->setToolTip(i18n("Renaming multiple items is always done with a dialog window."));
    topLayout->addRow(QString(), m_renameInline);

    m_hideXtrashFiles = new QCheckBox(i18nc("option:check", "Also hide backup files while hiding hidden files"));
    QMimeDatabase db;
    QMimeType mime = db.mimeTypeForName(QStringLiteral("application/x-trash"));
    m_hideXtrashFiles->setToolTip(i18nc("@info:tooltip %1 are the file patterns for mimetype application/x-trash",
                                        "Backup files are the files whose mime-type is application/x-trash, patterns: %1",
                                        (mime.globPatterns().join(", "))));
    topLayout->addRow(QString(), m_hideXtrashFiles);

    // --------------------- //
    // START double click view background

    // list of actions allowed to be triggered by double click
    // actions were selected based on their usefulness of being triggered with the mouse
    QStringList allowedActions{"new_tab",
                               "file_new",
                               "show_places_panel",
                               "show_information_panel",
                               "show_folders_panel",
                               "show_terminal_panel",
                               "open_terminal",
                               "go_up",
                               "go_back",
                               "go_home",
                               "view_redisplay",
                               "split_view",
                               "edit_select_all",
                               "toggle_selection_mode",
                               "create_dir",
                               "show_preview",
                               "show_hidden_files",
                               "show_in_groups",
                               "view_properties"};

    // create actions combo-box and add actions
    m_doubleClickViewComboBox = new QComboBox();
    m_doubleClickViewComboBox->setAccessibleDescription(i18nc("Accessible description for combobox with actions of double click view background setting",
                                                              "Action to trigger when double clicking view background"));
    // i18n: Completes the sentence "Double-click triggers [Nothing]".
    m_doubleClickViewComboBox->addItem(QIcon::fromTheme("empty"), i18nc("@item:inlistbox", "Nothing"), QStringLiteral("none"));
    m_doubleClickViewComboBox->addItem(QIcon::fromTheme("list-add"), i18nc("@item:inlistbox", "Custom Command"), customCommand);
    m_doubleClickViewComboBox->insertSeparator(2);

    DolphinMainWindow *mainWindow = static_cast<DolphinMainWindow *>(QApplication::activeWindow());
    if (mainWindow != nullptr) {
        KActionCollection *actions = mainWindow->actionCollection();
        // get the allowed actions from actionCollection and add them to the combobox
        for (const QString &actionName : allowedActions) {
            QAction *action = actions->action(actionName);
            if (action == nullptr) {
                qCWarning(DolphinDebug) << QStringLiteral("Double click view: action `%1` was not found").arg(actionName);
                continue;
            }

            QString actionText = action->text();
            // remove ampersand used to define the action's shortcut
            actionText.remove(QLatin1Char('&'));
            m_doubleClickViewComboBox->addItem(action->icon(), actionText, action->objectName());
        }
    }
    // i18n: This sentence is incomplete because the user can choose an action that is triggered in a combobox that will appear directly after "triggers".
    // (While using a left-to-right language it will be to the right of "triggers", in a right-to-left layout it will be to the left.)
    // So please try to keep this translation in a way that it is a complete sentence when reading the content of the combobox as part of the sentence.
    // There can be many possible actions in the combobox. The default is "Nothing". Other actions are "New Tab", "Create Folder", "Show Hidden Files", …
    QLabel *doubleClickViewLabel{new QLabel(i18nc("@info", "Double-click triggers"))};
    QHBoxLayout *doubleClickViewHLayout{new QHBoxLayout()};
    QWidget *doubleClickViewWidget{new QWidget()};
    doubleClickViewWidget->setLayout(doubleClickViewHLayout);
    doubleClickViewHLayout->addWidget(doubleClickViewLabel);
    doubleClickViewHLayout->setContentsMargins(0, 0, 0, 0);
    doubleClickViewHLayout->addWidget(m_doubleClickViewComboBox);
    topLayout->addRow(i18nc("@title:group", "Background: "), doubleClickViewWidget);

    m_doubleClickViewCustomAction = new QLineEdit();
    m_doubleClickViewCustomAction->setAccessibleDescription(
        i18nc("Accessible description for custom command text field of double click view background setting",
              "Enter custom command to trigger when double clicking view background"));
    m_doubleClickViewCustomAction->setPlaceholderText(i18nc("@info:placeholder for terminal command", "Command…"));
    topLayout->addRow(QString(), m_doubleClickViewCustomAction);

    m_doubleClickViewCustomActionInfo = new QLabel(i18nc("@label",
                                                         "Use {path} to get the path of the current folder. "
                                                         "Example: dolphin {path}"));
    m_doubleClickViewCustomActionInfo->setFont(QFontDatabase::systemFont(QFontDatabase::SmallestReadableFont));
    m_doubleClickViewCustomActionInfo->setWordWrap(true);
    m_doubleClickViewCustomActionInfo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    m_doubleClickViewCustomActionInfo->hide();
    m_doubleClickViewCustomActionInfo->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard
                                                               | Qt::LinksAccessibleByKeyboard); // for accessibility
    topLayout->addRow(QString(), m_doubleClickViewCustomActionInfo);
    // END double click view background
    // --------------------- //

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
    connect(m_doubleClickViewCustomAction, &QLineEdit::textChanged, this, &GeneralViewSettingsPage::changed);
    connect(m_doubleClickViewComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &GeneralViewSettingsPage::changed);
    connect(m_doubleClickViewComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &GeneralViewSettingsPage::updateCustomActionVisibility);
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
    settings->setDoubleClickViewCustomAction(m_doubleClickViewCustomAction->text());
    settings->setDoubleClickViewAction(m_doubleClickViewComboBox->currentData().toString());
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
    int index = m_doubleClickViewComboBox->findData(GeneralSettings::doubleClickViewAction());
    m_doubleClickViewComboBox->setCurrentIndex((index == -1) ? 0 : index);
    m_doubleClickViewCustomAction->setText(GeneralSettings::doubleClickViewCustomAction());
    updateCustomActionVisibility(m_doubleClickViewComboBox->currentIndex());
}

void GeneralViewSettingsPage::updateCustomActionVisibility(int doubleClickViewComboBoxCurrentIndex)
{
    auto data = m_doubleClickViewComboBox->itemData(doubleClickViewComboBoxCurrentIndex, Qt::UserRole);
    m_doubleClickViewCustomAction->setVisible(data == customCommand);
    m_doubleClickViewCustomActionInfo->setVisible(data == customCommand);
}

#include "moc_generalviewsettingspage.cpp"
