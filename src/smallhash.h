/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SMALLHASH_H
#define SMALLHASH_H

#include <QByteArray>
#include <QHash>
#include <QVariant>

#include <utility>
#include <vector>

/**
 * A minimal QHash-like map backed by a flat vector<pair<QByteArray, QVariant>>.
 *
 * Qt6's QHash allocates a ~3 KB minimum span on the first insert, so one QHash
 * per item (a handful of entries times N items) dominates memory; a flat vector
 * holds the same few entries in a fraction of that, with O(n) lookups that are
 * cheap for the small n involved. Only the subset of the QHash API the callers
 * need is provided, plus toHash()/fromHash() to bridge to code expecting a QHash.
 */
class SmallHash
{
public:
    bool isEmpty() const
    {
        return m_data.empty();
    }
    int count() const
    {
        return static_cast<int>(m_data.size());
    }
    bool contains(const QByteArray &key) const
    {
        return indexOf(key) >= 0;
    }
    QVariant value(const QByteArray &key, const QVariant &defaultValue = QVariant()) const
    {
        const int i = indexOf(key);
        return i >= 0 ? m_data[i].second : defaultValue;
    }
    void insert(const QByteArray &key, const QVariant &value)
    {
        (*this)[key] = value;
    }
    void reserve(int size)
    {
        m_data.reserve(size);
    }
    QVariant operator[](const QByteArray &key) const
    {
        return value(key);
    }
    // Returns a modifiable reference to the value for key, inserting a default
    // one if absent (like QHash). The reference is valid until the next insert.
    QVariant &operator[](const QByteArray &key)
    {
        const int i = indexOf(key);
        if (i >= 0) {
            return m_data[i].second;
        }
        m_data.emplace_back(key, QVariant());
        return m_data.back().second;
    }
    void remove(const QByteArray &key)
    {
        const int i = indexOf(key);
        if (i >= 0) {
            m_data.erase(m_data.begin() + i);
        }
    }
    void clear()
    {
        m_data.clear();
        m_data.shrink_to_fit();
    }
    QHash<QByteArray, QVariant> toHash() const
    {
        QHash<QByteArray, QVariant> hash;
        hash.reserve(static_cast<int>(m_data.size()));
        for (const auto &[key, value] : m_data) {
            hash.insert(key, value);
        }
        return hash;
    }
    static SmallHash fromHash(const QHash<QByteArray, QVariant> &hash)
    {
        SmallHash values;
        values.m_data.reserve(hash.size());
        for (auto it = hash.cbegin(); it != hash.cend(); ++it) {
            values.m_data.emplace_back(it.key(), it.value());
        }
        return values;
    }
    std::vector<std::pair<QByteArray, QVariant>>::const_iterator begin() const
    {
        return m_data.begin();
    }
    std::vector<std::pair<QByteArray, QVariant>>::const_iterator end() const
    {
        return m_data.end();
    }

private:
    int indexOf(const QByteArray &key) const
    {
        for (size_t i = 0; i < m_data.size(); ++i) {
            if (m_data[i].first == key) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }
    std::vector<std::pair<QByteArray, QVariant>> m_data;
};

#endif // SMALLHASH_H
