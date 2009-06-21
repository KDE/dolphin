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

#ifndef METATEXTLABEL_H
#define METATEXTLABEL_H

#include <QWidget>

/**
 * @brief Displays general meta in several lines.
 *
 * Each line contains a label and the meta information.
 */
class MetaTextLabel : public QWidget
{
    Q_OBJECT

public:
    explicit MetaTextLabel(QWidget* parent = 0);
    virtual ~MetaTextLabel();

    void clear();
    void add(const QString& labelText, const QString& infoText);

protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void resizeEvent(QResizeEvent* event);

private:
    enum { Spacing = 2 };

    struct MetaInfo
    {
        QString label;
        QString info;
    };

    QList<MetaInfo> m_metaInfos;

    /**
     * Returns the required height in pixels for the
     * label text and information text provided by \a info.
     */
    int requiredHeight(const MetaInfo& info) const;

    /** Helper method for requiredHeight(const MetaInfo& info). */
    int requiredHeight(const QString& text, int width) const;
    int labelTextWidth() const;
    int infoTextWidth() const;

    /**
     * Returns the maximum height in pixels for the text of
     * one added line. The returned value does not contain
     * any additional spacing between texts.
     */
    int maxHeightPerLine() const;
};

inline int MetaTextLabel::labelTextWidth() const
{
    return width() / 2 - 2 * Spacing;
}

inline int MetaTextLabel::infoTextWidth() const
{
    return width() / 2;
}

#endif
