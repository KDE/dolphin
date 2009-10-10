/***************************************************************************
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

#ifndef TAGGING_WIDGET
#define TAGGING_WIDGET

#include <nepomuk/tag.h>
#include <QString>
#include <QWidget>

class QLabel;

class TaggingWidget : public QWidget
{
    Q_OBJECT

public:
    TaggingWidget(QWidget* parent);
    virtual ~TaggingWidget();

    void setTags(const QList<Nepomuk::Tag>& tags);
    QList<Nepomuk::Tag> tags() const;

private slots:
    void slotLinkActivated(const QString& link);

private:
    QLabel* m_label;
    QList<Nepomuk::Tag> m_tags;
    QString m_tagsText;
};

#endif
