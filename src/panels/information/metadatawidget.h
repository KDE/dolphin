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

#include <QList>
#include <QWidget>

class KFileItem;
class KFileItemList;

/**
 * @brief Shows the meta data of one or more file items.
 *
 * Meta data like name, size, rating, comment, ... are
 * shown as several rows containing a description and
 * the meta data value. It is possible for the user
 * to change specific meta data like rating, tags and
 * comment.
 */
class MetaDataWidget : public QWidget
{
    Q_OBJECT

public:
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
     * Opens a dialog which allows to configure the visibility
     * of meta data.
     */
    void openConfigurationDialog();

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
    const QList<Nepomuk::Tag> tags() const;

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
     * Note that the signal is not emitted if the rating has
     * indirectly been changed by MetaDataWidget::setItem() or
     * MetaDataWidget::setItems(). In this case connect to
     * the signal loadingFinished() instead.
     */
    void ratingChanged(const int rating);

    /**
     * Is emitted after the user has changed the tags.
     * Note that the signal is not emitted if the rating has
     * indirectly been changed by MetaDataWidget::setItem() or
     * MetaDataWidget::setItems(). In this case connect to
     * the signal loadingFinished() instead.
     */
    void tagsChanged(const QList<Nepomuk::Tag>& tags);

    /**
     * Is emitted after the user has changed the comment.
     * Note that the signal is not emitted if the rating has
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

#endif
