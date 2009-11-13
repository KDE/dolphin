/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
 *   Copyright (C) 2009 by Matthias Fuchs <mat69@gmx.net>                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/
#ifndef DOLPHINSEARCHBOX_H
#define DOLPHINSEARCHBOX_H

#include <QWidget>

#include <KIcon>

class KLineEdit;
class KUrl;
class QCompleter;
class QModelIndex;
class QStandardItemModel;

/**
 * @brief Helper class used for completition for the DolphinSearchBox.
 */
class DolphinSearchCompleter : public QObject
{
    Q_OBJECT

public:
    DolphinSearchCompleter(KLineEdit *linedit);

public slots:
    void highlighted(const QModelIndex& index);
    void slotTextEdited(const QString &text);

private:
    void addCompletionItem(const QString& displayed, const QString& usedForCompletition, const QString& description = QString(), const QString& toolTip = QString(), const KIcon& icon = KIcon());

    void findText(int* wordStart, int* wordEnd, QString* newWord, int cursorPos, const QString &input);

private:
    KLineEdit* q;
    QCompleter* m_completer;
    QStandardItemModel* m_completionModel;
    QString m_userText;
    int m_wordStart;
    int m_wordEnd;
};

/**
 * @brief Input box for searching files with Nepomuk.
 */
class DolphinSearchBox : public QWidget
{
    Q_OBJECT

public:
    DolphinSearchBox(QWidget* parent = 0);
    virtual ~DolphinSearchBox();

    /**
     * Returns the text that should be used as input
     * for searching.
     */
    QString text() const;

protected:
    virtual bool event(QEvent* event);
    virtual bool eventFilter(QObject* watched, QEvent* event);

signals:
    /**
     * Is emitted when the user pressed Return or Enter
     * and provides the text that should be used as input
     * for searching.
     */
    void search(const QString& text);

    /**
     * Is emitted if the search box gets the focus and
     * requests the need for a UI that allows to adjust
     * search options. It is up to the application to ignore
     * this request.
     */
    void requestSearchOptions();

private slots:
    void emitSearchSignal();

private:
    KLineEdit* m_searchInput;
    DolphinSearchCompleter* m_completer;
};

#endif
