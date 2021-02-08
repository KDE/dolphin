/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2018 Roman Inflianskas <infroma@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef DOLPHINTRASH_H
#define DOLPHINTRASH_H

#include <QWidget>

#include <KIO/EmptyTrashJob>
#include <KIOWidgets/KDirLister>

class Trash: public QObject
{
    Q_OBJECT

public:
    // delete copy and move constructors and assign operators
    Trash(Trash const&) = delete;
    Trash(Trash&&) = delete;
    Trash& operator=(Trash const&) = delete;
    Trash& operator=(Trash &&) = delete;

    static Trash& instance();
    static KIO::Job* empty(QWidget *window);
    static bool isEmpty();

Q_SIGNALS:
    void emptinessChanged(bool isEmpty);

private:
    KDirLister *m_trashDirLister;

    Trash();
    ~Trash();
};

#endif // DOLPHINTRASH_H
