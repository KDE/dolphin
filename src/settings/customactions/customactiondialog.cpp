/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "customactiondialog.h"

#include "customactions.h"

#include <KIconButton>
#include <KLocalizedString>

#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QToolButton>
#include <QVBoxLayout>

CustomActionDialog::CustomActionDialog(QWidget *parent, CustomActions::Target target, const CustomActions::Entry &entry)
    : QDialog(parent)
{
    const bool isDirectory = target == CustomActions::Target::Directories;
    setWindowTitle(entry.name.isEmpty() ? i18nc("@title:window", "Add Entry") : i18nc("@title:window", "Edit Entry"));

    m_name = new QLineEdit(entry.name, this);
    m_name->setPlaceholderText(isDirectory ? i18nc("@info:placeholder", "Open Terminal Here") : i18nc("@info:placeholder", "Check Signature"));

    m_command = new QLineEdit(entry.command, this);
    m_command->setPlaceholderText(isDirectory ? QStringLiteral("konsole --workdir %f") : QStringLiteral("gpg --verify %f"));

    m_icon = new KIconButton(this);
    m_icon->setIcon(entry.icon.isEmpty() ? QStringLiteral("application-x-executable") : entry.icon);

    auto *hint = new QLabel(i18nc("@info",
                                  "<para><interface>%f</interface> is the path of the item, <interface>%F</interface> the paths of all "
                                  "selected items, <interface>%u</interface> and <interface>%U</interface> their URLs, and "
                                  "<interface>%d</interface> the folder they are in. A command without any of these gets the paths "
                                  "appended.</para>"),
                            this);
    hint->setWordWrap(true);
    hint->setTextFormat(Qt::RichText);

    // Picking the program from disk saves typing its path; parameters can then
    // be added by hand after it.
    auto *browseButton = new QToolButton(this);
    browseButton->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    browseButton->setToolTip(i18nc("@info:tooltip", "Pick an application…"));
    connect(browseButton, &QToolButton::clicked, this, &CustomActionDialog::browseForApplication);

    auto *commandRow = new QHBoxLayout();
    commandRow->setContentsMargins(0, 0, 0, 0);
    commandRow->addWidget(m_command);
    commandRow->addWidget(browseButton);

    auto *form = new QFormLayout();
    form->addRow(i18nc("@label:textbox", "Name:"), m_name);
    form->addRow(i18nc("@label:textbox", "Command:"), commandRow);
    form->addRow(i18nc("@label:chooser", "Icon:"), m_icon);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    m_okButton = buttons->button(QDialogButtonBox::Ok);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(hint);
    layout->addWidget(buttons);

    connect(m_name, &QLineEdit::textChanged, this, &CustomActionDialog::updateOkButton);
    connect(m_command, &QLineEdit::textChanged, this, &CustomActionDialog::updateOkButton);
    updateOkButton();

    m_name->setFocus();
}

CustomActions::Entry CustomActionDialog::entry() const
{
    CustomActions::Entry result;
    result.name = m_name->text().trimmed();
    result.command = m_command->text().trimmed();
    result.icon = m_icon->icon();
    return result;
}

void CustomActionDialog::browseForApplication()
{
    const QString path = QFileDialog::getOpenFileName(this,
                                                      i18nc("@title:window", "Pick an Application"),
                                                      QStringLiteral("/usr/bin"),
                                                      // All files first: it is the default, and /usr/bin is full of
                                                      // binaries without an extension that a *.desktop filter hides.
                                                      i18nc("@item:inlistbox file filter", "All files (*);;Application launchers (*.desktop)"));
    if (path.isEmpty()) {
        return;
    }

    QString name = m_name->text().trimmed();
    QString icon = m_icon->icon();
    const QString program = CustomActions::commandFor(path, name.isEmpty() ? &name : nullptr, &icon);
    if (program.isEmpty()) {
        return;
    }

    // Whatever parameters are already typed are kept, only the program changes.
    m_command->setText(CustomActions::replaceProgram(m_command->text(), program));
    if (m_name->text().trimmed().isEmpty() && !name.isEmpty()) {
        m_name->setText(name);
    }
    if (!icon.isEmpty()) {
        m_icon->setIcon(icon);
    }

    m_command->setFocus();
    m_command->setCursorPosition(m_command->text().length());
}

void CustomActionDialog::updateOkButton()
{
    // An entry without both a name and something to run would never show up.
    m_okButton->setEnabled(!m_name->text().trimmed().isEmpty() && !m_command->text().trimmed().isEmpty());
}
