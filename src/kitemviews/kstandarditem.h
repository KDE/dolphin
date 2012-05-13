/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KSTANDARDITEM_H
#define KSTANDARDITEM_H

#include <libdolphin_export.h>

#include <QByteArray>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QVariant>

class KStandardItemModel;

/**
 * @brief Represents and item of KStandardItemModel.
 *
 * Provides setter- and getter-methods for the most commonly
 * used roles. It is possible to assign values for custom
 * roles by using setDataValue().
 */
class LIBDOLPHINPRIVATE_EXPORT KStandardItem
{

public:
    explicit KStandardItem(KStandardItem* parent = 0);
    explicit KStandardItem(const QString& text, KStandardItem* parent = 0);
    KStandardItem(const QString& icon, const QString& text, KStandardItem* parent = 0);
    KStandardItem(const KStandardItem& item);
    virtual ~KStandardItem();

    /**
     * Sets the text for the "text"-role.
     */
    void setText(const QString& text);
    QString text() const;

    /**
     * Sets the icon for the "iconName"-role.
     */
    void setIcon(const QString& icon);
    QString icon() const;

    void setIconOverlays(const QStringList& overlays);
    QStringList iconOverlays() const;

    /**
     * Sets the group for the "group"-role.
     */
    void setGroup(const QString& group);
    QString group() const;

    void setDataValue(const QByteArray& role, const QVariant& value);
    QVariant dataValue(const QByteArray& role) const;

    void setParent(KStandardItem* parent);
    KStandardItem* parent() const;

    void setData(const QHash<QByteArray, QVariant>& values);
    QHash<QByteArray, QVariant> data() const;

    QList<KStandardItem*> children() const;

protected:
    virtual void onDataValueChanged(const QByteArray& role,
                                    const QVariant& current,
                                    const QVariant& previous);

    virtual void onDataChanged(const QHash<QByteArray, QVariant>& current,
                               const QHash<QByteArray, QVariant>& previous);

private:
    KStandardItem* m_parent;
    QList<KStandardItem*> m_children;
    KStandardItemModel* m_model;

    QHash<QByteArray, QVariant> m_data;

    friend class KStandardItemModel;
};

#endif


