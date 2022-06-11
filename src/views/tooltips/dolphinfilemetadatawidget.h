/*
 * SPDX-FileCopyrightText: 2010 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2008 Fredrik Höglund <fredrik@kde.org>
 * SPDX-FileCopyrightText: 2012 Mark Gaiser <markg85@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINFILEMETADATAWIDGET_H
#define DOLPHINFILEMETADATAWIDGET_H

#include <config-dolphin.h>

#include <QWidget>

class KFileItemList;
class QLabel;

namespace Baloo {
    class FileMetaDataWidget;
}

/**
 * @brief Widget that shows the meta information and a preview of one
 *        or more files inside a KToolTipWidget.
 */
class DolphinFileMetaDataWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DolphinFileMetaDataWidget(QWidget* parent = nullptr);
    ~DolphinFileMetaDataWidget() override;

    void setPreview(const QPixmap& pixmap);
    QPixmap preview() const;

    void setName(const QString& name);
    QString name() const;

    /**
     * Sets the items for which the meta data should be shown.
     * The signal metaDataRequestFinished() will be emitted,
     * as soon as the meta data for the items has been received.
     */
    void setItems(const KFileItemList& items);
    KFileItemList items() const;

Q_SIGNALS:
    /**
     * Is emitted after the meta data has been received for the items
     * set by DolphinFileMetaDataWidget::setItems().
     */
    void metaDataRequestFinished(const KFileItemList& items);

    /**
     * Is emitted when the user clicks a tag or a link
     * in the metadata widget.
     */
    void urlActivated(const QUrl& url);

private:
    QLabel* m_preview;
    QLabel* m_name;
    Baloo::FileMetaDataWidget* m_fileMetaDataWidget;
};

#endif
