/*
    SPDX-FileCopyrightText: 2025 Felix Ernst <felixernst@kde.org>

    SPDX-License-Identifier: LGPL-2.1-only OR LGPL-3.0-only OR LicenseRef-KDE-Accepted-LGPL
*/

#ifndef FILETYPESELECTOR_H
#define FILETYPESELECTOR_H

#include "../updatablestateinterface.h"

#include <QComboBox>

namespace Search
{

class FileTypeSelector : public QComboBox, public UpdatableStateInterface
{
    Q_OBJECT

public:
    explicit FileTypeSelector(std::shared_ptr<const DolphinQuery> dolphinQuery, QWidget *parent = nullptr);

    /** Causes configurationChanged() to be emitted with a DolphinQuery object that does not contain any restriction settable by this class. */
    void removeRestriction();

Q_SIGNALS:
    /** Is emitted whenever settings have changed and a new search might be necessary. */
    void configurationChanged(const Search::DolphinQuery &dolphinQuery);

private:
    void updateState(const std::shared_ptr<const DolphinQuery> &dolphinQuery) override;
};
}

#endif // FILETYPESELECTOR_H
