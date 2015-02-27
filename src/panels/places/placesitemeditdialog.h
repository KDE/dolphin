/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on KFilePlaceEditDialog from kdelibs:                           *
 *   Copyright (C) 2001,2002,2003 Carsten Pfeiffer <pfeiffer@kde.org>      *
 *   Copyright (C) 2007 Kevin Ottens <ervin@kde.org>                       *                                                                         *
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

#ifndef PLACESITEMEDITDIALOG_H
#define PLACESITEMEDITDIALOG_H

#include <QDialog>
#include <QUrl>

class KIconButton;
class KUrlRequester;
class QLineEdit;
class QCheckBox;
class QPushButton;

class PlacesItemEditDialog: public QDialog
{
    Q_OBJECT

public:
    explicit PlacesItemEditDialog(QWidget* parent = 0);
    virtual ~PlacesItemEditDialog();

    void setIcon(const QString& icon);
    QString icon() const;

    void setText(const QString& text);
    QString text() const;

    void setUrl(const QUrl& url);
    QUrl url() const;

    void setAllowGlobal(bool allow);
    bool allowGlobal() const;

protected:
    virtual bool event(QEvent* event) Q_DECL_OVERRIDE;

private slots:
    void slotUrlChanged(const QString& text);

private:
    void initialize();

private:
    QString m_icon;
    QString m_text;
    QUrl m_url;
    bool m_allowGlobal;

    KUrlRequester* m_urlEdit;
    QLineEdit* m_textEdit;
    KIconButton* m_iconButton;
    QCheckBox* m_appLocal;
    QPushButton *m_okButton;
};

#endif
