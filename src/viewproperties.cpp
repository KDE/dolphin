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

#include <assert.h>

#include <qdatetime.h>
#include <qdir.h>
#include <qfile.h>

#include <klocale.h>
#include <kstandarddirs.h>
#include <kurl.h>

#include "viewproperties.h"

#include "dolphinsettings.h"

#define FILE_NAME "/.directory"

ViewProperties::ViewProperties(KUrl url) :
      m_changedProps(false),
      m_autoSave(true),
      m_subDirValidityHidden(false),
      m_node(0)
{
    url.cleanPath(true);
    m_filepath = url.path();

    if ((m_filepath.length() < 1) || (m_filepath.at(0) != QChar('/'))) {
        m_node = new ViewPropertySettings();
        return;
    }

    // we try and save it to a file in the directory being viewed
    // if the directory is not writable by the user or the directory is not local
    // we store the properties information in a local file
    QString rootDir("/"); // TODO: should this be set to the root of the bookmark, if any?
    if (url.isLocalFile()) {
        QFileInfo info(m_filepath);

        if (!info.isWritable()) {
            QString basePath = KGlobal::instance()->instanceName();
            basePath.append("/view_properties/local");
            rootDir = locateLocal("data", basePath);
            m_filepath = rootDir + m_filepath;
        }
    }
    else {
        QString basePath = KGlobal::instance()->instanceName();
        basePath.append("/view_properties/remote/").append(url.host());
        rootDir = locateLocal("data", basePath);
        m_filepath = rootDir + m_filepath;
    }

    m_node = new ViewPropertySettings(KSharedConfig::openConfig(m_filepath + FILE_NAME));

    QDir dir(m_filepath);
    const bool isValidForSubDirs = m_node->validForSubDirs();
    while ((dir.path() != rootDir) && dir.cdUp()) {
        QString parentPath(dir.path() + FILE_NAME);

        if (!QFile::exists(parentPath))
        {
            continue;
        }

        ViewPropertySettings parentNode(KSharedConfig::openConfig(dir.path() + FILE_NAME));
        const bool inheritProps = parentNode.validForSubDirs() &&
                                  (parentNode.timestamp() > m_node->timestamp());

        if (inheritProps) {
            *m_node = parentNode;
            break;
        }
    }

    if (isValidForSubDirs) {
        m_subDirValidityHidden = true;
    }
}

ViewProperties::~ViewProperties()
{
    if (m_changedProps && m_autoSave) {
        save();
    }

    delete m_node;
}

void ViewProperties::setViewMode(DolphinView::Mode mode)
{
    if (m_node->viewMode() != mode) {
        m_node->setViewMode(mode);
        updateTimeStamp();
    }
}

DolphinView::Mode ViewProperties::viewMode() const
{
    return static_cast<DolphinView::Mode>(m_node->viewMode());
}

void ViewProperties::setShowHiddenFilesEnabled(bool show)
{
    if (m_node->showHiddenFiles() != show) {
        m_node->setShowHiddenFiles(show);
        updateTimeStamp();
    }
}

bool ViewProperties::isShowHiddenFilesEnabled() const
{
    return m_node->showHiddenFiles();
}

void ViewProperties::setSorting(DolphinView::Sorting sorting)
{
    if (m_node->sorting() != sorting) {
        m_node->setSorting(sorting);
        updateTimeStamp();
    }
}

DolphinView::Sorting ViewProperties::sorting() const
{
    return static_cast<DolphinView::Sorting>(m_node->sorting());
}

void ViewProperties::setSortOrder(Qt::SortOrder sortOrder)
{
    if (m_node->sortOrder() != sortOrder) {
        m_node->setSortOrder(sortOrder);
        updateTimeStamp();
    }
}

Qt::SortOrder ViewProperties::sortOrder() const
{
    return static_cast<Qt::SortOrder>(m_node->sortOrder());
}

void ViewProperties::setValidForSubDirs(bool valid)
{
    if (m_node->validForSubDirs() != valid) {
        m_node->setValidForSubDirs(valid);
        updateTimeStamp();
    }
}

bool ViewProperties::isValidForSubDirs() const
{
    return m_node->validForSubDirs();
}

void ViewProperties::setAutoSaveEnabled(bool autoSave)
{
    m_autoSave = autoSave;
}

bool ViewProperties::isAutoSaveEnabled() const
{
    return m_autoSave;
}

void ViewProperties::updateTimeStamp()
{
    m_changedProps = true;
    m_node->setTimestamp(QDateTime::currentDateTime());
}

void ViewProperties::save()
{
    KStandardDirs::makeDir(m_filepath);
    m_node->writeConfig();
    m_changedProps = false;
}

ViewProperties& ViewProperties::operator = (const ViewProperties& props)
{
    if (&props != this) {
        m_changedProps = props.m_changedProps;
        m_autoSave = props.m_autoSave;
        m_subDirValidityHidden = props.m_subDirValidityHidden;
        m_filepath = props.m_filepath;
        m_node = new ViewPropertySettings();
        //*m_node = *(props.m_node);
    }

    return *this;
}
