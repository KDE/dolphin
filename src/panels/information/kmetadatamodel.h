/*****************************************************************************
 * Copyright (C) 2010 by Peter Penz <peter.penz@gmx.at>                      *
 *                                                                           *
 * This library is free software; you can redistribute it and/or             *
 * modify it under the terms of the GNU Library General Public               *
 * License version 2 as published by the Free Software Foundation.           *
 *                                                                           *
 * This library is distributed in the hope that it will be useful,           *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU         *
 * Library General Public License for more details.                          *
 *                                                                           *
 * You should have received a copy of the GNU Library General Public License *
 * along with this library; see the file COPYING.LIB.  If not, write to      *
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,      *
 * Boston, MA 02110-1301, USA.                                               *
 *****************************************************************************/

#ifndef KMETADATAMODEL_H
#define KMETADATAMODEL_H

#include <kurl.h>

#include <QObject>
#include <QMap>
#include <QString>


#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #define DISABLE_NEPOMUK_LEGACY
    #include <nepomuk/variant.h>
#endif

class KFileItemList;
class KUrl;

/**
 * @brief Provides the data for the KMetaDataWidget.
 *
 * The default implementation provides all meta data
 * that are available due to Strigi and Nepomuk. If custom
 * meta data should be added, the method KMetaDataModel::loadData()
 * must be overwritten.
 *
 * @see KMetaDataWidget
 */
class KMetaDataModel : public QObject
{
    Q_OBJECT

public:
    explicit KMetaDataModel(QObject* parent = 0);
    virtual ~KMetaDataModel();

    /**
     * Sets the items, where the meta data should be
     * requested. The loading of the meta data is done
     * asynchronously. The signal loadingFinished() is
     * emitted, as soon as the loading has been finished.
     * The meta data can be retrieved by
     * KMetaDataModel::data() afterwards.
     */
    void setItems(const KFileItemList& items);
    KFileItemList items() const;

#ifdef HAVE_NEPOMUK
    /**
     * @return Meta data for the items that have been set by
     *         KMetaDataModel::setItems(). The method should
     *         be invoked after the signal loadingFinished() has
     *         been received (otherwise no data will be returned).
     */
    QMap<KUrl, Nepomuk::Variant> data() const;

protected:
    /**
     * Implement this method if custom meta data should be retrieved
     * and added to the list returned by KMetaDataModel::data().
     * Use KMetaDataModel::items() to get the list of items, where
     * the meta data should be requested. The method is invoked in
     * a custom thread context, so that the user interface won't get
     * blocked if the operation takes longer. The default implementation
     * returns an empty list.
     */
    virtual QMap<KUrl, Nepomuk::Variant> loadData() const;
#endif

signals:
    /**
     * Is emitted after the loading triggered by KMetaDataModel::setItems()
     * has been finished.
     */
    void loadingFinished();

private:
    class Private;
    Private* d;

    Q_PRIVATE_SLOT(d, void slotLoadingFinished())

    friend class KLoadMetaDataThread; // invokes KMetaDataObject::loadData()
};

#endif
