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

#ifndef ICONSIZEGROUPBOX_H
#define ICONSIZEGROUPBOX_H

#include <QGroupBox>

class QSlider;

/**
 * @short Provides a group box for adjusting the icon sizes.
 *
 * It is possible to adjust the default icon size and the icon
 * size when previews are used.
 */
class IconSizeGroupBox : public QGroupBox
{
    Q_OBJECT

public:
    explicit IconSizeGroupBox(QWidget* parent);
    virtual ~IconSizeGroupBox();

    void setDefaultSizeRange(int min, int max);
    void setPreviewSizeRange(int min, int max);

    void setDefaultSizeValue(int value);
    int defaultSizeValue() const;

    void setPreviewSizeValue(int value);
    int previewSizeValue() const;

signals:
    void defaultSizeChanged(int value);
    void previewSizeChanged(int value);

private slots:
    void slotDefaultSliderMoved(int value);
    void slotPreviewSliderMoved(int value);

private:
    void showToolTip(QSlider* slider, int value);

private:
    QSlider* m_defaultSizeSlider;
    QSlider* m_previewSizeSlider;
};

#endif
