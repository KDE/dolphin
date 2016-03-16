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

#include "placesitemeditdialog.h"

#include <KAboutData>
#include "dolphindebug.h"
#include <KFile>
#include <KIconButton>
#include <KLocalizedString>
#include <QMimeDatabase>
#include <KUrlRequester>
#include <QCheckBox>
#include <QEvent>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QLineEdit>

PlacesItemEditDialog::PlacesItemEditDialog(QWidget* parent) :
    QDialog(parent),
    m_icon(),
    m_text(),
    m_url(),
    m_allowGlobal(false),
    m_urlEdit(0),
    m_textEdit(0),
    m_iconButton(0),
    m_appLocal(0)
{
}

void PlacesItemEditDialog::setIcon(const QString& icon)
{
    m_icon = icon;
}

QString PlacesItemEditDialog::icon() const
{
    return m_iconButton->icon();
}

void PlacesItemEditDialog::setText(const QString& text)
{
    m_text = text;
}

QString PlacesItemEditDialog::text() const
{
    QString text = m_textEdit->text();
    if (text.isEmpty()) {
        const QUrl url = m_urlEdit->url();
        text = url.fileName().isEmpty() ? url.toDisplayString(QUrl::PreferLocalFile) : url.fileName();
    }
    return text;
}

void PlacesItemEditDialog::setUrl(const QUrl& url)
{
    m_url = url;
}

QUrl PlacesItemEditDialog::url() const
{
    return m_urlEdit->url();
}

void PlacesItemEditDialog::setAllowGlobal(bool allow)
{
    m_allowGlobal = allow;
}

bool PlacesItemEditDialog::allowGlobal() const
{
    return m_allowGlobal;
}

bool PlacesItemEditDialog::event(QEvent* event)
{
    if (event->type() == QEvent::Polish) {
        initialize();
    }
    return QWidget::event(event);
}

void PlacesItemEditDialog::slotUrlChanged(const QString& text)
{
    m_okButton->setEnabled(!text.isEmpty());
}

PlacesItemEditDialog::~PlacesItemEditDialog()
{
}

void PlacesItemEditDialog::initialize()
{
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok|QDialogButtonBox::Cancel);
    m_okButton = buttonBox->button(QDialogButtonBox::Ok);
    m_okButton->setDefault(true);
    m_okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &PlacesItemEditDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &PlacesItemEditDialog::reject);
    setModal(true);
    m_okButton->setDefault(true);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    setLayout(mainLayout);
    QWidget* mainWidget = new QWidget(this);
    mainLayout->addWidget(mainWidget);
    mainLayout->addWidget(buttonBox);

    QVBoxLayout* vBox = new QVBoxLayout(mainWidget);

    QFormLayout* formLayout = new QFormLayout();
    vBox->addLayout( formLayout );

    m_textEdit = new QLineEdit(mainWidget);
    formLayout->addRow(i18nc("@label", "Label:"), m_textEdit);
    m_textEdit->setText(m_text);
    m_textEdit->setPlaceholderText(i18n("Enter descriptive label here"));

    m_urlEdit = new KUrlRequester(m_url, mainWidget);
    m_urlEdit->setMode(KFile::Directory);
    formLayout->addRow(i18nc("@label", "Location:"), m_urlEdit);
    // Provide room for at least 40 chars (average char width is half of height)
    m_urlEdit->setMinimumWidth(m_urlEdit->fontMetrics().height() * (40 / 2));
    connect(m_urlEdit, &KUrlRequester::textChanged, this, &PlacesItemEditDialog::slotUrlChanged);

    m_iconButton = new KIconButton(mainWidget);
    formLayout->addRow(i18nc("@label", "Choose an icon:"), m_iconButton);
    m_iconButton->setIconSize(IconSize(KIconLoader::Desktop));
    m_iconButton->setIconType(KIconLoader::NoGroup, KIconLoader::Place);
    if (m_icon.isEmpty()) {
        QMimeDatabase db;
        m_iconButton->setIcon(db.mimeTypeForUrl(m_url).iconName());
    } else {
        m_iconButton->setIcon(m_icon);
    }

    if (m_allowGlobal) {
        const QString appName = KAboutData::applicationData().displayName();
        m_appLocal = new QCheckBox( i18n("&Only show when using this application (%1)",  appName ), mainWidget );
        m_appLocal->setChecked(false);
        vBox->addWidget(m_appLocal);
    }

    if (m_text.isEmpty()) {
        m_urlEdit->setFocus();
    } else {
        m_textEdit->setFocus();
    }

}

