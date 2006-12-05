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
#ifndef VIEWPROPSAPPLIER_H
#define VIEWPROPSAPPLIER_H

#include <QObject>
#include <kdirlister.h>

class KUrl;
class KDirLister;
class ViewProperties;

/**
 * @brief Applies view properties recursively to sub directories.
 *
 * BIG TODO: This class must be rewritten by inheriting from KJob (see
 * also KDirSize as reference). The current implementation gets nuts
 * for deep directory hierarchies, as an incredible number of parallel
 * KDirLister instances will be created.
 *
 * Anyhow when writing this class I was not aware about the capabilities
 * of the KJob class -> lessons learned ;-)
 */
class ViewPropsApplier : public QObject
{
    Q_OBJECT

public:
    /**
     * @param dir       Directory where the view properties should be applied to
     *                  (including sub directories).
     * @param viewProps View properties for the directory \a dir including its
     *                  sub directories.
     */
    ViewPropsApplier(const KUrl& dir,
                     const ViewProperties* viewProps = 0);
    virtual ~ViewPropsApplier();

signals:
    void progress(const KUrl& dir, int count);
    void completed();

private slots:
    void slotCompleted(const KUrl& dir);
    void countSubDirs();

private:
    struct RootData {
        int refCount;
        const ViewProperties* viewProps;
        ViewPropsApplier* rootApplier;
    };

    ViewPropsApplier(const KUrl& dir,
                     RootData* rootData);


    void start(const KUrl& dir);
    void emitProgress(const KUrl& dir, int count);
    void emitCompleted();

    RootData* m_rootData;
    KDirLister* m_dirLister;
};

#endif
