/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at) and              *
 *   and Patrice Tremblay                                                  *
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
#ifndef STATUSBARSPACEINFO_H
#define STATUSBARSPACEINFO_H

#include <qwidget.h>
#include <qstring.h>
//Added by qt3to4:
#include <QPaintEvent>
#include <kurl.h>
#include <qcolor.h>

class KDiskFreeSp;

/**
 * @short Shows the available space for the current volume as part
 *        of the status bar.
 */
class StatusBarSpaceInfo : public QWidget
{
    Q_OBJECT

public:
    StatusBarSpaceInfo(QWidget* parent);
    virtual ~StatusBarSpaceInfo();

    void setUrl(const KUrl& url);
    const KUrl& url() const { return m_url; }

protected:
    /** @see QWidget::paintEvent */
    virtual void paintEvent(QPaintEvent* event);

private slots:
    /**
     * The strange signature of this method is due to a compiler
     * bug (?). More details are given inside the class KDiskFreeSp (see
     * KDE Libs documentation).
     */
    void slotFoundMountPoint(const unsigned long& kBSize,
                             const unsigned long& kBUsed,
                             const unsigned long& kBAvailable,
                             const QString& mountPoint);
    void slotDone();

    /** Refreshs the space information for the current set Url. */
    void refresh();

private:
    /**
     * Returns a color for the progress bar by respecting
     * the given background color \a bgColor. It is assured
     * that enough contrast is given to have a visual indication.
     */
    QColor progressColor(const QColor& bgColor) const;

    KUrl m_url;
    bool m_gettingSize;
    unsigned long m_kBSize;
    unsigned long m_kBAvailable;

};

#endif
