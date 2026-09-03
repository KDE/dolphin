/*
    SPDX-FileCopyrightText: 2019 Ismael Asensio <isma.af@mgmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef PARSINGUTIL_H
#define PARSINGUTIL_H

#include <QRegularExpression>

namespace Search
{

QString stripQuotes(const QString &text)
{
    if (text.length() >= 2 && text.at(0) == QLatin1Char('"') && text.back() == QLatin1Char('"')) {
        return text.mid(1, text.size() - 2);
    }
    return text;
}

QStringList splitOutsideQuotes(const QString &text)
{
    // Match groups on 3 possible conditions:
    //   - Groups with two leading quotes must close both on them (filename:""abc xyz" tuv")
    //   - Groups enclosed in quotes
    //   - Words separated by spaces
    static const QRegularExpression subTermsRegExp("(\\S*?\"\"[^\"]+\"[^\"]+\"+|\\S*?\"[^\"]+\"+|(?<=\\s|^)\\S+(?=\\s|$))");
    auto subTermsMatchIterator = subTermsRegExp.globalMatch(text);

    QStringList textParts;
    while (subTermsMatchIterator.hasNext()) {
        textParts << subTermsMatchIterator.next().captured(0);
    }
    return textParts;
}

QString trimChar(const QString &text, const QLatin1Char aChar)
{
    const int start = text.startsWith(aChar) ? 1 : 0;
    const int end = (text.length() > 1 && text.endsWith(aChar)) ? 1 : 0;

    return text.mid(start, text.length() - start - end);
}

}

#endif // PARSINGUTIL_H
