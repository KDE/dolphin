/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kstandarditem.h"
#include "kstandarditemmodel.h"

KStandardItem::KStandardItem(KStandardItem* parent) :
    QObject(parent),
    m_model(nullptr),
    m_data()
{
}

KStandardItem::KStandardItem(const QString& text, KStandardItem* parent) :
    QObject(parent),
    m_model(nullptr),
    m_data()
{
    setText(text);
}

KStandardItem::KStandardItem(const QString& icon, const QString& text, KStandardItem* parent) :
    QObject(parent),
    m_model(nullptr),
    m_data()
{
    setIcon(icon);
    setText(text);
}

KStandardItem::~KStandardItem()
{
}

void KStandardItem::setText(const QString& text)
{
    setDataValue("text", text);
}

QString KStandardItem::text() const
{
    return m_data["text"].toString();
}

void KStandardItem::setIcon(const QString& icon)
{
    setDataValue("iconName", icon);
}

QString KStandardItem::icon() const
{
    return m_data["iconName"].toString();
}

void KStandardItem::setIconOverlays(const QStringList& overlays)
{
    setDataValue("iconOverlays", overlays);
}

QStringList KStandardItem::iconOverlays() const
{
    return m_data["iconOverlays"].toStringList();
}

void KStandardItem::setGroup(const QString& group)
{
    setDataValue("group", group);
}

QString KStandardItem::group() const
{
    return m_data["group"].toString();
}

void KStandardItem::setDataValue(const QByteArray& role, const QVariant& value)
{
    const QVariant previous = m_data.value(role);
    if (previous == value) {
        return;
    }

    m_data.insert(role, value);
    onDataValueChanged(role, value, previous);

    if (m_model) {
        const int index = m_model->index(this);
        QSet<QByteArray> changedRoles;
        changedRoles.insert(role);
        m_model->onItemChanged(index, changedRoles);
        Q_EMIT m_model->itemsChanged(KItemRangeList() << KItemRange(index, 1), changedRoles);
    }
}

QVariant KStandardItem::dataValue(const QByteArray& role) const
{
    return m_data[role];
}

void KStandardItem::setData(const QHash<QByteArray, QVariant>& values)
{
    const QHash<QByteArray, QVariant> previous = m_data;
    m_data = values;
    onDataChanged(values, previous);
}

void KStandardItem::onDataValueChanged(const QByteArray& role,
                                       const QVariant& current,
                                       const QVariant& previous)
{
    Q_UNUSED(role)
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

void KStandardItem::onDataChanged(const QHash<QByteArray, QVariant>& current,
                                  const QHash<QByteArray, QVariant>& previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
}

