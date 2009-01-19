/***************************************************************************
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

class KLineEdit;
class KUrl;
class QToolButton;

/**
 * @brief Input box for searching files with Nepomuk.
 */
class DolphinSearchBox : public QWidget
{
    Q_OBJECT

public:
    DolphinSearchBox(QWidget* parent = 0);
    virtual ~DolphinSearchBox();

protected:
    virtual bool event(QEvent* event);

signals:
    /**
     * Is emitted when the user pressed Return or Enter
     * and provides the Nepomuk URL that should be used
     * for searching.
     */
    void search(const KUrl& url);

private slots:
    void emitSearchSignal();

private:
    QToolButton* m_searchButton;
    KLineEdit* m_searchInput;
};

#endif
