/*
    This file is part of the KDE project
    SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef POPUP_H
#define POPUP_H

#include "dolphinquery.h"
#include "updatablestateinterface.h"
#include "widgetmenu.h"

#include <KMessageWidget>

#include <QUrl>

class KContextualHelpButton;
class QRadioButton;
class QToolButton;

namespace Search
{
class DateSelector;
class FileTypeSelector;
class MinimumRatingSelector;
class TagsSelector;

/** @returns the localized name for the Filenamesearch search tool for use in user interfaces. */
QString filenamesearchUiName();

/** @returns the localized name for the Baloo search tool for use in user interfaces. */
QString balooUiName();

/**
 * This object contains most of the UI to set the search configuration.
 */
class Popup : public WidgetMenu, public UpdatableStateInterface
{
    Q_OBJECT

public:
    explicit Popup(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent = nullptr);

Q_SIGNALS:
    /** Is emitted whenever settings have changed and a new search might be necessary. */
    void configurationChanged(const DolphinQuery &dolphinQuery);

    /**
     * Requests for @p message with the given @p messageType to be shown to the user in a non-modal way.
     */
    void showMessage(const QString &message, KMessageWidget::MessageType messageType);

    /**
     * Requests for a progress update to be shown to the user in a non-modal way.
     * @param currentlyRunningTaskTitle     The task that is currently progressing.
     * @param installationProgressPercent   The current percentage of completion.
     */
    void showInstallationProgress(const QString &currentlyRunningTaskTitle, int installationProgressPercent);

private:
    QWidget *init() override;

    void updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery) override;

private Q_SLOTS:
    /**
     * Opens KFind if KFind is installed.
     * If KFind is not installed, this method asynchronously starts a Filelight installation using DolphinPackageInstaller. @see DolphinPackageInstaller.
     * Installation success or failure is reported through showMessage(). @see Popup::showMessage().
     * Installation progress is reported through showInstallationProgress(). @see Popup::showInstallationProgress().
     */
    void slotKFindButtonClicked();

private:
    QRadioButton *m_searchInFileNamesRadioButton = nullptr;
    QRadioButton *m_searchInFileContentsRadioButton = nullptr;
    QRadioButton *m_filenamesearchRadioButton = nullptr;
    KContextualHelpButton *m_filenamesearchContextualHelpButton = nullptr;
    QRadioButton *m_balooRadioButton = nullptr;
    KContextualHelpButton *m_balooContextualHelpButton = nullptr;
    /** A container widget for easy showing/hiding of all selectors. */
    QWidget *m_selectorsLayoutWidget = nullptr;
    /** Allows to set the file type each search result is expected to have. */
    FileTypeSelector *m_typeSelector = nullptr;
    /** Allows to set a date since when each search result needs to have been modified. */
    DateSelector *m_dateSelector = nullptr;
    /** Allows selecting the minimum rating search results are expected to have. */
    MinimumRatingSelector *m_ratingSelector = nullptr;
    /** Allows to set tags which each search result is required to have. */
    TagsSelector *m_tagsSelector = nullptr;
    /** A button that allows installing or opening KFind. */
    QToolButton *m_kFindButton = nullptr;
};

}

#endif // POPUP_H
