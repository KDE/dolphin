/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINFEEDBACKPROVIDER_H
#define DOLPHINFEEDBACKPROVIDER_H

#include <KUserFeedback/Provider>

class DolphinFeedbackProvider : public KUserFeedback::Provider
{
    Q_OBJECT

public:
    static DolphinFeedbackProvider *instance();

    DolphinFeedbackProvider(const DolphinFeedbackProvider&) = delete;
    DolphinFeedbackProvider(DolphinFeedbackProvider&&) = delete;
    DolphinFeedbackProvider& operator=(const DolphinFeedbackProvider&) = delete;
    DolphinFeedbackProvider& operator=(DolphinFeedbackProvider&&) = delete;

private:
    DolphinFeedbackProvider();
};

#endif // DOLPHINFEEDBACKPROVIDER_H
