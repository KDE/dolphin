/*
 * Copyright (c) 2000 Malte Starostik <malte@kde.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <qlabel.h>
#include <qlayout.h>
#include <qpushbutton.h>

#include <kapplication.h>
#include <klocale.h>
#include <kglobal.h>
#include <kcharsets.h>
#include <klineedit.h>
#include <kcombobox.h>
#include <kmessagebox.h>

#include "searchproviderdlg_ui.h"
#include "searchproviderdlg.h"
#include "searchprovider.h"

SearchProviderDialog::SearchProviderDialog(SearchProvider *provider,
                                           QWidget *parent, const char *name)
                     :KDialogBase(parent, name, true, QString::null, Ok|Cancel),
                      m_provider(provider)
{
    m_dlg = new SearchProviderDlgUI (this);
    setMainWidget(m_dlg);

    enableButtonSeparator(true);

    m_dlg->leQuery->setMinimumWidth(kapp->fontMetrics().maxWidth() * 40);

    connect(m_dlg->leName, SIGNAL(textChanged(const QString &)), SLOT(slotChanged()));
    connect(m_dlg->leQuery, SIGNAL(textChanged(const QString &)), SLOT(slotChanged()));
    connect(m_dlg->leShortcut, SIGNAL(textChanged(const QString &)), SLOT(slotChanged()));

    // Data init
    QStringList charsets = KGlobal::charsets()->availableEncodingNames();
    charsets.prepend(i18n("Default"));
    m_dlg->cbCharset->insertStringList(charsets);

    if (m_provider)
    {
        setPlainCaption(i18n("Modify Search Provider"));
        m_dlg->leName->setText(m_provider->name());
        m_dlg->leQuery->setText(m_provider->query());
        m_dlg->leShortcut->setText(m_provider->keys().join(","));
        m_dlg->cbCharset->setCurrentItem(m_provider->charset().isEmpty() ? 0 : charsets.findIndex(m_provider->charset()));
        m_dlg->leName->setEnabled(false);
        m_dlg->leQuery->setFocus();
    }
    else
    {
        setPlainCaption(i18n("New Search Provider"));
        m_dlg->leName->setFocus();
        enableButton(Ok, false);
    }
}

void SearchProviderDialog::slotChanged()
{
    enableButton(Ok, !(m_dlg->leName->text().isEmpty()
                       || m_dlg->leShortcut->text().isEmpty()
                       || m_dlg->leQuery->text().isEmpty()));
}

void SearchProviderDialog::slotOk()
{
    if ((m_dlg->leQuery->text().find("\\{") == -1)
        && KMessageBox::warningContinueCancel(0,
            i18n("The URI does not contain a \\{...} placeholder for the user query.\n"
                 "This means that the same page is always going to be visited, "
                 "regardless of what the user types."),
            QString::null, i18n("Keep It")) == KMessageBox::Cancel)
        return;

    if (!m_provider)
        m_provider = new SearchProvider;
    m_provider->setName(m_dlg->leName->text().trimmed());
    m_provider->setQuery(m_dlg->leQuery->text().trimmed());
    m_provider->setKeys(QStringList::split(",", m_dlg->leShortcut->text().trimmed()));
    m_provider->setCharset(m_dlg->cbCharset->currentItem() ? m_dlg->cbCharset->currentText() : QString::null);
    KDialog::accept();
}

#include "searchproviderdlg.moc"
