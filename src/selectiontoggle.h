/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef SELECTIONTOGGLE_H
#define SELECTIONTOGGLE_H

#include <QAbstractButton>
#include <QPixmap>
#include <QTimer>

/**
 * @brief Toggle button for changing the selection of an hovered item.
 * @see SelectionManager
 */
class SelectionToggle : public QAbstractButton
{
    Q_OBJECT

public:
    explicit SelectionToggle(QWidget* parent);
    virtual ~SelectionToggle();
    virtual QSize sizeHint() const;

public slots:
    virtual void setVisible(bool visible);

protected:
    virtual bool eventFilter(QObject* obj, QEvent* event);
    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void paintEvent(QPaintEvent* event);

private slots:
    void showIcon();

private:
    bool m_showIcon;
    bool m_isHovered;
    QPixmap m_icon;
    QTimer* m_timer;
};

#endif
