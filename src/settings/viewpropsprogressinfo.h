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
#ifndef VIEWPROPSPROGRESSINFO_H
#define VIEWPROPSPROGRESSINFO_H

#include <KDialog>
#include <kio/directorysizejob.h>
#include <KUrl>

class ApplyViewPropsJob;
class QLabel;
class QProgressBar;
class QTimer;
class ViewProperties;

/**
 * @brief Shows the progress information when applying view properties
 *        recursively to a given directory.
 *
 * It is possible to cancel the applying. In this case the already applied
 * view properties won't get reverted.
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
                          const ViewProperties& viewProps);

    virtual ~ViewPropsProgressInfo();

protected:
    virtual void closeEvent(QCloseEvent* event);

private slots:
    void updateProgress();
    void applyViewProperties();
    void cancelApplying();

private:
    KUrl m_dir;
    ViewProperties* m_viewProps;

    QLabel* m_label;
    QProgressBar* m_progressBar;

    KIO::DirectorySizeJob* m_dirSizeJob;
    ApplyViewPropsJob* m_applyViewPropsJob;
    QTimer* m_timer;
};

#endif
