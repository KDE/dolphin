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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef STATUSBARMESSAGELABEL_H
#define STATUSBARMESSAGELABEL_H

#include <dolphinstatusbar.h>

#include <QPixmap>
#include <QString>
#include <QWidget>

class QPaintEvent;
class QResizeEvent;
class QPushButton;
class QTimer;

/**
 * @brief Represents a message text label as part of the status bar.
 *
 * Dependant from the given type automatically a corresponding icon
 * is shown in front of the text. For message texts having the type
 * DolphinStatusBar::Error a dynamic color blending is done to get the
 * attention from the user.
 */
class StatusBarMessageLabel : public QWidget
{
    Q_OBJECT

public:
    explicit StatusBarMessageLabel(QWidget* parent);
    virtual ~StatusBarMessageLabel();

    void setType(DolphinStatusBar::Type type);
    DolphinStatusBar::Type type() const { return m_type; }

    void setText(const QString& text);
    const QString& text() const { return m_text; }

    // TODO: maybe a better approach is possible with the size hint
    void setMinimumTextHeight(int min);
    int minimumTextHeight() const { return m_minTextHeight; }

    /**
     * Returns the gap of the width of the current set text to the
     * width of the message label. A gap <= 0 means that the text
     * fits into the available width.
     */
    int widthGap() const;

protected:
    /** @see QWidget::paintEvent() */
    virtual void paintEvent(QPaintEvent* event);

    /** @see QWidget::resizeEvent() */
    virtual void resizeEvent(QResizeEvent* event);

private slots:
    void timerDone();

    /**
     * Increases the height of the message label so that
     * the given text fits into given area.
     */
    void assureVisibleText();

    /**
     * Returns the available width in pixels for the text.
     */
    int availableTextWidth() const;

    /**
     * Moves the close button to the upper right corner
     * of the message label.
     */
    void updateCloseButtonPosition();

private:
    enum State {
        Default,
        Illuminate,
        Illuminated,
        Desaturate
    };

    enum { GeometryTimeout = 100 };

    DolphinStatusBar::Type m_type;
    State m_state;
    int m_illumination;
    int m_minTextHeight;
    QTimer* m_timer;
    QString m_text;
    QPixmap m_pixmap;
    QPushButton* m_closeButton;

    QColor mixColors(const QColor& c1,
                     const QColor& c2,
                     int percent) const;

    int borderGap() const { return 3; }
};

#endif
