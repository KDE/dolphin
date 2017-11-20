/***************************************************************************
 *   Copyright (C) 2006-2010 by Peter Penz <peter.penz19@gmail.com>        *
 *   Copyright (C) 2006 by Gregor Kališnik <gregor@podnapisi.net>          *
 *   Copyright (C) 2012 by Stuart Citrin <ctrn3e8@gmail.com>               *
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

#ifndef FILTERBAR_H
#define FILTERBAR_H

#include <QWidget>

class QLineEdit;
class QToolButton;

/**
 * @brief Provides an input field for filtering the currently shown items.
 *
 * @author Gregor Kališnik <gregor@podnapisi.net>
 */
class FilterBar : public QWidget
{
    Q_OBJECT

public:
    explicit FilterBar(QWidget* parent = 0);
    ~FilterBar() override;

    /** Called by view container to hide this **/
    void closeFilterBar();

    /**
     * Selects the whole text of the filter bar.
     */
    void selectAll();

public slots:
    /** Clears the input field. */
    void clear();
    /** Clears the input field if the "lock button" is disabled. */
    void slotUrlChanged();
    /** The input field is cleared also if the "lock button" is released. */
    void slotToggleLockButton(bool checked);

signals:
    /**
     * Signal that reports the name filter has been
     * changed to \a nameFilter.
     */
    void filterChanged(const QString& nameFilter);

    /**
     * Emitted as soon as the filterbar should get closed.
     */
    void closeRequest();

    /*
     * Emitted as soon as the focus should be returned back to the view.
     */
    void focusViewRequest();

protected:
    void showEvent(QShowEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    QLineEdit* m_filterInput;
    QToolButton* m_lockButton;
};

#endif
