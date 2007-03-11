/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef ICONSIZEDIALOG_H
#define ICONSIZEDIALOG_H

#include <kdialog.h>

class QSlider;
class PixmapViewer;

/**
 * @brief Allows to adjust the size for icons and previews.
 *
 * Default usage:
 * \code
 *  IconSizeDialog dialog(this);
 *  if (dialog.exec() == QDialog::Accepted) {
 *      const int iconSize = dialog.iconSize();
 *      const int previewSize = dialog.previewSize();
 *      // ...
 *  }
 * \endcode
 */
class IconSizeDialog : public KDialog
{
    Q_OBJECT

public:
    explicit IconSizeDialog(QWidget* parent);
    virtual ~IconSizeDialog();

    int iconSize() const { return m_iconSize; }
    int previewSize() const { return m_previewSize; }

protected slots:
    virtual void slotButtonClicked(int button);

private slots:
    void updateIconSize(int value);
    void updatePreviewSize(int value);

private:
    /** Returns the icon size for the given slider value. */
    int iconSize(int sliderValue) const;

    /** Returns the slider value for the given icon size. */
    int sliderValue(int iconSize) const;

private:
    int m_iconSize;
    int m_previewSize;

    QSlider* m_iconSizeSlider;
    PixmapViewer* m_iconSizeViewer;
    QSlider* m_previewSizeSlider;
    PixmapViewer* m_previewSizeViewer;
};

#endif
