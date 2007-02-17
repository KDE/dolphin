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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef URLNAVIGATORBUTTON_H
#define URLNAVIGATORBUTTON_H

#include <kio/global.h>
#include <urlbutton.h>

class KJob;
class KUrl;
class UrlNavigator;
class QPainter;
class QPaintEvent;

namespace KIO
{
    class Job;
}

/**
 * @brief Button of the URL navigator which contains one part of an URL.
 *
 * It is possible to drop a various number of items to an UrlNavigatorButton. In this case
 * a context menu is opened where the user must select whether he wants
 * to copy, move or link the dropped items to the URL part indicated by
 * the button.
 */
class UrlNavigatorButton : public UrlButton
{
    Q_OBJECT

public:
    explicit UrlNavigatorButton(int index, UrlNavigator* parent);
    virtual ~UrlNavigatorButton();
    void setIndex(int index);
    int index() const { return m_index; }

    /** @see QWidget::sizeHint() */
    virtual QSize sizeHint() const;

protected:
    virtual void paintEvent(QPaintEvent* event);
    virtual void enterEvent(QEvent* event);
    virtual void leaveEvent(QEvent* event);
    virtual void dropEvent(QDropEvent* event);
    virtual void dragEnterEvent(QDragEnterEvent* event);
    virtual void dragLeaveEvent(QDragLeaveEvent* event);

private slots:
    void updateNavigatorUrl();
    void startPopupDelay();
    void stopPopupDelay();
    void startListJob();
    void entriesList(KIO::Job* job, const KIO::UDSEntryList& entries);
    void listJobFinished(KJob* job);

private:
    int arrowWidth() const;
    bool isTextClipped() const;

private:
    int m_index;
    QTimer* m_popupDelay;
    KIO::Job* m_listJob;
    QStringList m_subdirs;
};

#endif
