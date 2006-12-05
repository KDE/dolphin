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
#ifndef VIEWPROPSPROGRESSINFO_H
#define VIEWPROPSPROGRESSINFO_H

#include <kdialog.h>

class KDirSize;
class KJob;
class KUrl;
class QLabel;
class QProgressBar;
class QTimer;
class ViewProperties;

/**
 * @brief Shows the progress when applying view properties recursively to
 *        sub directories.
 */
class ViewPropsProgressInfo : public KDialog
{
    Q_OBJECT

public:
    /**
     * @param parent    Parent widget of the dialog.
     * @param dir       Directory where the view properties should be applied to
     *                  (including sub directories).
     * @param viewProps View properties for the directory \a dir including its
     *                  sub directories.
     */
    ViewPropsProgressInfo(QWidget* parent,
                          const KUrl& dir,
                          const ViewProperties* viewProps);

    virtual ~ViewPropsProgressInfo();

private slots:
    void countDirs(const KUrl& dir, int count);
    //void updateDirCounter();
    //void slotResult(KJob* job);
    void applyViewProperties();
    void showProgress(const KUrl& url, int count);

private:
    int m_dirCount;
    int m_applyCount;
    const KUrl& m_dir;
    const ViewProperties* m_viewProps;
    QLabel* m_label;
    QProgressBar* m_progressBar;
    KDirSize* m_dirSizeJob;
    QTimer* m_timer;
};

#endif
