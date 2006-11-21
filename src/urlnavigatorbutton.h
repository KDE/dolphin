/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at)                  *
 *   Copyright (C) 2006 by Aaron J. Seigo (<aseigo@kde.org>)               *
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
#ifndef URLNAVIGATORBUTTON_H
#define URLNAVIGATORBUTTON_H

#include <qstringlist.h>
//Added by qt3to4:
#include <QEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QDragEnterEvent>

#include <kio/global.h>

#include <urlbutton.h>

class KUrl;
class URLNavigator;
class QPainter;

namespace KIO
{
    class Job;
}

/**
 * @brief Button of the URL navigator which contains one part of an URL.
 *
 * It is possible to drop a various number of items to an URL button. In this case
 * a context menu is opened where the user must select whether he wants
 * to copy, move or link the dropped items to the URL part indicated by
 * the button.
 */
class URLNavigatorButton : public URLButton
{
    Q_OBJECT

public:
    URLNavigatorButton(int index, URLNavigator* parent = 0);
    virtual ~URLNavigatorButton();
    void setIndex(int index);
    int index() const;

protected:
    virtual void drawButton(QPainter* painter);
    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);

private slots:
    void updateNavigatorURL();
    void startPopupDelay();
    void stopPopupDelay();
    void startListJob();
    void entriesList(KIO::Job* job, const KIO::UDSEntryList& entries);
    void listJobFinished(KIO::Job* job);

private:
    int arrowWidth() const;
    bool isTextClipped() const;

    int m_index;
    QTimer* m_popupDelay;
    KIO::Job* m_listJob;
    QStringList m_subdirs;
};

#endif
