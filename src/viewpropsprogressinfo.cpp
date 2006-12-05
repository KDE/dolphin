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

#include "viewpropsprogressinfo.h"
#include "viewpropsapplier.h"
#include "viewproperties.h"

#include <QLabel>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>

#include <kdirsize.h>
#include <klocale.h>
#include <kurl.h>
#include <kio/jobclasses.h>

ViewPropsProgressInfo::ViewPropsProgressInfo(QWidget* parent,
                                             const KUrl& dir,
                                             const ViewProperties* viewProps) :
    KDialog(parent),
    m_dirCount(0),
    m_applyCount(0),
    m_dir(dir),
    m_viewProps(viewProps),
    m_label(0),
    m_progressBar(0),
    m_dirSizeJob(0),
    m_timer(0)
{
    setCaption(i18n("Applying view properties"));
    setButtons(KDialog::Cancel);

    QWidget* main = new QWidget();
    QVBoxLayout* topLayout = new QVBoxLayout();

    m_label = new QLabel(i18n("Counting folders: %1", 0), main);
    m_progressBar = new QProgressBar(main);

    topLayout->addWidget(m_label);
    topLayout->addWidget(m_progressBar);

    main->setLayout(topLayout);
    setMainWidget(main);

    ViewPropsApplier* applier = new ViewPropsApplier(dir);
    connect(applier, SIGNAL(progress(const KUrl&, int)),
            this, SLOT(countDirs(const KUrl&, int)));
    connect(applier, SIGNAL(completed()),
            this, SLOT(applyViewProperties()));

    // TODO: use KDirSize job instead. Current issue: the signal
    // 'result' is not emitted with kdelibs 2006-12-05.

    /*m_dirSizeJob = KDirSize::dirSizeJob(dir);
    connect(m_dirSizeJob, SIGNAL(result(KJob*)),
            this, SLOT(slotResult(KJob*)));

    m_timer = new QTimer(this);
    connect(m_timer, SIGNAL(timeout()),
            this, SLOT(updateDirCounter()));
    m_timer->start(300);*/
}

ViewPropsProgressInfo::~ViewPropsProgressInfo()
{
}

void ViewPropsProgressInfo::countDirs(const KUrl& /*dir*/, int count)
{
    m_dirCount += count;
    m_label->setText(i18n("Counting folders: %1", m_dirCount));
}

/*void ViewPropsProgressInfo::updateDirCounter()
{
    const int subdirs = m_dirSizeJob->totalSubdirs();
    m_label->setText(i18n("Counting folders: %1", subdirs));
}

void ViewPropsProgressInfo::slotResult(KJob*)
{
    QTimer::singleShot(0, this, SLOT(applyViewProperties()));
}*/

void ViewPropsProgressInfo::applyViewProperties()
{
    //m_timer->stop();
    //const int subdirs = m_dirSizeJob->totalSubdirs();
    //m_label->setText(i18n("Folders: %1", subdirs));
    //m_progressBar->setMaximum(subdirs);

    m_label->setText(i18n("Folders: %1", m_dirCount));
    m_progressBar->setMaximum(m_dirCount);

    ViewPropsApplier* applier = new ViewPropsApplier(m_dir, m_viewProps);
    connect(applier, SIGNAL(progress(const KUrl&, int)),
            this, SLOT(showProgress(const KUrl&, int)));
    connect(applier, SIGNAL(completed()),
            this, SLOT(close()));
}

void ViewPropsProgressInfo::showProgress(const KUrl& url, int count)
{
    m_applyCount += count;
    m_progressBar->setValue(m_applyCount);
}

#include "viewpropsprogressinfo.moc"
