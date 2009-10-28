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

#ifndef KCOMMENT_WIDGET
#define KCOMMENT_WIDGET

#include <QString>
#include <QWidget>

class QLabel;

/**
 * @brief Allows to edit and show a comment as part of KMetaDataWidget.
 */
class KCommentWidget : public QWidget
{
    Q_OBJECT

public:
    KCommentWidget(QWidget* parent);
    virtual ~KCommentWidget();

    void setText(const QString& comment);
    QString text() const;

signals:
    void commentChanged(const QString& comment);

private slots:
    void slotLinkActivated(const QString& link);

private:
    QLabel* m_label;
    QString m_comment;
};

#endif
