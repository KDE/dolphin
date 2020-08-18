/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef PLACESDATASOURCE_H
#define PLACESDATASOURCE_H

#include <KUserFeedback/AbstractDataSource>

class DolphinMainWindow;

class PlacesDataSource : public KUserFeedback::AbstractDataSource
{
public:
    PlacesDataSource();

    QString name() const override;
    QString description() const override;
    QVariant data() override;

};

#endif // PLACESDATASOURCE_H
