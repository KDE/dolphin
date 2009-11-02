/*****************************************************************************
 * Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                     *
 * Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                      *
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

#ifndef KMETADATAWIDGET_H
#define KMETADATAWIDGET_H

#include <kfile_export.h>
#include <kfileitem.h>

#include <QList>
#include <QWidget>

class KUrl;

/**
 * @brief Shows the meta data of one or more file items.
 *
 * Meta data like name, size, rating, comment, ... are
 * shown as several rows containing a description and
 * the meta data value. It is possible for the user
 * to change specific meta data like rating, tags and
 * comment. The changes are stored automatically by the
 * meta data widget.
 */
class KMetaDataWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Allows to specify which general data types should be shown
     * by the meta data widget.
     * @see KMetaDataWidget::setVisibleDataTypes()
     * @see KMetaDataWidget::visibleDataTypes()
     */
    enum MetaDataType
    {
        None = 0,
        TypeData = 1,
        SizeData = 2,
        ModifiedData = 4,
        OwnerData =  8,
        PermissionsData = 16,
        RatingData = 32,
        TagsData = 64,
        CommentData = 128
    };
    Q_DECLARE_FLAGS(MetaDataTypes, MetaDataType)

    explicit KMetaDataWidget(QWidget* parent = 0);
    virtual ~KMetaDataWidget();

    /**
     * Triggers the asynchronous loading of the meta data
     * for the file item \p item. Connect to the signal
     * loadingFinished() to be able to read the meta
     * data.
     */
    void setItem(const KFileItem& item);

    /**
     * Triggers the asynchronous loading of the meta data
     * for the file items \p items. Connect to the signal
     * loadingFinished() to be able to read the meta
     * data.
     */
    void setItems(const KFileItemList& items);

    /**
     * Convenience method for KMetaDataWidget::setItem(const KFileItem&),
     * if the application has only an URL and no file item.
     * For performance reason it is recommended to use this convenience
     * method only if the application does not have a file item already.
     */
    void setItem(const KUrl& url);

    /**
     * Convenience method for KMetaDataWidget::setItems(const KFileItemList&),
     * if the application has only URLs and no file items.
     * For performance reason it is recommended to use this convenience
     * method only if the application does not have a file items already.
     */
    void setItems(const QList<KUrl>& urls);

    KFileItemList items() const;

    /**
     * Specifies which kind of data types should be shown (@see KMetaDataWidget::Data).
     * Example: metaDataWidget->setVisibleDataTypes(KMetaDataWidget::TypeData | KMetaDataWidget::ModifiedData);
     * Per default all data types are shown.
     */
    void setVisibleDataTypes(MetaDataTypes data);

    /**
     * Returns which kind of data is shown (@see KMetaDataWidget::Data).
     * Example: if (metaDataWidget->shownData() & KMetaDataWidget::TypeData) ...
     */
    MetaDataTypes visibleDataTypes() const;

private:
    class Private;
    Private* d;

    Q_PRIVATE_SLOT(d, void slotLoadingFinished())
    Q_PRIVATE_SLOT(d, void slotRatingChanged(unsigned int rating))
    Q_PRIVATE_SLOT(d, void slotTagsChanged(const QList<Nepomuk::Tag>& tags))
    Q_PRIVATE_SLOT(d, void slotCommentChanged(const QString& comment))
    Q_PRIVATE_SLOT(d, void slotMetaDataUpdateDone())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(KMetaDataWidget::MetaDataTypes)

#endif
