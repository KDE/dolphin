/***************************************************************************
 *   Copyright (C) 2008 by Sebastian Trueg <trueg@kde.org>                 *
 *   Copyright (C) 2009 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef METADATAWIDGET_H
#define METADATAWIDGET_H

#include <config-nepomuk.h>
#ifdef HAVE_NEPOMUK
    #include <Nepomuk/Tag>
#else
    // The HAVE_NEPOMUK macro cannot be used in combination with
    // Q_PRIVATE_SLOT, hence a workaround is used for environments
    // where Nepomuk is not available:
    namespace Nepomuk {
        typedef int Tag;
    }
#endif

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
class MetaDataWidget : public QWidget
{
    Q_OBJECT

public:
    /**
     * Allows to specifiy which general data should be hidden
     * by the meta data widget.
     * @see MetaDataWidget::setHiddenData()
     * @see MetaDataWidget::hiddenData()
     */
    enum MetaDataType
    {
        None = 0,
        TypeData = 1,
        SizeData= 2,
        ModifiedData = 4,
        OwnerData =  8,
        PermissionsData = 16,
        RatingData = 32,
        TagsData = 64,
        CommentData = 128
    };
    Q_DECLARE_FLAGS( MetaDataTypes, MetaDataType )

    explicit MetaDataWidget(QWidget* parent = 0);
    virtual ~MetaDataWidget();

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
     * Convenience method for MetaDataWidget::setItem(const KFileItem&),
     * if the application has only an URL and no file item.
     * For performance reason it is recommended to use this convenience
     * method only if the application does not have a file item already.
     */
    void setItem(const KUrl& url);

    /**
     * Convenience method for MetaDataWidget::setItems(const KFileItemList&),
     * if the application has only URLs and no file items.
     * For performance reason it is recommended to use this convenience
     * method only if the application does not have a file items already.
     */
    void setItems(const QList<KUrl>& urls);

    KFileItemList items() const;

    /**
     * Specifies which kind of data should be hidden (@see MetaDataWidget::Data).
     * Example: metaDataWidget->setHiddenData(MetaDataWidget::TypeData | ModifiedData);
     * Per default no data is hidden.
     */
    void setHiddenData(MetaDataTypes data);

    /**
     * Returns which kind of data is hidden (@see MetaDataWidget::Data).
     * Example: if (metaDataWidget->hiddenData() & MetaDataWidget::TypeData) ...
     */
    MetaDataTypes hiddenData() const;

    /**
     * Returns the rating for the currently set item(s). It is required
     * to wait for the signal loadingFinished() or ratingChanged()
     * to get a valid result.
     */
    unsigned int rating() const;

    /**
     * Returns the tags for the currently set item(s). It is required
     * to wait for the signal loadingFinished() or tagsChanged()
     * to get a valid result.
     */
    QList<Nepomuk::Tag> tags() const;

    /**
     * Returns the comment for the currently set item(s). It is required
     * to wait for the signal loadingFinished() or commentChanged()
     * to get a valid result.
     */
    QString comment() const;

signals:
    /**
     * Is emitted if the loading of the meta data has been finished
     * after invoking MetaDataWidget::setItem() or MetaDataWidget::setItems().
     */
    void loadingFinished();

    /**
     * Is emitted after the user has changed the rating.
     * The changed rating is automatically stored already by
     * the meta data widget.
     * Note that the signal is not emitted if the rating has
     * indirectly been changed by MetaDataWidget::setItem() or
     * MetaDataWidget::setItems(). In this case connect to
     * the signal loadingFinished() instead.
     */
    void ratingChanged(const int rating);

    /**
     * Is emitted after the user has changed the tags.
     * The changed tags are automatically stored already by
     * the meta data widget.
     * Note that the signal is not emitted if the tags have
     * indirectly been changed by MetaDataWidget::setItem() or
     * MetaDataWidget::setItems(). In this case connect to
     * the signal loadingFinished() instead.
     */
    void tagsChanged(const QList<Nepomuk::Tag>& tags);

    /**
     * Is emitted after the user has changed the comment.
     * The changed comment is automatically stored already by
     * the meta data widget.
     * Note that the signal is not emitted if the comment has
     * indirectly been changed by MetaDataWidget::setItem() or
     * MetaDataWidget::setItems(). In this case connect to
     * the signal loadingFinished() instead.
     */
    void commentChanged(const QString& comment);

private:
    class Private;
    Private* d;

    Q_PRIVATE_SLOT(d, void slotLoadingFinished())
    Q_PRIVATE_SLOT(d, void slotRatingChanged(unsigned int rating))
    Q_PRIVATE_SLOT(d, void slotTagsChanged(const QList<Nepomuk::Tag>& tags))
    Q_PRIVATE_SLOT(d, void slotCommentChanged(const QString& comment))
    Q_PRIVATE_SLOT(d, void slotMetaDataUpdateDone())
};

Q_DECLARE_OPERATORS_FOR_FLAGS(MetaDataWidget::MetaDataTypes)

#endif
