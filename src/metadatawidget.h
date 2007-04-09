/***************************************************************************
 *   Copyright (C) 2007 by Sebastian Trueg <trueg@kde.org>                 *
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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef METADATA_WIDGET_H
#define METADATA_WIDGET_H

#include <QWidget>

#include <kurl.h>


class MetaDataWidget : public QWidget
{
    Q_OBJECT

public:
    MetaDataWidget(QWidget* parent = 0);
    ~MetaDataWidget();

    /**
     * \return true if the KMetaData system could be found and initialized.
     * false if KMetaData was not available at compile time or if it has not
     * been initialized properly.
     */
    static bool metaDataAvailable();

public Q_SLOTS:
    void setFile(const KUrl& url);
    void setFiles(const KUrl::List urls);

signals:
    /**
     * This signal gets emitted if the metadata for the set file was changed on the
     * outside. NOT IMPLEMENTED YET.
     */
    void metaDataChanged();

private Q_SLOTS:
    void slotCommentChanged();
    void slotRatingChanged(int r);

protected:
    bool eventFilter(QObject* obj, QEvent* event);

private:
    class Private;
    Private* d;
};

#endif
