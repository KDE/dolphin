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

#include "viewpropsprogressinfo.h"
#include "applyviewpropsjob.h"

#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QBoxLayout>

#include <KLocale>
#include <KIO/JobClasses>

#include <views/viewproperties.h>

ViewPropsProgressInfo::ViewPropsProgressInfo(QWidget* parent,
                                             const KUrl& dir,
                                             const ViewProperties& viewProps) :
    KDialog(parent),
    m_dir(dir),
    m_viewProps(0),
    m_label(0),
    m_progressBar(0),
    m_dirSizeJob(0),
    m_applyViewPropsJob(0),
    m_timer(0)
{
    const QSize minSize = minimumSize();
    setMinimumSize(QSize(320, minSize.height()));

    setCaption(i18nc("@title:window", "Applying View Properties"));
    setButtons(KDialog::Cancel);

    m_viewProps = new ViewProperties(dir);
    m_viewProps->setDirProperties(viewProps);

    // the view properties are stored by the ViewPropsApplierJob, so prevent
    // that the view properties are saved twice:
    m_viewProps->setAutoSaveEnabled(false);

    QWidget* main = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout();

    m_label = new QLabel(i18nc("@info:progress", "Counting folders: %1", 0), main);
    m_progressBar = new QProgressBar(main);
    m_progressBar->setMinimum(0);
    m_progressBar->setMaximum(0);
    m_progressBar->setValue(0);

    topLayout->addWidget(m_label);
    topLayout->addWidget(m_progressBar);

    main->setLayout(topLayout);
    setMainWidget(main);

    // Use the directory size job to count the number of directories first. This
    // allows to give a progress indication for the user when applying the view
    // properties later.
    m_dirSizeJob = KIO::directorySize(dir);
    connect(m_dirSizeJob, SIGNAL(result(KJob*)),
            this, SLOT(applyViewProperties()));

    // The directory size job cannot emit any progress signal, as it is not aware
    // about the total number of directories. Therefor a timer is triggered, which
    // periodically updates the current directory count.
    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(updateProgress()));
    m_timer->start(300);

    connect(this, SIGNAL(cancelClicked()), this, SLOT(cancelApplying()));
}

ViewPropsProgressInfo::~ViewPropsProgressInfo()
{
    delete m_viewProps;
    m_viewProps = 0;
}

void ViewPropsProgressInfo::closeEvent(QCloseEvent* event)
{
    m_timer->stop();
    m_applyViewPropsJob = 0;
    KDialog::closeEvent(event);
}

void ViewPropsProgressInfo::updateProgress()
{
    if (m_dirSizeJob != 0) {
        const int subdirs = m_dirSizeJob->totalSubdirs();
        m_label->setText(i18nc("@info:progress", "Counting folders: %1", subdirs));
    }

    if (m_applyViewPropsJob != 0) {
        const int progress = m_applyViewPropsJob->progress();
        m_progressBar->setValue(progress);
    }
}

void ViewPropsProgressInfo::applyViewProperties()
{
    if (m_dirSizeJob->error()) {
        return;
    }

    const int subdirs = m_dirSizeJob->totalSubdirs();
    m_label->setText(i18nc("@info:progress", "Folders: %1", subdirs));
    m_progressBar->setMaximum(subdirs);

    m_dirSizeJob = 0;

    m_applyViewPropsJob = new ApplyViewPropsJob(m_dir, *m_viewProps);
    connect(m_applyViewPropsJob, SIGNAL(result(KJob*)),
            this, SLOT(close()));
}

void ViewPropsProgressInfo::cancelApplying()
{
    if (m_dirSizeJob != 0) {
        m_dirSizeJob->kill();
        m_dirSizeJob = 0;
    }

    if (m_applyViewPropsJob != 0) {
        m_applyViewPropsJob->kill();
        m_applyViewPropsJob = 0;
    }
}

#include "viewpropsprogressinfo.moc"
