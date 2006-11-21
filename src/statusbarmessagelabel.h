/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef STATUSBARMESSAGELABEL_H
#define STATUSBARMESSAGELABEL_H

#include <qwidget.h>
#include <qpixmap.h>
#include <qstring.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <QResizeEvent>
#include <dolphinstatusbar.h>
class QTimer;

/**
 * @brief Represents a message text label as part of the status bar.
 *
 * Dependant from the given type automatically a corresponding icon
 * is shown in front of the text. For message texts having the type
 * DolphinStatusBar::Error a dynamic color blending is done to get the
 * attention from the user.
 *
 * @author Peter Penz
 */
class StatusBarMessageLabel : public QWidget
{
    Q_OBJECT

public:
    StatusBarMessageLabel(QWidget* parent);
    virtual ~StatusBarMessageLabel();

    void setType(DolphinStatusBar::Type type);
    DolphinStatusBar::Type type() const { return m_type; }

    void setText(const QString& text);
    const QString& text() const { return m_text; }

    // TODO: maybe a better approach is possible with the size hint
    void setMinimumTextHeight(int min);
    int minimumTextHeight() const { return m_minTextHeight; }

protected:
    /** @see QWidget::paintEvent */
    virtual void paintEvent(QPaintEvent* event);

    /** @see QWidget::resizeEvent */
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void timerDone();
    void assureVisibleText();

private:
    enum State {
        Default,
        Illuminate,
        Illuminated,
        Desaturate
    };

    DolphinStatusBar::Type m_type;
    State m_state;
    int m_illumination;
    int m_minTextHeight;
    QTimer* m_timer;
    QString m_text;
    QPixmap m_pixmap;

    QColor mixColors(const QColor& c1,
                     const QColor& c2,
                     int percent) const;

    int pixmapGap() const { return 3; }
};

#endif
