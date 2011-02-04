/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#include <kurl.h>
#include <QList>
#include <QWidget>

class AbstractSearchFilterWidget;
class KLineEdit;
class KSeparator;
class QFormLayout;
class QPushButton;
class QVBoxLayout;

/**
 * @brief Input box for searching files with or without Nepomuk.
 *
 * The widget allows to specify:
 * - Where to search: Everywhere or below the current directory
 * - What to search: Filenames or content
 *
 * If Nepomuk is available and the current folder is indexed, further
 * options are offered.
 */
class DolphinSearchBox : public QWidget {
    Q_OBJECT

public:
    DolphinSearchBox(QWidget* parent = 0);
    virtual ~DolphinSearchBox();

    /**
     * Returns the text that should be used as input
     * for searching.
     */
    QString text() const;

    /**
     * Sets the current path that is used as root for
     * searching files, if "From Here" has been selected.
     */
    void setSearchPath(const KUrl& url);
    KUrl searchPath() const;

    /** @return URL that will start the searching of files. */
    KUrl urlForSearching() const;

    /**
     * Selects the whole text of the search box.
     */
    void selectAll();

    void clearText();

protected:
    virtual bool event(QEvent* event);
    virtual void showEvent(QShowEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);

signals:
    /**
     * Is emitted when a searching should be triggered
     * and provides the text that should be used as input
     * for searching.
     */
    void search(const QString& text);

    /**
     * Is emitted when the user has changed a character of
     * the text that should be used as input for searching.
     */
    void searchTextChanged(const QString& text);

    void returnPressed(const QString& text);

    /**
     * Emitted as soon as the search box should get closed.
     */
    void closeRequest();

private slots:
    void emitSearchSignal();
    void slotConfigurationChanged();
    void slotSearchTextChanged(const QString& text);
    void slotReturnPressed(const QString& text);

private:
    void initButton(QPushButton* button);
    void loadSettings();
    void saveSettings();
    void init();

    /**
     * @return URL that represents the Nepomuk query for starting the search.
     */
    KUrl nepomukUrlForSearching() const;

private:
    bool m_startedSearching;

    QVBoxLayout* m_topLayout;

    KLineEdit* m_searchInput;
    QPushButton* m_fileNameButton;
    QPushButton* m_contentButton;
    KSeparator* m_separator;
    QPushButton* m_fromHereButton;
    QPushButton* m_everywhereButton;

    KUrl m_searchPath;

    QTimer* m_startSearchTimer;
};

#endif
