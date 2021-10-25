/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KSTANDARDITEM_H
#define KSTANDARDITEM_H

#include "dolphin_export.h"

#include <QByteArray>
#include <QHash>
#include <QObject>
#include <QSet>
#include <QVariant>

class KStandardItemModel;

/**
 * @brief Represents and item of KStandardItemModel.
 *
 * Provides setter- and getter-methods for the most commonly
 * used roles. It is possible to assign values for custom
 * roles by using setDataValue().
 */
class DOLPHIN_EXPORT KStandardItem : public QObject
{
    Q_OBJECT
public:
    explicit KStandardItem(KStandardItem* parent = nullptr);
    explicit KStandardItem(const QString& text, KStandardItem* parent = nullptr);
    KStandardItem(const QString& icon, const QString& text, KStandardItem* parent = nullptr);
    ~KStandardItem() override;

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

    void setData(const QHash<QByteArray, QVariant>& values);
    QHash<QByteArray, QVariant> data() const;

protected:
    virtual void onDataValueChanged(const QByteArray& role,
                                    const QVariant& current,
                                    const QVariant& previous);

    virtual void onDataChanged(const QHash<QByteArray, QVariant>& current,
                               const QHash<QByteArray, QVariant>& previous);

private:
    KStandardItemModel* m_model;

    QHash<QByteArray, QVariant> m_data;

    friend class KStandardItemModel;
};

#endif


