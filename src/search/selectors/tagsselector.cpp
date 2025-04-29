/*
    SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "tagsselector.h"

#include "../dolphinquery.h"

#include <KCoreDirLister>
#include <KLocalizedString>
#include <KProtocolInfo>

#include <QMenu>
#include <QStringList>

using namespace Search;

namespace
{
/**
 * @brief Provides the list of tags to all TagsSelectors.
 *
 * This QStringList of tags populates itself. Additional tags the user is actively searching for can be added with addTag() even though we assume that no file
 * with such a tag exists if we did not find it automatically.
 * @note Use the tagsList() function below instead of constructing TagsList objects yourself.
 */
class TagsList : public QStringList, public QObject
{
public:
    TagsList()
        : QStringList{}
    {
        m_tagsLister->openUrl(QUrl(QStringLiteral("tags:/")), KCoreDirLister::OpenUrlFlag::Reload);
        connect(m_tagsLister.get(), &KCoreDirLister::itemsAdded, this, [this](const QUrl &, const KFileItemList &items) {
            for (const KFileItem &item : items) {
                append(item.text());
            }
            removeDuplicates();
            sort(Qt::CaseInsensitive);
        });
    };

    virtual ~TagsList() = default;

    void addTag(const QString &tag)
    {
        if (contains(tag)) {
            return;
        }
        append(tag);
        sort(Qt::CaseInsensitive);
    };

    /** Used to access to the itemsAdded signal so outside users of this class know when items were added. */
    KCoreDirLister *tagsLister() const
    {
        return m_tagsLister.get();
    };

private:
    std::unique_ptr<KCoreDirLister> m_tagsLister = std::make_unique<KCoreDirLister>();
};

/**
 * @returns a list of all tags found since the construction of the first TagsSelector object.
 * @note Use this function instead of constructing additional TagsList objects.
 */
TagsList *tagsList()
{
    static TagsList *g_tagsList = new TagsList;
    return g_tagsList;
}
}

Search::TagsSelector::TagsSelector(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent)
    : QToolButton{parent}
    , UpdatableStateInterface{dolphinQuery}
{
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    setPopupMode(QToolButton::InstantPopup);

    auto menu = new QMenu{this};
    setMenu(menu);
    connect(menu, &QMenu::aboutToShow, this, [this]() {
        TagsList *tags = tagsList();
        // The TagsList might not have been updated for a while and new tags might be available. We update now, but this is unfortunately not instant.
        // However this selector is connected to the itemsAdded() signal, so we will add any new tags eventually.
        tags->tagsLister()->updateDirectory(tags->tagsLister()->url());
        updateMenu(m_searchConfiguration);
    });

    TagsList *tags = tagsList();
    if (tags->isEmpty()) {
        // Either there really are no tags or the TagsList has not loaded the tags yet. It only begins loading the first time tagsList() is globally called.
        setEnabled(false);
        connect(
            tags->tagsLister(),
            &KCoreDirLister::itemsAdded,
            this,
            [this]() {
                setEnabled(true);
            },
            Qt::SingleShotConnection);
    }

    connect(tags->tagsLister(), &KCoreDirLister::itemsAdded, this, [this, menu]() {
        if (menu->isVisible()) {
            updateMenu(m_searchConfiguration);
        }
    });

    updateStateToMatch(std::move(dolphinQuery));
}

void TagsSelector::removeRestriction()
{
    Q_ASSERT(!m_searchConfiguration->requiredTags().isEmpty());
    DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
    searchConfigurationCopy.setRequiredTags({});
    Q_EMIT configurationChanged(std::move(searchConfigurationCopy));
}

void TagsSelector::updateMenu(const std::shared_ptr<const DolphinQuery> &dolphinQuery)
{
    if (!KProtocolInfo::isKnownProtocol(QStringLiteral("tags"))) {
        return;
    }
    const bool menuWasVisible = menu()->isVisible();
    if (menuWasVisible) {
        menu()->hide(); // The menu needs to be hidden now, then updated, and then shown again.
    }
    // Delete all existing actions in the menu
    const auto actions = menu()->actions();
    for (QAction *action : actions) {
        action->deleteLater();
    }
    menu()->clear();
    // Populate the menu
    const TagsList *tags = tagsList();
    const bool onlyOneTagExists = tags->count() == 1;

    for (const QString &tag : *tags) {
        QAction *tagAction = new QAction{QIcon::fromTheme(QStringLiteral("tag")), tag, menu()};
        tagAction->setCheckable(true);
        tagAction->setChecked(dolphinQuery->requiredTags().contains(tag));
        connect(tagAction, &QAction::triggered, this, [this, tag, onlyOneTagExists](bool checked) {
            QStringList requiredTags = m_searchConfiguration->requiredTags();
            if (checked == requiredTags.contains(tag)) {
                return; // Already selected.
            }
            if (checked) {
                requiredTags.append(tag);
            } else {
                requiredTags.removeOne(tag);
            }
            DolphinQuery searchConfigurationCopy = *m_searchConfiguration;
            searchConfigurationCopy.setRequiredTags(requiredTags);
            Q_EMIT configurationChanged(std::move(searchConfigurationCopy));

            if (!onlyOneTagExists) {
                // Keep the menu open to allow easier tag multi-selection.
                menu()->show();
            }
        });
        menu()->addAction(tagAction);
    }
    if (menuWasVisible) {
        menu()->show();
    }
}

void TagsSelector::updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery)
{
    if (dolphinQuery->requiredTags().count()) {
        setIcon(QIcon::fromTheme(QStringLiteral("tag")));
        setText(dolphinQuery->requiredTags().join(i18nc("list separator for file tags e.g. all images tagged 'family & party & 2025'", " && ")));
    } else {
        setIcon(QIcon{}); // No icon for the empty state
        setText(i18nc("@action:button Required tags for search results: None", "None"));
    }
    const auto tags = dolphinQuery->requiredTags();
    for (const auto &tag : tags) {
        tagsList()->addTag(tag); // We add it just in case this tag is not (or no longer) available on the system. This way the UI always works as expected.
    }
    if (menu()->isVisible()) {
        updateMenu(dolphinQuery);
    }
}
